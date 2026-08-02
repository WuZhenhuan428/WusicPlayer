#pragma once

#include "core/types.h"
#include <QObject>

class LibraryWidget;
class PlaybackController;
class PlaylistController;

class LibraryInteractionService : public QObject
{
    Q_OBJECT

public:
    explicit LibraryInteractionService(LibraryWidget* library_widget,
                                       PlaybackController* playback_ctl,
                                       PlaylistController* playlist_ctl, QObject* parent);
    ~LibraryInteractionService();

    void bind();

signals:
    void sgnTrackPropertyRequested(EntryId tid, QString filepath, TrackMetaData meta);

private:
    void refreshPlaylistView();

private:
    LibraryWidget* m_library_widget;
    PlaybackController* m_playback_ctl;
    PlaylistController* m_playlist_ctl;

    bool m_bound = false;
};
