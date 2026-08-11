#include "view/main_window.h"

#include <QHeaderView>
#include <QJsonObject>
#include <QPointer>
#include <QStatusBar>
#include <QTimer>

MainWindow::MainWindow(PlaybackController* playback_controller,
                       PlaylistController* playlist_controller, QWidget* parent) :
    QMainWindow(parent), playback_controller_(playback_controller),
    playlist_controller_(playlist_controller)
{
    this->setMinimumSize(960, 540);
    this->init_ui();
    this->init_connection();
}

MainWindow::~MainWindow() {}

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
    return m_playlist_tree_widget;
}

LibraryBrowserWidget* MainWindow::library_browser() const
{
    return m_library_browser;
}

SongTableView* MainWindow::song_table_view() const
{
    return m_song_table_view;
}

SidePanel* MainWindow::side_panel() const
{
    return m_side_panel;
}

ControlBar* MainWindow::control_bar_widget() const
{
    return m_control_bar;
}

DesktopLyricsWidget* MainWindow::desktop_lyrics_widget() const
{
    return m_desktop_lyrics_widget;
}

void MainWindow::play_track_in_ui(const QString& filepath)
{
    emit sgnPlayTrackRequested(filepath);
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    if (m_cache_load_scheduled) {
        return;
    }
    m_cache_load_scheduled = true;
    QTimer::singleShot(0, playlist_controller_, &PlaylistController::load_cache_after_shown);
}

void MainWindow::init_connection()
{
    init_menu_connections();
}

