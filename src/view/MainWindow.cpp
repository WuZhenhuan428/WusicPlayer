#include "MainWindow.h"
#include <QHeaderView>
#include <QTimer>

#include "core/types.h"
#include "view/playlist/playlist_widgets.h"
#include "core/utils/AudioUtils.h"

MainWindow::MainWindow(PlaybackController* playback_controller, QWidget *parent)
    : m_playbackController(playback_controller), QMainWindow(parent)
{    
    m_playlistManager = new PlaylistManager(this);
    m_playlistController = new PlaylistController(m_playlistManager, this, this);

    // [Fix] Initialize view with default sort rules if needed, 
    // or trigger a rebuild if playlist already exists
    SortRule defaultRule;
    defaultRule.type = SortType::album; // Or whatever default
    m_playlistController->viewModel()->setSingleGrouping(defaultRule);
    this->setMinimumSize(960, 540);
    this->initUI();
    this->initConnection();
    this->applyConfig();
}

MainWindow::~MainWindow() {}

void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);
    if (m_cacheLoadScheduled) {
        return;
    }
    m_cacheLoadScheduled = true;
    QTimer::singleShot(0, m_playlistController, &PlaylistController::loadCacheAfterShown);
}

void MainWindow::initConnection()
{
    connect(controlBar, &WControlBar::sgnBtnPlayClicked, m_playbackController, &PlaybackController::play);
    connect(controlBar, &WControlBar::sgnBtnPauseClicked, m_playbackController, &PlaybackController::pause);
    connect(controlBar, &WControlBar::sgnBtnStopClicked, m_playbackController, &PlaybackController::stop);
    
    connect(controlBar, &WControlBar::sgnBtnMuteClicked, m_playbackController, &PlaybackController::flipMute);
    
    connect(controlBar, &WControlBar::sgnInOrder, this, [this]() {
        m_playbackController->setPlayMode(PlayMode::in_order);
    });
    connect(controlBar, &WControlBar::sgnLoop, this, [this]() {
        m_playbackController->setPlayMode(PlayMode::loop);
    });
    connect(controlBar, &WControlBar::sgnShuffle, this, [this]() {
        m_playbackController->setPlayMode(PlayMode::shuffle);
    });
    connect(controlBar, &WControlBar::sgnOutOfOrderTrack, this, [this]() {
        m_playbackController->setPlayMode(PlayMode::out_of_order_track);
    });
    connect(controlBar, &WControlBar::sgnOutOfOrderGroup, this, [this]() {
        m_playbackController->setPlayMode(PlayMode::out_of_order_group);
    });
    connect(controlBar, &WControlBar::sgnSliderPositionReleased, this, [this](int percent){
        m_playbackController->setPosition(percent * 1000);
    });
    connect(controlBar, &WControlBar::sgnSliderVolumeReleased, m_playbackController, &PlaybackController::setVolume);
    connect(controlBar, &WControlBar::sgnSliderVolumeMoved, m_playbackController, &PlaybackController::setVolume);
    
    connect(controlBar, &WControlBar::sgnBtnNextClicked, this, [this](){
        QString next_track = m_playlistController->nextTrack(m_playbackController->playMode());
        if (!next_track.isEmpty()) {
            playTrack(next_track);
        }
    });
    connect(controlBar, &WControlBar::sgnBtnPrevClicked, this, [this](){
        QString prev_track = m_playlistController->prevTrack(m_playbackController->playMode());
        if (!prev_track.isEmpty()) {
            playTrack(prev_track);
        }
    });
    connect(m_playbackController, &PlaybackController::sgnDevicesChanged, controlBar, &WControlBar::setDevice);
    connect(controlBar, &WControlBar::sgnSelectDeviceId, m_playbackController, &PlaybackController::setDeviceById);
    controlBar->setDevice(m_playbackController->availableDevices(), m_playbackController->currentDeviceId());

    connect(m_playbackController, &PlaybackController::sgnPositionChanged, controlBar, &WControlBar::updatePosition);
    connect(m_playbackController, &PlaybackController::sgnPlaybackStateChanged, controlBar, &WControlBar::onPlayerStateChanged);
    connect(m_playbackController, &PlaybackController::sgnDurationChanged, controlBar, &WControlBar::updateDuration);
    connect(m_playbackController, &PlaybackController::sgnPlayModeChanged, this, [this](PlayMode mode){
        controlBar->setPlayMode(mode);
    });

    // Menu
    connect(actOpenFile, &QAction::triggered, this, &MainWindow::onOpenFile);
    connect(actAddFile, &QAction::triggered, this, [this](){ m_playlistController->importFiles(); });
    connect(actAddFolder, &QAction::triggered, this, [this](){ m_playlistController->importDir(); });
    connect(actNewPlaylist, &QAction::triggered, m_playlistController, &PlaylistController::createNewPlaylist);
    connect(actLoadPlaylist, &QAction::triggered, m_playlistController, &PlaylistController::loadPlaylist);
    connect(actCopyPlaylist, &QAction::triggered, this, [this](){
        m_playlistController->copyPlaylist();
    });
    connect(actRenamePlaylist, &QAction::triggered, this, [this](){
        m_playlistController->renamePlaylist();
    });
    connect(actRemovePlaylist, &QAction::triggered, this, [this](){
        m_playlistController->removePlaylist();
    });
    connect(actSavePlaylist, &QAction::triggered, this, [this](){
        m_playlistController->savePlaylist();
    });

    connect(m_libraryPanel, &LibraryWidget::sgnRenamePlaylist, m_playlistController, &PlaylistController::renamePlaylist);
    connect(m_libraryPanel, &LibraryWidget::sgnRemovePlaylist, m_playlistController, &PlaylistController::removePlaylist);
    connect(m_libraryPanel, &LibraryWidget::sgnSavePlaylist, m_playlistController, &PlaylistController::savePlaylist);
    connect(m_libraryPanel, &LibraryWidget::sgnCopyPlaylist, m_playlistController, &PlaylistController::copyPlaylist);

    connect(actExit, &QAction::triggered, this, &QWidget::close);
    connect(actAbout, &QAction::triggered, this, [=](){
        QMessageBox* msg = new QMessageBox(this);
        msg->setWindowTitle("About");
        msg->setText("This is a ABOUT message box.");
        msg->setIcon(QMessageBox::Information);
        msg->setStandardButtons(QMessageBox::Ok);
        msg->show();
        msg->setAttribute(Qt::WA_DeleteOnClose);
    });
    connect(actSetSortRule, &QAction::triggered, this, [this](){
        WSortTypeSetDialog dialog;
        if (dialog.exec() == QDialog::Accepted) {
            QString input = dialog.getText();
            m_playlistController->viewModel()->setSortExpression(input);
        }
    });
    connect(actInsertColumn, &QAction::triggered, this, [this](){
        WInsertColumnDialog dialog;
        int maxIndex = m_playlistController->viewModel()->getColumns().size();
        dialog.setMaxIndex(maxIndex);
        dialog.setIndex(1);
        int result = dialog.exec();
        if (result == QDialog::Accepted) {
            TableColumn column = dialog.getRule();
            int index = dialog.index();
            m_playlistController->viewModel()->insertColumn(index, column);
        }
    });
    connect(actRemoveColumn, &QAction::triggered, this, [this](){
        WColumnIndexDialog dialog(tr("Remove column"), tr("Input the column index except 0"), this);
        int maxIndex = m_playlistController->viewModel()->getColumns().size() - 1;
        dialog.setMaxIndex(maxIndex);
        dialog.setIndex(1);
        if (dialog.exec() == QDialog::Accepted) {
            m_playlistController->viewModel()->removeColumn(dialog.index());
        }
    });
    // read file
    connect(this, &MainWindow::sgnLoadPlaylist, m_playlistController, &PlaylistController::loadPlaylist);

    connect(m_playbackController, &PlaybackController::sgnMediaStateChanged, [=](QMediaPlayer::MediaStatus status){
        if (status == QMediaPlayer::MediaStatus::EndOfMedia) {
            QString next_track = m_playlistController->nextTrack(m_playbackController->playMode());
            if (!next_track.isEmpty()) {
                playTrack(next_track);
            }
        }
    });

    // main window: playlist & song table
    connect(m_playlistController, &PlaylistController::requestPlay, this, &MainWindow::playTrack);
    connect(m_playlistController, &PlaylistController::playlistChanged, this, &MainWindow::updatePlaylist);

    connect(m_playlistController->viewModel(), &QAbstractItemModel::modelReset, this, [this](){
        QTreeView* view = m_libraryPanel->songTreeView();
        if (!view || !view->model()) return;
        QAbstractItemModel* model = view->model();
        for (int i = 0; i < model->rowCount(); ++i) {
            QModelIndex idx = model->index(i, 0);
            if (model->hasChildren(idx)) {
                view->setFirstColumnSpanned(i, QModelIndex(), true);
                view->setExpanded(idx, true);
            }
        }
    });
    
    // lrc panel
    connect(m_playbackController, &PlaybackController::sgnPositionChanged, m_sidePanel->getLyricsPanel(), &WLyricsPanel::getCurrentRow);

    // search panel
    connect(actSearchPanel, &QAction::triggered, this, &MainWindow::onOpenSearchPanel);

    // library panel
    connect(m_libraryPanel, &LibraryWidget::sgnImportFiles, m_playlistController, &PlaylistController::importFiles);
    connect(m_libraryPanel, &LibraryWidget::sgnImportDir, m_playlistController, &PlaylistController::importDir);
    connect(m_libraryPanel, &LibraryWidget::sgnSwitchPlaylist, m_playlistController, &PlaylistController::switchToPlaylist);
    connect(m_libraryPanel, &LibraryWidget::sgnPlayTrackByModelIndex, this, [this](const QModelIndex& index){
        auto* model = m_playlistController->viewModel();
        if(!model) return;
        trackId id = model->trackAt(index);
        if (id.isNull()) return;
        int queueIndex = model->playbackQueue().indexOf(id);
        if (queueIndex >= 0) {
            m_playlistController->play(queueIndex);
        }
    });
}

