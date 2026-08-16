#include "model/playlist/playlist.h"

#include <QFileInfo>

#include "core/logger/logger_manager.h"
namespace
{
Logger* logger = LoggerManager::file_logger("playlist", {"console", "gui"});
}

/**
 * @brief: 创建播放列表时生成UUID
 */
Playlist::Playlist(const QString& name)
{
    m_name = name;
    m_pid  = PlaylistId::create_uuid();

    logger->info("Create playlist uuid: {}", m_pid.to_string());
}

Playlist::~Playlist()
{
    logger->info("Remove playlist uuid: {}", m_pid.to_string());
}

/**
 * @brief: 清除列表内容并回收空间
 */
void Playlist::clear_list()
{
    m_tracks.clear();
    m_tracks.shrink_to_fit();
    m_track_index.clear();
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
    m_track_index.insert(track.entry_id, m_tracks.size());
    m_tracks.emplace_back(track);
}

bool Playlist::update_track_meta(const EntryId& eid, const TrackMetaData& meta)
{
    auto it = m_track_index.constFind(eid);
    if (it == m_track_index.constEnd()) {
        return false;
    }

    Track& t = m_tracks[it.value()];
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
    auto it = m_track_index.constFind(eid);
    if (it == m_track_index.constEnd()) {
        return false;
    }
    m_tracks[it.value()].missing = missing;
    return true;
}

int Playlist::refresh_library_tracks(
    const std::function<std::optional<LibraryTrack>(const TrackId&)>& resolver)
{
    int updated = 0;
    for (auto& t : m_tracks) {
        if (t.source != TrackSource::library) {
            continue;
        }
        const auto lib = resolver(t.library_track_id);
        if (lib) {
            t.meta    = lib->meta;
            t.missing = lib->missing;
        } else {
            t.missing = true; // 库中已无该曲目 → 标记缺失
        }
        ++updated;
    }
    return updated;
}

int Playlist::upgrade_external_tracks(
    const std::function<std::optional<LibraryTrack>(const QString& path)>& resolver)
{
    int upgraded = 0;
    for (auto& t : m_tracks) {
        if (t.source == TrackSource::library) {
            continue;
        }
        const auto lib = resolver(t.filepath);
        if (!lib) {
            continue;
        }
        t.source           = TrackSource::library;
        t.library_track_id = lib->track_id;
        t.meta             = lib->meta;
        t.missing          = lib->missing;
        ++upgraded;
    }
    return upgraded;
}

int Playlist::remove_missing_tracks()
{
    int removed = 0;
    for (auto it = m_tracks.begin(); it != m_tracks.end();) {
        if (it->missing) {
            it = m_tracks.erase(it);
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
    auto it = m_track_index.constFind(eid);
    if (it == m_track_index.constEnd()) {
        return;
    }
    const qsizetype idx = it.value();
    const QString path  = m_tracks[idx].filepath;
    m_tracks.erase(m_tracks.begin() + idx);
    m_track_index.erase(it);

    for (auto it = m_track_index.begin(); it != m_track_index.end(); ++it) {
        if (it.value() > idx) {
            --it.value();
        }
    }

    logger->info("Remove UUID={}, filepath={}", eid.to_string(), path);
};

/**
 * @return: 检查播放列表是否为空并返回bool
 */
bool Playlist::isEmpty()
{
    return m_tracks.empty();
};

PlaylistId Playlist::id() const
{
    return m_pid;
}

QString Playlist::name()
{
    return m_name;
}

void Playlist::set_playlist_name(QString setname)
{
    m_name = setname;
}

void Playlist::new_uuid()
{
    m_pid = PlaylistId::create_uuid();
}

void Playlist::new_uuid(const PlaylistId& pid)
{
    m_pid = pid;
}

const Track* Playlist::find_track_by_id(const EntryId& eid) const
{
    auto it = m_track_index.constFind(eid);
    if (it == m_track_index.constEnd()) {
        logger->warn("track {} does not exist!", eid.to_string());
        return nullptr;
    }
    logger->info("find track {} at playlist {}", eid.to_string(), m_name);
    return &m_tracks[it.value()];
}

const Track* Playlist::find_track_by_filepath(const QString& filepath) const
{
    const QString norm = utils::path::normalize_path(filepath);
    for (auto it = m_tracks.begin(); it != m_tracks.end(); ++it) {
        if (utils::path::normalize_path(it->filepath) == norm) {
            return &(*it);
        }
    }
    return nullptr;
}

const QVector<Track>& Playlist::get_tracks() const
{
    return m_tracks;
}

size_t Playlist::track_count()
{
    return m_tracks.size();
}

void Playlist::rebuild_index()
{
    m_track_index.clear();
    m_track_index.reserve(m_tracks.size());
    for (qsizetype i = 0; i < m_tracks.size(); ++i) {
        m_track_index.insert(m_tracks[i].entry_id, i);
    }
}
