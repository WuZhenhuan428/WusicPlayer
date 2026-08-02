#include "library_interaction_service.h"

#include "controller/PlaybackController.h"
#include "controller/PlaylistController.h"
#include "view/LibraryWidget/LibraryWidget.h"

LibraryInteractionService::LibraryInteractionService(LibraryWidget* library_widget,
                                                     PlaybackController* playback_ctl,
                                                     PlaylistController* playlist_ctl,
                                                     QObject* parent) :
    QObject(parent), m_library_widget(library_widget), m_playback_ctl(playback_ctl),
    m_playlist_ctl(playlist_ctl)
{
    assert(m_library_widget && m_playback_ctl && m_playlist_ctl);
}

LibraryInteractionService::~LibraryInteractionService() {}

void LibraryInteractionService::bind()
{
    if (!m_library_widget || !m_playback_ctl || !m_playlist_ctl) {}
    if (m_bound == true)
        return;

    connect(m_library_widget, &LibraryWidget::sgnImportFiles, m_playlist_ctl,
            &PlaylistController::importFiles);
    connect(m_library_widget, &LibraryWidget::sgnImportDir, m_playlist_ctl,
            &PlaylistController::importDir);
    connect(m_library_widget, &LibraryWidget::sgnSwitchPlaylist, m_playlist_ctl,
            &PlaylistController::switchToPlaylist);
    connect(m_library_widget, &LibraryWidget::sgnRenamePlaylist, m_playlist_ctl,
            &PlaylistController::renamePlaylist);
    connect(m_library_widget, &LibraryWidget::sgnRemovePlaylist, m_playlist_ctl,
            &PlaylistController::removePlaylist);
    connect(m_library_widget, &LibraryWidget::sgnSavePlaylist, m_playlist_ctl,
            &PlaylistController::savePlaylist);
    connect(m_library_widget, &LibraryWidget::sgnCopyPlaylist, m_playlist_ctl,
            &PlaylistController::copyPlaylist);
    connect(m_library_widget, &LibraryWidget::sgnRemoveTrackRequested, this,
            [this](const EntryId& tid) {
                if (tid.isNull()) {
                    return;
                }
                if (m_playlist_ctl->currentTrackId() == tid) {
                    m_playback_ctl->stop();
                }
                m_playlist_ctl->removeTrack(tid);
            });
    connect(m_library_widget, &LibraryWidget::sgnPlayTrackByModelIndex, this,
            [this](const QModelIndex& index) {
                auto* model = m_playlist_ctl->viewModel();
                if (!model)
                    return;
                EntryId id = model->trackAt(index);
                if (id.isNull())
                    return;
                int queueIndex = model->playbackQueue().indexOf(id);
                if (queueIndex >= 0) {
                    m_playlist_ctl->play(queueIndex);
                }
            });
    connect(m_playlist_ctl->viewModel(), &QAbstractItemModel::modelReset, this, [this]() {
        QTreeView* view = m_library_widget->songTreeView();
        if (!view || !view->model()) {
            return;
        }
        QAbstractItemModel* model = view->model();
        for (int i = 0; i < model->rowCount(); ++i) {
            QModelIndex idx = model->index(i, 0);
            if (model->hasChildren(idx)) {
                view->setFirstColumnSpanned(i, QModelIndex(), true);
                view->setExpanded(idx, true);
            }
        }
    });

    connect(m_library_widget, &LibraryWidget::sgnTrackPropertyRequested, this,
            [this](EntryId tid, QString filepath, TrackMetaData meta) {
                emit sgnTrackPropertyRequested(tid, filepath, meta);
            });

    connect(m_playlist_ctl, &PlaylistController::playlistChanged, this,
            &LibraryInteractionService::refreshPlaylistView);

    m_bound = true;
}

void LibraryInteractionService::refreshPlaylistView()
{
    QVector<QPair<PlaylistId, QString>> items;
    const auto& lists = m_playlist_ctl->playlists();
    items.reserve(static_cast<int>(lists.size()));
    for (const auto& list : lists) {
        items.push_back({list->id(), list->name()});
    }
    m_library_widget->setPlaylists(items);
}
