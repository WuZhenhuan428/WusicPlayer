#include "view/main_window.h"

#include <QApplication>
#include <QHeaderView>
#include <QJsonObject>
#include <QPointer>
#include <QStatusBar>
#include <QTimer>

#include "core/logger/logger_manager.h"
namespace
{
// NOLINTNEXTLINE
Logger* logger = LoggerManager::file_logger("main_window", {"console", "gui"});
} // namespace

MainWindow::MainWindow(PlaybackController* playback_controller,
                       PlaylistController* playlist_controller, QWidget* parent) :
    QMainWindow(parent), playback_controller_(playback_controller),
    playlist_controller_(playlist_controller)
{
    this->setMinimumSize(960, 540);
    this->init_ui();
    this->init_connection();
}

PlaylistController* MainWindow::playlist_controller() const
{
    return playlist_controller_;
}

PlaybackController* MainWindow::playback_controller() const
{
    return playback_controller_;
}

PlaylistTreeWidget* MainWindow::playlist_tree_widget() const
{
    return playlist_tree_widget_;
}

LibraryBrowserWidget* MainWindow::library_browser() const
{
    return library_browser_;
}

SongTableView* MainWindow::song_table_view() const
{
    return song_table_view_;
}

SidePanel* MainWindow::side_panel() const
{
    return side_panel_;
}

ControlBar* MainWindow::control_bar_widget() const
{
    return control_bar_;
}

DesktopLyricsWidget* MainWindow::desktop_lyrics_widget() const
{
    return desktop_lyrics_widget_;
}

void MainWindow::play_track_in_ui(const QString& filepath)
{
    emit sgn_play_track_requested(filepath);
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    if (cache_load_scheduled_) {
        return;
    }
    cache_load_scheduled_ = true;
    QTimer::singleShot(0, playlist_controller_, &PlaylistController::load_cache_after_shown);
}

void MainWindow::init_connection()
{
    init_menu_connections();
}

void MainWindow::init_menu_connections()
{
    connect(act_open_file_, &QAction::triggered, this, &MainWindow::on_open_file);
    connect(act_new_playlist_, &QAction::triggered, this,
            &MainWindow::sgn_create_playlist_requested);
    connect(act_load_playlist_, &QAction::triggered, this, &MainWindow::sgn_load_playlist);

    connect(act_exit_, &QAction::triggered, this, &MainWindow::quit_application);
    connect(act_about_, &QAction::triggered, this,
            &MainWindow::sgn_show_about_messagebox_requested);
    connect(act_set_sort_rule_, &QAction::triggered, this,
            &MainWindow::sgn_set_sort_rule_requested);

    connect(act_open_eq_, &QAction::triggered, this, &MainWindow::sgn_open_eq_widget_requested);

    connect(act_settings_, &QAction::triggered, this,
            &MainWindow::sgn_open_settings_panel_requested);

    connect(act_search_panel_, &QAction::triggered, this,
            &MainWindow::sgn_oepn_search_panel_requested);
    connect(act_log_viewer_, &QAction::triggered, this, &MainWindow::sgn_open_log_viewer_requested);
    connect(act_show_desktop_lyrics_, &QAction::toggled, this, [this](bool checked) {
        if (desktop_lyrics_widget_) {
            desktop_lyrics_widget_->setVisible(checked);
        }
    });
    connect(act_lock_desktop_lyrics_, &QAction::triggered, this, [this]() {
        if (desktop_lyrics_widget_) {
            desktop_lyrics_widget_->set_locked(!desktop_lyrics_widget_->is_locked());
        }
    });
}

void MainWindow::init_ui()
{
    // Global config
    this->setContextMenuPolicy(Qt::NoContextMenu);

    build_menu_bar();
    build_bottom_tool_bar();
    build_central_area();
}