void MainWindow::initUI()
{
// Global config
    this->setContextMenuPolicy(Qt::NoContextMenu);
 
// MenuBar
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
    menuView->addAction(actSetSortRule);
    menuView->addAction(actInsertColumn);
    menuView->addAction(actRemoveColumn);
    menuView->addAction(actSearchPanel);
    mainMenuBar->addMenu(menuView);

    menuHelp = new QMenu("&Help", mainMenuBar);
    actManual = new QAction("&Manual", menuHelp);
    actAbout = new QAction("&About", menuHelp);
    menuHelp->addAction(actManual);
    menuHelp->addSeparator();
    menuHelp->addAction(actAbout);
    mainMenuBar->addMenu(menuHelp);

    setMenuBar(mainMenuBar);

// Bottom toolbar, btn & progress bar
    /// PushButton instant -> BottomToolBarArea
    bottomToolBar = new QToolBar(this);
    bottomToolBar->setObjectName("BottomToolBar");
    bottomToolBar->setMovable(false);
    bottomToolBar->setFloatable(false);
    controlBar = new WControlBar(bottomToolBar);
    bottomToolBar->addWidget(controlBar);
    addToolBar(Qt::BottomToolBarArea, bottomToolBar);

// Main window
    /// playlist & table with splitter
    
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
}

