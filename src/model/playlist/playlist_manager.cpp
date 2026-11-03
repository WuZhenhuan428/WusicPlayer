#include "model/playlist/playlist_manager.h"

#include "core/utils/audio.hpp"
#include "core/utils/path.hpp"
#include "model/library/library_manager.h"
#include <QFileInfo>
#include <QTimer>

#include "core/logger/logger_manager.h"
namespace
{
// NOLINTNEXTLINE
Logger* logger = LoggerManager::file_logger("playlist_manager", {"console", "gui"});
} // namespace

PlaylistManager::PlaylistManager(QObject* parent) : QObject(parent)
{
    context_ = new PlaylistContext(this);
    repo_    = new PlaylistRepo(this);
    view_    = new PlaylistViewModel(repo_, this);

    connect(context_, &PlaylistContext::sgn_current_list_changed, view_,
            &PlaylistViewModel::set_playlist);
    connect(context_, &PlaylistContext::sgn_current_track_changed, view_,
            &PlaylistViewModel::set_active_track);

    connect(repo_, &PlaylistRepo::sgn_playlist_changed, this,
            &PlaylistManager::retransmission_playlist_changed);
    connect(repo_, &PlaylistRepo::sgn_cache_load_started, this,
            &PlaylistManager::sgn_cache_load_started);
    connect(repo_, &PlaylistRepo::sgn_playlist_load_started, this,
            &PlaylistManager::sgn_playlist_load_started);
    connect(repo_, &PlaylistRepo::sgn_playlist_load_finished, this,
            &PlaylistManager::sgn_playlist_load_finished);
    connect(repo_, &PlaylistRepo::sgn_cache_load_finished, this, [this](int count) {
        emit sgn_cache_load_finished(count);
        if (context_->get_playlist_id().is_null()) {
            auto lists = repo_->get_lists();
            if (!lists.isEmpty()) {
                context_->set_playlist(lists.first()->id());
            }
        }
    });
}

void PlaylistManager::create_playlist() const
{
    repo_->create_list();
}

void PlaylistManager::remove_playlist(const PlaylistId& to_remove_uuid) const
{
    const auto& pl = repo_->find_playlist_by_id(to_remove_uuid);
    if (!pl) {
        return;
    }

    repo_->remove_list(to_remove_uuid);
    if (context_->get_playlist_id() == to_remove_uuid) {
        auto remaining = repo_->get_lists();
        if (!remaining.isEmpty()) {
            context_->set_playlist(remaining.first()->id());
        } else {
            context_->set_playlist(PlaylistId());
        }
    }
}

void PlaylistManager::copy_playlist(const PlaylistId& pid) const
{
    repo_->copy_list(pid);
}

void PlaylistManager::reorder_playlists(const QVector<PlaylistId>& ordered_ids) const
{
    if (repo_) {
        repo_->reorder_lists(ordered_ids);
    }
}

void PlaylistManager::load_playlist(const QString& playlist_path) const
{
    PlaylistId new_id = repo_->load_list_batched(playlist_path, 500);
    if (!new_id.is_null()) {
        context_->set_playlist(new_id);
    }
}

void PlaylistManager::rename_playlist(const PlaylistId& src_pid, const QString& dst_name) const
{
    repo_->rename_list(src_pid, dst_name);
}

void PlaylistManager::save_playlist(const PlaylistId& pid, const QString& save_path) const
{
    auto pl = repo_->find_playlist_by_id(pid);
    if (!pl->empty()) {
        repo_->save_list(pid, save_path);
        logger->info("save playlist {} at {}", pid.to_string(), save_path);
    }
}

void PlaylistManager::load_cache_after_shown() const
{
    repo_->load_cache_async();
}

void PlaylistManager::add_track(const PlaylistId& pid, const QString& filepath,
                                AddFilePolicy policy)
{
    // 单文件默认:仅外部文件;import_to_library 时把父目录注册到库(扫描后 upgrade 为库引用)
    const AddFilePolicy eff = resolve_effective_policy(policy, AddFilePolicy::KeepExternal);
    if (library_ && eff == AddFilePolicy::ImportToLibrary) {
        const QString norm = utils::path::normalize_path(filepath);
        if (!library_->track_by_path(norm)) {
            library_->add_watched_folder(QFileInfo(norm).absolutePath());
        }
    }
    repo_->add_track_object(pid, resolve_track(filepath));
}

