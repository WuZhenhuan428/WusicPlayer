#include "PlaybackConfigBinder.hpp"

#include "MainWindowConfigContext.hpp"
#include "controller/PlaybackController.h"
#include "controller/PlaylistController.h"
#include "view/ConfigBinder/PlaybackConfigSection.hpp"
#include "view/WControlBar/WControlBar.h"
#include <QSlider>
#include <QDebug>

void PlaybackConfigBinder::apply(MainWindowConfigContext& ctx) {
    if (!ctx.playbackController || !ctx.playbackController) {
        qDebug() << "[DEBUG] ctx.playbackController & ctx.playbackController not available!";
        return;
    }
    ctx.playbackController->setVolume(ctx.playbackSec->volume);
    ctx.playbackController->setMute(ctx.playbackSec->muted);

    const auto sliders = ctx.controlBar->findChildren<QSlider*>();
    for (QSlider* s : sliders) {
        if (s && s->orientation() == Qt::Horizontal && s->maximum() == 100  && s->maximumWidth() == 100) {
            s->setValue(ctx.playbackSec->volume);
            break;
        }
    }

    ctx.playbackController->setPlayMode(ctx.playbackSec->play_mode);
}

void PlaybackConfigBinder::collect(MainWindowConfigContext& ctx) {
    QPointer<QSlider> volumeSliderPtr = ctx.controlBar->getVolumeSlider();
    if (volumeSliderPtr) {
        ctx.playbackSec->volume = volumeSliderPtr->value();
    }
    ctx.playbackSec->muted = ctx.playbackController->getMute();
    ctx.playbackSec->play_mode = ctx.playbackController->playMode();
    ctx.playbackSec->last_device = ctx.playbackController->currentDeviceId();

    do {
        const playlistId last_pid = ctx.playlistController->currentPlaylist();
        const trackId last_tid = ctx.playlistController->currentTrackId();
        if (last_pid.isNull() || last_tid.isNull()) break;
        ctx.playbackSec->last_playlist_id = last_pid;
        ctx.playbackSec->last_track_id = last_tid;
        ctx.playbackSec->last_position_ms
            = ctx.playbackController->getMediaPlayer()->hasAudio() 
            ? ctx.playbackController->position() 
            : 0;
    } while (0);
}