#pragma once

#include "core/types.h"

#include <QObject>
#include <QWidget>

class AppContext;
class MainWindow;
class ControlBar;
class PlaybackController;
class PlaylistController;

class PlaybackService : public QObject
{
    Q_OBJECT

public:
    explicit PlaybackService(AppContext& ctx, QObject* parent);
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

    void play_next_request();
    void play_prev_request();
    bool is_playing();

signals:
    void sgnLocateCurrentTrack();

private:
    void play_track_request(const EntryId& eid);
    void handle_play_track_request(const QString& filepath);

private:
    AppContext& ctx_;

    // alias from AppContext
    MainWindow* main_window           = nullptr;
    ControlBar* control_bar           = nullptr;
    PlaybackController* playback_ctl  = nullptr;
    PlaylistController* playlist_ctl  = nullptr;

    bool locate_on_next_play_request_ = false;
    bool m_bound                      = false;
};
