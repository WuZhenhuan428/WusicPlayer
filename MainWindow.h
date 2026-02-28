#pragma once

#include <QMainWindow>
#include <QDebug>
#include <QString>
#include <QDialog>

#include <QResizeEvent>
#include <QShowEvent>

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

#include <QCloseEvent>
#include "src/ConfigManager/ConfigManager.h"

#include "src/playlist/playlist_manager.h"
#include "src/playlist/playlist_widgets.h"
#include "src/playlist/playlist_search_panel.h"
#include "src/WControlBar/WControlBar.h"
#include "src/SidePanel/SidePanel.h"

#include "src/LibraryWidget/LibraryWidget.h"
#include "src/controller/PlaylistController.h"

#include "src/controller/PlaybackController.h"

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
    PlaylistManager* m_playlistManager;
    PlaylistController* m_playlistController;
    PlaybackController* m_playbackController;

    bool m_cacheLoadScheduled = false;
    void restoreLastTrackWhenModelReady(int retry, qint64 last_pos);

    void initUI();
    void initConnection();
    
    // Configmanager
    void applyConfig();
    void saveConfig();

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

private slots:
    void updatePlaylist();
    
signals:
    void sgnLoadPlaylist();
    void sgnCurrentPlaylistChanged(playlistId pid);
};