void MainWindow::build_menu_bar()
{
    menubar_main_      = new QMenuBar;

    menu_file_         = new QMenu(tr("&File"), menubar_main_);
    act_open_file_     = new QAction(tr("&Open"), menu_file_);
    act_new_playlist_  = new QAction(tr("&New playlist"), menu_file_);
    act_load_playlist_ = new QAction(tr("&Load playlist"), menu_file_);
    act_exit_          = new QAction(tr("&Exit"), menu_file_);
    menu_file_->addAction(act_open_file_);
    menu_file_->addSeparator();
    menu_file_->addAction(act_new_playlist_);
    menu_file_->addAction(act_load_playlist_);
    menu_file_->addSeparator();
    menu_file_->addAction(act_exit_);
    menubar_main_->addMenu(menu_file_);

    menu_view_         = new QMenu(tr("&View"), menubar_main_);
    act_set_sort_rule_ = new QAction(tr("Set sort rule"), menu_view_);
    act_search_panel_  = new QAction(tr("Open search panel (&S)"), menu_view_);
    act_log_viewer_    = new QAction(tr("Open Log (&L)"), menu_view_);
    menu_view_->addAction(act_set_sort_rule_);
    menu_view_->addAction(act_search_panel_);
    menu_view_->addAction(act_log_viewer_);
    menu_view_->addSeparator();
    act_show_desktop_lyrics_ = new QAction(tr("Show Desktop Lyrics (&D)"), menu_view_);
    act_show_desktop_lyrics_->setCheckable(true);
    act_lock_desktop_lyrics_ = new QAction(tr("Lock Desktop Lyrics"), menu_view_);
    act_lock_desktop_lyrics_->setCheckable(true);
    menu_view_->addAction(act_show_desktop_lyrics_);
    menu_view_->addAction(act_lock_desktop_lyrics_);
    menubar_main_->addMenu(menu_view_);

    menu_playback_ = new QMenu(tr("&Playback"), menubar_main_);
    act_open_eq_   = new QAction(tr("&Open EQ"), menu_playback_);
    menu_playback_->addAction(act_open_eq_);
    menubar_main_->addMenu(menu_playback_);

    menu_settings_ = new QMenu(tr("&Settings"), menubar_main_);
    act_settings_  = new QAction(tr("&Settings"), menu_settings_);
    menu_settings_->addAction(act_settings_);
    menubar_main_->addMenu(menu_settings_);

    menu_help_ = new QMenu(tr("&Help"), menubar_main_);
    act_about_ = new QAction(tr("&About"), menu_help_);
    menu_help_->addAction(act_about_);
    menubar_main_->addMenu(menu_help_);

    setMenuBar(menubar_main_);
}

void MainWindow::build_bottom_tool_bar()
{
    // Bottom toolbar, btn & progress bar
    // PushButton instant -> BottomToolBarArea
    bottom_toolbar_ = new QToolBar(this);
    bottom_toolbar_->setObjectName("BottomToolBar");
    bottom_toolbar_->setMovable(false);
    bottom_toolbar_->setFloatable(false);
    control_bar_ = new ControlBar(bottom_toolbar_);
    bottom_toolbar_->addWidget(control_bar_);
    addToolBar(Qt::BottomToolBarArea, bottom_toolbar_);
    control_bar_->set_device(playback_controller_->available_devices(),
                             playback_controller_->current_device_id());
}

void MainWindow::build_central_area()
{
    // center window
    // 内容区:左侧(播放列表导航 + 媒体库浏览,垂直)、右侧当前播放列表歌曲表
    center_widget_        = new QWidget(this);
    playlist_tree_widget_ = new PlaylistTreeWidget(center_widget_);
    library_browser_      = new LibraryBrowserWidget(center_widget_);
    left_splitter_        = new QSplitter(Qt::Vertical, center_widget_);
    left_splitter_->addWidget(playlist_tree_widget_);
    left_splitter_->addWidget(library_browser_);
    left_splitter_->setStretchFactor(0, 3);
    left_splitter_->setStretchFactor(1, 2);
    left_splitter_->setChildrenCollapsible(true);

    song_table_view_ = new SongTableView(center_widget_);
    song_table_view_->set_model(playlist_controller_->view_model());
    main_splitter_ = new QSplitter(Qt::Horizontal, center_widget_);
    main_splitter_->addWidget(left_splitter_);
    main_splitter_->addWidget(song_table_view_);
    main_splitter_->setStretchFactor(0, 1);
    main_splitter_->setStretchFactor(1, 3);
    main_splitter_->setChildrenCollapsible(false);
    main_splitter_->setContentsMargins(0, 0, 0, 0);

    side_panel_ = new SidePanel(center_widget_);
    connect(side_panel_, &SidePanel::sgn_toggle_desktop_lyrics, this, [this]() {
        if (desktop_lyrics_widget_) {
            desktop_lyrics_widget_->setVisible(!desktop_lyrics_widget_->isVisible());
        }
    });
    connect(side_panel_, &SidePanel::sgn_toggle_desktop_lyricsLock, this, [this]() {
        if (desktop_lyrics_widget_) {
            desktop_lyrics_widget_->set_locked(!desktop_lyrics_widget_->is_locked());
        }
    });
    hbl_centre_ = new QHBoxLayout(center_widget_);
    hbl_centre_->setContentsMargins(0, 0, 0, 0);
    hbl_centre_->setSpacing(0);
    hbl_centre_->addWidget(main_splitter_, 7);
    hbl_centre_->addWidget(side_panel_, 2);
    hbl_centre_->setContentsMargins(0, 0, 0, 0);
    setCentralWidget(center_widget_);

    // desktop lrc panel
    /**
     * WARN: to use on wayland, here can not set parent object, but may cause memory leak
     */
    desktop_lyrics_widget_ = new DesktopLyricsWidget();
    // Keep View menu check state in sync with widget
    connect(desktop_lyrics_widget_, &DesktopLyricsWidget::sgn_visibility_changed,
            act_show_desktop_lyrics_, &QAction::setChecked);
    connect(desktop_lyrics_widget_, &DesktopLyricsWidget::sgn_lock_changed,
            act_lock_desktop_lyrics_, &QAction::setChecked);
}

