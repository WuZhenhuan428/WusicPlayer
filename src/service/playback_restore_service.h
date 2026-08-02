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
    int findQueueIndexByTrackId(const EntryId& tid);
    void finalizeRestoreWhenReady(int retry);
    void onCacheLoadFinished();
    void onModelReset();

    PlaylistId m_pending_pid;
    EntryId m_pending_tid;
    int m_pending_pos_ms         = 0;
    bool m_pending_should_resume = false;

    bool m_restored              = false;
};
