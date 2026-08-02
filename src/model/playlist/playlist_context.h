/*
用于管理播放器会话状态，包括
    1. 当前列表
    2. 当前音轨
    3. 播放模式
*/

#pragma once

#include "core/types.h"

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
    void setPlaylist(const PlaylistId& pid);
    void setPlayTrack(const EntryId& tid);

    const PlaylistId& getPlaylistId();
    const EntryId& getPlayTrackId();
    PlayMode getPlayMode();

signals:
    void changedCurrentListId(const PlaylistId& pid);
    void changedCurrentTrackId(const EntryId& tid);
    void changedCurrentPlayMode(const PlayMode& mode);

private:
    PlaylistId m_current_playlist_id;
    EntryId m_current_track_id;
    PlayMode m_mode;
};

namespace PlaylistNavigator
{
EntryId nextOfInOrder(const QVector<EntryId>& queue, EntryId current);
EntryId nextOfLoop(const QVector<EntryId>& queue, EntryId current);
EntryId nextOfOutOfOrderTrack(const QVector<EntryId>& queue, EntryId current);
EntryId nextOfShuffle(const QVector<EntryId>& queue);
EntryId nextOfOutOfOrderGroup(const QVector<EntryId>& queue, EntryId current);

EntryId previousOfInOrder(const QVector<EntryId>& queue, EntryId current);
EntryId previousOfLoop(const QVector<EntryId>& queue, EntryId current);
EntryId previousOfOutOfOrderTrack(const QVector<EntryId>& queue, EntryId current);
EntryId previousOfShuffle(const QVector<EntryId>& queue);
EntryId previousOfOutOfOrderGroup(const QVector<EntryId>& queue, EntryId current);
size_t generate_random_index(size_t max_index);
}; // namespace PlaylistNavigator
