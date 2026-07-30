#include "playback_restore_service.h"

#include "controller/PlaybackController.h"
#include "controller/PlaylistController.h"

PlaybackRestoreService::PlaybackRestoreService(PlaylistController* playlist_ctl,
                                               PlaybackController* playback_ctl, QObject* parent) :
    QObject(parent), m_playlist_ctl(playlist_ctl), m_playback_ctl(playback_ctl)
{
    assert(m_playlist_ctl && m_playback_ctl);
}

PlaybackRestoreService::~PlaybackRestoreService() {}

void PlaybackRestoreService::restore()
{
    if (!m_playlist_ctl || !m_playback_ctl) {
        qFatal() << "[PlaybackRestoreService] !m_playlist_ctl || !m_playback_ctl";
        return;
    }

    if (m_restored == true)
        return;

    m_pending_pid           = m_playlist_ctl->lastPlaylistId();
    m_pending_tid           = m_playlist_ctl->lastTrackId();
    m_pending_pos_ms        = m_playback_ctl->lastPositionMs();
    m_pending_should_resume = m_playback_ctl->lastWasPlaying();

    if (m_pending_pid.isNull())
        return;

    connect(m_playlist_ctl, &PlaylistController::cacheLoadFinished, this,
            &PlaybackRestoreService::onCacheLoadFinished, Qt::SingleShotConnection);

    m_restored = true;
}

int PlaybackRestoreService::findQueueIndexByTrackId(const trackId& tid)
{
    if (tid.isNull() || !m_playlist_ctl->viewModel()) {
        return -1;
    }
    const auto& queue = m_playlist_ctl->viewModel()->playbackQueue();
    return queue.indexOf(tid);
}

void PlaybackRestoreService::finalizeRestoreWhenReady(int retry)
{
    if (retry > 30) {
        return;
    }

    const PlayingState curr_state = m_playback_ctl->state();
    if (curr_state == PlayingState::PLAYING || curr_state == PlayingState::PAUSE) {
        if (m_pending_pos_ms > 0) {
            m_playback_ctl->setPosition(m_pending_pos_ms);
        }
        if (!m_pending_should_resume) {
            m_playback_ctl->pause();
        }
        return;
    }
    QTimer::singleShot(50, this, [this, retry]() { finalizeRestoreWhenReady(retry + 1); });
}

void PlaybackRestoreService::onCacheLoadFinished()
{
    if (m_pending_pid.isNull())
        return;

    m_playlist_ctl->switchToPlaylist(m_pending_pid);
    connect(m_playlist_ctl->viewModel(), &QAbstractItemModel::modelReset, this,
            &PlaybackRestoreService::onModelReset, Qt::SingleShotConnection);
}

void PlaybackRestoreService::onModelReset()
{
    if (m_pending_tid.isNull()) {
        qDebug() << "PlaybackRestoreService: m_pending_tid.isNull()";
        return;
    }
    const int queue_index = findQueueIndexByTrackId(m_pending_tid);
    if (queue_index < 0)
        return;
    m_playlist_ctl->play(queue_index);
    QTimer::singleShot(0, this, [this]() { finalizeRestoreWhenReady(0); });
}
