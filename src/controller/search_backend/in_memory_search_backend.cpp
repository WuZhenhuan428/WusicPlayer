#include "in_memory_search_backend.h"

#include "controller/PlaylistController.h"
#include "model/playlist/playlist.h"

#include <algorithm>

namespace {

struct RankedHit
{
    SearchHint hint;
    int score = -1;
};

bool byScoreDesc(const RankedHit& lhs, const RankedHit& rhs) {
    if (lhs.score != rhs.score) {
        return lhs.score > rhs.score;
    }
    return lhs.hint.title.localeAwareCompare(rhs.hint.title) < 0;
}

int clampScore(int score) {
    if (score < 0) {
        return 0;
    }
    if (score > 255) {
        return 255;
    }
    return score;
}

} // namespace

InMemorySearchBackend::InMemorySearchBackend(PlaylistController* playlist_controller)
    : m_playlist_controller(playlist_controller)
{
}

void InMemorySearchBackend::warmup(const playlistId& pid) {
    if (!m_playlist_controller) {
        return;
    }

    if (pid.isNull()) {
        const playlistId current = m_playlist_controller->currentPlaylist();
        if (!current.isNull()) {
            rebuildIndex(current);
        }
        return;
    }

    rebuildIndex(pid);
}

void InMemorySearchBackend::invalidate(const playlistId& pid) {
    if (pid.isNull()) {
        m_index_by_playlist.clear();
        return;
    }
    m_index_by_playlist.remove(pid);
}

QVector<SearchHint> InMemorySearchBackend::search(const SearchQuery& query) {
    QVector<SearchHint> hits;

    if (!m_playlist_controller) {
        return hits;
    }

    const playlistId pid = resolvePid(query);
    if (pid.isNull()) {
        return hits;
    }
    if (!ensureIndexReady(pid)) {
        return hits;
    }

    const QString keyword = query.keyword.trimmed();
    if (keyword.isEmpty()) {
        return hits;
    }

    const QString keyword_norm = normalize(keyword);
    const QVector<IndexedTrack>& index = m_index_by_playlist.value(pid);

    QVector<RankedHit> ranked_hits;
    ranked_hits.reserve(index.size());

    for (const IndexedTrack& track : index) {
        const int score = scoreTrack(track, query, keyword_norm);
        if (score < 0) {
            continue;
        }

        RankedHit ranked;
        ranked.hint = track.hint;
        ranked.score = score;
        ranked.hint.score = static_cast<unsigned char>(clampScore(score));
        ranked_hits.push_back(std::move(ranked));
    }

    std::sort(ranked_hits.begin(), ranked_hits.end(), byScoreDesc);

    hits.reserve(ranked_hits.size());
    for (const RankedHit& ranked_hit : ranked_hits) {
        hits.push_back(ranked_hit.hint);
    }

    return hits;
}

playlistId InMemorySearchBackend::resolvePid(const SearchQuery& query) const {
    if (!query.pid.isNull()) {
        return query.pid;
    }
    if (!m_playlist_controller) {
        return playlistId{};
    }
    return m_playlist_controller->currentPlaylist();
}

void InMemorySearchBackend::rebuildIndex(const playlistId& pid) {
    if (!m_playlist_controller || pid.isNull()) {
        return;
    }

    std::shared_ptr<Playlist> playlist = m_playlist_controller->findPlaylistById(pid);
    if (!playlist) {
        m_index_by_playlist.remove(pid);
        return;
    }

    QVector<IndexedTrack> indexed_tracks;
    const QVector<Track>& tracks = playlist->getTracks();
    indexed_tracks.reserve(tracks.size());

    for (const Track& track : tracks) {
        IndexedTrack indexed;
        indexed.hint.track_id = track.tid;
        indexed.hint.title = track.meta.title;
        indexed.hint.artist = track.meta.artist;
        indexed.hint.album_artist = track.meta.album_artist;
        indexed.hint.album = track.meta.album;
        indexed.hint.duration_s = track.meta.duration_s;
        indexed.hint.score = 0;

        indexed.title_norm = normalize(indexed.hint.title);
        indexed.artist_norm = normalize(indexed.hint.artist);
        indexed.album_artist_norm = normalize(indexed.hint.album_artist);
        indexed.album_norm = normalize(indexed.hint.album);

        indexed_tracks.push_back(std::move(indexed));
    }

    m_index_by_playlist.insert(pid, std::move(indexed_tracks));
}

bool InMemorySearchBackend::ensureIndexReady(const playlistId& pid) {
    if (m_index_by_playlist.contains(pid)) {
        return true;
    }

    rebuildIndex(pid);
    return m_index_by_playlist.contains(pid);
}

QString InMemorySearchBackend::normalize(const QString& text) {
    return text.trimmed().toCaseFolded();
}

bool InMemorySearchBackend::fuzzyMatch(const QString& text, const QString& pattern) {
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

int InMemorySearchBackend::scoreTrack(const IndexedTrack& track, const SearchQuery& query, const QString& keyword_norm) const {
    const bool case_sensitive = query.case_sensitive;
    const QString keyword = case_sensitive ? query.keyword.trimmed() : keyword_norm;

    const QString title = case_sensitive ? track.hint.title : track.title_norm;
    const QString artist = case_sensitive ? track.hint.artist : track.artist_norm;
    const QString album_artist = case_sensitive ? track.hint.album_artist : track.album_artist_norm;
    const QString album = case_sensitive ? track.hint.album : track.album_norm;

    auto prefix_score = [&keyword](const QString& field, int base) -> int {
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
        score = std::max(score, prefix_score(title, 120));
        score = std::max(score, prefix_score(artist, 90));
        score = std::max(score, prefix_score(album_artist, 85));
        score = std::max(score, prefix_score(album, 80));
        return score;
    }

    case SearchQueryMode::Fuzzy: {
        int score = -1;
        if (fuzzyMatch(title, keyword)) score = std::max(score, 85);
        if (fuzzyMatch(artist, keyword)) score = std::max(score, 70);
        if (fuzzyMatch(album_artist, keyword)) score = std::max(score, 65);
        if (fuzzyMatch(album, keyword)) score = std::max(score, 60);
        return score;
    }

    case SearchQueryMode::Plain:
    default: {
        int score = -1;
        score = std::max(score, contains_score(title, 100));
        score = std::max(score, contains_score(artist, 80));
        score = std::max(score, contains_score(album_artist, 75));
        score = std::max(score, contains_score(album, 70));
        return score;
    }
    }
}
