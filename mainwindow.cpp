#include "mainwindow.h"
#include <QHeaderView>

#define SLIDER_VOLUME_MIN_WIDTH 80
#define SLIDER_VOLUME_MAX_WIDTH 80

MainWindow::MainWindow(Player* player, QWidget *parent)
    : m_player(player), QMainWindow(parent)
{
    m_playlistManager = new PlaylistManager(this);
    
    this->setMinimumSize(800, 600);
    this->initUI();
    this->initConnection();
    // refresh playlist view
    refreshPlaylistTree();
}

MainWindow::~MainWindow() {}

void MainWindow::initConnection()
{
    // PushButton: play | pause | stop
    connect(btnPlay, &QPushButton::clicked, m_player, &Player::play);
    connect(btnPause, &QPushButton::clicked, m_player, &Player::pause);
    connect(btnStop, &QPushButton::clicked, m_player, &Player::stop);

    // Menu
    connect(actOpenFile, &QAction::triggered, this, &MainWindow::onOpenFile);
    connect(actAddFile, &QAction::triggered, this, &MainWindow::onAddFile);
    connect(actAddFolder, &QAction::triggered, this, &MainWindow::onAddFolder);
    connect(actNewPlaylist, &QAction::triggered, this, &MainWindow::onNewPlaylist);
    connect(actLoadPlaylist, &QAction::triggered, this, &MainWindow::onLoadPlaylist);
    connect(actCopyPlaylist, &QAction::triggered, this, &MainWindow::onCopyPlaylist);
    connect(actSaveCurrPlaylist, &QAction::triggered, this, &MainWindow::onSaveCurrPlaylist);

    connect(actExit, &QAction::triggered, this, &QWidget::close);
    connect(actAbout, &QAction::triggered, this, [=](){
        QMessageBox* msg = new QMessageBox(this);
        msg->setWindowTitle("About");
        msg->setText("Author: WZH\nUnder XXX license.");
        msg->setIcon(QMessageBox::Information);
        msg->setStandardButtons(QMessageBox::Ok);
        msg->show();
        msg->setAttribute(Qt::WA_DeleteOnClose);
    });

    // read file
    connect(this, &MainWindow::sgnFilepathChanged, m_player, &Player::read);
    connect(this, &MainWindow::sgnLoadPlaylist, m_playlistManager, &PlaylistManager::loadPlaylist);

    connect(m_player, &Player::stateChanged, this, &MainWindow::onPlayerStateChanged);
    
    connect(m_player, &Player::positionChanged, this, &MainWindow::updatePosition);
    connect(m_player, &Player::durationChanged, this, &MainWindow::updateDuration);

    connect(sliderPostion, &QSlider::sliderReleased, this, [this]() {
        m_player->setPosition(sliderPostion->value() * 1000);
    });
    connect(sliderPostion, &QSlider::sliderMoved, this, [this](int value) {
        timeProgress->setCurrentTime(value);
    });
    connect(btnMute, &QPushButton::clicked, m_player, [this](){
        m_player->flipMute();
        // then switch icon
    });
    connect(sliderVolume, &QSlider::sliderMoved, this, [this](int value) {
        m_player->setVolume(value);
    });

    // main window: playlist & song table
    connect(m_playlistManager, &PlaylistManager::requestPlay, m_player, &Player::read);
    connect(songTableView, &QTableView::doubleClicked, this, [this](const QModelIndex &index){
        m_playlistManager->play(index.row());
    });
    connect(m_playlistManager, &PlaylistManager::playlistChanged, this, &MainWindow::updatePlaylist);
    connect(playlistTree, &QTreeWidget::itemDoubleClicked, this,
        [this](QTreeWidgetItem* item, int column) {
            WPlayListWidgetItem* temp = dynamic_cast<WPlayListWidgetItem*>(item);
            if(temp) {
                m_playlistManager->switchToPlaylist(temp->id());
            }
        }
    );
}

