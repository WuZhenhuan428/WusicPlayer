#pragma once

#include <QMainWindow>
#include <QDebug>
#include <QString>
#include <QByteArray>

#include <QResizeEvent>
#include <QShowEvent>
#include <QCloseEvent>

#include <QDialog>
#include <QMenuBar>
#include <QToolBar>
#include <QMenu>
#include <QAction>
#include <QPushButton>
#include <QSlider>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTableView>
#include <QLabel>
#include <QListView>
#include <QHBoxLayout>

#include <memory>

#include "core/ConfigManager/ConfigManager.h"

#include "model/playlist/playlist_manager.h"
#include "view/playlist/playlist_widgets.h"
#include "view/search_panel/playlist_search_panel.h"
#include "view/WControlBar/WControlBar.h"
#include "view/SidePanel/SidePanel.h"

#include "view/LibraryWidget/LibraryWidget.h"
#include "controller/PlaylistController.h"

#include "controller/PlaybackController.h"
#include "view/DesktopLyricsWidget/DesktopLyricsWidget.h"


#include "view/ConfigBinder/IConfigSection.hpp"
#include "view/ConfigBinder/DesktopLyricsSection.hpp"
#include "view/ConfigBinder/LibraryViewSection.hpp"
#include "view/ConfigBinder/PlaybackConfigSection.hpp"
#include "view/ConfigBinder/SearchPanelSection.hpp"
#include "view/ConfigBinder/WindowConfigSection.hpp"

#include "view/ConfigBinder/MainWindowConfigContext.hpp"
#include "view/ConfigBinder/IConfigBinder.hpp"
#include "view/ConfigBinder/DesktopLyricsBinder.hpp"
#include "view/ConfigBinder/LibraryViewBinder.hpp"
#include "view/ConfigBinder/PlaybackConfigBinder.hpp"
#include "view/ConfigBinder/SearchPanelBinder.hpp"
#include "view/ConfigBinder/WindowConfigBinder.hpp"

#include "PlaybackRestoreCoordinator.hpp"

#include "view/SettingsPanel/SettingsPanel.hpp"
#include "view/SettingsPanel/ShortcutsPanel/ShortcutsPanel.hpp"
#include "controller/shortcuts_controller.hpp"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(PlaybackController* playback_controller, QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    PlaybackController* m_playbackController;
    PlaylistManager* m_playlistManager;
    PlaylistController* m_playlistController;

    std::unique_ptr<DesktopLyricsSection> m_desktopLyricsSection;
    std::unique_ptr<LibraryViewSection> m_libraryViewSection;
    std::unique_ptr<PlaybackConfigSection> m_playbackConfigSection;
    std::unique_ptr<SearchPanelSection> m_searchPanelSection;
    std::unique_ptr<WindowConfigSection> m_windowConfigSection;

    MainWindowConfigContext buildConfigContext();
    
    QVector<IConfigBinder*> m_binders;
    std::unique_ptr<DesktopLyricsBinder> m_desktopLyricsBinder;
    std::unique_ptr<LibraryViewBinder> m_libraryViewBinder;
    std::unique_ptr<PlaybackConfigBinder> m_playbackConfigBinder;
    std::unique_ptr<SearchPanelBinder> m_searchPanelBinder;
    std::unique_ptr<WindowConfigBinder> m_windowConfigBinder;
    std::unique_ptr<PlaybackRestoreCoordinator> m_playbackRestoreCoordinator;

    bool m_cacheLoadScheduled = false;

    playlistId m_pendingRestorePlaylistId;
    trackId m_pendingRestoreTrackIdId;
    int m_pendingRestorePositionMs = 0;

    void initUI();
    void initConnection();
    
    // Config Manager
    void applyConfig();
    void saveConfig();
    
    // UI Action
    void onOpenFile();
    
    void onOpenSearchPanel();
    void onOpenSettingsPanel();
    void playTrack(const QString& filepath);
    
    // UI Widgets declaraion
    /// Menu widgets
    QMenuBar* mainMenuBar;
    QToolBar* bottomToolBar;
    WControlBar* controlBar = nullptr;

    // menu File
    QMenu* menuFile;
    QAction* actOpenFile;
    QAction* actAddFile;
    QAction* actAddFolder;
    QAction* actNewPlaylist;
    QAction* actLoadPlaylist;
    QAction* actCopyPlaylist;
    QAction* actRenamePlaylist;
    QAction* actRemovePlaylist;
    QAction* actSavePlaylist;
    QAction* actExit;
    
    // menu View
    QMenu* menuView;
    QAction* actSetSortRule;
    QAction* actInsertColumn;
    QAction* actRemoveColumn;
    QAction* actSearchPanel;
    QAction* actShowDesktopLyrics;
    
    // menu Help
    QMenu* menuHelp;
    QAction* actManual;
    QAction* actAbout;

    // menu settings
    QMenu* menuSettings;
    QAction* actSettings;
    
    // +++main window
    /// Playlist | song table | cover & rolling lyrics
    
    LibraryWidget* m_libraryPanel = nullptr;
    SidePanel* m_sidePanel = nullptr;
    
    QWidget* centerWidget;
    QHBoxLayout* mainLayout;
    // ---main window

    PlaylistSearchPanel* searchPanel = nullptr;
    SettingsPanel* m_settingsPanel = nullptr;
    
    DesktopLyricsWidget* m_desktoplyricsWidget = nullptr;

    ShortcutsPanel* m_shortcutsPanel = nullptr;
    ShortcutsController* m_shortcutsController = nullptr;

public:
    QByteArray m_searchPanelHeaderStateCache;
    QByteArray m_searchPanelGeoCache;

private slots:
    void updatePlaylist();
    
signals:
    void sgnLoadPlaylist();
    void sgnCurrentPlaylistChanged(playlistId pid);
};
