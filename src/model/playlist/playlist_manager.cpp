#include "model/playlist/playlist_manager.h"

#include "core/utils/audio.hpp"
#include "core/utils/path.hpp"
#include "model/library/library_manager.h"
#include <QFileInfo>
#include <QTimer>

#include <optional>

#include "core/logger/logger_manager.h"
namespace
{
Logger* logger = LoggerManager::file_logger("playlist_manager", {"console", "gui"});
}

PlaylistManager::PlaylistManager(QObject* parent) : QObject(parent)
{
    m_context = new PlaylistContext(this);
    m_repo    = new PlaylistRepo(this);
    m_view    = new PlaylistViewModel(m_repo, this);

    connect(m_context, &PlaylistContext::sgn_current_list_changed, m_view,
            &PlaylistViewModel::set_playlist);
    connect(m_context, &PlaylistContext::sgn_current_track_changed, m_view,
            &PlaylistViewModel::set_active_track);

    connect(m_repo, &PlaylistRepo::sgn_playlist_changed, this,
            &PlaylistManager::retransmission_playlist_changed);
    connect(m_repo, &PlaylistRepo::sgn_cache_load_started, this,
            &PlaylistManager::sgn_cache_load_started);
    connect(m_repo, &PlaylistRepo::sgn_playlist_load_started, this,
            &PlaylistManager::sgn_playlist_load_started);
    connect(m_repo, &PlaylistRepo::sgn_playlist_load_finished, this,
            &PlaylistManager::sgn_playlist_load_finished);
    connect(m_repo, &PlaylistRepo::sgn_cache_load_finished, this, [this](int count) {
        emit sgn_cache_load_finished(count);
        if (m_context->get_playlist_id().isNull()) {
            auto lists = m_repo->get_lists();
            if (!lists.isEmpty()) {
                m_context->set_playlist(lists.first()->id());
            }
        }
    });
}

PlaylistManager::~PlaylistManager() {}

void PlaylistManager::create_playlist()
{
    m_repo->create_list();
}

void PlaylistManager::remove_playlist(const PlaylistId& to_remove_uuid)
{
    const auto& pl = m_repo->find_playlist_by_id(to_remove_uuid);
    if (!pl)
        return;

    m_repo->remove_list(to_remove_uuid);
    if (m_context->get_playlist_id() == to_remove_uuid) {
        auto remaining = m_repo->get_lists();
        if (!remaining.isEmpty()) {
            m_context->set_playlist(remaining.first()->id());
        } else {
            m_context->set_playlist(PlaylistId());
        }
    }
}

void PlaylistManager::copy_playlist(const PlaylistId& pid)
{
    m_repo->copy_list(pid);
}

void PlaylistManager::reorder_playlists(const QVector<PlaylistId>& ordered_ids)
{
    if (m_repo) {
        m_repo->reorder_lists(ordered_ids);
    }
}

void PlaylistManager::load_playlist(const QString& playlist_path)
{
    PlaylistId new_id = m_repo->load_list_batched(playlist_path, 500);
    if (!new_id.isNull()) {
        m_context->set_playlist(new_id);
    }
}

void PlaylistManager::rename_playlist(const PlaylistId& src_pid, const QString dst_name)
{
    m_repo->rename_list(src_pid, dst_name);
}

void PlaylistManager::save_playlist(const PlaylistId& pid, const QString& save_path)
{
    auto pl = m_repo->find_playlist_by_id(pid);
    if (!pl->isEmpty()) {
        m_repo->save_list(pid, save_path);
        logger->info("[INFO] save playlist {} at {}", pid.toString(), save_path);
    }
}

void PlaylistManager::load_cache_after_shown()
{
    m_repo->load_cache_async();
}

void PlaylistManager::add_track(const PlaylistId& pid, const QString& filepath,
                                AddFilePolicy policy)
{
    // 单文件默认:仅外部文件;import_to_library 时把父目录注册到库(扫描后 upgrade 为库引用)
    const AddFilePolicy eff = resolve_effective_policy(policy, AddFilePolicy::keep_external);
    if (m_library && eff == AddFilePolicy::import_to_library) {
        const QString norm = utils::path::normalize_path(filepath);
        if (!m_library->track_by_path(norm).has_value()) {
            m_library->add_watched_folder(QFileInfo(norm).absolutePath());
        }
    }
    m_repo->add_track_object(pid, resolve_track(filepath));
}