// 通过音乐库解析曲目:库中有则引用库条目(元数据走库缓存);否则作为外部条目
Track PlaylistManager::resolve_track(const QString& filepath) const
{
    const QString norm = utils::path::normalize_path(filepath);
    if (library_) {
        const auto lib = library_->track_by_path(norm);
        if (lib) {
            Track t;
            t.source           = TrackSource::Library;
            t.library_track_id = lib->track_id;
            t.filepath         = lib->filepath;
            t.meta             = lib->meta;
            t.missing          = lib->missing;
            return t;
        }
    }
    return Track::from_filepath(filepath); // 外部条目(不强制入库)
}

int PlaylistManager::add_library_tracks(const PlaylistId& pid, const QVector<TrackId>& track_ids)
{
    if (track_ids.isEmpty() || !repo_) {
        return 0;
    }
    QVector<Track> tracks;
    tracks.reserve(track_ids.size());
    for (const TrackId& tid : track_ids) {
        if (tid.is_null()) {
            continue;
        }
        // 库曲目:优先走库解析(元数据由库维护);库未注入时按 TrackId 直查
        const auto lib = library_ ? library_->track_by_id(tid) : nullptr;
        if (lib) {
            Track t;
            t.source           = TrackSource::Library;
            t.library_track_id = lib->track_id;
            t.filepath         = lib->filepath;
            t.meta             = lib->meta;
            t.missing          = lib->missing;
            tracks.push_back(t);
        } else {
            logger->warn("[PlaylistManager] add_library_tracks: TrackId not in library {}",
                         tid.to_string());
        }
    }
    if (tracks.isEmpty()) {
        return 0;
    }
    repo_->add_track_objects(pid, tracks);
    return static_cast<int>(tracks.size());
}

int PlaylistManager::copy_tracks_to_playlist(const PlaylistId& src_pid,
                                             const QVector<EntryId>& entry_ids,
                                             const PlaylistId& dst_pid) const
{
    if (entry_ids.isEmpty() || !repo_ || src_pid == dst_pid) {
        return 0;
    }
    auto src = repo_->find_playlist_by_id(src_pid);
    auto dst = repo_->find_playlist_by_id(dst_pid);
    if (!src || !dst) {
        return 0;
    }
    QVector<Track> tracks;
    tracks.reserve(entry_ids.size());
    for (const EntryId& eid : entry_ids) {
        const Track* src_track = src->find_track_by_id(eid);
        if (!src_track) {
            continue;
        }
        // 复制条目:保留来源(库引用/外部)与元数据,生成新的条目身份
        Track copy    = *src_track;
        copy.entry_id = EntryId::create_uuid();
        tracks.push_back(copy);
    }
    if (tracks.isEmpty()) {
        return 0;
    }
    repo_->add_track_objects(dst_pid, tracks);
    return static_cast<int>(tracks.size());
}

void PlaylistManager::remove_track(const EntryId& tid)
{
    if (tid.is_null() || !repo_ || !context_) {
        return;
    }

    auto playlist = repo_->find_playlist_by_id(context_->get_playlist_id());
    if (!playlist) {
        return;
    }

    const Track* track = playlist->find_track_by_id(tid);
    if (!track) {
        return;
    }

    playlist->remove_track(tid);
    repo_->save_list_to_cache(playlist);

    if (context_->get_play_track_id() == tid) {
        context_->set_play_track(EntryId());
    }

    // 行级移除: 不重建视图 → 滚动位置与展开状态保持
    if (view_) {
        view_->remove_tracks_by_ids({tid});
    }

    emit sgn_playlist_changed();
}

void PlaylistManager::remove_missing_tracks()
{
    auto playlist = repo_->find_playlist_by_id(context_->get_playlist_id());
    if (!playlist) {
        return;
    }
    QVector<EntryId> removed_ids;
    if (playlist->remove_missing_tracks(&removed_ids) == 0) {
        return;
    }
    repo_->save_list_to_cache(playlist);

    // 行级移除: 不重建视图 → 滚动位置与展开状态保持
    if (view_ && !removed_ids.isEmpty()) {
        view_->remove_tracks_by_ids(removed_ids);
    }

    // 当前播放被清理 → 清空播放上下文
    if (removed_ids.contains(context_->get_play_track_id())) {
        context_->set_play_track(EntryId());
    }

    emit sgn_playlist_changed();
}

