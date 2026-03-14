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

class PlaybackController;
class MainWindow;

class IConfigBinder;
class DesktopLyricsBinder;
class LibraryViewBinder;
class PlaybackConfigBinder;
class SearchPanelBinder;
class WindowConfigBinder;
class SettingsPanelBinder;

class DesktopLyricsSection;
class LibraryViewSection;
class PlaybackConfigSection;
class SearchPanelSection;
class WindowConfigSection;
class SettingsPanelSection;

class PlaybackRestoreCoordinator;
class MainWindowConfigContext;

class AppController : public QObject
{
    Q_OBJECT
public:
    explicit AppController(PlaybackController* playbackController, QObject* parent = nullptr);
    ~AppController() override;

    void showMainWindow();
    QByteArray m_settingsPanelGeoCache;

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
    void ensureShortcutsPage();
    void ensureSearchPanel();
    void initializeConfig();
    void applyConfig();
    void saveConfig();
    MainWindowConfigContext buildConfigContext() const;

private slots:
    void onOpenSettingsPanelRequested();
    void onOpenSearchPanelRequested();

private:
    PlaybackController* m_playbackController = nullptr;
    std::unique_ptr<PlaylistManager> m_playlistManager;
    std::unique_ptr<PlaylistController> m_playlistController;
    std::unique_ptr<MainWindow> m_mainWindow;

    std::unique_ptr<DesktopLyricsSection> m_desktopLyricsSection;
    std::unique_ptr<LibraryViewSection> m_libraryViewSection;
    std::unique_ptr<PlaybackConfigSection> m_playbackConfigSection;
    std::unique_ptr<SearchPanelSection> m_searchPanelSection;
    std::unique_ptr<WindowConfigSection> m_windowConfigSection;
    std::unique_ptr<SettingsPanelSection> m_settingsPanelSection;

    QVector<IConfigBinder*> m_binders;
    std::unique_ptr<DesktopLyricsBinder> m_desktopLyricsBinder;
    std::unique_ptr<LibraryViewBinder> m_libraryViewBinder;
    std::unique_ptr<PlaybackConfigBinder> m_playbackConfigBinder;
    std::unique_ptr<SearchPanelBinder> m_searchPanelBinder;
    std::unique_ptr<WindowConfigBinder> m_windowConfigBinder;
    std::unique_ptr<SettingsPanelBinder> m_settingsPanelBinder;

    std::unique_ptr<PlaybackRestoreCoordinator> m_playbackRestoreCoordinator;

    QPointer<SettingsPanel> m_settingsPanel;
    QPointer<ShortcutsPanel> m_shortcutsPanel;
    QPointer<ShortcutsController> m_shortcutsController;
    QPointer<PlaylistSearchPanel> m_searchPanel;
    QListWidgetItem* m_shortcutsPageItem = nullptr;
    bool m_desktopLyricsVisibleCache = false;
    bool m_hasSavedConfigOnExit = false;
};
