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

    void warmup(const PlaylistId& pid) override;
    void invalidate(const PlaylistId& pid) override;
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

    PlaylistId resolvePid(const SearchQuery& query) const;
    void rebuildIndex(const PlaylistId& pid);
    bool ensureIndexReady(const PlaylistId& pid);

    static QString normalize(const QString& text);
    static bool fuzzyMatch(const QString& text, const QString& pattern);
    int scoreTrack(const IndexedTrack& track, const SearchQuery& query,
                   const QString& keyword_norm) const;

private:
    PlaylistController* playlist_controller_ = nullptr;
    QHash<PlaylistId, QVector<IndexedTrack>> m_index_by_playlist;
};
