#pragma once

#include "i_search_backend.h"

#include <QHash>
#include <QString>
#include <QVector>

class PlaylistController;

class InMemorySearchBackend : public ISearchBackend
{
public:
    explicit InMemorySearchBackend(PlaylistController* playlist_controller);
    ~InMemorySearchBackend() override = default;

    void warmup(const playlistId& pid) override;
    void invalidate(const playlistId& pid) override;
    QVector<SearchHint> search(const SearchQuery& query) override;

private:
    struct IndexedTrack
    {
        SearchHint hint;
        QString title_norm;
        QString artist_norm;
        QString album_artist_norm;
        QString album_norm;
    };

    playlistId resolvePid(const SearchQuery& query) const;
    void rebuildIndex(const playlistId& pid);
    bool ensureIndexReady(const playlistId& pid);

    static QString normalize(const QString& text);
    static bool fuzzyMatch(const QString& text, const QString& pattern);
    int scoreTrack(const IndexedTrack& track, const SearchQuery& query, const QString& keyword_norm) const;

private:
    PlaylistController* m_playlist_controller = nullptr;
    QHash<playlistId, QVector<IndexedTrack>> m_index_by_playlist;
};