void MainWindow::onOpenFile() {
    QString filepath = QFileDialog::getOpenFileName(
        this,
        tr("Open Audio File"),
        QString(),
        tr("*.mp3 *.wav *.flac")
    );

    if (!filepath.isEmpty()) {
        playTrack(filepath);
    }
    else {
        qDebug() << "[INFO] filepath is empty!";
    }
}

void MainWindow::onOpenSearchPanel() {
    if (!searchPanel) {
        searchPanel = new PlaylistSearchPanel;
        searchPanel->setWindowFlag(Qt::Window, true);
        searchPanel->setAttribute(Qt::WA_DeleteOnClose, true);
        searchPanel->setSourceModel(m_playlistController->viewModel());

        if (!m_searchPanelGeoCache.isEmpty()) {
            searchPanel->restoreGeometry(m_searchPanelGeoCache);
        }

        searchPanel->applyHeaderStateDeferred(m_searchPanelHeaderStateCache);

        connect(m_playlistController->viewModel(), &QAbstractItemModel::modelReset, searchPanel, [this](){
            if (searchPanel) {
                searchPanel->applyHeaderStateDeferred(m_searchPanelHeaderStateCache);
            }
        }, Qt::SingleShotConnection);

        connect(searchPanel, &PlaylistSearchPanel::sgnRequestPlayTrack, this, [this](const QModelIndex &source_index) {
            auto* model = m_playlistController->viewModel();
            if (!model) return;
            trackId id = model->trackAt(source_index);
            if (id.isNull()) return;

            int queue_index = model->playbackQueue().indexOf(id);
            if (queue_index >= 0) {
                m_playlistController->play(queue_index);
            }
        });

        connect(searchPanel, &PlaylistSearchPanel::sgnAboutToClose, this,
            [this](const QByteArray &geometry, const QByteArray &header){
                m_searchPanelGeoCache = geometry;
                m_searchPanelHeaderStateCache = header;
            }
        );
        
        connect(searchPanel, &QObject::destroyed, this, [this](){
            searchPanel = nullptr;
        });
    }
    
    searchPanel->show();
    searchPanel->raise();
    searchPanel->activateWindow();
}

