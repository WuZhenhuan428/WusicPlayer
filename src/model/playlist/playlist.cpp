#include "model/playlist/playlist.h"

#include <QFileInfo>

#include "core/logger/logger_manager.h"
namespace
{
// NOLINTNEXTLINE
Logger* logger = LoggerManager::file_logger("playlist", {"console", "gui"});
} // namespace

/**
 * @brief: 创建播放列表时生成UUID
 */
Playlist::Playlist(const QString& name)
{
    name_ = name;
    pid_  = PlaylistId::create_uuid();

    logger->info("Create playlist uuid: {}", pid_.to_string());
}

Playlist::~Playlist()
{
    logger->info("Remove playlist uuid: {}", pid_.to_string());
}

/**
 * @brief: 清除列表内容并回收空间
 */
void Playlist::clear_list()
{
    tracks_.clear();
    tracks_.shrink_to_fit();
    track_index_.clear();
}
/**
 * @brief: 添加音轨, 原来为空时自动指向第一首
 * @return: 所添加音轨的Uuid
 */
Track Playlist::add_track(const QString& filepath)
{
    Track t = Track::from_filepath(filepath);
    add_track_object(t);
    return t;
}

Track Playlist::add_track_with_id(const EntryId& eid, const QString& filepath)
{
    Track t = Track::from_entry(eid, filepath);
    add_track_object(t);
    return t;
}

void Playlist::add_track_object(const Track& track)
{
    track_index_.insert(track.entry_id, tracks_.size());
    tracks_.emplace_back(track);
}

bool Playlist::update_track_meta(const EntryId& eid, const TrackMetaData& meta)
{
    auto it = track_index_.constFind(eid);
    if (it == track_index_.constEnd()) {
        return false;
    }

    Track& t = tracks_[it.value()];
    t.meta   = meta;
    if (t.meta.filepath.isEmpty()) {
        t.meta.filepath = t.filepath;
    }
    if (t.meta.filename.isEmpty()) {
        t.meta.filename = QFileInfo(t.filepath).fileName();
    }
    return true;
}

bool Playlist::set_track_missing(const EntryId& eid, bool missing)
{
    auto it = track_index_.constFind(eid);
    if (it == track_index_.constEnd()) {
        return false;
    }
    tracks_[it.value()].missing = missing;
    return true;
}

int Playlist::refresh_library_tracks(
    const std::function<std::shared_ptr<const LibraryTrack>(const TrackId&)>& resolver,
    QVector<EntryId>* meta_changed_out, QVector<EntryId>* missing_changed_out)
{
    // 返回"真正发生变化"的条目数(而非遍历数): 库每次扫描都会回调本方法,
    // 若把遍历数当变化数, 会导致订阅者无条件重建视图。
    int updated = 0;
    for (auto& t : tracks_) {
        if (t.source != TrackSource::Library) {
            continue;
        }
        const auto lib = resolver(t.library_track_id);
        if (lib) {
            bool changed = false;
            if (!(t.meta == lib->meta)) {
                t.meta  = lib->meta;
                changed = true;
                if (meta_changed_out) {
                    meta_changed_out->append(t.entry_id);
                }
            }
            if (t.missing != lib->missing) {
                t.missing = lib->missing;
                changed   = true;
                if (missing_changed_out) {
                    missing_changed_out->append(t.entry_id);
                }
            }
            if (changed) {
                ++updated;
            }
        } else if (!t.missing) {
            t.missing = true; // 库中已无该曲目 → 标记缺失
            ++updated;
            if (missing_changed_out) {
                missing_changed_out->append(t.entry_id);
            }
        }
    }
    return updated;
}

int Playlist::upgrade_external_tracks(
    const std::function<std::shared_ptr<const LibraryTrack>(const QString& path)>& resolver)
{
    int upgraded = 0;
    for (auto& t : tracks_) {
        if (t.source == TrackSource::Library) {
            continue;
        }
        const auto lib = resolver(t.filepath);
        if (!lib) {
            continue;
        }
        t.source           = TrackSource::Library;
        t.library_track_id = lib->track_id;
        t.meta             = lib->meta;
        t.missing          = lib->missing;
        ++upgraded;
    }
    return upgraded;
}

int Playlist::remove_missing_tracks(QVector<EntryId>* removed_out)
{
    int removed = 0;
    for (auto it = tracks_.begin(); it != tracks_.end();) {
        // 判定"缺失": 显式 missing 标记, 或文件当前实际不存在。
        // 不能只依赖 missing 标志——它可能过时(被库刷新覆盖)或对
        // 外部条目从未设置(文件被删后仍残留, 双击即崩溃)。
        const bool gone =
            !it->filepath.isEmpty() && (it->missing || !QFileInfo(it->filepath).exists());
        if (gone) {
            if (removed_out) {
                removed_out->append(it->entry_id);
            }
            it = tracks_.erase(it);
            ++removed;
        } else {
            ++it;
        }
    }
    this->rebuild_index();
    return removed;
}

/**
 * @brief: 查找并删除音轨
 * @note: 如果删除当前音轨, 则暂停播放
 */
void Playlist::remove_track(const EntryId& eid)
{
    auto it = track_index_.constFind(eid);
    if (it == track_index_.constEnd()) {
        return;
    }
    const qsizetype idx = it.value();
    const QString path  = tracks_[idx].filepath;
    tracks_.erase(tracks_.begin() + idx);
    track_index_.erase(it);

    for (auto it = track_index_.begin(); it != track_index_.end(); ++it) {
        if (it.value() > idx) {
            --it.value();
        }
    }

    logger->info("Remove UUID={}, filepath={}", eid.to_string(), path);
};

/**
 * @return: 检查播放列表是否为空并返回bool
 */
bool Playlist::empty()
{
    return tracks_.empty();
};

PlaylistId Playlist::id() const
{
    return pid_;
}

QString Playlist::name()
{
    return name_;
}

void Playlist::set_playlist_name(QString setname)
{
    name_ = std::move(setname);
}

void Playlist::new_uuid()
{
    pid_ = PlaylistId::create_uuid();
}

void Playlist::new_uuid(const PlaylistId& pid)
{
    pid_ = pid;
}

const Track* Playlist::find_track_by_id(const EntryId& eid) const
{
    auto it = track_index_.constFind(eid);
    if (it == track_index_.constEnd()) {
        logger->warn("track {} does not exist!", eid.to_string());
        return nullptr;
    }
    logger->info("find track {} at playlist \'{}\'", eid.to_string(), name_);
    return &tracks_[it.value()];
}

const Track* Playlist::find_track_by_filepath(const QString& filepath) const
{
    const QString norm = utils::path::normalize_path(filepath);
    for (auto it = tracks_.begin(); it != tracks_.end(); ++it) {
        if (utils::path::normalize_path(it->filepath) == norm) {
            return &(*it);
        }
    }
    return nullptr;
}

const QVector<Track>& Playlist::get_tracks() const
{
    return tracks_;
}

size_t Playlist::track_count()
{
    return tracks_.size();
}

void Playlist::rebuild_index()
{
    track_index_.clear();
    track_index_.reserve(tracks_.size());
    for (qsizetype i = 0; i < tracks_.size(); ++i) {
        track_index_.insert(tracks_[i].entry_id, i);
    }
}
