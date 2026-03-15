#include "MainWindow.h"
#include <QHeaderView>
#include <QTimer>
#include <QPointer>

#include "core/types.h"
#include "view/playlist/playlist_widgets.h"

MainWindow::MainWindow(PlaybackController* playback_controller, PlaylistController* playlist_controller, QWidget *parent)
    : QMainWindow(parent),
      m_playbackController(playback_controller),
            m_playlistController(playlist_controller)
{
    this->setMinimumSize(960, 540);
    this->initUI();
    this->initConnection();
}

MainWindow::~MainWindow() {}

PlaylistController* MainWindow::playlistController() const {
    return m_playlistController;
}

PlaybackController* MainWindow::playbackController() const {
    return m_playbackController;
}

LibraryWidget* MainWindow::libraryPanel() const {
    return m_libraryPanel;
}

SidePanel* MainWindow::sidePanel() const {
    return m_sidePanel;
}

WControlBar* MainWindow::controlBarWidget() const {
    return controlBar;
}

DesktopLyricsWidget* MainWindow::desktopLyricsWidget() const {
    return m_desktoplyricsWidget;
}

PlaylistSearchPanel* MainWindow::searchPanelWidget() const {
    return searchPanel;
}

void MainWindow::playTrackInUi(const QString& filepath) {
    emit sgnPlayTrackRequested(filepath);
}

void MainWindow::setSearchPanel(PlaylistSearchPanel* panel) {
    searchPanel = panel;
}

QByteArray MainWindow::searchPanelHeaderStateCache() const {
    return m_searchPanelHeaderStateCache;
}

QByteArray MainWindow::searchPanelGeometryCache() const {
    return m_searchPanelGeoCache;
}

void MainWindow::setSearchPanelStateCache(const QByteArray& geometry, const QByteArray& header) {
    m_searchPanelGeoCache = geometry;
    m_searchPanelHeaderStateCache = header;
}

void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);
    if (m_cacheLoadScheduled) {
        return;
    }
    m_cacheLoadScheduled = true;
    QTimer::singleShot(0, m_playlistController, &PlaylistController::loadCacheAfterShown);
}

void MainWindow::initConnection() {
    initMenuConnections();
}

void MainWindow::initMenuConnections() {
    connect(actOpenFile, &QAction::triggered, this, &MainWindow::onOpenFile);
    connect(actAddFile, &QAction::triggered, this, &MainWindow::sgnImportFilesRequested);
    connect(actAddFolder, &QAction::triggered, this, &MainWindow::sgnImportFolderRequested);
    connect(actNewPlaylist, &QAction::triggered, this, &MainWindow::sgnCreatePlaylistRequested);
    connect(actLoadPlaylist, &QAction::triggered, this, &MainWindow::sgnLoadPlaylist);
    connect(actCopyPlaylist, &QAction::triggered, this, &MainWindow::sgnCopyPlaylistRequested);
    connect(actRenamePlaylist, &QAction::triggered, this, &MainWindow::sgnRenamePlaylistRequested);
    connect(actRemovePlaylist, &QAction::triggered, this, &MainWindow::sgnRemovePlaylistRequested);
    connect(actSavePlaylist, &QAction::triggered, this, &MainWindow::sgnSavePlaylistRequested);

    connect(actExit, &QAction::triggered, this, &QWidget::close);
    connect(actAbout, &QAction::triggered, this, &MainWindow::sgnShowAboutMessagebox);
    connect(actSetSortRule, &QAction::triggered, this, &MainWindow::sgnSetSortRuleRequested);
    connect(actInsertColumn, &QAction::triggered, this, &MainWindow::sgnInsertColumnRequested);
    connect(actRemoveColumn, &QAction::triggered, this, &MainWindow::sgnRemoveColumnRequested);

    connect(actSettings, &QAction::triggered, this, &MainWindow::sgnOpenSettingsPanelRequested);

    connect(actSearchPanel, &QAction::triggered, this, &MainWindow::sgnOpenSearchPanelRequested);
    connect(actShowDesktopLyrics, &QAction::triggered, this, &MainWindow::sgnShowDesktopLyricsRequested);
}

void MainWindow::initUI() {
    // Global config
    this->setContextMenuPolicy(Qt::NoContextMenu);

    buildMenuBar();
    buildBottomToolBar();
    buildCentralArea();
}

