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

private slots:
    void onOpenSettingsPanelRequested();
    void onOpenSearchPanelRequested();

private:
    PlaybackController* m_playback_controller = nullptr;
    std::unique_ptr<PlaylistManager> m_playlist_manager;
    std::unique_ptr<PlaylistController> m_playlist_controller;
    std::unique_ptr<InMemorySearchBackend> m_search_backend;
    std::unique_ptr<MainWindow> m_main_window;

    std::unique_ptr<PlaybackService> m_playback_service;
    std::unique_ptr<PlaybackRestoreService> m_playback_restore_service;
    std::unique_ptr<LibraryInteractionService> m_library_interaction_serivce;
    std::unique_ptr<TagWritebackService> m_tag_writeback_service;
    std::unique_ptr<ThemeService> m_theme_service;

    QPointer<SettingsPanel> m_settings_panel;
    QPointer<ShortcutsPanel> m_shortcuts_panel;
    QPointer<ShortcutsController> m_shortcuts_controller;
    QPointer<SearchPanel> m_search_panel;
    QPointer<LyricsSettingPanel> m_lyrics_settings_panel;
    QPointer<ThemeSettingsPage> m_theme_settings_page;
    QPointer<TagEditWidget> m_tag_edit_widget;
    QPointer<EQWidget> m_eq_widget;
    bool m_shortcuts_registered        = false;
    bool m_has_saved_config_on_exit    = false;
    bool m_locate_on_next_play_request = false;
    QMetaObject::Connection m_lyrics_follow_conn;
};