void PlaylistManager::set_library_manager(LibraryManager* lib)
{
    if (library_ == lib) {
        return;
    }
    if (library_) {
        disconnect(library_, nullptr, this, nullptr);
    }
    library_ = lib;
    if (library_) {
        connect(library_, &LibraryManager::sgn_library_changed, this,
                &PlaylistManager::on_library_changed);
    }
}

void PlaylistManager::set_add_file_policy(AddFilePolicy policy)
{
    add_file_policy_ = policy;
}

AddFilePolicy PlaylistManager::resolve_effective_policy(AddFilePolicy requested,
                                                        AddFilePolicy by_operation_default) const
{
    if (requested != AddFilePolicy::ByOperation) {
        return requested; // 显式策略(import/external;ask 由上层展开,不应到达 model)
    }
    switch (add_file_policy_) {
    case AddFilePolicy::ImportToLibrary:
    case AddFilePolicy::KeepExternal:
        return add_file_policy_;
    default: // by_operation / always_ask:按操作类型默认
        return by_operation_default;
    }
}

void PlaylistManager::on_library_changed()
{
    if (!library_) {
        return;
    }
    const PlaylistId current = context_ ? context_->get_playlist_id() : PlaylistId();

    bool changed             = false;
    bool need_full_rebuild   = false; // 当前列表: 元数据/来源变化 → 结构可能变, 需重建
    QVector<EntryId> missing_changed_for_current;

    for (const auto& pl : repo_->get_lists()) {
        QVector<EntryId> meta_changed;
        QVector<EntryId> missing_changed;
        const int refreshed = pl->refresh_library_tracks(
            [this](const TrackId& id) { return library_->track_by_id(id); }, &meta_changed,
            &missing_changed);
        const int upgraded = pl->upgrade_external_tracks(
            [this](const QString& path) { return library_->track_by_path(path); });

        if (pl->id() == current) {
            // 元数据变化可能影响排序/分组键与列内容 → 保守走重建;
            // 仅缺失状态翻转(未恢复/新标记) → 内容级通知, 保持滚动与展开。
            if (!meta_changed.isEmpty() || upgraded > 0) {
                need_full_rebuild = true;
            } else {
                missing_changed_for_current += missing_changed;
            }
        }

        if (refreshed > 0 || upgraded > 0) {
            changed = true;
        }
    }

    if (!changed) {
        return;
    }
    if (view_) {
        if (need_full_rebuild) {
            view_->rebuild_async();
        } else if (!missing_changed_for_current.isEmpty()) {
            view_->apply_missing_changes(missing_changed_for_current);
        }
    }
    emit sgn_playlist_changed();
}

// a wrap of this->add_track
void PlaylistManager::add_folder(const PlaylistId& pid, const QString& directory,
                                 AddFilePolicy policy)
{
    // 文件夹添加默认同步入库:目录注册到库(异步扫描),未命中条目随后升级为库引用
    const AddFilePolicy eff = resolve_effective_policy(policy, AddFilePolicy::ImportToLibrary);
    if (library_ && eff == AddFilePolicy::ImportToLibrary) {
        library_->add_watched_folder(directory);
    }

    PlaylistId curr_pid = pid;
    if (pid.is_null()) {
        curr_pid = repo_->create_list();
        context_->set_playlist(curr_pid);
    }

    const auto& files = utils::audio::find_all(directory);
    QVector<Track> tracks_to_add;
    tracks_to_add.reserve(static_cast<int>(files.size()));

    for (const auto& file : files) {
        if (utils::audio::is_audio_file(file)) {
            tracks_to_add.append(resolve_track(utils::audio::from_fs_path(file)));
        }
    }

    if (!tracks_to_add.isEmpty()) {
        repo_->add_track_objects(curr_pid, tracks_to_add);
    }
}

EntryId PlaylistManager::next_track(PlayMode mode) const
{
    EntryId next_id = EntryId();
    EntryId curr_id = context_->get_play_track_id();
    switch (mode) {
    case PlayMode::InOrder:
        next_id =
            playlist_navigator::next_of_in_order(view_->playback_queue_snapshot().queue, curr_id);
        break;
    case PlayMode::Loop:
        next_id = playlist_navigator::next_of_loop(view_->playback_queue_snapshot().queue, curr_id);
        break;
    case PlayMode::Shuffle:
        next_id = playlist_navigator::next_of_shuffle(view_->playback_queue_snapshot().queue);
        break;
    case PlayMode::OutOfOrderByTrack:
        next_id = playlist_navigator::next_of_out_of_order_track(
            view_->single_shuffle_queue_snapshot().queue, curr_id);
        break;
    case PlayMode::OutOfOrderByGroup:
        next_id = playlist_navigator::next_of_out_of_order_group(
            view_->group_shuffle_queue_snapshot().queue, curr_id);
        break;
    }

    if (!next_id.is_null()) {
        context_->set_play_track(next_id);
    }
    return next_id;
}

