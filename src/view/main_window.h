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
    // widget getter
    [[nodiscard]] PlaylistController* playlist_controller() const;
    [[nodiscard]] PlaybackController* playback_controller() const;
    [[nodiscard]] PlaylistTreeWidget* playlist_tree_widget() const;
    [[nodiscard]] LibraryBrowserWidget* library_browser() const;
    [[nodiscard]] SongTableView* song_table_view() const;
    [[nodiscard]] SidePanel* side_panel() const;
    [[nodiscard]] ControlBar* control_bar_widget() const;
    [[nodiscard]] DesktopLyricsWidget* desktop_lyrics_widget() const;
    void play_track_in_ui(const QString& filepath);

    void load_from_json(const QJsonObject& json) override;
    QJsonObject save_to_json() override;
    [[nodiscard]] QString config_sub_key() const override;

    void quit_application();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    PlaybackController* playback_controller_;
    PlaylistController* playlist_controller_;

    bool cache_load_scheduled_ = false;
    bool is_quit_              = false; // 关联任务栏托盘行为

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
    QMenuBar* menubar_main_                     = nullptr;
    QToolBar* bottom_toolbar_                   = nullptr;
    ControlBar* control_bar_                    = nullptr;

    // menu File
    QMenu* menu_file_                           = nullptr;
    QAction* act_open_file_                     = nullptr;
    QAction* act_new_playlist_                  = nullptr;
    QAction* act_load_playlist_                 = nullptr;
    QAction* act_exit_                          = nullptr;

    // menu View
    QMenu* menu_view_                           = nullptr;
    QAction* act_set_sort_rule_                 = nullptr;
    QAction* act_search_panel_                  = nullptr;
    QAction* act_log_viewer_                    = nullptr;
    QAction* act_show_desktop_lyrics_           = nullptr;
    QAction* act_lock_desktop_lyrics_           = nullptr;

    // menu playback
    QMenu* menu_playback_                       = nullptr;
    QAction* act_open_eq_                       = nullptr;

    // menu Help
    QMenu* menu_help_                           = nullptr;
    QAction* act_about_                         = nullptr;

    // menu Settings
    QMenu* menu_settings_                       = nullptr;
    QAction* act_settings_                      = nullptr;

    // center window:组合三个内容控件 + 歌词面板
    PlaylistTreeWidget* playlist_tree_widget_   = nullptr; // 左上方:播放列表导航
    LibraryBrowserWidget* library_browser_      = nullptr; // 左下方:媒体库浏览
    SongTableView* song_table_view_             = nullptr; // 右侧:当前播放列表歌曲表
    QSplitter* left_splitter_                   = nullptr; // 左:播放列表树(上)+ 库控件(下)
    QSplitter* main_splitter_                   = nullptr; // 主:左 + 歌曲表
    SidePanel* side_panel_                      = nullptr;
    QWidget* center_widget_                     = nullptr;
    QHBoxLayout* hbl_centre_                    = nullptr;

    DesktopLyricsWidget* desktop_lyrics_widget_ = nullptr;

signals:
    void sgn_load_playlist();
    void sgn_current_playlist_changed(PlaylistId pid);
    void sgn_about_to_close();
    void sgn_oepn_search_panel_requested();
    void sgn_open_log_viewer_requested();
    void sgn_open_settings_panel_requested();
    void sgn_create_playlist_requested();
    void sgn_play_track_requested(const QString& filepath);
    void sgn_set_sort_rule_requested();
    void sgn_show_about_messagebox_requested();
    void sgn_show_desktop_lirics_requested();
    void sgn_open_eq_widget_requested();
};
