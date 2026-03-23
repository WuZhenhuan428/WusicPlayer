#include "MainWindow.h"
#include <QHeaderView>
#include <QTimer>
#include <QPointer>


MainWindow::MainWindow(PlaybackController* playback_controller, PlaylistController* playlist_controller, QWidget *parent)
    : QMainWindow(parent),
      m_playback_controller(playback_controller),
      m_playlist_controller(playlist_controller)
{
    this->setMinimumSize(960, 540);
    this->initUI();
    this->initConnection();
}

MainWindow::~MainWindow() {}

PlaylistController* MainWindow::playlistController() const {
    return m_playlist_controller;
}

PlaybackController* MainWindow::playbackController() const {
    return m_playback_controller;
}

LibraryWidget* MainWindow::libraryPanel() const {
    return m_library_panel;
}

SidePanel* MainWindow::sidePanel() const {
    return m_side_panel;
}

WControlBar* MainWindow::controlBarWidget() const {
    return m_control_bar;
}

DesktopLyricsWidget* MainWindow::desktopLyricsWidget() const {
    return m_desktop_lyrics_widget;
}

PlaylistSearchPanel* MainWindow::searchPanelWidget() const {
    return m_search_panel;
}

void MainWindow::playTrackInUi(const QString& filepath) {
    emit sgnPlayTrackRequested(filepath);
}

void MainWindow::setSearchPanel(PlaylistSearchPanel* panel) {
    m_search_panel = panel;
}

QByteArray MainWindow::searchPanelHeaderStateCache() const {
    return m_search_panel_header_state_cache;
}

QByteArray MainWindow::searchPanelGeometryCache() const {
    return m_search_panel_geo_cache;
}

void MainWindow::setSearchPanelStateCache(const QByteArray& geometry, const QByteArray& header) {
    m_search_panel_geo_cache = geometry;
    m_search_panel_header_state_cache = header;
}

void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);
    if (m_cache_load_scheduled) {
        return;
    }
    m_cache_load_scheduled = true;
    QTimer::singleShot(0, m_playlist_controller, &PlaylistController::loadCacheAfterShown);
}

void MainWindow::initConnection() {
    initMenuConnections();
}

void MainWindow::initMenuConnections() {
    connect(m_act_open_file, &QAction::triggered, this, &MainWindow::onOpenFile);
    connect(m_act_add_file, &QAction::triggered, this, &MainWindow::sgnImportFilesRequested);
    connect(m_act_add_folder, &QAction::triggered, this, &MainWindow::sgnImportFolderRequested);
    connect(m_act_new_playlist, &QAction::triggered, this, &MainWindow::sgnCreatePlaylistRequested);
    connect(m_act_load_playlist, &QAction::triggered, this, &MainWindow::sgnLoadPlaylist);
    connect(m_act_copy_playlist, &QAction::triggered, this, &MainWindow::sgnCopyPlaylistRequested);
    connect(m_act_rename_playlist, &QAction::triggered, this, &MainWindow::sgnRenamePlaylistRequested);
    connect(m_act_remove_playlist, &QAction::triggered, this, &MainWindow::sgnRemovePlaylistRequested);
    connect(m_act_save_playlist, &QAction::triggered, this, &MainWindow::sgnSavePlaylistRequested);

    connect(m_act_exit, &QAction::triggered, this, &QWidget::close);
    connect(m_act_about, &QAction::triggered, this, &MainWindow::sgnShowAboutMessagebox);
    connect(m_act_set_sort_rule, &QAction::triggered, this, &MainWindow::sgnSetSortRuleRequested);
    connect(m_act_insert_column, &QAction::triggered, this, &MainWindow::sgnInsertColumnRequested);
    connect(m_act_remove_column, &QAction::triggered, this, &MainWindow::sgnRemoveColumnRequested);

    connect(m_act_settings, &QAction::triggered, this, &MainWindow::sgnOpenSettingsPanelRequested);

    connect(m_act_search_panel, &QAction::triggered, this, &MainWindow::sgnOpenSearchPanelRequested);
    connect(m_act_show_desktop_lyrics, &QAction::triggered, this, &MainWindow::sgnShowDesktopLyricsRequested);
}

void MainWindow::initUI() {
    // Global config
    this->setContextMenuPolicy(Qt::NoContextMenu);

    buildMenuBar();
    buildBottomToolBar();
    buildCentralArea();
}

