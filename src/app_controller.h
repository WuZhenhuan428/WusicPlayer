#pragma once

#include <QByteArray>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QVector>
#include <memory>

class PlaylistManager;
class PlaylistController;
class InMemorySearchBackend;

class PlaybackController;
class MainWindow;
class StatusBarController;
class PanelCoordinator;

class PlaybackService;
class PlaybackRestoreService;
class LibraryInteractionService;
class TagWritebackService;
class ThemeService;
class LibraryManager;
class PlaybackQueueService;
class NotificationService;

class EventBus;

class QSystemTrayIcon;

#include "app_context.h"

class AppController : public QObject
{
    Q_OBJECT
public:
    explicit AppController(PlaybackController* playback_controller, QObject* parent = nullptr);
    ~AppController() override;

    void show_main_window();

private:
    void initialize_core_connections();
    void locate_current_track_in_view();
    void handle_set_sort_rule_requested();
    void handle_insert_column_requested();
    void handle_remove_column_requested();
    void handle_show_about_messagebox();
    void handle_show_desktop_lyrics_requested();

    void configure_desktop_lyrics_window_relation();
    void initialize_config();
    void save_config();

    void setup_status_bar_connections();

    void initialize_sys_tray();

private:
    PlaybackController* playback_controller_ = nullptr;
    std::unique_ptr<PlaylistManager> playlist_manager_;
    std::unique_ptr<PlaylistController> playlist_controller_;
    std::unique_ptr<LibraryManager> library_manager_;
    std::unique_ptr<InMemorySearchBackend> search_backend_;
    std::unique_ptr<MainWindow> main_window_;
    std::unique_ptr<StatusBarController> status_bar_controller_;

    std::unique_ptr<PlaybackService> playback_service_;
    std::unique_ptr<PlaybackRestoreService> playback_restore_service_;
    std::unique_ptr<LibraryInteractionService> library_interaction_service_;
    std::unique_ptr<TagWritebackService> tag_writeback_service_;
    std::unique_ptr<ThemeService> theme_service_;
    std::unique_ptr<PlaybackQueueService> playback_queue_service_;
    std::unique_ptr<PanelCoordinator> panel_coordinator_;
    std::unique_ptr<NotificationService> notification_service_;

    std::unique_ptr<EventBus> event_bus_;

    QSystemTrayIcon* sys_tray_;

    AppContext app_context_;

    bool has_saved_config_on_exit_    = false;
    bool locate_on_next_play_request_ = false;
    QMetaObject::Connection lyrics_follow_conn_;
};