void MainWindow::buildMenuBar() {
    mainMenuBar = new QMenuBar;

    menuFile = new QMenu("&File", mainMenuBar);
    actOpenFile = new QAction("&Open", menuFile);
    actAddFile = new QAction("Add file", menuFile);
    actAddFolder = new QAction("Add folder", menuFile);
    actNewPlaylist = new QAction("New playlist", menuFile);
    actLoadPlaylist = new QAction("&Load playlist", menuFile);
    actCopyPlaylist = new QAction("Copy playlist", menuFile);
    actRenamePlaylist = new QAction("&Rename playlist", menuFile);
    actSavePlaylist = new QAction("&Save current playlist", menuFile);
    actRemovePlaylist = new QAction("Remove current palylist", menuFile);
    actExit = new QAction("&Exit", menuFile);
    menuFile->addAction(actOpenFile);
    menuFile->addSeparator();
    menuFile->addAction(actAddFile);
    menuFile->addAction(actAddFolder);
    menuFile->addSeparator();
    menuFile->addAction(actNewPlaylist);
    menuFile->addAction(actLoadPlaylist);
    menuFile->addAction(actSavePlaylist);
    menuFile->addAction(actCopyPlaylist);
    menuFile->addAction(actRenamePlaylist);
    menuFile->addAction(actRemovePlaylist);
    menuFile->addSeparator();
    menuFile->addAction(actExit);
    mainMenuBar->addMenu(menuFile);

    menuView = new QMenu("&View", mainMenuBar);
    actSetSortRule = new QAction("Set sort rule (&R)", menuView);
    actInsertColumn = new QAction("Insert a column (&I)", menuView);
    actRemoveColumn = new QAction("Remove a column (&R)", menuView);
    actSearchPanel = new QAction("Open search panel (&S)", menuView);
    actShowDesktopLyrics = new QAction("Show Desktop Lyrics (&D)", menuView);
    menuView->addAction(actSetSortRule);
    menuView->addAction(actInsertColumn);
    menuView->addAction(actRemoveColumn);
    menuView->addAction(actSearchPanel);
    menuView->addAction(actShowDesktopLyrics);
    mainMenuBar->addMenu(menuView);

    menuSettings = new QMenu("&Settings", mainMenuBar);
    actSettings = new QAction("&Settings", menuSettings);
    menuSettings->addAction(actSettings);
    mainMenuBar->addMenu(menuSettings);

    menuHelp = new QMenu("&Help", mainMenuBar);
    actManual = new QAction("&Manual", menuHelp);
    actAbout = new QAction("&About", menuHelp);
    menuHelp->addAction(actManual);
    menuHelp->addSeparator();
    menuHelp->addAction(actAbout);
    mainMenuBar->addMenu(menuHelp);

    setMenuBar(mainMenuBar);
}

void MainWindow::buildBottomToolBar() {
    // Bottom toolbar, btn & progress bar
    // PushButton instant -> BottomToolBarArea
    bottomToolBar = new QToolBar(this);
    bottomToolBar->setObjectName("BottomToolBar");
    bottomToolBar->setMovable(false);
    bottomToolBar->setFloatable(false);
    controlBar = new WControlBar(bottomToolBar);
    bottomToolBar->addWidget(controlBar);
    addToolBar(Qt::BottomToolBarArea, bottomToolBar);
    controlBar->setDevice(m_playbackController->availableDevices(), m_playbackController->currentDeviceId());
}

void MainWindow::buildCentralArea() {
    // Main window
    // playlist & table with splitter
    centerWidget = new QWidget(this);
    m_libraryPanel = new LibraryWidget(m_playlistController->viewModel(), centerWidget);
    m_sidePanel = new SidePanel(centerWidget);
    mainLayout = new QHBoxLayout(centerWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(m_libraryPanel, 3);
    mainLayout->addWidget(m_sidePanel, 1);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    setCentralWidget(centerWidget);

    // desktop lrc panel
    m_desktoplyricsWidget = new DesktopLyricsWidget();
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
    if (m_desktoplyricsWidget) {
        m_desktoplyricsWidget->close();
    }
    QMainWindow::closeEvent(event);
}
