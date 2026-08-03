#pragma once

#include "core/types.h"
#include <QObject>

class PlaybackController;
class PlaylistController;

class PlaybackRestoreService : public QObject
{
    Q_OBJECT

public:
    explicit PlaybackRestoreService(PlaylistController* playlist_ctl,
                                    PlaybackController* playback_ctl, QObject* parent);
    ~PlaybackRestoreService();

    void restore();

private:
    PlaylistController* m_playlist_ctl = nullptr;
    PlaybackController* m_playback_ctl = nullptr;

private:
    int find_queue_index_by_track_id(const EntryId& tid);
    void finalize_restore_when_ready(int retry);
    void on_cache_load_finished();
    void on_model_reset();

    PlaylistId m_pending_pid;
    EntryId m_pending_tid;
    int m_pending_pos_ms         = 0;
    bool m_pending_should_resume = false;

    bool m_restored              = false;
};
