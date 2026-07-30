#include "tag_writeback_service.h"

#include "controller/PlaybackController.h"
#include "controller/PlaylistController.h"
#include "core/utils/AudioUtils.h"
#include "core/utils/path_utils.h"
#include "view/MainWindow.h"
#include "view/tag_edit_widget/tag_edit_widget.h"

#include <QString>
#include <QThread>

TagWritebackService::TagWritebackService(PlaylistController* playlist_ctl,
                                         PlaybackController* playback_ctl,
                                         PlaylistManager* playlist_manager, MainWindow* main_window,
                                         QObject* parent) :
    QObject(parent), m_playlist_ctl(playlist_ctl), m_playback_ctl(playback_ctl),
    m_playlist_manager(playlist_manager), m_main_window(main_window)
{
    assert(m_playlist_ctl && m_playback_ctl && m_playlist_manager && m_main_window);
}

TagWritebackService::~TagWritebackService() {}

void TagWritebackService::requestTrackProperty(trackId tid, QString filepath, TrackMetaData meta)
{
    Q_UNUSED(filepath);

    if (m_tag_edit_widget) {
        m_tag_edit_widget.clear();
    }
    m_tag_edit_widget = new TagEditWidget(meta, tid);

    connect(
        m_tag_edit_widget, &TagEditWidget::sgnSaveTags, this,
        [this](QMap<QString, QStringList> tags, trackId changedTid) {
            // create snapshot
            auto curr_id           = m_playlist_ctl->currentTrackId();
            qint64 curr_pos_ms     = m_playback_ctl->position();
            PlayingState old_state = m_playback_ctl->state();

            if (curr_id == changedTid) {
                if (old_state == PlayingState::PLAYING || old_state == PlayingState::PAUSE) {
                    m_playback_ctl->stop();
                }
            }

            // confirm filepath
            QString target_filepath;
            auto playlist = m_playlist_ctl->findPlaylistById(m_playlist_ctl->currentPlaylist());
            if (playlist) {
                Track* track = playlist->findTrackByID(changedTid);
                if (track) {
                    target_filepath = track->filepath;
                }
            }

            if (target_filepath.isEmpty()) {
                target_filepath = m_playlist_ctl->currentMetadata().filepath;
            }

            // work:
            QPointer<TagWritebackService> self(this);
            QThread* worker = QThread::create(
                [self, tags, changedTid, curr_id, curr_pos_ms, old_state, target_filepath]() {
                    const bool write_ok = AudioUtils::taglib_writeback(target_filepath, tags);
                    TrackMetaData refreshed;
                    if (write_ok) {
                        refreshed = AudioUtils::parse_to_local_meta(target_filepath);
                        refreshed = AudioUtils::format(refreshed);
                    }

                    QMetaObject::invokeMethod(
                        self,
                        [self, write_ok, refreshed, target_filepath, changedTid, curr_id,
                         curr_pos_ms, old_state]() {
                            if (!self) {
                                return;
                            }

                            if (!write_ok) {
                                qDebug() << "[TAG] Failed to writeback tag";
                            } else {
                                const QString target_normalized =
                                    PathUtils::normalize_path(target_filepath);
                                int updated_tracks       = 0;

                                const auto all_playlists = self->m_playlist_ctl->playlists();
                                for (const auto& playlist : all_playlists) {
                                    if (!playlist) {
                                        continue;
                                    }

                                    bool playlist_changed = false;
                                    QVector<trackId> to_update;
                                    const auto& tracks = playlist->getTracks();
                                    for (const auto& track : tracks) {
                                        if (PathUtils::normalize_path(track.filepath) ==
                                            target_normalized) {
                                            to_update.push_back(track.tid);
                                        }
                                    }

                                    for (const auto& tid : to_update) {
                                        if (playlist->updateTrackMeta(tid, refreshed)) {
                                            playlist_changed = true;
                                            ++updated_tracks;
                                        }
                                    }

                                    if (playlist_changed && self->m_playlist_manager &&
                                        self->m_playlist_manager->m_repo) {
                                        self->m_playlist_manager->m_repo->saveListToCache(playlist);
                                    }
                                }

                                auto* model = self->m_playlist_ctl->viewModel();
                                if (model && updated_tracks > 0) {
                                    model->rebuildAsync();
                                }

                                if (curr_id == changedTid) {
                                    auto* sidePanel = self->m_main_window
                                                          ? self->m_main_window->sidePanel()
                                                          : nullptr;
                                    if (sidePanel) {
                                        sidePanel->loadLyrics(refreshed);
                                        sidePanel->loadMetaData(refreshed);
                                    }
                                }
                            }

                            if (curr_id == changedTid) {
                                auto* model = self->m_playlist_ctl->viewModel();
                                if (!model) {
                                    return;
                                }

                                int queue_index = model->playbackQueue().indexOf(changedTid);
                                if (queue_index >= 0) {
                                    self->m_playlist_ctl->play(queue_index);
                                }
                                if (old_state != PlayingState::PLAYING) {
                                    self->m_playback_ctl->pause();
                                }

                                self->m_playback_ctl->setPosition(curr_pos_ms);
                            }
                        },
                        Qt::QueuedConnection);
                });

            connect(worker, &QThread::finished, worker, &QObject::deleteLater);
            worker->start();
        },
        static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::SingleShotConnection));

    m_tag_edit_widget->setAttribute(Qt::WA_DeleteOnClose);
    m_tag_edit_widget->show();
}
