#pragma once
#include <QObject>
#include <QTimer>
#include <QDebug>

#include "core/types.h"
#include "view/ConfigBinder/PlaybackConfigSection.hpp"
#include "controller/PlaylistController.h"
#include "controller/PlaybackController.h"

class PlaybackRestoreCoordinator : public QObject
{
    Q_OBJECT
public:
    explicit PlaybackRestoreCoordinator(
        PlaybackConfigSection* plCfgSec, 
        PlaylistController* plCtr,
        PlaybackController* pbCtr,
        QObject* parent
    ) : QObject(parent),
        m_playback_config_section(plCfgSec),
        m_playlist_controller(plCtr),
        m_playback_controller(pbCtr) {}
public:
    void restorePlaybackState() {
        if (!m_playlist_controller || !m_playback_config_section || !m_playback_controller) {
            qDebug() << "invalid member of PlaybackRestoreCoordinator";
            return;
        }
        m_pending_pid = m_playback_config_section->last_playlist_id;
        m_pending_tid = m_playback_config_section->last_track_id;
        m_pending_pos_ms = m_playback_config_section->last_position_ms;
        m_pending_should_resume = m_playback_config_section->last_was_playing;

        if (m_pending_pid.isNull()) {
            return;
        }

        connect(m_playlist_controller, &PlaylistController::cacheLoadFinished, this,
                &PlaybackRestoreCoordinator::onCacheLoadFinished, Qt::SingleShotConnection);
    };
private:
    PlaybackConfigSection* m_playback_config_section;
    PlaylistController* m_playlist_controller;
    PlaybackController* m_playback_controller;

    playlistId m_pending_pid;
    trackId m_pending_tid;
    int m_pending_pos_ms = 0;
    bool m_pending_should_resume = false;

private:
    int findQueueIndexByTrackId(const trackId& tid) {
        if (tid.isNull() || !m_playlist_controller || !m_playlist_controller->viewModel()) {
            return -1;
        }
        const auto& queue = m_playlist_controller->viewModel()->playbackQueue();
        return queue.indexOf(tid);
    }

    void finalizeRestoreWhenReady(int retry) {
        if (retry > 30) {
            return;
        }

        if (!m_playback_controller) {
            return;
        }

        const PlayingState curr_state = m_playback_controller->state();
        if (curr_state == PlayingState::PLAYING || curr_state == PlayingState::PAUSE) {
            if (m_pending_pos_ms > 0) {
                m_playback_controller->setPosition(m_pending_pos_ms);
            }
            if (!m_pending_should_resume) {
                m_playback_controller->pause();
            }
            return;
        }
        QTimer::singleShot(50, this, [this, retry]() {
            finalizeRestoreWhenReady(retry + 1);
        });
    }

private slots:
    void onCacheLoadFinished() {
        if (m_pending_pid.isNull()) return;

        m_playlist_controller->switchToPlaylist(m_pending_pid);
        connect(m_playlist_controller->viewModel(), &QAbstractItemModel::modelReset, 
                this, &PlaybackRestoreCoordinator::onModelReset,
                Qt::SingleShotConnection);
    }
    void onModelReset() {
        if (m_pending_tid.isNull()) {
            qDebug() << "PlaybackRestoreCoordinator:onModelReset: m_pending_tid.isNull()";
            return;
        }
        const int queue_index = findQueueIndexByTrackId(m_pending_tid);
        if (queue_index < 0) return;
        m_playlist_controller->play(queue_index);
        QTimer::singleShot(0, this, [this]() { finalizeRestoreWhenReady(0); });
    }
};