void MainWindow::init_menu_connections()
{
    connect(m_act_open_file, &QAction::triggered, this, &MainWindow::on_open_file);
    connect(m_act_new_playlist, &QAction::triggered, this, &MainWindow::sgnCreatePlaylistRequested);
    connect(m_act_load_playlist, &QAction::triggered, this, &MainWindow::sgnLoadPlaylist);

    connect(m_act_exit, &QAction::triggered, this, &QWidget::close);
    connect(m_act_about, &QAction::triggered, this, &MainWindow::sgnShowAboutMessagebox);
    connect(m_act_set_sort_rule, &QAction::triggered, this, &MainWindow::sgnSetSortRuleRequested);
    connect(m_act_insert_column, &QAction::triggered, this, &MainWindow::sgnInsertColumnRequested);
    connect(m_act_remove_column, &QAction::triggered, this, &MainWindow::sgnRemoveColumnRequested);

    connect(m_act_open_eq, &QAction::triggered, this, &MainWindow::sgnOpenEQWidgetRequested);

    connect(m_act_settings, &QAction::triggered, this, &MainWindow::sgnOpenSettingsPanelRequested);

    connect(m_act_search_panel, &QAction::triggered, this,
            &MainWindow::sgnOpenSearchPanelRequested);
    connect(m_act_log_viewer, &QAction::triggered, this, &MainWindow::sgnOpenLogViewerRequested);
    connect(m_act_show_desktop_lyrics, &QAction::toggled, this, [this](bool checked) {
        if (m_desktop_lyrics_widget)
            m_desktop_lyrics_widget->setVisible(checked);
    });
    connect(m_act_lock_desktop_lyrics, &QAction::triggered, this, [this]() {
        if (m_desktop_lyrics_widget) {
            m_desktop_lyrics_widget->set_locked(!m_desktop_lyrics_widget->isLocked());
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
    m_menubar_main      = new QMenuBar;

    m_menu_file         = new QMenu("&File", m_menubar_main);
    m_act_open_file     = new QAction("&Open", m_menu_file);
    m_act_new_playlist  = new QAction("New playlist", m_menu_file);
    m_act_load_playlist = new QAction("&Load playlist", m_menu_file);
    m_act_exit          = new QAction("&Exit", m_menu_file);
    m_menu_file->addAction(m_act_open_file);
    m_menu_file->addSeparator();
    m_menu_file->addAction(m_act_new_playlist);
    m_menu_file->addAction(m_act_load_playlist);
    m_menu_file->addSeparator();
    m_menu_file->addAction(m_act_exit);
    m_menubar_main->addMenu(m_menu_file);

    m_menu_view         = new QMenu("&View", m_menubar_main);
    m_act_set_sort_rule = new QAction("Set sort rule", m_menu_view);
    m_act_insert_column = new QAction("Insert a column (&I)", m_menu_view);
    m_act_remove_column = new QAction("Remove a column (&R)", m_menu_view);
    m_act_search_panel  = new QAction("Open search panel (&S)", m_menu_view);
    m_act_log_viewer    = new QAction("Open Log (&L)", m_menu_view);
    m_menu_view->addAction(m_act_set_sort_rule);
    m_menu_view->addAction(m_act_insert_column);
    m_menu_view->addAction(m_act_remove_column);
    m_menu_view->addAction(m_act_search_panel);
    m_menu_view->addAction(m_act_log_viewer);
    m_menu_view->addSeparator();
    m_act_show_desktop_lyrics = new QAction("Show Desktop Lyrics (&D)", m_menu_view);
    m_act_show_desktop_lyrics->setCheckable(true);
    m_act_lock_desktop_lyrics = new QAction("Lock Desktop Lyrics", m_menu_view);
    m_act_lock_desktop_lyrics->setCheckable(true);
    m_menu_view->addAction(m_act_show_desktop_lyrics);
    m_menu_view->addAction(m_act_lock_desktop_lyrics);
    m_menubar_main->addMenu(m_menu_view);

    m_menu_playback = new QMenu("&Playback", m_menubar_main);
    m_act_open_eq   = new QAction("&Open EQ", m_menu_playback);
    m_menu_playback->addAction(m_act_open_eq);
    m_menubar_main->addMenu(m_menu_playback);

    m_menu_settings = new QMenu("&Settings", m_menubar_main);
    m_act_settings  = new QAction("&Settings", m_menu_settings);
    m_menu_settings->addAction(m_act_settings);
    m_menubar_main->addMenu(m_menu_settings);

    m_menu_help = new QMenu("&Help", m_menubar_main);
    m_act_about = new QAction("&About", m_menu_help);
    m_menu_help->addAction(m_act_about);
    m_menubar_main->addMenu(m_menu_help);

    setMenuBar(m_menubar_main);
}

void MainWindow::build_bottom_tool_bar()
{
    // Bottom toolbar, btn & progress bar
    // PushButton instant -> BottomToolBarArea
    m_bottom_toolbar = new QToolBar(this);
    m_bottom_toolbar->setObjectName("BottomToolBar");
    m_bottom_toolbar->setMovable(false);
    m_bottom_toolbar->setFloatable(false);
    m_control_bar = new ControlBar(m_bottom_toolbar);
    m_bottom_toolbar->addWidget(m_control_bar);
    addToolBar(Qt::BottomToolBarArea, m_bottom_toolbar);
    m_control_bar->set_device(playback_controller_->available_devices(),
                              playback_controller_->current_device_id());
}

void MainWindow::build_central_area()
{
    // center window
    // 内容区:左侧(播放列表导航 + 媒体库浏览,垂直)、右侧当前播放列表歌曲表
    m_center_widget        = new QWidget(this);
    m_playlist_tree_widget = new PlaylistTreeWidget(m_center_widget);
    m_library_browser      = new LibraryBrowserWidget(m_center_widget);
    m_left_splitter        = new QSplitter(Qt::Vertical, m_center_widget);
    m_left_splitter->addWidget(m_playlist_tree_widget);
    m_left_splitter->addWidget(m_library_browser);
    m_left_splitter->setStretchFactor(0, 3);
    m_left_splitter->setStretchFactor(1, 2);
    m_left_splitter->setChildrenCollapsible(true);

    m_song_table_view = new SongTableView(m_center_widget);
    m_song_table_view->setModel(playlist_controller_->view_model());
    m_main_splitter = new QSplitter(Qt::Horizontal, m_center_widget);
    m_main_splitter->addWidget(m_left_splitter);
    m_main_splitter->addWidget(m_song_table_view);
    m_main_splitter->setStretchFactor(0, 1);
    m_main_splitter->setStretchFactor(1, 3);
    m_main_splitter->setChildrenCollapsible(false);
    m_main_splitter->setContentsMargins(0, 0, 0, 0);

    m_side_panel = new SidePanel(m_center_widget);
    connect(m_side_panel, &SidePanel::sgnToggleDesktopLyrics, this, [this]() {
        if (m_desktop_lyrics_widget)
            m_desktop_lyrics_widget->setVisible(!m_desktop_lyrics_widget->isVisible());
    });
    connect(m_side_panel, &SidePanel::sgnToggleDesktopLyricsLock, this, [this]() {
        if (m_desktop_lyrics_widget)
            m_desktop_lyrics_widget->set_locked(!m_desktop_lyrics_widget->isLocked());
    });
    m_hbl_centre = new QHBoxLayout(m_center_widget);
    m_hbl_centre->setContentsMargins(0, 0, 0, 0);
    m_hbl_centre->setSpacing(0);
    m_hbl_centre->addWidget(m_main_splitter, 7);
    m_hbl_centre->addWidget(m_side_panel, 2);
    m_hbl_centre->setContentsMargins(0, 0, 0, 0);
    setCentralWidget(m_center_widget);

    // desktop lrc panel
    /**
     * WARN: to use on wayland, here can not set parent object, but may cause memory leak
     */
    m_desktop_lyrics_widget = new DesktopLyricsWidget();
    // Keep View menu check state in sync with widget
    connect(m_desktop_lyrics_widget, &DesktopLyricsWidget::sgnVisibilityChanged,
            m_act_show_desktop_lyrics, &QAction::setChecked);
    connect(m_desktop_lyrics_widget, &DesktopLyricsWidget::sgnLockChanged,
            m_act_lock_desktop_lyrics, &QAction::setChecked);
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
    m_main_splitter->restoreState(
        QByteArray::fromBase64(obj.value("splitter_state").toString().toUtf8()));
    const int orientation =
        obj.value("splitter_orientation").toInt(static_cast<int>(Qt::Horizontal));
    if (orientation == Qt::Horizontal || orientation == Qt::Vertical) {
        m_main_splitter->setOrientation(static_cast<Qt::Orientation>(orientation));
    }
    m_left_splitter->restoreState(
        QByteArray::fromBase64(obj.value("left_splitter_state").toString().toUtf8()));
}

QJsonObject MainWindow::save_to_json()
{
    QJsonObject obj;
    obj["geometry"]             = QString::fromUtf8(saveGeometry().toBase64());
    obj["state"]                = QString::fromUtf8(saveState().toBase64());
    obj["splitter_state"]       = QString::fromUtf8(m_main_splitter->saveState().toBase64());
    obj["splitter_orientation"] = static_cast<int>(m_main_splitter->orientation());
    obj["left_splitter_state"]  = QString::fromUtf8(m_left_splitter->saveState().toBase64());
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
        emit sgnPlayTrackRequested(filepath);
    } else {
        qDebug() << "[INFO] filepath is empty!";
    }
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    emit sgnAboutToClose();
    if (m_desktop_lyrics_widget) {
        m_desktop_lyrics_widget->close();
    }
    QMainWindow::closeEvent(event);
}
