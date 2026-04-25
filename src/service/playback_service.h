#pragma once

#include <QObject>
#include <QWidget>

#include "view/MainWindow.h"
#include "controller/PlaybackController.h"
#include "controller/PlaylistController.h"
#include "view/WControlBar/WControlBar.h"

class PlaybackService : public QObject
{
    Q_OBJECT

public:
    explicit PlaybackService(MainWindow* main_window, PlaybackController* playback_ctl,
                             PlaylistController* playlist_ctl, QObject *parent);
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
    void handlePlayTrackRequest(const QString &filepath);

private:
    // just "borrow" a "pointer_view", so use raw pointer is safe
    MainWindow* m_main_window = nullptr;
    PlaybackController* m_playback_ctl = nullptr;
    PlaylistController* m_playlist_ctl = nullptr;
    WControlBar* m_control_bar = nullptr;

    bool m_locate_on_next_play_request = false;
    bool m_bound = false;
};