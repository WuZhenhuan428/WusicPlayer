#pragma once

#include <QObject>
#include <QPointer>
#include "core/types.h"


class PlaylistController;
class PlaybackController;
class PlaylistManager;
class TagEditWidget;
class MainWindow;

class TagWritebackService : public QObject
{
    Q_OBJECT

public:
    explicit TagWritebackService(PlaylistController* playlist_ctl, 
                                 PlaybackController* playback_ctl,
                                 PlaylistManager* playlist_manager,
                                 MainWindow* main_window,
                                 QObject* parent);
    ~TagWritebackService();

    void requestTrackProperty(trackId tid, QString filepath, TrackMetaData meta);
private:
    PlaylistController* m_playlist_ctl;
    PlaybackController* m_playback_ctl;
    PlaylistManager* m_playlist_manager;
    MainWindow* m_main_window;
    QPointer<TagEditWidget> m_tag_edit_widget;

    bool m_bound = false;
};