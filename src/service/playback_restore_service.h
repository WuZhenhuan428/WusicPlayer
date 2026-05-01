#pragma once

#include <QObject>
#include "core/types.h"

class PlaybackController;
class PlaylistController;

class PlaybackRestoreService : public QObject
{
    Q_OBJECT

public:
    explicit PlaybackRestoreService(PlaylistController* playlist_ctl,
                                    PlaybackController* playback_ctl,
                                    QObject* parent);
    ~PlaybackRestoreService();

    void restore();

private:
    PlaylistController* m_playlist_ctl = nullptr;
    PlaybackController* m_playback_ctl = nullptr;

private:
    int findQueueIndexByTrackId(const trackId& tid);
    void finalizeRestoreWhenReady(int retry);
    void onCacheLoadFinished();
    void onModelReset();

    playlistId m_pending_pid;
    trackId m_pending_tid;
    int m_pending_pos_ms = 0;
    bool m_pending_should_resume = false;

    bool m_restored = false;
};