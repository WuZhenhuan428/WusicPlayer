#pragma once

#include <QObject>
#include <memory>
#include <QPointer>
#include <QVector>
#include <QByteArray>

class QListWidgetItem;
class PlaylistSearchPanel;
class SettingsPanel;
class ShortcutsPanel;
class ShortcutsController;
class PlaylistManager;
class PlaylistController;
class InMemorySearchBackend;

class PlaybackController;
class MainWindow;

class IConfigBinder;
class DesktopLyricsBinder;
class LibraryViewBinder;
class PlaybackConfigBinder;
class SearchPanelBinder;
class WindowConfigBinder;
class SettingsPanelBinder;
class ShortcutsBinder;

class DesktopLyricsSection;
class LibraryViewSection;
class PlaybackConfigSection;
class SearchPanelSection;
class WindowConfigSection;
class SettingsPanelSection;
class ShortcutsSection;

class PlaybackRestoreCoordinator;
class MainWindowConfigContext;

class LyricsSettingPanel;

class AppController : public QObject
{
    Q_OBJECT
public:
    explicit AppController(PlaybackController* playbackController, QObject* parent = nullptr);
    ~AppController() override;

    void showMainWindow();
    QByteArray m_settings_panel_geo_cache;

private:
    void initializeCoreConnections();
    void handlePlayTrackRequest(const QString& filepath);
    void handleSetSortRuleRequested();
    void handleInsertColumnRequested();
    void handleRemoveColumnRequested();
    void handleShowAboutMessagebox();
    void handleShowDesktopLyricsRequested();
    void configureDesktopLyricsWindowRelation();
    void refreshPlaylistView();
    void ensureSettingsPanel();
    void ensureShortcutsController();
    void ensureShortcutsPage();
    void registerDefaultShortcuts();
    void ensureSearchPanel();
    void initializeConfig();
    void applyConfig();
    void saveConfig();
    MainWindowConfigContext buildConfigContext() const;

private slots:
    void onOpenSettingsPanelRequested();
    void onOpenSearchPanelRequested();

private:
    PlaybackController* m_playback_controller = nullptr;
    std::unique_ptr<PlaylistManager> m_playlist_manager;
    std::unique_ptr<PlaylistController> m_playlist_controller;
    std::unique_ptr<InMemorySearchBackend> m_search_backend;
    std::unique_ptr<MainWindow> m_main_window;

    std::unique_ptr<DesktopLyricsSection> m_desktop_lyrics_section;
    std::unique_ptr<LibraryViewSection> m_library_view_section;
    std::unique_ptr<PlaybackConfigSection> m_playback_config_section;
    std::unique_ptr<SearchPanelSection> m_search_panel_section;
    std::unique_ptr<WindowConfigSection> m_window_config_section;
    std::unique_ptr<SettingsPanelSection> m_settings_panel_section;
    std::unique_ptr<ShortcutsSection> m_shortcuts_section;

    QVector<IConfigBinder*> m_binders;
    std::unique_ptr<DesktopLyricsBinder> m_desktop_lyrics_binder;
    std::unique_ptr<LibraryViewBinder> m_library_view_binder;
    std::unique_ptr<PlaybackConfigBinder> m_playback_config_binder;
    std::unique_ptr<SearchPanelBinder> m_search_panel_binder;
    std::unique_ptr<WindowConfigBinder> m_window_config_binder;
    std::unique_ptr<SettingsPanelBinder> m_settings_sanel_binder;
    std::unique_ptr<ShortcutsBinder> m_shortcuts_binder;

    std::unique_ptr<PlaybackRestoreCoordinator> m_playback_restore_coordinator;

    QPointer<SettingsPanel> m_settings_panel;
    QPointer<ShortcutsPanel> m_shortcuts_panel;
    QPointer<ShortcutsController> m_shortcuts_controller;
    QPointer<PlaylistSearchPanel> m_search_panel;
    QPointer<LyricsSettingPanel> m_lyrics_settings_panel;
    bool m_shortcuts_registered = false;
    bool m_desktop_lyrics_visible_cache = false;
    bool m_has_saved_config_on_exit = false;
};
