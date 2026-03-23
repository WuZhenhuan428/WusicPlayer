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
    PlaybackController* m_playback_controller;
    PlaylistController* m_playlist_controller;

    bool m_cache_load_scheduled = false;

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
    QMenuBar* m_menubar_main;
    QToolBar* m_bottom_toolbar;
    WControlBar* m_control_bar = nullptr;

    // menu File
    QMenu* m_menu_file;
    QAction* m_act_open_file;
    QAction* m_act_add_file;
    QAction* m_act_add_folder;
    QAction* m_act_new_playlist;
    QAction* m_act_load_playlist;
    QAction* m_act_copy_playlist;
    QAction* m_act_rename_playlist;
    QAction* m_act_remove_playlist;
    QAction* m_act_save_playlist;
    QAction* m_act_exit;
    
    // menu View
    QMenu* m_menu_view;
    QAction* m_act_set_sort_rule;
    QAction* m_act_insert_column;
    QAction* m_act_remove_column;
    QAction* m_act_search_panel;
    QAction* m_act_show_desktop_lyrics;
    
    // menu Help
    QMenu* m_menu_help;
    QAction* m_act_manual;
    QAction* m_act_about;

    // menu Settings
    QMenu* m_menu_settings;
    QAction* m_act_settings;
    
    // +++main window
    LibraryWidget* m_library_panel = nullptr;
    SidePanel* m_side_panel = nullptr;
    QWidget* m_center_widget;
    QHBoxLayout* m_hbl_main;

    PlaylistSearchPanel* m_search_panel = nullptr;
    DesktopLyricsWidget* m_desktop_lyrics_widget = nullptr;

public:
    QByteArray m_search_panel_header_state_cache;
    QByteArray m_search_panel_geo_cache;
    
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
