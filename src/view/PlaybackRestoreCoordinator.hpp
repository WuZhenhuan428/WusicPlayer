#pragma once
#include <QObject>
#include <QTimer>
#include <QMediaPlayer>
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

private:
    int findQueueIndexByTrackId(const trackId& tid) {
        if (tid.isNull() || !m_playlist_controller || !m_playlist_controller->viewModel()) {
            return -1;
        }
        const auto& queue = m_playlist_controller->viewModel()->playbackQueue();
        return queue.indexOf(tid);
    }

    void seekWhenMediaReady(int retry) {
        if (retry > 30 || m_pending_pos_ms <= 0) return;

        QMediaPlayer* media_player = const_cast<QMediaPlayer*>(m_playback_controller->getMediaPlayer());
        if (!media_player) {
            return;
        }

        const auto status = media_player->mediaStatus();
        const bool can_seek = (
            status == QMediaPlayer::LoadedMedia ||
            status == QMediaPlayer::BufferedMedia ||
            status == QMediaPlayer::BufferingMedia)
            && (media_player->duration() > 0
        );
        if (can_seek) {
            m_playback_controller->setPosition(m_pending_pos_ms);
            m_playback_controller->pause();
            return;
        }
        if (++retry > 30) {    // 1.5s timeout
            return;
        }
        QTimer::singleShot(50, this, [this, retry]() {
            seekWhenMediaReady(retry + 1);
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
        QTimer::singleShot(0, this, [this]() { seekWhenMediaReady(0); });
    }
};