#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QDebug>
#include <QUuid>

#include <memory>

#include "playlist.h"
#include "../../include/audio.h"

class PlaylistRepo : public QObject
{
    Q_OBJECT
public:
    explicit PlaylistRepo(QObject *parent = nullptr);
    ~PlaylistRepo();

// Playlist management
public:
    void clearList();
    QUuid createList();
    QUuid loadList(const QString& filepath);
    QUuid loadListBatched(const QString& filepath, int batchSize = 500);
    void saveList(const QUuid& uuid, const QString& toPath);
    void renameList(const QUuid& uuid, const QString& name);
    void removeList(const QUuid& uuid);
    void copyList(const QUuid& src);
    std::shared_ptr<Playlist> findPlaylistById(const QUuid& uuid);
    void addTrackToPlaylist(const QUuid& playlistId, const QString& filepath);
    void addTracksToPlaylist(const QUuid& playlistId, const QStringList& filepaths);
    bool isEmpty();
    const QVector<std::shared_ptr<Playlist>>& getLists();

    void saveListToCache(std::shared_ptr<Playlist> playlist);
    void loadCache();
    void loadCacheAsync();

signals:
    void playlistChanged();
    void cacheLoadStarted();
    void cacheLoadFinished(int playlistCount);
    void playlistLoadStarted(const QUuid& playlistId, int totalCount);
    void playlistBatchLoaded(const QUuid& playlistId, int loadedCount, int totalCount);
    void playlistLoadFinished(const QUuid& playlistId);

private:
    QString cacheFilePath(const QUuid& id) const;
    void loadCacheFromDisk();
    QVector<std::shared_ptr<Playlist>> loadCacheFromDiskToVector() const;
    bool loadJsonPlaylist(const QByteArray& data, const QString& fallbackName, std::shared_ptr<Playlist>& outPlaylist) const;
    bool writeJsonPlaylist(QIODevice& device, const std::shared_ptr<Playlist>& playlist) const;

    QVector<std::shared_ptr<Playlist>> m_list;
    QString m_cacheDir;     // <standard app data dir>/playlists
    static constexpr int kSchemaVersion = 1;
};