#include "controller/search_backend/in_memory_search_backend.h"

#include "controller/playlist_controller.h"
#include "model/playlist/playlist.h"

#include <algorithm>

namespace
{

struct RankedHit
{
    SearchHint hint;
    int score = -1;
};

bool by_score_desc(const RankedHit& lhs, const RankedHit& rhs)
{
    if (lhs.score != rhs.score) {
        return lhs.score > rhs.score;
    }
    return lhs.hint.title.localeAwareCompare(rhs.hint.title) < 0;
}

int clamp_score(int score)
{
    if (score < 0) {
        return 0;
    }
    if (score > 255) {
        return 255;
    }
    return score;
}

} // namespace

InMemorySearchBackend::InMemorySearchBackend(PlaylistController* playlist_controller) :
    playlist_controller_(playlist_controller)
{}

void InMemorySearchBackend::warmup(const PlaylistId& pid)
{
    if (!playlist_controller_) {
        return;
    }

    if (pid.is_null()) {
        const PlaylistId current = playlist_controller_->current_playlist_id();
        if (!current.is_null()) {
            rebuild_index(current);
        }
        return;
    }

    rebuild_index(pid);
}

void InMemorySearchBackend::invalidate(const PlaylistId& pid)
{
    if (pid.is_null()) {
        m_index_by_playlist.clear();
        return;
    }
    m_index_by_playlist.remove(pid);
}

QVector<SearchHint> InMemorySearchBackend::search(const SearchQuery& query)
{
    QVector<SearchHint> hits;

    if (!playlist_controller_) {
        return hits;
    }

    const PlaylistId pid = resolve_pid(query);
    if (pid.is_null()) {
        return hits;
    }
    if (!ensure_index_ready(pid)) {
        return hits;
    }

    const QString keyword = query.keyword.trimmed();
    if (keyword.isEmpty()) {
        return hits;
    }

    const QString keyword_norm         = normalize(keyword);
    const QVector<IndexedTrack>& index = m_index_by_playlist.value(pid);

    QVector<RankedHit> ranked_hits;
    ranked_hits.reserve(index.size());

    for (const IndexedTrack& track : index) {
        const int score = score_track(track, query, keyword_norm);
        if (score < 0) {
            continue;
        }

        RankedHit ranked;
        ranked.hint       = track.hint;
        ranked.score      = score;
        ranked.hint.score = static_cast<unsigned char>(clamp_score(score));
        ranked_hits.push_back(std::move(ranked));
    }

    std::sort(ranked_hits.begin(), ranked_hits.end(), by_score_desc);

    hits.reserve(ranked_hits.size());
    for (const RankedHit& ranked_hit : ranked_hits) {
        hits.push_back(ranked_hit.hint);
    }

    return hits;
}

PlaylistId InMemorySearchBackend::resolve_pid(const SearchQuery& query) const
{
    if (!query.pid.is_null()) {
        return query.pid;
    }
    if (!playlist_controller_) {
        return PlaylistId{};
    }
    return playlist_controller_->current_playlist_id();
}

void InMemorySearchBackend::rebuild_index(const PlaylistId& pid)
{
    if (!playlist_controller_ || pid.is_null()) {
        return;
    }

    std::shared_ptr<Playlist> playlist = playlist_controller_->find_playlist_by_id(pid);
    if (!playlist) {
        m_index_by_playlist.remove(pid);
        return;
    }

    QVector<IndexedTrack> indexed_tracks;
    const QVector<Track>& tracks = playlist->get_tracks();
    indexed_tracks.reserve(tracks.size());

    for (const Track& track : tracks) {
        IndexedTrack indexed;
        // 阶段 5 后 track_id 为库级身份:库引用条目填库 id,外部条目留空
        indexed.hint.track_id     = track.library_track_id;
        // filepath 为播放依据(所有条目都有;AppController 据此直接播放)
        indexed.hint.filepath     = track.filepath;
        indexed.hint.title        = track.meta.title;
        indexed.hint.artist       = track.meta.artist;
        indexed.hint.album_artist = track.meta.album_artist;
        indexed.hint.album        = track.meta.album;
        indexed.hint.duration_s   = track.meta.duration_s;
        indexed.hint.score        = 0;

        indexed.title_norm        = normalize(indexed.hint.title);
        indexed.artist_norm       = normalize(indexed.hint.artist);
        indexed.album_artist_norm = normalize(indexed.hint.album_artist);
        indexed.album_norm        = normalize(indexed.hint.album);

        indexed_tracks.push_back(std::move(indexed));
    }

    m_index_by_playlist.insert(pid, std::move(indexed_tracks));
}

bool InMemorySearchBackend::ensure_index_ready(const PlaylistId& pid)
{
    if (m_index_by_playlist.contains(pid)) {
        return true;
    }

    rebuild_index(pid);
    return m_index_by_playlist.contains(pid);
}

QString InMemorySearchBackend::normalize(const QString& text)
{
    return text.trimmed().toCaseFolded();
}

bool InMemorySearchBackend::fuzzy_match(const QString& text, const QString& pattern)
{
    if (pattern.isEmpty()) {
        return true;
    }

    int pattern_index = 0;
    for (const QChar ch : text) {
        if (ch == pattern[pattern_index]) {
            ++pattern_index;
            if (pattern_index >= pattern.size()) {
                return true;
            }
        }
    }

    return false;
}

int InMemorySearchBackend::score_track(const IndexedTrack& track, const SearchQuery& query,
                                       const QString& keyword_norm) const
{
    const bool case_sensitive  = query.case_sensitive;
    const QString keyword      = case_sensitive ? query.keyword.trimmed() : keyword_norm;

    const QString title        = case_sensitive ? track.hint.title : track.title_norm;
    const QString artist       = case_sensitive ? track.hint.artist : track.artist_norm;
    const QString album_artist = case_sensitive ? track.hint.album_artist : track.album_artist_norm;
    const QString album        = case_sensitive ? track.hint.album : track.album_norm;

    auto prefix_score          = [&keyword](const QString& field, int base) -> int {
        if (!field.isEmpty() && field.startsWith(keyword)) {
            return base;
        }
        return -1;
    };

    auto contains_score = [&keyword](const QString& field, int base) -> int {
        if (!field.isEmpty() && field.contains(keyword)) {
            return base;
        }
        return -1;
    };

    switch (query.mode) {
    case SearchQueryMode::Prefix: {
        int score = -1;
        score     = std::max(score, prefix_score(title, 120));
        score     = std::max(score, prefix_score(artist, 90));
        score     = std::max(score, prefix_score(album_artist, 85));
        score     = std::max(score, prefix_score(album, 80));
        return score;
    }

    case SearchQueryMode::Fuzzy: {
        int score = -1;
        if (fuzzy_match(title, keyword))
            score = std::max(score, 85);
        if (fuzzy_match(artist, keyword))
            score = std::max(score, 70);
        if (fuzzy_match(album_artist, keyword))
            score = std::max(score, 65);
        if (fuzzy_match(album, keyword))
            score = std::max(score, 60);
        return score;
    }

    case SearchQueryMode::Plain:
    default: {
        int score = -1;
        score     = std::max(score, contains_score(title, 100));
        score     = std::max(score, contains_score(artist, 80));
        score     = std::max(score, contains_score(album_artist, 75));
        score     = std::max(score, contains_score(album, 70));
        return score;
    }
    }
}
