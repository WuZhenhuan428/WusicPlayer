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

#include "core/ConfigManager/IConfigSection.hpp"
#include "core/ConfigManager/WindowConfigSection.hpp"
#include "core/ConfigManager/PlaybackConfigSection.hpp"
#include "core/ConfigManager/SearchPanelSection.hpp"
#include "core/ConfigManager/LibraryViewSection.hpp"
#include "core/ConfigManager/DesktopLyricsSection.hpp"

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

    bool m_cacheLoadScheduled = false;
    void restoreLastTrackWhenModelReady(int retry, qint64 last_pos);

    playlistId m_pendingRestorePlaylistId;
    trackId m_pendingRestoreTrackIdId;
    int m_pendingRestorePositionMs = 0;

    void initUI();
    void initConnection();
    
    // Config Manager
    void applyConfig();
    void saveConfig();
    WindowConfigSection* m_windowConfigSection;
    PlaybackConfigSection* m_playbackConfigSection;
    SearchPanelSection* m_searchPanelSection;
    LibraryViewSection* m_libraryViewSection;
    DesktopLyricsSection* m_desktopLyricsSection;
    
    // UI Action
    void onOpenFile();
    
    void onOpenSearchPanel();
    void playTrack(const QString& filepath);
    
    // UI Widgets declaraion
    /// Menu widgets
    QMenuBar* mainMenuBar;
    QToolBar* bottomToolBar;
    WControlBar* controlBar;

    /// menu File
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
    
    // +++main window
    /// Playlist | song table | cover & rolling lyrics
    
    LibraryWidget* m_libraryPanel;
    SidePanel* m_sidePanel;
    
    QWidget* centerWidget;
    QHBoxLayout* mainLayout;
    // ---main window

    PlaylistSearchPanel* searchPanel = nullptr;
    QByteArray m_searchPanelHeaderStateCache;
    QByteArray m_searchPanelGeoCache;

    DesktopLyricsWidget* m_desktoplyricsWidget;

private slots:
    void updatePlaylist();
    
signals:
    void sgnLoadPlaylist();
    void sgnCurrentPlaylistChanged(playlistId pid);
};
