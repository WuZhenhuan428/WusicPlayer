#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QDebug>
#include <QUuid>

#include <memory>

#include "playlist.h"

class PlaylistRepo : public QObject
{
    Q_OBJECT
public:
    explicit PlaylistRepo(QObject *parent = nullptr);
    ~PlaylistRepo();

// Playlist management
public:
    void clearList();
    playlistId createList();
    playlistId loadList(const QString& filepath);
    playlistId loadListBatched(const QString& filepath, int batch_size = 500);
    void saveList(const playlistId& pid, const QString& dst_path);
    void renameList(const playlistId& pid, const QString& name);
    void removeList(const playlistId& pid);
    void copyList(const playlistId& src);
    std::shared_ptr<Playlist> findPlaylistById(const playlistId& pid);
    void addTrackToPlaylist(const playlistId& pid, const QString& filepath);
    void addTracksToPlaylist(const playlistId& pid, const QStringList& filepaths);
    bool isEmpty();
    const QVector<std::shared_ptr<Playlist>>& getLists();

    void saveListToCache(std::shared_ptr<Playlist> playlist);
    void loadCache();
    void loadCacheAsync();

signals:
    void playlistChanged();
    void cacheLoadStarted();
    void cacheLoadFinished(int playlist_count);
    void playlistLoadStarted(const playlistId& pid, int total_count);
    void playlistBatchLoaded(const playlistId& pid, int loaded_count, int total_count);
    void playlistLoadFinished(const playlistId& pid);

private:
    QString cacheFilePath(const playlistId& pid) const;
    void loadCacheFromDisk();
    QVector<std::shared_ptr<Playlist>> loadCacheFromDiskToVector() const;
    bool loadJsonPlaylist(const QByteArray& data, const QString& fallbackName, std::shared_ptr<Playlist>& out_playlist) const;
    bool writeJsonPlaylist(QIODevice& device, const std::shared_ptr<Playlist>& playlist) const;

    QVector<std::shared_ptr<Playlist>> m_list;
    QString m_cache_dir;     // <standard app data dir>/playlists
    static constexpr int kSchemaVersion = 1;
};