void MainWindow::initUI()
{
// MenuBar
    mainMenuBar = new QMenuBar;

    menuFile = new QMenu("&File");
    actOpenFile = new QAction("&Open");
    actAddFile = new QAction("Add file");
    actAddFolder = new QAction("Add folder");
    actNewPlaylist = new QAction("New playlist");
    actLoadPlaylist = new QAction("&Load playlist");
    actCopyPlaylist = new QAction("Copy playlist");
    actSaveCurrPlaylist = new QAction("&Save current playlist");
    actExit = new QAction("&Exit");
    menuFile->addAction(actOpenFile);
    menuFile->addSeparator();
    menuFile->addAction(actAddFile);
    menuFile->addAction(actAddFolder);
    menuFile->addSeparator();
    menuFile->addAction(actNewPlaylist);
    menuFile->addAction(actLoadPlaylist);
    menuFile->addAction(actSaveCurrPlaylist);
    menuFile->addAction(actCopyPlaylist);
    menuFile->addSeparator();
    menuFile->addAction(actExit);
    mainMenuBar->addMenu(menuFile);

    menuHelp = new QMenu("&Help");
    actManual = new QAction("&Manual");
    actAbout = new QAction("&About");
    menuHelp->addAction(actManual);
    menuHelp->addSeparator();
    menuHelp->addAction(actAbout);
    mainMenuBar->addMenu(menuHelp);

    setMenuBar(mainMenuBar);

// Bottom toolbar, btn & progress bar
    /// PushButton instant -> BottomToolBarArea
    bottomToolBar = new QToolBar;
    bottomToolBar->setMovable(false);
    bottomToolBar->setFloatable(false);

    btnPlay = new QPushButton("Play");
    btnPause = new QPushButton("Pause");
    btnStop = new QPushButton("Stop");
    btnMute = new QPushButton("Mute");

    /// Position Bar: position/Duration
    sliderPostion = new QSlider(Qt::Horizontal);
    sliderPostion->setRange(0, 100);
    /// bar's time progress
    timeProgress = new WTimeProgress;
    sliderVolume = new QSlider(Qt::Horizontal);
    sliderVolume->setRange(0, 100);
    sliderVolume->setValue(100);
    sliderVolume->setMinimumWidth(SLIDER_VOLUME_MIN_WIDTH);
    sliderVolume->setMaximumWidth(SLIDER_VOLUME_MAX_WIDTH);

    bottomToolBar->addWidget(btnPlay);
    bottomToolBar->addWidget(btnPause);
    bottomToolBar->addWidget(btnStop);
    bottomToolBar->addWidget(sliderPostion);
    bottomToolBar->addWidget(timeProgress);
    bottomToolBar->addWidget(btnMute);
    bottomToolBar->addWidget(sliderVolume);
    addToolBar(Qt::BottomToolBarArea, bottomToolBar);

// Main window
    /// cover & lyrics
    coverSplitter = new QSplitter(Qt::Vertical, this);
    coverImageLabel = new QLabel();
    coverImageLabel->setPixmap(QPixmap("/home/wuzhenhuan/pictures/tieba_huaji.png"));
    lrcListView = new QListView;
    // @TODO: Lyrics display
    coverSplitter->addWidget(coverImageLabel);
    coverSplitter->addWidget(lrcListView);
    coverSplitter->setStretchFactor(0, 1);
    coverSplitter->setStretchFactor(1, 1);
    /// playlist & table with splitter
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    playlistTree = new QTreeWidget();
    playlistTree->setHeaderLabel("Playlist");

    songTableView = new QTableView();
    songTableView->setSelectionMode(QAbstractItemView::ContiguousSelection);
    songTableView->horizontalHeader()->setSectionsMovable(true);
    songTableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    songTableView->verticalHeader()->hide();
    songTableView->horizontalHeader()->setHighlightSections(false);
    songTableView->setShowGrid(false);
    // @TODO: 保持Column对应
    songTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    songTableView->setModel(m_playlistManager->getViewModel());
    songTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    mainSplitter->addWidget(playlistTree);
    mainSplitter->addWidget(songTableView);
    mainSplitter->addWidget(coverSplitter);
    mainSplitter->setStretchFactor(0, 2);
    mainSplitter->setStretchFactor(1, 6);
    mainSplitter->setStretchFactor(2, 3);
    setCentralWidget(mainSplitter);
}

