#pragma once

#include "controller/playback_controller.h"
#include "controller/playlist_controller.h"
#include "view/control_bar/control_bar.h"
#include "view/main_window.h"

#include <QObject>
#include <QWidget>

class PlaybackService : public QObject
{
    Q_OBJECT

public:
    explicit PlaybackService(MainWindow* main_window, PlaybackController* playback_ctl,
                             PlaylistController* playlist_ctl, QObject* parent);
    ~PlaybackService();

    /**
     * general methods
     * bind(): setup connections (stolen from AppController)
     * start(): prepare resources, setup status
     * shutdown(): save status, disconnect (if need)
     */
    void bind();
    void start();
    void shutdown();

signals:
    void sgnLocateCurrentTrack();

private:
    void handle_play_track_request(const QString& filepath);

private:
    // just "borrow" a "pointer_view", so use raw pointer is safe
    MainWindow* main_window_           = nullptr;
    PlaybackController* m_playback_ctl = nullptr;
    PlaylistController* m_playlist_ctl = nullptr;
    ControlBar* m_control_bar          = nullptr;

    bool locate_on_next_play_request_  = false;
    bool m_bound                       = false;
};
