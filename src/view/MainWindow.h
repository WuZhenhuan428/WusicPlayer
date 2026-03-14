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
#include <QListWidgetItem>
#include <QTableView>
#include <QLabel>
#include <QListView>
#include <QHBoxLayout>

#include "view/playlist/playlist_widgets.h"
#include "view/search_panel/playlist_search_panel.h"
#include "view/WControlBar/WControlBar.h"
#include "view/SidePanel/SidePanel.h"

#include "view/LibraryWidget/LibraryWidget.h"
#include "controller/PlaylistController.h"

#include "controller/PlaybackController.h"
#include "view/DesktopLyricsWidget/DesktopLyricsWidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(PlaybackController* playback_controller, PlaylistController* playlist_controller, QWidget *parent = nullptr);
    ~MainWindow();
    // widget getter
    PlaylistController* playlistController() const;
    PlaybackController* playbackController() const;
    LibraryWidget* libraryPanel() const;
    SidePanel* sidePanel() const;
    WControlBar* controlBarWidget() const;
    DesktopLyricsWidget* desktopLyricsWidget() const;
    PlaylistSearchPanel* searchPanelWidget() const;
    
    void playTrackInUi(const QString& filepath);
    void setSearchPanel(PlaylistSearchPanel* panel);
    QByteArray searchPanelHeaderStateCache() const;
    QByteArray searchPanelGeometryCache() const;
    void setSearchPanelStateCache(const QByteArray& geometry, const QByteArray& header);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    PlaybackController* m_playbackController;
    PlaylistController* m_playlistController;

    bool m_cacheLoadScheduled = false;

    void initUI();
    void buildMenuBar();
    void buildBottomToolBar();
    void buildCentralArea();
    void initConnection();
    void initMenuConnections();
    
    // UI Action
    void onOpenFile();
    
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

    // menu Settings
    QMenu* menuSettings;
    QAction* actSettings;
    
    // +++main window
    LibraryWidget* m_libraryPanel = nullptr;
    SidePanel* m_sidePanel = nullptr;
    QWidget* centerWidget;
    QHBoxLayout* mainLayout;

    PlaylistSearchPanel* searchPanel = nullptr;
    DesktopLyricsWidget* m_desktoplyricsWidget = nullptr;

public:
    QByteArray m_searchPanelHeaderStateCache;
    QByteArray m_searchPanelGeoCache;
    
signals:
    void sgnLoadPlaylist();
    void sgnCurrentPlaylistChanged(playlistId pid);
    void sgnAboutToClose();
    void sgnOpenSearchPanelRequested();
    void sgnOpenSettingsPanelRequested();
    void sgnImportFilesRequested();
    void sgnImportFolderRequested();
    void sgnCreatePlaylistRequested();
    void sgnCopyPlaylistRequested();
    void sgnRenamePlaylistRequested();
    void sgnRemovePlaylistRequested();
    void sgnSavePlaylistRequested();
    void sgnPlayTrackRequested(const QString& filepath);
    void sgnSetSortRuleRequested();
    void sgnInsertColumnRequested();
    void sgnRemoveColumnRequested();
    void sgnShowAboutMessagebox();
    void sgnShowDesktopLyricsRequested();
};
