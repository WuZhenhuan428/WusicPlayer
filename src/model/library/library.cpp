#include "library.h"

void Library::upsert(const LibraryTrack& track)
{
    const QString old_path = m_id_to_path.value(track.track_id);
    if (!old_path.isEmpty() && old_path != track.filepath) {
        m_by_path.remove(old_path); // 同一 TrackId 换了路径(重命名迁移预留)
    }
    m_by_path.insert(track.filepath, track);
    m_id_to_path.insert(track.track_id, track.filepath);
}

void Library::remove_by_path(const QString& normalized_path)
{
    const auto it = m_by_path.find(normalized_path);
    if (it == m_by_path.end()) {
        return;
    }
    m_id_to_path.remove(it->track_id);
    m_by_path.erase(it);
}

bool Library::mark_missing(const QString& normalized_path, bool missing)
{
    auto it = m_by_path.find(normalized_path);
    if (it == m_by_path.end()) {
        return false;
    }
    it->missing = missing;
    return true;
}

void Library::clear()
{
    m_by_path.clear();
    m_id_to_path.clear();
}

std::optional<LibraryTrack> Library::track_by_path(const QString& normalized_path) const
{
    const auto it = m_by_path.constFind(normalized_path);
    if (it == m_by_path.constEnd()) {
        return std::nullopt;
    }
    return it.value();
}

std::optional<LibraryTrack> Library::track_by_id(TrackId id) const
{
    const auto it = m_id_to_path.constFind(id);
    if (it == m_id_to_path.constEnd()) {
        return std::nullopt;
    }
    return track_by_path(it.value());
}

const QHash<QString, LibraryTrack>& Library::index() const
{
    return m_by_path;
}

int Library::track_count() const
{
    return m_by_path.size();
}
