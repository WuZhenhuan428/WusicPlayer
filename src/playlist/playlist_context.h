/*
用于管理播放器会话状态，包括
    1. 当前列表
    2. 当前音轨
    3. 播放模式
*/

#pragma once

#include "playlist.h"

#include <QObject>
#include <QUuid>

class PlaylistContext : public QObject
{
    Q_OBJECT
public:
    explicit PlaylistContext(QObject* parent = nullptr);
    ~PlaylistContext();

public:
    void setPlayMode(PlayMode mode);
    void setPlaylist(const playlistId& pid);
    void setPlayTrack(const trackId& tid);

    const playlistId& getPlaylistId();
    const trackId& getPlayTrackId();
    PlayMode getPlayMode();

signals:
    void changedCurrentListId(const playlistId& pid);
    void changedCurrentTrackId(const trackId& tid);
    void changedCurrentPlayMode(const PlayMode& mode);

private:
    playlistId m_currentPlaylistId;
    trackId m_currentTrackId;
    PlayMode m_mode;
};
