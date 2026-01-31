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
    void saveList(const QUuid& uuid, const QString& toPath);
    void removeList(const QUuid& uuid);
    void copyList(const QUuid& src);
    std::shared_ptr<Playlist> findPlaylistById(const QUuid& uuid);
    void addTrackToPlaylist(const QUuid& playlistId, const QString& filepath);
    void addTracksToPlaylist(const QUuid& playlistId, const QStringList& filepaths);
    bool isEmpty();
    const QVector<std::shared_ptr<Playlist>>& getLists();

signals:
    void playlistChanged();

private:
    QVector<std::shared_ptr<Playlist>> m_list;
};