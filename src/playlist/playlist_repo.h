#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QDebug>
#include <QUuid>

#include <memory>

#include "playlist.h"
#include "../../include/header.h"
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
    void removeList(QUuid& uuid);
    void copyList(const QUuid& src);
    std::shared_ptr<Playlist> findPlaylistById(const QUuid& uuid);
    void addTrackToPlaylist(const QUuid& playlistId, const QString& filepath);
    bool isEmpty();

signals:
    void playlistChanged();

private:
    QVector<std::shared_ptr<Playlist>> m_list;
};