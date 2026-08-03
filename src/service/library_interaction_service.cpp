#include "library_interaction_service.h"

#include "controller/PlaybackController.h"
#include "controller/PlaylistController.h"
#include "view/playlist_tree/PlaylistTreeWidget.h"
#include "view/song_table/SongTableView.h"

#include <QTreeView>

LibraryInteractionService::LibraryInteractionService(PlaylistTreeWidget* playlist_tree_widget,
                                                     SongTableView* song_table_view,
                                                     PlaybackController* playback_ctl,
                                                     PlaylistController* playlist_ctl,
                                                     QObject* parent) :
    QObject(parent), m_playlist_tree_widget(playlist_tree_widget),
    m_song_table_view(song_table_view), m_playback_ctl(playback_ctl), m_playlist_ctl(playlist_ctl)
{
    assert(m_playlist_tree_widget && m_song_table_view && m_playback_ctl && m_playlist_ctl);
}

LibraryInteractionService::~LibraryInteractionService() {}

void LibraryInteractionService::bind()
{
    if (m_bound == true)
        return;

    // 播放列表导航(播放列表树)
    connect(m_playlist_tree_widget, &PlaylistTreeWidget::sgnImportFiles, m_playlist_ctl,
            &PlaylistController::importFiles);
    connect(m_playlist_tree_widget, &PlaylistTreeWidget::sgnImportDir, m_playlist_ctl,
            &PlaylistController::importDir);
    connect(m_playlist_tree_widget, &PlaylistTreeWidget::sgnSwitchPlaylist, m_playlist_ctl,
            &PlaylistController::switchToPlaylist);
    connect(m_playlist_tree_widget, &PlaylistTreeWidget::sgnRenamePlaylist, m_playlist_ctl,
            &PlaylistController::renamePlaylist);
    connect(m_playlist_tree_widget, &PlaylistTreeWidget::sgnRemovePlaylist, m_playlist_ctl,
            &PlaylistController::removePlaylist);
    connect(m_playlist_tree_widget, &PlaylistTreeWidget::sgnSavePlaylist, m_playlist_ctl,
            &PlaylistController::savePlaylist);
    connect(m_playlist_tree_widget, &PlaylistTreeWidget::sgnCopyPlaylist, m_playlist_ctl,
            &PlaylistController::copyPlaylist);

    // 歌曲表(当前播放列表)
    connect(m_song_table_view, &SongTableView::sgnRemoveTrackRequested, this,
            [this](const EntryId& tid) {
                if (tid.isNull()) {
                    return;
                }
                if (m_playlist_ctl->currentTrackId() == tid) {
                    m_playback_ctl->stop();
                }
                m_playlist_ctl->removeTrack(tid);
            });
    connect(m_song_table_view, &SongTableView::sgnRemoveMissingTracksRequested, m_playlist_ctl,
            &PlaylistController::removeMissingTracks);
    connect(m_song_table_view, &SongTableView::sgnPlayTrackByModelIndex, this,
            [this](const QModelIndex& index) {
                auto* model = m_playlist_ctl->viewModel();
                if (!model)
                    return;
                const EntryId id = model->trackAt(index);
                if (id.isNull())
                    return;
                const int queueIndex = model->playbackQueue().indexOf(id);
                if (queueIndex >= 0) {
                    m_playlist_ctl->play(queueIndex);
                }
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
    m_playlist_tree_widget->setPlaylists(items);
}
