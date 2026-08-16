#include "service/library_interaction_service.h"

#include "app_context.h"
#include "controller/playback_controller.h"
#include "controller/playlist_controller.h"
#include "view/main_window.h"
#include "view/playlist_tree/playlist_tree_widget.h"
#include "view/song_table/song_table_view.h"

#include <QTreeView>

LibraryInteractionService::LibraryInteractionService(AppContext& ctx, QObject* parent) :
    QObject(parent), ctx_(ctx)
{
    this->playlist_tree_widget_ = ctx_.main_window_->playlist_tree_widget();
    this->song_table_view_      = ctx_.main_window_->song_table_view();
    this->playback_ctl_         = ctx_.playback_controller_;
    this->playlist_ctl_         = ctx_.playlist_controller_;
    assert(playlist_tree_widget_ && song_table_view_ && playback_ctl_ && playlist_ctl_);
}

LibraryInteractionService::~LibraryInteractionService() {}

void LibraryInteractionService::bind()
{
    if (m_bound == true)
        return;

    // 播放列表导航(播放列表树)
    connect(playlist_tree_widget_, &PlaylistTreeWidget::sgnImportFiles, playlist_ctl_,
            &PlaylistController::import_files);
    connect(playlist_tree_widget_, &PlaylistTreeWidget::sgnImportDir, playlist_ctl_,
            &PlaylistController::import_dir);
    connect(playlist_tree_widget_, &PlaylistTreeWidget::sgnSwitchPlaylist, playlist_ctl_,
            &PlaylistController::switch_to_playlist);
    connect(playlist_tree_widget_, &PlaylistTreeWidget::sgnCreatePlaylist, playlist_ctl_,
            &PlaylistController::create_new_playlist);
    connect(playlist_tree_widget_, &PlaylistTreeWidget::sgnRenamePlaylist, playlist_ctl_,
            static_cast<void (PlaylistController::*)(const PlaylistId&, const QString&)>(
                &PlaylistController::rename_playlist));
    connect(playlist_tree_widget_, &PlaylistTreeWidget::sgnRemovePlaylist, playlist_ctl_,
            &PlaylistController::remove_playlist);
    connect(playlist_tree_widget_, &PlaylistTreeWidget::sgnSavePlaylist, playlist_ctl_,
            &PlaylistController::save_playlist);
    connect(playlist_tree_widget_, &PlaylistTreeWidget::sgnCopyPlaylist, playlist_ctl_,
            &PlaylistController::copy_playlist);
    connect(playlist_tree_widget_, &PlaylistTreeWidget::sgnReorderPlaylists, playlist_ctl_,
            &PlaylistController::reorder_playlists);

    // 歌曲表(当前播放列表)
    // Add to Playlist 目标列表提供者(排除当前列表)
    song_table_view_->set_playlist_list_provider([this]() {
        QVector<QPair<PlaylistId, QString>> lists;
        const PlaylistId current = playlist_ctl_->current_playlist_id();
        const auto all           = playlist_ctl_->playlists();
        for (const auto& pl : all) {
            if (pl && pl->id() != current) {
                lists.push_back({pl->id(), pl->name()});
            }
        }
        return lists;
    });

    connect(song_table_view_, &SongTableView::sgnRemoveTrackRequested, this,
            [this](const EntryId& eid) {
                if (eid.is_null()) {
                    return;
                }
                if (playlist_ctl_->current_track_id() == eid) {
                    playback_ctl_->stop();
                }
                playlist_ctl_->remove_track(eid);
            });
    connect(song_table_view_, &SongTableView::sgnRemoveMissingTracksRequested, playlist_ctl_,
            &PlaylistController::remove_missing_tracks);
    // 批量移除(多选):停播当前曲目后批量删除
    connect(song_table_view_, &SongTableView::sgnRemoveTracksRequested, this,
            [this](const QVector<EntryId>& ids) {
                if (ids.isEmpty()) {
                    return;
                }
                if (ids.contains(playlist_ctl_->current_track_id())) {
                    playback_ctl_->stop();
                }
                playlist_ctl_->remove_tracks(ids);
            });
    // 复制选中曲目到目标列表(Add to Playlist)
    connect(song_table_view_, &SongTableView::sgnCopyTracksToPlaylist, this,
            [this](const PlaylistId& dst_pid, const QVector<EntryId>& track_ids) {
                if (dst_pid.is_null() || track_ids.isEmpty()) {
                    return;
                }
                playlist_ctl_->copy_tracks_to_playlist(playlist_ctl_->current_playlist_id(),
                                                       track_ids, dst_pid);
            });
    connect(song_table_view_, &SongTableView::sgnPlayTrackByModelIndex, this,
            [this](const QModelIndex& index) {
                auto* model = playlist_ctl_->view_model();
                if (!model)
                    return;
                const EntryId id = model->track_at(index);
                if (id.is_null())
                    return;
                const int queueIndex = model->playback_queue().indexOf(id);
                if (queueIndex >= 0) {
                    playlist_ctl_->play(queueIndex);
                }
            });
    connect(playlist_ctl_, &PlaylistController::sgn_playlist_changed, this,
            &LibraryInteractionService::refresh_playlist_view);

    m_bound = true;
}

void LibraryInteractionService::refresh_playlist_view()
{
    QVector<QPair<PlaylistId, QString>> items;
    const auto& lists = playlist_ctl_->playlists();
    items.reserve(static_cast<int>(lists.size()));
    for (const auto& list : lists) {
        items.push_back({list->id(), list->name()});
    }
    playlist_tree_widget_->set_playlists(items);
}
