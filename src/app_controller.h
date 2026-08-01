#pragma once

#include <QByteArray>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QVector>
#include <memory>

class QListWidgetItem;
class SearchPanel;
class SettingsPanel;
class ShortcutsPanel;
class ShortcutsController;
class PlaylistManager;
class PlaylistController;
class InMemorySearchBackend;

class PlaybackController;
class MainWindow;
class StatusBarController;

class LyricsSettingPanel;
class TagEditWidget;
class EQWidget;

class PlaybackService;
class PlaybackRestoreService;
class LibraryInteractionService;
class TagWritebackService;
class ThemeService;
class ThemeSettingsPage;

class AppController : public QObject
{
    Q_OBJECT
public:
    explicit AppController(PlaybackController* playbackController, QObject* parent = nullptr);
    ~AppController() override;

    void showMainWindow();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void initializeCoreConnections();
    void locateCurrentTrackInView();
    void handleSetSortRuleRequested();
    void handleInsertColumnRequested();
    void handleRemoveColumnRequested();
    void handleShowAboutMessagebox();
    void handleShowDesktopLyricsRequested();
    void handleOpenEQRequested();

    void configureDesktopLyricsWindowRelation();
    void ensureSettingsPanel();
    void ensureShortcutsController();
    void ensureShortcutsPage();
    void registerDefaultShortcuts();
    void ensureSearchPanel();
    void initializeConfig();
    void saveConfig();

    void setup_status_bar_connections();

private slots:
    void onOpenSettingsPanelRequested();
    void onOpenSearchPanelRequested();

private:
    PlaybackController* playback_controller_ = nullptr;
    std::unique_ptr<PlaylistManager> playlist_manager_;
    std::unique_ptr<PlaylistController> playlist_controller_;
    std::unique_ptr<InMemorySearchBackend> search_backend_;
    std::unique_ptr<MainWindow> main_window_;
    std::unique_ptr<StatusBarController> status_bar_controller_;

    std::unique_ptr<PlaybackService> playback_service_;
    std::unique_ptr<PlaybackRestoreService> playback_restore_service_;
    std::unique_ptr<LibraryInteractionService> library_interaction_serivce_;
    std::unique_ptr<TagWritebackService> tag_writeback_service_;
    std::unique_ptr<ThemeService> theme_service_;

    QPointer<SettingsPanel> settings_panel_;
    QPointer<ShortcutsPanel> shortcuts_panel_;
    QPointer<ShortcutsController> shortcuts_controller_;
    QPointer<SearchPanel> search_panel_;
    QPointer<LyricsSettingPanel> lyrics_settings_panel_;
    QPointer<ThemeSettingsPage> theme_settings_page_;
    QPointer<TagEditWidget> tag_edit_widget_;
    QPointer<EQWidget> eq_widget_;
    bool has_shortcuts_registered_    = false;
    bool has_saved_config_on_exit_    = false;
    bool locate_on_next_play_request_ = false;
    QMetaObject::Connection lyrics_follow_conn_;
};
