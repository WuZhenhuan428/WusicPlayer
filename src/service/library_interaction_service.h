#pragma once

#include <QObject>

class AppContext;
class PlaybackController;
class PlaylistController;
class PlaylistTreeWidget;
class SongTableView;

class LibraryInteractionService : public QObject
{
    Q_OBJECT

public:
    explicit LibraryInteractionService(AppContext& ctx, QObject* parent);
    ~LibraryInteractionService();

    void bind();

private:
    void refresh_playlist_view();

private:
    AppContext& ctx_;
    PlaylistTreeWidget* playlist_tree_widget_;
    SongTableView* song_table_view_;
    PlaybackController* playback_ctl_;
    PlaylistController* playlist_ctl_;

    bool m_bound = false;
};
