#include "service/tag_writeback_service.h"

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

TagWritebackService::TagWritebackService(PlaylistController* playlist_ctl,
                                         PlaybackController* playback_ctl,
                                         PlaylistManager* playlist_manager, MainWindow* main_window,
                                         QObject* parent) :
    QObject(parent), m_playlist_ctl(playlist_ctl), m_playback_ctl(playback_ctl),
    playlist_manager_(playlist_manager), main_window_(main_window)
{
    assert(m_playlist_ctl && m_playback_ctl && playlist_manager_ && main_window_);
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
            auto curr_id           = m_playlist_ctl->current_track_id();
            qint64 curr_pos_ms     = m_playback_ctl->position();
            PlayingState old_state = m_playback_ctl->state();

            if (curr_id == changedTid) {
                if (old_state == PlayingState::PLAYING || old_state == PlayingState::PAUSE) {
                    m_playback_ctl->stop();
                }
            }

            // confirm filepath
            QString target_filepath;
            auto playlist =
                m_playlist_ctl->find_playlist_by_id(m_playlist_ctl->current_playlist_id());
            if (playlist) {
                const Track* track = playlist->find_track_by_id(changedTid);
                if (track) {
                    target_filepath = track->filepath;
                }
            }

            if (target_filepath.isEmpty()) {
                target_filepath = m_playlist_ctl->current_metadata().filepath;
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

                            const auto all_playlists = self->m_playlist_ctl->playlists();
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

                            auto* model = self->m_playlist_ctl->view_model();
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
                            auto* model = self->m_playlist_ctl->view_model();
                            if (!model) {
                                return;
                            }

                            int queue_index = model->playback_queue().indexOf(changedTid);
                            if (queue_index >= 0) {
                                self->m_playlist_ctl->play(queue_index);
                            }
                            if (old_state != PlayingState::PLAYING) {
                                self->m_playback_ctl->pause();
                            }

                            self->m_playback_ctl->set_position(curr_pos_ms);
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
