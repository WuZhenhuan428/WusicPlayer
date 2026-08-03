#pragma once

#include "playlist.h"

#include <QDebug>
#include <QObject>
#include <QString>
#include <QUuid>
#include <QVector>

#include <memory>
#include <utility>

class PlaylistRepo : public QObject
{
    Q_OBJECT
public:
    explicit PlaylistRepo(QObject* parent = nullptr);
    ~PlaylistRepo();

    // Playlist management
public:
    void clearList();
    PlaylistId createList();
    PlaylistId loadList(const QString& filepath);
    PlaylistId loadListBatched(const QString& filepath, int batch_size = 500);
    void saveList(const PlaylistId& pid, const QString& dst_path);
    void renameList(const PlaylistId& pid, const QString& name);
    void removeList(const PlaylistId& pid);
    void copyList(const PlaylistId& src);
    std::shared_ptr<Playlist> findPlaylistById(const PlaylistId& pid);
    void addTrackToPlaylist(const PlaylistId& pid, const QString& filepath);
    void addTracksToPlaylist(const PlaylistId& pid, const QStringList& filepaths);
    void addTrackObject(const PlaylistId& pid, const Track& track);
    void addTrackObjects(const PlaylistId& pid, const QVector<Track>& tracks);
    bool isEmpty();
    const QVector<std::shared_ptr<Playlist>>& getLists();

    void saveListToCache(std::shared_ptr<Playlist> playlist);
    void loadCache();
    void loadCacheAsync();

signals:
    void playlistChanged();
    void cacheLoadStarted();
    void cacheLoadFinished(int playlist_count);
    void playlistLoadStarted(const PlaylistId& pid, int total_count);
    void playlistBatchLoaded(const PlaylistId& pid, int loaded_count, int total_count);
    void playlistLoadFinished(const PlaylistId& pid);

private:
    QString cacheFilePath(const PlaylistId& pid) const;
    void loadCacheFromDisk();
    QVector<std::pair<std::shared_ptr<Playlist>, bool>> loadCacheFromDiskToVector() const;
    bool loadJsonPlaylist(const QByteArray& data, const QString& fallbackName,
                          std::shared_ptr<Playlist>& out_playlist,
                          bool* out_legacy_format = nullptr) const;
    bool writeJsonPlaylist(QIODevice& device, const std::shared_ptr<Playlist>& playlist) const;

    QVector<std::shared_ptr<Playlist>> m_list;
    QString m_cache_dir; // <standard app data dir>/playlists
    static constexpr int kSchemaVersion = 1;
};
