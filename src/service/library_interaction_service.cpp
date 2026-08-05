#include "service/library_interaction_service.h"

#include "controller/playback_controller.h"
#include "controller/playlist_controller.h"
#include "view/playlist_tree/playlist_tree_widget.h"
#include "view/song_table/song_table_view.h"

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
            &PlaylistController::import_files);
    connect(m_playlist_tree_widget, &PlaylistTreeWidget::sgnImportDir, m_playlist_ctl,
            &PlaylistController::import_dir);
    connect(m_playlist_tree_widget, &PlaylistTreeWidget::sgnSwitchPlaylist, m_playlist_ctl,
            &PlaylistController::switch_to_playlist);
    connect(m_playlist_tree_widget, &PlaylistTreeWidget::sgnCreatePlaylist, m_playlist_ctl,
            &PlaylistController::create_new_playlist);
    connect(m_playlist_tree_widget, &PlaylistTreeWidget::sgnRenamePlaylist, m_playlist_ctl,
            static_cast<void (PlaylistController::*)(const PlaylistId&, const QString&)>(
                &PlaylistController::rename_playlist));
    connect(m_playlist_tree_widget, &PlaylistTreeWidget::sgnRemovePlaylist, m_playlist_ctl,
            &PlaylistController::remove_playlist);
    connect(m_playlist_tree_widget, &PlaylistTreeWidget::sgnSavePlaylist, m_playlist_ctl,
            &PlaylistController::save_playlist);
    connect(m_playlist_tree_widget, &PlaylistTreeWidget::sgnCopyPlaylist, m_playlist_ctl,
            &PlaylistController::copy_playlist);
    connect(m_playlist_tree_widget, &PlaylistTreeWidget::sgnReorderPlaylists, m_playlist_ctl,
            &PlaylistController::reorder_playlists);

    // 歌曲表(当前播放列表)
    // Add to Playlist 目标列表提供者(排除当前列表)
    m_song_table_view->set_playlist_list_provider([this]() {
        QVector<QPair<PlaylistId, QString>> lists;
        const PlaylistId current = m_playlist_ctl->current_playlist_id();
        const auto all           = m_playlist_ctl->playlists();
        for (const auto& pl : all) {
            if (pl && pl->id() != current) {
                lists.push_back({pl->id(), pl->name()});
            }
        }
        return lists;
    });

    connect(m_song_table_view, &SongTableView::sgnRemoveTrackRequested, this,
            [this](const EntryId& tid) {
                if (tid.isNull()) {
                    return;
                }
                if (m_playlist_ctl->current_track_id() == tid) {
                    m_playback_ctl->stop();
                }
                m_playlist_ctl->remove_track(tid);
            });
    connect(m_song_table_view, &SongTableView::sgnRemoveMissingTracksRequested, m_playlist_ctl,
            &PlaylistController::remove_missing_tracks);
    // 批量移除(多选):停播当前曲目后批量删除
    connect(m_song_table_view, &SongTableView::sgnRemoveTracksRequested, this,
            [this](const QVector<EntryId>& ids) {
                if (ids.isEmpty()) {
                    return;
                }
                if (ids.contains(m_playlist_ctl->current_track_id())) {
                    m_playback_ctl->stop();
                }
                m_playlist_ctl->remove_tracks(ids);
            });
    // 复制选中曲目到目标列表(Add to Playlist)
    connect(m_song_table_view, &SongTableView::sgnCopyTracksToPlaylist, this,
            [this](const PlaylistId& dst_pid, const QVector<EntryId>& track_ids) {
                if (dst_pid.isNull() || track_ids.isEmpty()) {
                    return;
                }
                m_playlist_ctl->copy_tracks_to_playlist(m_playlist_ctl->current_playlist_id(),
                                                        track_ids, dst_pid);
            });
    connect(m_song_table_view, &SongTableView::sgnPlayTrackByModelIndex, this,
            [this](const QModelIndex& index) {
                auto* model = m_playlist_ctl->view_model();
                if (!model)
                    return;
                const EntryId id = model->track_at(index);
                if (id.isNull())
                    return;
                const int queueIndex = model->playback_queue().indexOf(id);
                if (queueIndex >= 0) {
                    m_playlist_ctl->play(queueIndex);
                }
            });
    connect(m_playlist_ctl, &PlaylistController::sgn_playlist_changed, this,
            &LibraryInteractionService::refresh_playlist_view);

    m_bound = true;
}

void LibraryInteractionService::refresh_playlist_view()
{
    QVector<QPair<PlaylistId, QString>> items;
    const auto& lists = m_playlist_ctl->playlists();
    items.reserve(static_cast<int>(lists.size()));
    for (const auto& list : lists) {
        items.push_back({list->id(), list->name()});
    }
    m_playlist_tree_widget->set_playlists(items);
}
