/*
用于管理播放器会话状态，包括
    1. 当前列表
    2. 当前音轨
    3. 播放模式
*/

#pragma once

#include "../../include/header.h"
#include "playlist.h"

#include <QObject>
#include <QUuid>

class PlaylistContext : public QObject
{
    Q_OBJECT;
public:
    explicit PlaylistContext(QObject* parent = nullptr);
    ~PlaylistContext();

public:
    void setPlayMode(PlayMode mode);
    void setPlaylist(const QUuid& playlistId);
    void setPlayTrack(const QUuid& trackUuid);

    const QUuid& getPlaylistId();
    const QUuid& getPlayTrackId();
    PlayMode getPlayMode();

signals:
    void changedCurrentListId(const QUuid& list_uuid);
    void changedCurrentTrackId(const QUuid& track_uuid);
    void changedCurrentPlayMode(const PlayMode& mode);

private:
    QUuid m_currentPlaylistId;
    QUuid m_currentTrackId;
    PlayMode m_mode;
};
