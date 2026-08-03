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

class AppController : public QObject
{
    Q_OBJECT
public:
    explicit AppController(PlaybackController* playbackController, QObject* parent = nullptr);
    ~AppController() override;

    void showMainWindow();

private:
    void initializeCoreConnections();
    void locateCurrentTrackInView();
    void handleSetSortRuleRequested();
    void handleInsertColumnRequested();
    void handleRemoveColumnRequested();
    void handleShowAboutMessagebox();
    void handleShowDesktopLyricsRequested();

    void configureDesktopLyricsWindowRelation();
    void initializeConfig();
    void saveConfig();

    void setup_status_bar_connections();

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
    std::unique_ptr<LibraryInteractionService> library_interaction_serivce_;
    std::unique_ptr<TagWritebackService> tag_writeback_service_;
    std::unique_ptr<ThemeService> theme_service_;
    std::unique_ptr<PlaybackQueueService> playback_queue_service_;
    std::unique_ptr<PanelCoordinator> panel_coordinator_;

    bool has_saved_config_on_exit_    = false;
    bool locate_on_next_play_request_ = false;
    QMetaObject::Connection lyrics_follow_conn_;
};