void MainWindow::updatePlaylist() {
    QVector<QPair<playlistId, QString>> items;
    const auto& lists = m_playlistController->playlists();
    items.reserve(static_cast<int>(lists.size()));
    for (const auto& list : lists) {
        items.push_back({list->id(), list->name()});
    }
    m_libraryPanel->setPlaylists(items);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
}

void MainWindow::playTrack(const QString& filepath) {
    if (filepath.isEmpty()) return;

    m_playbackController->read(filepath);
    m_sidePanel->loadCover(filepath);

    QModelIndex index = m_playlistController->viewModel()->getCurrentTrackIndex();
    if (index.isValid()) {
        m_libraryPanel->songTreeView()->scrollTo(index.siblingAtColumn(1), QAbstractItemView::PositionAtCenter);
    }

    TrackMetaData meta = m_playlistController->currentMetadata();
    m_sidePanel->loadLyrics(meta);
    m_sidePanel->loadMetaData(meta);
}


void MainWindow::restoreLastTrackWhenModelReady(int retry, qint64 last_pos) {
    if (retry > 20) return;

    QMediaPlayer* media_player = const_cast<QMediaPlayer*>(m_playbackController->getMediaPlayer());
    if (!media_player) {
        return;
    }

    const auto status = media_player->mediaStatus();
    const bool can_seek = (
        status == QMediaPlayer::LoadedMedia ||
        status == QMediaPlayer::BufferedMedia ||
        status == QMediaPlayer::BufferingMedia)
        && (media_player->duration() > 0
    );
    if (can_seek) {
        m_playbackController->setPosition(last_pos);
        m_playbackController->pause();
        return;
    }
    if (++retry > 30) {    // ~1.5s timeout
        return;
    }
    QTimer::singleShot(50, this, [this, retry, last_pos]() {
        restoreLastTrackWhenModelReady(retry + 1, last_pos);
    });
}

