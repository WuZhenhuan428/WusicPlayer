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
    void set_play_mode(PlayMode mode);
    void set_playlist(const PlaylistId& pid);
    void set_play_track(const EntryId& tid);

    const PlaylistId& get_playlist_id();
    const EntryId& get_play_track_id();
    PlayMode get_play_mode();

signals:
    void sgn_current_list_changed(const PlaylistId& pid);
    void sgn_current_track_changed(const EntryId& tid);
    void sgn_current_play_mode_changed(const PlayMode& mode);

private:
    PlaylistId m_current_playlist_id;
    EntryId m_current_track_id;
    PlayMode m_mode;
};

namespace PlaylistNavigator
{
EntryId next_of_in_order(const QVector<EntryId>& queue, EntryId current);
EntryId next_of_loop(const QVector<EntryId>& queue, EntryId current);
EntryId next_of_out_of_order_track(const QVector<EntryId>& queue, EntryId current);
EntryId next_of_shuffle(const QVector<EntryId>& queue);
EntryId next_of_out_of_order_group(const QVector<EntryId>& queue, EntryId current);

EntryId previous_of_in_order(const QVector<EntryId>& queue, EntryId current);
EntryId previous_of_loop(const QVector<EntryId>& queue, EntryId current);
EntryId previous_of_out_of_order_track(const QVector<EntryId>& queue, EntryId current);
EntryId previous_of_shuffle(const QVector<EntryId>& queue);
EntryId previous_of_out_of_order_group(const QVector<EntryId>& queue, EntryId current);
size_t generate_random_index(size_t max_index);
}; // namespace PlaylistNavigator
