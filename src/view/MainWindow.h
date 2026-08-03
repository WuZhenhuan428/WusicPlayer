#pragma once

#include "controller/PlaybackController.h"
#include "controller/PlaylistController.h"
#include "core/ConfigManager/IConfigurable.h"
#include "view/DesktopLyricsWidget/DesktopLyricsWidget.h"
#include "view/SidePanel/SidePanel.h"
#include "view/WControlBar/WControlBar.h"
#include "view/library_browser/LibraryBrowserWidget.h"
#include "view/playlist_tree/PlaylistTreeWidget.h"
#include "view/song_table/SongTableView.h"

#include <QAction>
#include <QByteArray>
#include <QCloseEvent>
#include <QDebug>
#include <QDialog>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListView>
#include <QListWidgetItem>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSlider>
#include <QSplitter>
#include <QString>
#include <QTableView>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

class MainWindow : public QMainWindow, public IConfigurable
{
    Q_OBJECT

public:
    MainWindow(PlaybackController* playback_controller, PlaylistController* playlist_controller,
               QWidget* parent = nullptr);
    ~MainWindow();
    // widget getter
    PlaylistController* playlistController() const;
    PlaybackController* playbackController() const;
    PlaylistTreeWidget* playlistTreeWidget() const;
    LibraryBrowserWidget* libraryBrowser() const;
    SongTableView* songTableView() const;
    SidePanel* sidePanel() const;
    WControlBar* controlBarWidget() const;
    DesktopLyricsWidget* desktopLyricsWidget() const;
    void playTrackInUi(const QString& filepath);

    void loadFromJson(const QJsonObject& json) override;
    QJsonObject saveToJson() override;
    QString configSubKey() const override;

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    PlaybackController* playback_controller_;
    PlaylistController* playlist_controller_;

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
    QAction* m_act_exit;

    // menu View
    QMenu* m_menu_view;
    QAction* m_act_set_sort_rule;
    QAction* m_act_insert_column;
    QAction* m_act_remove_column;
    QAction* m_act_search_panel;
    QAction* m_act_show_desktop_lyrics;
    QAction* m_act_lock_desktop_lyrics;

    // menu playback
    QMenu* m_menu_playback;
    QAction* m_act_open_eq;

    // menu Help
    QMenu* m_menu_help;
    QAction* m_act_about;

    // menu Settings
    QMenu* m_menu_settings;
    QAction* m_act_settings;

    // center window:组合三个内容控件 + 歌词面板
    PlaylistTreeWidget* m_playlist_tree_widget = nullptr; // 左上方:播放列表导航
    LibraryBrowserWidget* m_library_browser    = nullptr; // 左下方:媒体库浏览
    SongTableView* m_song_table_view           = nullptr; // 右侧:当前播放列表歌曲表
    QSplitter* m_left_splitter                 = nullptr; // 左:播放列表树(上)+ 库控件(下)
    QSplitter* m_main_splitter                 = nullptr; // 主:左 + 歌曲表
    SidePanel* m_side_panel                    = nullptr;
    QWidget* m_center_widget;
    QHBoxLayout* m_hbl_centre;

    DesktopLyricsWidget* m_desktop_lyrics_widget = nullptr;

signals:
    void sgnLoadPlaylist();
    void sgnCurrentPlaylistChanged(PlaylistId pid);
    void sgnAboutToClose();
    void sgnOpenSearchPanelRequested();
    void sgnOpenSettingsPanelRequested();
    void sgnImportFilesRequested();
    void sgnImportFolderRequested();
    void sgnCreatePlaylistRequested();
    void sgnPlayTrackRequested(const QString& filepath);
    void sgnSetSortRuleRequested();
    void sgnInsertColumnRequested();
    void sgnRemoveColumnRequested();
    void sgnShowAboutMessagebox();
    void sgnShowDesktopLyricsRequested();
    void sgnOpenEQWidgetRequested();
};