// 通过音乐库解析曲目:库中有则引用库条目(元数据走库缓存);否则作为外部条目
Track PlaylistManager::resolve_track(const QString& filepath) const
{
    const QString norm = utils::path::normalize_path(filepath);
    if (m_library) {
        const auto lib = m_library->track_by_path(norm);
        if (lib) {
            Track t;
            t.source           = TrackSource::library;
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
    if (track_ids.isEmpty() || !m_repo) {
        return 0;
    }
    QVector<Track> tracks;
    tracks.reserve(track_ids.size());
    for (const TrackId& tid : track_ids) {
        if (tid.isNull()) {
            continue;
        }
        // 库曲目:优先走库解析(元数据由库维护);库未注入时按 TrackId 直查
        const auto lib = m_library ? m_library->track_by_id(tid) : std::nullopt;
        if (lib) {
            Track t;
            t.source           = TrackSource::library;
            t.library_track_id = lib->track_id;
            t.filepath         = lib->filepath;
            t.meta             = lib->meta;
            t.missing          = lib->missing;
            tracks.push_back(t);
        } else {
            logger->warn("[PlaylistManager] add_library_tracks: TrackId not in library {}",
                         tid.toString());
        }
    }
    if (tracks.isEmpty()) {
        return 0;
    }
    m_repo->add_track_objects(pid, tracks);
    return tracks.size();
}

int PlaylistManager::copy_tracks_to_playlist(const PlaylistId& src_pid,
                                             const QVector<EntryId>& entry_ids,
                                             const PlaylistId& dst_pid)
{
    if (entry_ids.isEmpty() || !m_repo || src_pid == dst_pid) {
        return 0;
    }
    auto src = m_repo->find_playlist_by_id(src_pid);
    auto dst = m_repo->find_playlist_by_id(dst_pid);
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
        copy.entry_id = EntryId::createUuid();
        tracks.push_back(copy);
    }
    if (tracks.isEmpty()) {
        return 0;
    }
    m_repo->add_track_objects(dst_pid, tracks);
    return tracks.size();
}

void PlaylistManager::remove_track(const EntryId& tid)
{
    if (tid.isNull() || !m_repo || !m_context) {
        return;
    }

    auto playlist = m_repo->find_playlist_by_id(m_context->get_playlist_id());
    if (!playlist) {
        return;
    }

    const Track* track = playlist->find_track_by_id(tid);
    if (!track) {
        return;
    }

    playlist->remove_track(tid);
    m_repo->save_list_to_cache(playlist);

    if (m_context->get_play_track_id() == tid) {
        m_context->set_play_track(EntryId());
    }

    if (m_view) {
        m_view->rebuild_async();
    }

    emit sgn_playlist_changed();
}

void PlaylistManager::remove_missing_tracks()
{
    auto playlist = m_repo->find_playlist_by_id(m_context->get_playlist_id());
    if (!playlist) {
        return;
    }
    if (playlist->remove_missing_tracks() == 0) {
        return;
    }
    m_repo->save_list_to_cache(playlist);
    if (m_view) {
        m_view->rebuild_async();
    }
    emit sgn_playlist_changed();
}

void PlaylistManager::set_library_manager(LibraryManager* lib)
{
    if (m_library == lib) {
        return;
    }
    if (m_library) {
        disconnect(m_library, nullptr, this, nullptr);
    }
    m_library = lib;
    if (m_library) {
        connect(m_library, &LibraryManager::sgn_library_changed, this,
                &PlaylistManager::on_library_changed);
    }
}

void PlaylistManager::set_add_file_policy(AddFilePolicy policy)
{
    m_add_file_policy = policy;
}

AddFilePolicy PlaylistManager::resolve_effective_policy(AddFilePolicy requested,
                                                        AddFilePolicy by_operation_default) const
{
    if (requested != AddFilePolicy::by_operation) {
        return requested; // 显式策略(import/external;ask 由上层展开,不应到达 model)
    }
    switch (m_add_file_policy) {
    case AddFilePolicy::import_to_library:
    case AddFilePolicy::keep_external:
        return m_add_file_policy;
    default: // by_operation / always_ask:按操作类型默认
        return by_operation_default;
    }
}

void PlaylistManager::on_library_changed()
{
    if (!m_library) {
        return;
    }
    bool changed = false;
    for (const auto& pl : m_repo->get_lists()) {
        const int refreshed = pl->refresh_library_tracks(
            [this](const TrackId& id) { return m_library->track_by_id(id); });
        const int upgraded = pl->upgrade_external_tracks(
            [this](const QString& path) { return m_library->track_by_path(path); });
        if (refreshed > 0 || upgraded > 0) {
            changed = true;
        }
    }
    if (changed) {
        if (m_view) {
            m_view->rebuild_async();
        }
        emit sgn_playlist_changed();
    }
}

// a wrap of this->add_track
void PlaylistManager::add_folder(const PlaylistId& pid, const QString& directory,
                                 AddFilePolicy policy)
{
    // 文件夹添加默认同步入库:目录注册到库(异步扫描),未命中条目随后升级为库引用
    const AddFilePolicy eff = resolve_effective_policy(policy, AddFilePolicy::import_to_library);
    if (m_library && eff == AddFilePolicy::import_to_library) {
        m_library->add_watched_folder(directory);
    }

    PlaylistId curr_pid = pid;
    if (pid.isNull()) {
        curr_pid = m_repo->create_list();
        m_context->set_playlist(curr_pid);
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
        m_repo->add_track_objects(curr_pid, tracks_to_add);
    }
}

EntryId PlaylistManager::next_track(PlayMode mode)
{
    EntryId next_id = EntryId();
    EntryId curr_id = m_context->get_play_track_id();
    switch (mode) {
    case PlayMode::in_order:
        next_id =
            PlaylistNavigator::next_of_in_order(m_view->playback_queue_snapshot().queue, curr_id);
        break;
    case PlayMode::loop:
        next_id = PlaylistNavigator::next_of_loop(m_view->playback_queue_snapshot().queue, curr_id);
        break;
    case PlayMode::shuffle:
        next_id = PlaylistNavigator::next_of_shuffle(m_view->playback_queue_snapshot().queue);
        break;
    case PlayMode::out_of_order_track:
        next_id = PlaylistNavigator::next_of_out_of_order_track(
            m_view->single_shuffle_queue_snapshot().queue, curr_id);
        break;
    case PlayMode::out_of_order_group:
        next_id = PlaylistNavigator::next_of_out_of_order_group(
            m_view->group_shuffle_queue_snapshot().queue, curr_id);
        break;
    }

    if (!next_id.isNull()) {
        m_context->set_play_track(next_id);
    }
    return next_id;
}

EntryId PlaylistManager::prev_track(PlayMode mode)
{
    EntryId prev_id = EntryId();
    EntryId curr_id = m_context->get_play_track_id();
    switch (mode) {
    case PlayMode::in_order:
        prev_id = PlaylistNavigator::previous_of_in_order(m_view->playback_queue_snapshot().queue,
                                                          curr_id);
        break;
    case PlayMode::loop:
        prev_id =
            PlaylistNavigator::previous_of_loop(m_view->playback_queue_snapshot().queue, curr_id);
        break;
    case PlayMode::shuffle:
        prev_id = PlaylistNavigator::previous_of_shuffle(m_view->playback_queue_snapshot().queue);
        break;
    case PlayMode::out_of_order_track:
        prev_id = PlaylistNavigator::previous_of_out_of_order_track(
            m_view->single_shuffle_queue_snapshot().queue, curr_id);
        break;
    case PlayMode::out_of_order_group:
        prev_id = PlaylistNavigator::previous_of_out_of_order_group(
            m_view->group_shuffle_queue_snapshot().queue, curr_id);
        break;
    }

    if (!prev_id.isNull()) {
        m_context->set_play_track(prev_id);
    }
    return prev_id;
}

PlaylistViewModel* PlaylistManager::get_view_model()
{
    return this->m_view;
}

void PlaylistManager::play(int index)
{
    EntryId tid = m_view->track_at(index);
    m_context->set_play_track(tid);

    auto pid      = m_context->get_playlist_id();
    auto playlist = m_repo->find_playlist_by_id(pid);
    if (!playlist) {
        return;
    }
    const Track* t = playlist->find_track_by_id(tid);
    if (t) {
        emit sgn_request_play(t->filepath);
    }
}

void PlaylistManager::set_current_track(const EntryId& tid)
{
    m_context->set_play_track(tid);
}

QString PlaylistManager::get_current_track() const
{
    EntryId tid = m_context->get_play_track_id();
    auto pl     = m_repo->find_playlist_by_id(m_context->get_playlist_id());
    if (!pl) {
        return QString();
    }
    const Track* track = pl->find_track_by_id(tid);
    if (!track) {
        return QString();
    }
    return track->filepath;
}

QString PlaylistManager::get_current_playlist_name() const
{
    PlaylistId pid = m_context->get_playlist_id();
    auto pl        = m_repo->find_playlist_by_id(pid);
    return pl ? pl->name() : QString();
}

const EntryId& PlaylistManager::get_current_track_id() const
{
    return this->m_context->get_play_track_id();
}

const PlaylistId& PlaylistManager::get_current_playlist_id() const
{
    return this->m_context->get_playlist_id();
}

QVector<PlaylistInfo> PlaylistManager::get_all_playlists()
{
    QVector<PlaylistInfo> infos;

    auto playlists = m_repo->get_lists();
    for (const auto& pl : playlists) {
        infos.append({pl->id(), pl->name()});
    }
    return infos;
}

void PlaylistManager::retransmission_playlist_changed()
{
    emit sgn_playlist_changed();
}

void PlaylistManager::switch_to_playlist(const PlaylistId& pid)
{
    m_context->set_playlist(pid);
}

QVector<std::shared_ptr<Playlist>> PlaylistManager::get_playlists()
{
    return m_repo->get_lists();
}

TrackMetaData PlaylistManager::get_current_metadata()
{
    EntryId tid   = m_context->get_play_track_id();
    auto playlist = m_repo->find_playlist_by_id(m_context->get_playlist_id());

    if (playlist) {
        const Track* track = playlist->find_track_by_id(tid);
        if (track) {
            return track->meta;
        }
    }
    TrackMetaData empty_meta;
    empty_meta.isValid = false;
    return empty_meta;
}

QString PlaylistManager::get_playlist_by_id(const PlaylistId& pid) const
{
    auto pl = m_repo->find_playlist_by_id(pid);
    if (!pl || pl->isEmpty()) {
        return QString();
    }
    return pl->name();
}