EntryId PlaylistManager::prev_track(PlayMode mode) const
{
    EntryId prev_id = EntryId();
    EntryId curr_id = context_->get_play_track_id();
    switch (mode) {
    case PlayMode::InOrder:
        prev_id = playlist_navigator::previous_of_in_order(view_->playback_queue_snapshot().queue,
                                                           curr_id);
        break;
    case PlayMode::Loop:
        prev_id =
            playlist_navigator::previous_of_loop(view_->playback_queue_snapshot().queue, curr_id);
        break;
    case PlayMode::Shuffle:
        prev_id = playlist_navigator::previous_of_shuffle(view_->playback_queue_snapshot().queue);
        break;
    case PlayMode::OutOfOrderByTrack:
        prev_id = playlist_navigator::previous_of_out_of_order_track(
            view_->single_shuffle_queue_snapshot().queue, curr_id);
        break;
    case PlayMode::OutOfOrderByGroup:
        prev_id = playlist_navigator::previous_of_out_of_order_group(
            view_->group_shuffle_queue_snapshot().queue, curr_id);
        break;
    }

    if (!prev_id.is_null()) {
        context_->set_play_track(prev_id);
    }
    return prev_id;
}

PlaylistViewModel* PlaylistManager::get_view_model() const
{
    return this->view_;
}

void PlaylistManager::play(int index)
{
    EntryId tid = view_->track_at(index);
    context_->set_play_track(tid);

    auto pid      = context_->get_playlist_id();
    auto playlist = repo_->find_playlist_by_id(pid);
    if (!playlist) {
        return;
    }
    const Track* t = playlist->find_track_by_id(tid);
    if (t) {
        emit sgn_request_play(t->filepath);
    }
}

void PlaylistManager::set_current_track(const EntryId& tid) const
{
    context_->set_play_track(tid);
}

QString PlaylistManager::get_current_track() const
{
    EntryId tid = context_->get_play_track_id();
    auto pl     = repo_->find_playlist_by_id(context_->get_playlist_id());
    if (!pl) {
        return {};
    }
    const Track* track = pl->find_track_by_id(tid);
    if (!track) {
        return {};
    }
    return track->filepath;
}

QString PlaylistManager::get_current_playlist_name() const
{
    PlaylistId pid = context_->get_playlist_id();
    auto pl        = repo_->find_playlist_by_id(pid);
    return pl ? pl->name() : QString();
}

const EntryId& PlaylistManager::get_current_track_id() const
{
    return this->context_->get_play_track_id();
}

const PlaylistId& PlaylistManager::get_current_playlist_id() const
{
    return this->context_->get_playlist_id();
}

QVector<PlaylistInfo> PlaylistManager::get_all_playlists() const
{
    QVector<PlaylistInfo> infos;

    auto playlists = repo_->get_lists();
    for (const auto& pl : playlists) {
        infos.append({pl->id(), pl->name()});
    }
    return infos;
}

void PlaylistManager::retransmission_playlist_changed()
{
    emit sgn_playlist_changed();
}

void PlaylistManager::switch_to_playlist(const PlaylistId& pid) const
{
    context_->set_playlist(pid);
}

QVector<std::shared_ptr<Playlist>> PlaylistManager::get_playlists() const
{
    return repo_->get_lists();
}

TrackMetaData PlaylistManager::get_current_metadata() const
{
    EntryId tid   = context_->get_play_track_id();
    auto playlist = repo_->find_playlist_by_id(context_->get_playlist_id());

    if (playlist) {
        const Track* track = playlist->find_track_by_id(tid);
        if (track) {
            return track->meta;
        }
    }
    TrackMetaData empty_meta{};
    empty_meta.is_valid = false;
    return empty_meta;
}

QString PlaylistManager::get_playlist_by_id(const PlaylistId& pid) const
{
    auto pl = repo_->find_playlist_by_id(pid);
    if (!pl || pl->empty()) {
        return {};
    }
    return pl->name();
}