void MainWindow::load_from_json(const QJsonObject& json)
{
    const QJsonObject obj     = json.value(this->config_sub_key()).toObject();
    const QByteArray geometry = QByteArray::fromBase64(obj.value("geometry").toString().toUtf8());
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    const QByteArray state = QByteArray::fromBase64(obj.value("state").toString().toUtf8());
    if (!state.isEmpty()) {
        restoreState(state);
    }
    // 内容区 splitter 布局:主(左 + 歌曲表)、左垂直(播放列表树 + 库控件)
    // 注意:Qt::Orientation 枚举值 Horizontal=1、Vertical=2;配置缺失时 toInt() 返回 0,
    // 直接强转会把方向设为无效值(按垂直处理 → 布局变成上中下),故先校验。
    main_splitter_->restoreState(
        QByteArray::fromBase64(obj.value("splitter_state").toString().toUtf8()));
    const int orientation =
        obj.value("splitter_orientation").toInt(static_cast<int>(Qt::Horizontal));
    if (orientation == Qt::Horizontal || orientation == Qt::Vertical) {
        main_splitter_->setOrientation(static_cast<Qt::Orientation>(orientation));
    }
    left_splitter_->restoreState(
        QByteArray::fromBase64(obj.value("left_splitter_state").toString().toUtf8()));
}

QJsonObject MainWindow::save_to_json()
{
    QJsonObject obj;
    obj["geometry"]             = QString::fromUtf8(saveGeometry().toBase64());
    obj["state"]                = QString::fromUtf8(saveState().toBase64());
    obj["splitter_state"]       = QString::fromUtf8(main_splitter_->saveState().toBase64());
    obj["splitter_orientation"] = static_cast<int>(main_splitter_->orientation());
    obj["left_splitter_state"]  = QString::fromUtf8(left_splitter_->saveState().toBase64());
    return obj;
}

QString MainWindow::config_sub_key() const
{
    return "window";
}

void MainWindow::on_open_file()
{
    QString filepath = QFileDialog::getOpenFileName(this, tr("Open Audio File"), QString(),
                                                    tr("*.mp3 *.wav *.flac"));

    if (!filepath.isEmpty()) {
        emit sgn_play_track_requested(filepath);
    } else {
        logger->info("filepath is empty!");
    }
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (this->is_quit_) {
        logger->debug("MainWindow close event called, close branch");
        emit sgn_about_to_close();
        if (desktop_lyrics_widget_) {
            desktop_lyrics_widget_->close();
        }
        event->accept();
        // 托盘图标无法实际退出, 能重复进入此分支, 但是不重复保存配置
    } else {
        event->ignore();
        this->hide();
    }
}

void MainWindow::quit_application()
{
    this->is_quit_ = true;
    this->close();
    // 窗口可能已通过 closeEvent 的 hide 分支隐藏到托盘:此时 close() 不会触发
    // lastWindowClosed(), quitOnLastWindowClosed 不生效, 必须显式退出事件循环。
    QApplication::quit();
}
