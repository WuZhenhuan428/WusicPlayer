#pragma once

#include "controller/playback_controller.h"
#include "controller/playlist_controller.h"
#include "core/config_manager/i_configurable.h"
#include "view/control_bar/control_bar.h"
#include "view/desktop_lyrics_widget/desktop_lyrics_widget.h"
#include "view/library_browser/library_browser_widget.h"
#include "view/playlist_tree/playlist_tree_widget.h"
#include "view/side_panel/side_panel.h"
#include "view/song_table/song_table_view.h"

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
    PlaylistController* playlist_controller() const;
    PlaybackController* playback_controller() const;
    PlaylistTreeWidget* playlist_tree_widget() const;
    LibraryBrowserWidget* library_browser() const;
    SongTableView* song_table_view() const;
    SidePanel* side_panel() const;
    ControlBar* control_bar_widget() const;
    DesktopLyricsWidget* desktop_lyrics_widget() const;
    void play_track_in_ui(const QString& filepath);

    void load_from_json(const QJsonObject& json) override;
    QJsonObject save_to_json() override;
    QString config_sub_key() const override;

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    PlaybackController* playback_controller_;
    PlaylistController* playlist_controller_;

    bool m_cache_load_scheduled = false;

    void init_ui();
    void build_menu_bar();
    void build_bottom_tool_bar();
    void build_central_area();
    void init_connection();
    void init_menu_connections();

    // UI Action
    void on_open_file();

    // UI Widgets declaraion
    /// Menu widgets
    QMenuBar* m_menubar_main;
    QToolBar* m_bottom_toolbar;
    ControlBar* m_control_bar = nullptr;

    // menu File
    QMenu* m_menu_file;
    QAction* m_act_open_file;
    QAction* m_act_new_playlist;
    QAction* m_act_load_playlist;
    QAction* m_act_exit;

    // menu View
    QMenu* m_menu_view;
    QAction* m_act_set_sort_rule;
    QAction* m_act_insert_column;
    QAction* m_act_remove_column;
    QAction* m_act_search_panel;
    QAction* m_act_log_viewer;
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
    void sgnOpenLogViewerRequested();
    void sgnOpenSettingsPanelRequested();
    void sgnCreatePlaylistRequested();
    void sgnPlayTrackRequested(const QString& filepath);
    void sgnSetSortRuleRequested();
    void sgnInsertColumnRequested();
    void sgnRemoveColumnRequested();
    void sgnShowAboutMessagebox();
    void sgnShowDesktopLyricsRequested();
    void sgnOpenEQWidgetRequested();
};
