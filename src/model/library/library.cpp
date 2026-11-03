#include "library.h"

void Library::upsert(const LibraryTrack& track)
{
    const QString old_path = id_to_path_.value(track.track_id);
    if (!old_path.isEmpty() && old_path != track.filepath) {
        by_path_.remove(old_path); // 同一 TrackId 换了路径(重命名迁移预留)
    }
    by_path_.insert(track.filepath, std::make_shared<const LibraryTrack>(track));
    id_to_path_.insert(track.track_id, track.filepath);
}

void Library::remove_by_path(const QString& normalized_path)
{
    const auto it = by_path_.find(normalized_path);
    if (it == by_path_.end()) {
        return;
    }
    id_to_path_.remove(it.value()->track_id);
    by_path_.erase(it);
}

bool Library::mark_missing(const QString& normalized_path, bool missing)
{
    auto it = by_path_.find(normalized_path);
    if (it == by_path_.end()) {
        return false;
    }
    if (it.value()->missing == missing) {
        return true;
    }
    // 共享对象为 const, 需重建以更新缺失标记(低频操作, 可接受)
    auto copy                 = std::make_shared<LibraryTrack>(*it.value());
    copy->missing             = missing;
    by_path_[normalized_path] = std::move(copy);
    return true;
}

void Library::clear()
{
    by_path_.clear();
    id_to_path_.clear();
}

std::shared_ptr<const LibraryTrack> Library::track_by_path(const QString& normalized_path) const
{
    const auto it = by_path_.constFind(normalized_path);
    if (it == by_path_.constEnd()) {
        return nullptr;
    }
    return it.value();
}

std::shared_ptr<const LibraryTrack> Library::track_by_id(TrackId id) const
{
    const auto it = id_to_path_.constFind(id);
    if (it == id_to_path_.constEnd()) {
        return nullptr;
    }
    return track_by_path(it.value());
}

const QHash<QString, std::shared_ptr<const LibraryTrack>>& Library::index() const
{
    return by_path_;
}

int Library::track_count() const
{
    return static_cast<int>(by_path_.size());
}
