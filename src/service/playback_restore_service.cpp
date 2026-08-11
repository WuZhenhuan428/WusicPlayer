#include "service/playback_restore_service.h"

#include "controller/playback_controller.h"
#include "controller/playlist_controller.h"

#include "core/logger/log.h"


WUSIC_LOG_MODULE(playback_restore)

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
        WUSIC_LOG_FATAL(playback_restore, "[PlaybackRestoreService] !m_playlist_ctl || !m_playback_ctl");
        return;
    }

    if (m_restored == true)
        return;

    m_pending_pid           = m_playlist_ctl->last_playlist_id();
    m_pending_tid           = m_playlist_ctl->last_track_id();
    m_pending_pos_ms        = m_playback_ctl->last_position_ms();
    m_pending_should_resume = m_playback_ctl->last_was_playing();

    if (m_pending_pid.isNull())
        return;

    connect(m_playlist_ctl, &PlaylistController::sgn_cache_load_finished, this,
            &PlaybackRestoreService::on_cache_load_finished, Qt::SingleShotConnection);

    m_restored = true;
}

int PlaybackRestoreService::find_queue_index_by_track_id(const EntryId& tid)
{
    if (tid.isNull() || !m_playlist_ctl->view_model()) {
        return -1;
    }
    const auto& queue = m_playlist_ctl->view_model()->playback_queue();
    return queue.indexOf(tid);
}

void PlaybackRestoreService::finalize_restore_when_ready(int retry)
{
    if (retry > 30) {
        return;
    }

    const PlayingState curr_state = m_playback_ctl->state();
    if (curr_state == PlayingState::PLAYING || curr_state == PlayingState::PAUSE) {
        if (m_pending_pos_ms > 0) {
            m_playback_ctl->set_position(m_pending_pos_ms);
        }
        if (!m_pending_should_resume) {
            m_playback_ctl->pause();
        }
        return;
    }
    QTimer::singleShot(50, this, [this, retry]() { finalize_restore_when_ready(retry + 1); });
}

void PlaybackRestoreService::on_cache_load_finished()
{
    if (m_pending_pid.isNull())
        return;

    m_playlist_ctl->switch_to_playlist(m_pending_pid);
    connect(m_playlist_ctl->view_model(), &QAbstractItemModel::modelReset, this,
            &PlaybackRestoreService::on_model_reset, Qt::SingleShotConnection);
}

void PlaybackRestoreService::on_model_reset()
{
    if (m_pending_tid.isNull()) {
        WUSIC_LOG(playback_restore, debug, "PlaybackRestoreService: m_pending_tid.isNull()");
        return;
    }
    const int queue_index = find_queue_index_by_track_id(m_pending_tid);
    if (queue_index < 0)
        return;
    m_playlist_ctl->play(queue_index);
    QTimer::singleShot(0, this, [this]() { finalize_restore_when_ready(0); });
}
