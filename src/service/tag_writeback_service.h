#pragma once

#include "core/types.h"
#include <QObject>
#include <QPointer>

class PlaylistController;
class PlaybackController;
class PlaylistManager;
class TagEditWidget;
class MainWindow;

class TagWritebackService : public QObject
{
    Q_OBJECT

public:
    explicit TagWritebackService(PlaylistController* playlist_ctl, PlaybackController* playback_ctl,
                                 PlaylistManager* playlist_manager, MainWindow* main_window,
                                 QObject* parent);
    ~TagWritebackService();

    void requestTrackProperty(trackId tid, QString filepath, TrackMetaData meta);

private:
    PlaylistController* m_playlist_ctl;
    PlaybackController* m_playback_ctl;
    PlaylistManager* playlist_manager_;
    MainWindow* main_window_;
    QPointer<TagEditWidget> tag_edit_widget_;

    bool m_bound = false;
};