void MainWindow::buildMenuBar() {
    m_menubar_main = new QMenuBar;

    m_menu_file = new QMenu("&File", m_menubar_main);
    m_act_open_file = new QAction("&Open", m_menu_file);
    m_act_add_file = new QAction("Add file", m_menu_file);
    m_act_add_folder = new QAction("Add folder", m_menu_file);
    m_act_new_playlist = new QAction("New playlist", m_menu_file);
    m_act_load_playlist = new QAction("&Load playlist", m_menu_file);
    m_act_copy_playlist = new QAction("Copy playlist", m_menu_file);
    m_act_rename_playlist = new QAction("&Rename playlist", m_menu_file);
    m_act_remove_playlist = new QAction("&Save current playlist", m_menu_file);
    m_act_save_playlist = new QAction("Remove current palylist", m_menu_file);
    m_act_exit = new QAction("&Exit", m_menu_file);
    m_menu_file->addAction(m_act_open_file);
    m_menu_file->addSeparator();
    m_menu_file->addAction(m_act_add_file);
    m_menu_file->addAction(m_act_add_folder);
    m_menu_file->addSeparator();
    m_menu_file->addAction(m_act_new_playlist);
    m_menu_file->addAction(m_act_load_playlist);
    m_menu_file->addAction(m_act_save_playlist);
    m_menu_file->addAction(m_act_copy_playlist);
    m_menu_file->addAction(m_act_rename_playlist);
    m_menu_file->addAction(m_act_remove_playlist);
    m_menu_file->addSeparator();
    m_menu_file->addAction(m_act_exit);
    m_menubar_main->addMenu(m_menu_file);

    m_menu_view = new QMenu("&View", m_menubar_main);
    m_act_set_sort_rule = new QAction("Set sort rule (&R)", m_menu_view);
    m_act_insert_column = new QAction("Insert a column (&I)", m_menu_view);
    m_act_remove_column = new QAction("Remove a column (&R)", m_menu_view);
    m_act_search_panel = new QAction("Open search panel (&S)", m_menu_view);
    m_act_show_desktop_lyrics = new QAction("Show Desktop Lyrics (&D)", m_menu_view);
    m_menu_view->addAction(m_act_set_sort_rule);
    m_menu_view->addAction(m_act_insert_column);
    m_menu_view->addAction(m_act_remove_column);
    m_menu_view->addAction(m_act_search_panel);
    m_menu_view->addAction(m_act_show_desktop_lyrics);
    m_menubar_main->addMenu(m_menu_view);

    m_menu_settings = new QMenu("&Settings", m_menubar_main);
    m_act_settings = new QAction("&Settings", m_menu_settings);
    m_menu_settings->addAction(m_act_settings);
    m_menubar_main->addMenu(m_menu_settings);

    m_menu_help = new QMenu("&Help", m_menubar_main);
    m_act_manual = new QAction("&Manual", m_menu_help);
    m_act_about = new QAction("&About", m_menu_help);
    m_menu_help->addAction(m_act_manual);
    m_menu_help->addSeparator();
    m_menu_help->addAction(m_act_about);
    m_menubar_main->addMenu(m_menu_help);

    setMenuBar(m_menubar_main);
}

void MainWindow::buildBottomToolBar() {
    // Bottom toolbar, btn & progress bar
    // PushButton instant -> BottomToolBarArea
    m_bottom_toolbar = new QToolBar(this);
    m_bottom_toolbar->setObjectName("BottomToolBar");
    m_bottom_toolbar->setMovable(false);
    m_bottom_toolbar->setFloatable(false);
    m_control_bar = new WControlBar(m_bottom_toolbar);
    m_bottom_toolbar->addWidget(m_control_bar);
    addToolBar(Qt::BottomToolBarArea, m_bottom_toolbar);
    m_control_bar->setDevice(m_playback_controller->availableDevices(), m_playback_controller->currentDeviceId());
}

void MainWindow::buildCentralArea() {
    // Main window
    // playlist & table with splitter
    m_center_widget = new QWidget(this);
    m_library_panel = new LibraryWidget(m_playlist_controller->viewModel(), m_center_widget);
    m_side_panel = new SidePanel(m_center_widget);
    m_hbl_main = new QHBoxLayout(m_center_widget);
    m_hbl_main->setContentsMargins(0, 0, 0, 0);
    m_hbl_main->setSpacing(0);
    m_hbl_main->addWidget(m_library_panel, 3);
    m_hbl_main->addWidget(m_side_panel, 1);
    m_hbl_main->setContentsMargins(0, 0, 0, 0);
    setCentralWidget(m_center_widget);

    // desktop lrc panel
    m_desktop_lyrics_widget = new DesktopLyricsWidget();
}

void MainWindow::onOpenFile() {
    QString filepath = QFileDialog::getOpenFileName(
        this,
        tr("Open Audio File"),
        QString(),
        tr("*.mp3 *.wav *.flac")
    );

    if (!filepath.isEmpty()) {
        emit sgnPlayTrackRequested(filepath);
    }
    else {
        qDebug() << "[INFO] filepath is empty!";
    }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    emit sgnAboutToClose();
    if (m_desktop_lyrics_widget) {
        m_desktop_lyrics_widget->close();
    }
    QMainWindow::closeEvent(event);
}
