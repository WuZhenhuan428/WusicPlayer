#pragma once

#include <QObject>

class PlaybackController;
class PlaylistController;
class PlaylistTreeWidget;
class SongTableView;

class LibraryInteractionService : public QObject
{
    Q_OBJECT

public:
    explicit LibraryInteractionService(PlaylistTreeWidget* playlist_tree_widget,
                                       SongTableView* song_table_view,
                                       PlaybackController* playback_ctl,
                                       PlaylistController* playlist_ctl, QObject* parent);
    ~LibraryInteractionService();

    void bind();

private:
    void refresh_playlist_view();

private:
    PlaylistTreeWidget* m_playlist_tree_widget;
    SongTableView* m_song_table_view;
    PlaybackController* m_playback_ctl;
    PlaylistController* m_playlist_ctl;

    bool m_bound = false;
};
