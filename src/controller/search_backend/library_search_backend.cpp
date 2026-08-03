#include "library_search_backend.h"

#include "model/library/library_manager.h"

namespace
{
constexpr int kMaxResults = 200;
}

LibrarySearchBackend::LibrarySearchBackend(LibraryManager* library) : library_(library) {}

void LibrarySearchBackend::warmup(const PlaylistId&)
{
    // 库索引由 LibraryManager 持续维护,无需预热
}

void LibrarySearchBackend::invalidate(const PlaylistId&)
{
    // 库索引由 LibraryManager 持续维护,无需失效
}

QVector<SearchHint> LibrarySearchBackend::search(const SearchQuery& query)
{
    QVector<SearchHint> hints;
    if (!library_) {
        return hints;
    }
    const QString keyword = query.keyword.trimmed();
    if (keyword.isEmpty()) {
        return hints;
    }

    const auto tracks = library_->search(keyword, query.mode, kMaxResults);
    hints.reserve(tracks.size());
    for (const auto& t : tracks) {
        SearchHint h;
        h.track_id     = t.track_id;
        h.title        = t.meta.title;
        h.artist       = t.meta.artist;
        h.album_artist = t.meta.album_artist;
        h.album        = t.meta.album;
        h.duration_s   = t.meta.duration_s;
        h.score        = 0;

        // FTS5 默认大小写不敏感;case_sensitive 时后置精确过滤
        if (query.case_sensitive) {
            const bool hit = h.title.contains(keyword) || h.artist.contains(keyword) ||
                             h.album_artist.contains(keyword) || h.album.contains(keyword);
            if (!hit) {
                continue;
            }
        }
        hints.append(h);
    }
    return hints;
}