void MainWindow::onOpenFile() {
    QString filepath = QFileDialog::getOpenFileName(
        this,
        tr("Open Audio File"),
        QString(),
        tr("*.mp3 *.wav *.flac")
    );

    if (!filepath.isEmpty()) {
        emit sgnFilepathChanged(filepath);
    }
    else {
        qDebug() << "[INFO] filepath is empty!";
    }
}

void MainWindow::onLoadPlaylist() {
    QString playlist_path = QFileDialog::getOpenFileName(
        this,
        tr("Open Playlist File"),
        QString(),
        tr("*.wcpl")
    );

    if (!playlist_path.isEmpty()) {
        emit sgnLoadPlaylist(playlist_path);
    } else {
        qDebug() << "[INFO] playlist filepath is empty!";
    }
}

void MainWindow::onCopyPlaylist() {
    const auto& temp = m_playlistManager->getCurrentPlaylist();
    m_playlistManager->copyPlaylist(temp);
}

void MainWindow::onPlayerStateChanged(Player::State state) {
    btnPlay->setEnabled( state != Player::State::PLAYING );
    btnPause->setEnabled( state != Player::State::PAUSED );
    // btnPause->setDisabled( state == Player::State::STOPPED);
}

void MainWindow::updateDuration(qint64 duration_ms) {
    qint64 duration_s = duration_ms / 1000;
    sliderPostion->setRange(0, duration_s);
    timeProgress->setTotalTime(duration_s);
}

void MainWindow::updatePosition(qint64 position_ms) {
    if (!sliderPostion->isSliderDown())
    {
        sliderPostion->setValue(position_ms/1000);
        timeProgress->setCurrentTime(position_ms/1000);
    }
}

// @TODO: set default type name
void MainWindow::onSaveCurrPlaylist() {
    QString filename = QFileDialog::getSaveFileName(
        this,
        tr("Save playlist file"),
        QString(),
        tr("*.wcpl")
    );
    if (!filename.isEmpty()) {
        qDebug() << "[INFO] Save current playlist as " << filename;
        m_playlistManager->saveCurrentPlaylist(filename);
    } else {
        qDebug() << "[INFO] Cancel saving current playlist";
    }
}

void MainWindow::refreshPlaylistTree() {
    
}

void MainWindow::updatePlaylist() {
    const auto& lists = m_playlistManager->getPlaylists();
    this->playlistTree->clear();
    for (const auto& list : lists) {
        new WPlayListWidgetItem(this->playlistTree, list->name(), list->id());
    }
}

void MainWindow::onAddFile() {
    QString filepath = QFileDialog::getOpenFileName(
        this,
        tr("Open Audio File"),
        QString(),
        tr("*.mp3 *.wav *.flac")
    );
    if (!filepath.isEmpty()) {
        m_playlistManager->addTrack(filepath);
    } else {
        qDebug() << "[INFO] Cancel adding track to current playlist.";
    }
}

void MainWindow::onAddFolder() {
    QString directory = QFileDialog::getExistingDirectory(
        this,
        tr("Select Folder"),
        "/home",
        QFileDialog::ShowDirsOnly
    );
    if (!directory.isEmpty()) {
        m_playlistManager->addFolder(directory);
    } else {
        qDebug() << "[INFO] Cancel adding folder to current playlist.";
    }
}

void MainWindow::onNewPlaylist() {
    m_playlistManager->createPlaylist();
}