void MainWindow::applyConfig() {
    const AppConfig& cfg = ConfigManager::getInstance().getAppConfig();

    // load geometry & state
    if (!cfg.window.geometry.isEmpty()) {
        this->restoreGeometry(cfg.window.geometry);
    }
    if (!cfg.window.state.isEmpty()) {
        this->restoreState(cfg.window.state);
    }

    // window: flip
    m_playbackController->setVolume(cfg.window.volume);
    m_playbackController->setMute(cfg.window.isMuted);
    // window: volume
    const auto sliders = controlBar->findChildren<QSlider*>();
    for (QSlider* s : sliders) {
        if (s && s->orientation() == Qt::Horizontal && s->maximum() == 100  && s->maximumWidth() == 100) {
            s->setValue(cfg.window.volume);
            break;
        }
    }

    // last_playlist & last_track (must wait cache + model rebuild)
    auto restorePlaybackState = [this, &cfg]() {
        const playlistId last_pid = cfg.playback.last_pid;
        const trackId last_tid = cfg.playback.last_tid;
        const int last_position_ms = cfg.playback.position_ms;

        if (last_pid.isNull()) {
            return;
        }

        auto findQueueIndexByTrackId = [this](const trackId& tid) -> int {
            if (tid.isNull()) {
                return -1;
            }
            const auto& queue = m_playlistController->viewModel()->playbackQueue();
            return queue.indexOf(tid);
        };

        auto seekWhenMediaReady = [this](int target_ms) {
            if (target_ms <= 0) {
                return;
            }
            auto retry_count = std::make_shared<int>(0);

            QTimer::singleShot(0, this, [this, retry_count, target_ms]() {
                restoreLastTrackWhenModelReady(*retry_count, target_ms);
            });
        };

        auto restoreAfterModelReset = [this, last_tid, last_position_ms, findQueueIndexByTrackId, seekWhenMediaReady] () {
            if (last_tid.isNull()) {
                return;
            }
            const int queue_index = findQueueIndexByTrackId(last_tid);
            if (queue_index < 0) {
                return;
            }
            m_playlistController->play(queue_index);
            seekWhenMediaReady(last_position_ms);
        };

        auto restorAfterCacheLoaded = [this, last_pid, restoreAfterModelReset](int) {
            m_playlistController->switchToPlaylist(last_pid);
            connect(m_playlistController->viewModel(), &QAbstractItemModel::modelReset, this,
                    restoreAfterModelReset, Qt::SingleShotConnection);
        };

        connect(m_playlistController, &PlaylistController::cacheLoadFinished, this,
                restorAfterCacheLoaded, Qt::SingleShotConnection);
    };
    restorePlaybackState();
    
    // view: columns
    if (!cfg.view.columns.isEmpty()) {
        QVector<TableColumn> columns;
        columns.reserve(cfg.view.columns.size());
        for (const auto& c : cfg.view.columns) {
            columns.append(c);
        }
        m_playlistController->viewModel()->setColumns(columns);
    }
    // view: song_tree_header_state
    auto restoreHeaderState = [this, &cfg]() {
        m_libraryPanel->songTreeHeader()->restoreState(cfg.view.state);
    };
    connect(m_playlistController->viewModel(), &QAbstractItemModel::modelReset, this,
            restoreHeaderState, Qt::SingleShotConnection);

    // playback: mode
    m_playbackController->setPlayMode(cfg.playback.play_mode);

    // search panel
    if (!cfg.search_panel.geometry.isEmpty()) {
        m_searchPanelGeoCache = cfg.search_panel.geometry;
    }
    if (!cfg.search_panel.state.isEmpty()) {
        m_searchPanelHeaderStateCache = cfg.search_panel.state;
    }
}

void MainWindow::saveConfig() {
    ConfigManager& cm = ConfigManager::getInstance();

    cm.setWindowGeometry(this->saveGeometry());
    cm.setWindowState(this->saveState());

    int volume = 100;
    const auto sliders = controlBar->findChildren<QSlider*>();
    for (QSlider* s : sliders) {
        if (s && s->orientation() == Qt::Horizontal && s->maximum() == 100 && s->maximumWidth() == 100) {
            volume = s->value();
            break;
        }
    }
    cm.setVolume(volume);
    cm.setPlayMode(m_playbackController->playMode());
    bool muted = false;
    muted = m_playbackController->getMute();
    cm.setMute(muted);

    do {
        const playlistId last_pid = m_playlistController->currentPlaylist();
        const trackId last_tid = m_playlistController->currentTrackId();
        if (last_pid.isNull() || last_tid.isNull()) {
            break;
        }
        cm.setLastPlayInfo(
            last_pid,
            last_tid,
            m_playbackController->getMediaPlayer()->hasAudio() ? static_cast<int>(m_playbackController->position()) : 0
        );
        
    } while (0);
    
    QList<TableColumn> cols;
    const QVector<TableColumn>& vmCols = m_playlistController->viewModel()->getColumns();
    for (const auto& c : vmCols) {
        cols.append(c);
    }
    cm.setTableColumns(cols);

    QByteArray song_tree_view_header = m_libraryPanel->songTreeHeader()->saveState();
    cm.setSongTreeViewHeader(song_tree_view_header);

    if (searchPanel) {
        m_searchPanelGeoCache = searchPanel->saveGeometry();
        if (searchPanel->getView() && searchPanel->getView()->header()) {
            m_searchPanelHeaderStateCache = searchPanel->getView()->header()->saveState();
        }
    }
    cm.setSearchPanelHeader(m_searchPanelHeaderStateCache);
    cm.setSearchPanelGeometry(m_searchPanelGeoCache);

    cm.save();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    saveConfig();
    QMainWindow::closeEvent(event);
}
