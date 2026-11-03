#include "service/playback_restore_service.h"

#include "app_context.h"
#include "controller/playback_controller.h"
#include "controller/playlist_controller.h"
#include "core/event_bus.h"
#include "core/notification_types.h"

#include <QString>

#include "core/logger/logger_manager.h"
namespace
{
// NOLINTNEXTLINE
Logger* logger = LoggerManager::file_logger("playback_restore_service", {"console", "gui"});
} // namespace

PlaybackRestoreService::PlaybackRestoreService(AppContext& ctx, QObject* parent) :
    QObject(parent), ctx_(ctx)
{
    assert(ctx_.playlist_controller && ctx_.playback_controller);
    this->playlist_ctl_ = ctx_.playlist_controller;
    this->playback_ctl_ = ctx_.playback_controller;
}

void PlaybackRestoreService::restore()
{
    if (!playlist_ctl_ || !playback_ctl_) {
        logger->fatal("!playlist_ctl_ || !playback_ctl_");
        return;
    }

    if (restored_ == true) {
        return;
    }

    pending_pid_           = playlist_ctl_->last_playlist_id();
    pending_tid_           = playlist_ctl_->last_track_id();
    pending_pos_ms_        = playback_ctl_->last_position_ms();
    pending_should_resume_ = playback_ctl_->last_was_playing();

    if (pending_pid_.is_null()) {
        return;
    }

    connect(playlist_ctl_, &PlaylistController::sgn_cache_load_finished, this,
            &PlaybackRestoreService::on_cache_load_finished, Qt::SingleShotConnection);
    connect(playlist_ctl_, &PlaylistController::sgn_cache_load_finished, this, [this]() {
        //
        this->ctx_.event_bus->publish(EventBus::Topic::NotificationShown,
                                      AppNotification{
                                          .level       = AppNotification::Level::Info,
                                          .message     = tr("Playlist Load Finished"),
                                          .duration_ms = 5000,
                                      });
    });

    restored_ = true;
}

int PlaybackRestoreService::find_queue_index_by_track_id(const EntryId& tid)
{
    if (tid.is_null() || !playlist_ctl_->view_model()) {
        return -1;
    }
    const auto& queue = playlist_ctl_->view_model()->playback_queue();
    return static_cast<int>(queue.indexOf(tid));
}

void PlaybackRestoreService::finalize_restore_when_ready(int retry)
{
    if (retry > 30) {
        return;
    }

    const PlayingState curr_state = playback_ctl_->state();
    if (curr_state == PlayingState::Playing || curr_state == PlayingState::Pause) {
        if (pending_pos_ms_ > 0) {
            playback_ctl_->set_position(pending_pos_ms_);
        }
        if (!pending_should_resume_) {
            playback_ctl_->pause();
        }
        return;
    }
    QTimer::singleShot(50, this, [this, retry]() { finalize_restore_when_ready(retry + 1); });
}

void PlaybackRestoreService::on_cache_load_finished()
{
    if (pending_pid_.is_null()) {
        return;
    }

    playlist_ctl_->switch_to_playlist(pending_pid_);
    connect(playlist_ctl_->view_model(), &QAbstractItemModel::modelReset, this,
            &PlaybackRestoreService::on_model_reset, Qt::SingleShotConnection);
}

void PlaybackRestoreService::on_model_reset()
{
    if (pending_tid_.is_null()) {
        logger->debug("PlaybackRestoreService: pending_tid_.isNull()");
        return;
    }
    const int queue_index = find_queue_index_by_track_id(pending_tid_);
    if (queue_index < 0) {
        return;
    }
    playlist_ctl_->play(queue_index);
    QTimer::singleShot(0, this, [this]() { finalize_restore_when_ready(0); });
}
