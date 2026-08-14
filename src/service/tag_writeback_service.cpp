#include "service/tag_writeback_service.h"

#include "app_context.h"
#include "controller/playback_controller.h"
#include "controller/playlist_controller.h"
#include "core/utils/audio.hpp"
#include "core/utils/path.hpp"
#include "view/main_window.h"
#include "view/tag_edit_widget/tag_edit_widget.h"

#include <QString>
#include <QThread>

#include "core/logger/logger_manager.h"
namespace
{
Logger* logger = LoggerManager::file_logger("tag_writeback", {"console", "gui"});
}

TagWritebackService::TagWritebackService(AppContext& ctx, QObject* parent) :
    QObject(parent), ctx_(ctx)
{
    this->playlist_ctl_     = this->ctx_.playlist_controller_;
    this->playback_ctl_     = this->ctx_.playback_controller_;
    this->playlist_manager_ = this->ctx_.playlist_manager_;
    this->main_window_      = this->ctx_.main_window_;
    assert(playlist_ctl_ && playback_ctl_ && playlist_manager_ && main_window_);
}

TagWritebackService::~TagWritebackService() {}

void TagWritebackService::request_track_property(EntryId tid, [[maybe_unused]] QString filepath,
                                                 TrackMetaData meta)
{
    if (tag_edit_widget_) {
        tag_edit_widget_.clear();
    }
    tag_edit_widget_ = new TagEditWidget(meta, tid);

    connect(
        tag_edit_widget_, &TagEditWidget::sgnSaveTags, this,
        [this](QMap<QString, QStringList> tags, EntryId changedTid) {
            // create snapshot
            auto curr_id           = playlist_ctl_->current_track_id();
            qint64 curr_pos_ms     = playback_ctl_->position();
            PlayingState old_state = playback_ctl_->state();

            if (curr_id == changedTid) {
                if (old_state == PlayingState::PLAYING || old_state == PlayingState::PAUSE) {
                    playback_ctl_->stop();
                }
            }

            // confirm filepath
            QString target_filepath;
            auto playlist =
                playlist_ctl_->find_playlist_by_id(playlist_ctl_->current_playlist_id());
            if (playlist) {
                const Track* track = playlist->find_track_by_id(changedTid);
                if (track) {
                    target_filepath = track->filepath;
                }
            }

            if (target_filepath.isEmpty()) {
                target_filepath = playlist_ctl_->current_metadata().filepath;
            }

            // work:
            QPointer<TagWritebackService> self(this);
            QThread* worker = QThread::create([self, tags, changedTid, curr_id, curr_pos_ms,
                                               old_state, target_filepath]() {
                const bool write_ok = utils::audio::taglib_writeback(target_filepath, tags);
                TrackMetaData refreshed;
                if (write_ok) {
                    refreshed = utils::audio::parse_to_local_meta(target_filepath);
                    refreshed = utils::audio::format(refreshed);
                }

                QMetaObject::invokeMethod(
                    self,
                    [self, write_ok, refreshed, target_filepath, changedTid, curr_id, curr_pos_ms,
                     old_state]() {
                        if (!self) {
                            return;
                        }

                        if (!write_ok) {
                            logger->debug("[TAG] Failed to writeback tag");
                        } else {
                            const QString target_normalized =
                                utils::path::normalize_path(target_filepath);
                            int updated_tracks       = 0;

                            const auto all_playlists = self->playlist_ctl_->playlists();
                            for (const auto& playlist : all_playlists) {
                                if (!playlist) {
                                    continue;
                                }

                                bool playlist_changed = false;
                                QVector<EntryId> to_update;
                                const auto& tracks = playlist->get_tracks();
                                for (const auto& track : tracks) {
                                    if (utils::path::normalize_path(track.filepath) ==
                                        target_normalized) {
                                        to_update.push_back(track.entry_id);
                                    }
                                }

                                for (const auto& tid : to_update) {
                                    if (playlist->update_track_meta(tid, refreshed)) {
                                        playlist_changed = true;
                                        ++updated_tracks;
                                    }
                                }

                                if (playlist_changed && self->playlist_manager_ &&
                                    self->playlist_manager_->m_repo) {
                                    self->playlist_manager_->m_repo->save_list_to_cache(playlist);
                                }
                            }

                            auto* model = self->playlist_ctl_->view_model();
                            if (model && updated_tracks > 0) {
                                model->rebuild_async();
                            }

                            if (curr_id == changedTid) {
                                auto* side_panel =
                                    self->main_window_ ? self->main_window_->side_panel() : nullptr;
                                if (side_panel) {
                                    side_panel->load_lyrics(refreshed);
                                    side_panel->load_meta_data(refreshed);
                                }
                            }
                        }

                        if (curr_id == changedTid) {
                            auto* model = self->playlist_ctl_->view_model();
                            if (!model) {
                                return;
                            }

                            int queue_index = model->playback_queue().indexOf(changedTid);
                            if (queue_index >= 0) {
                                self->playlist_ctl_->play(queue_index);
                            }
                            if (old_state != PlayingState::PLAYING) {
                                self->playback_ctl_->pause();
                            }

                            self->playback_ctl_->set_position(curr_pos_ms);
                        }
                    },
                    Qt::QueuedConnection);
            });

            connect(worker, &QThread::finished, worker, &QObject::deleteLater);
            worker->start();
        },
        static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));

    tag_edit_widget_->setAttribute(Qt::WA_DeleteOnClose);
    tag_edit_widget_->show();
}
