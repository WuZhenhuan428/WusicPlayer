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
        m_playbackConfigSection(plCfgSec),
        m_playlistController(plCtr),
        m_playbackController(pbCtr) {}
public:
    void restorePlaybackState() {
        if (!m_playlistController || !m_playbackConfigSection || !m_playbackController) {
            qDebug() << "invalid member of PlaybackRestoreCoordinator";
            return;
        }
        m_pendingPID = m_playbackConfigSection->last_playlist_id;
        m_pendingTID = m_playbackConfigSection->last_track_id;
        m_pendingPosMs = m_playbackConfigSection->last_position_ms;

        if (m_pendingPID.isNull()) {
            return;
        }

        connect(m_playlistController, &PlaylistController::cacheLoadFinished, this,
                &PlaybackRestoreCoordinator::onCacheLoadFinished, Qt::SingleShotConnection);
    };
private:
    PlaybackConfigSection* m_playbackConfigSection;
    PlaylistController* m_playlistController;
    PlaybackController* m_playbackController;

    playlistId m_pendingPID;
    trackId m_pendingTID;
    int m_pendingPosMs = 0;

private:
    int findQueueIndexByTrackId(const trackId& tid) {
        if (tid.isNull() || !m_playlistController || !m_playlistController->viewModel()) {
            return -1;
        }
        const auto& queue = m_playlistController->viewModel()->playbackQueue();
        return queue.indexOf(tid);
    }

    void seekWhenMediaReady(int retry) {
        if (retry > 30 || m_pendingPosMs <= 0) return;

        QMediaPlayer* media_player = const_cast<QMediaPlayer*>(m_playbackController->getMediaPlayer());
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
            m_playbackController->setPosition(m_pendingPosMs);
            m_playbackController->pause();
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
        if (m_pendingPID.isNull()) return;

        m_playlistController->switchToPlaylist(m_pendingPID);
        connect(m_playlistController->viewModel(), &QAbstractItemModel::modelReset, 
                this, &PlaybackRestoreCoordinator::onModelReset,
                Qt::SingleShotConnection);
    }
    void onModelReset() {
        if (m_pendingTID.isNull()) {
            qDebug() << "PlaybackRestoreCoordinator:onModelReset: m_pendingTID.isNull()";
            return;
        }
        const int queue_index = findQueueIndexByTrackId(m_pendingTID);
        if (queue_index < 0) return;
        m_playlistController->play(queue_index);
        QTimer::singleShot(0, this, [this]() { seekWhenMediaReady(0); });
    }
};