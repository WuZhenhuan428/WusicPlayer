#include "MainWindow.h"
#include <QHeaderView>

#include "src/playlist/playlist_definitions.h"

#define SLIDER_VOLUME_MIN_WIDTH 80
#define SLIDER_VOLUME_MAX_WIDTH 80

MainWindow::MainWindow(Player* player, QWidget *parent)
    : m_player(player), QMainWindow(parent)
{
    m_playlistManager = new PlaylistManager(this);
    
    this->setMinimumSize(800, 600);
    this->initUI();
    this->initConnection();
}

MainWindow::~MainWindow() {}

void MainWindow::initConnection()
{
    // PushButton: play | pause | stop
    connect(btnPlay, &QPushButton::clicked, m_player, &Player::play);
    connect(btnPause, &QPushButton::clicked, m_player, &Player::pause);
    connect(btnStop, &QPushButton::clicked, m_player, &Player::stop);
    connect(btnNext, &QPushButton::clicked, this, [this](){
        QString next_track = m_playlistManager->nextTrack();
        if (!next_track.isEmpty()) {
            emit sgnFilepathChanged(next_track);
        }
    });
    connect(btnPrev, &QPushButton::clicked, this, [this](){
        QString prev_track = m_playlistManager->prevTrack();
        if (!prev_track.isEmpty()) {
            emit sgnFilepathChanged(prev_track);
        }
    });

    // Menu
    connect(actOpenFile, &QAction::triggered, this, &MainWindow::onOpenFile);
    connect(actAddFile, &QAction::triggered, this, &MainWindow::onAddFile);
    connect(actAddFolder, &QAction::triggered, this, &MainWindow::onAddFolder);
    connect(actNewPlaylist, &QAction::triggered, this, &MainWindow::onNewPlaylist);
    connect(actLoadPlaylist, &QAction::triggered, this, &MainWindow::onLoadPlaylist);
    connect(actCopyPlaylist, &QAction::triggered, this, &MainWindow::onCopyPlaylist);
    connect(actRenamePlaylist, &QAction::triggered, this, &MainWindow::onRenamePlaylist);
    connect(actRemovePlaylist, &QAction::triggered, this, &MainWindow::onRemovePlaylist);
    connect(actSavePlaylist, &QAction::triggered, this, &MainWindow::onSavePlaylist);

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

    playlistTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(playlistTree, &QTreeWidget::customContextMenuRequested, this, &MainWindow::onTreeContextMenuRequested);

    connect(songTreeView, &QTreeView::doubleClicked, this, [this](const QModelIndex &index){
        trackId id = m_playlistManager->getViewModel()->trackAt(index);
        if (!id.isNull()) {
            int queueIdx = m_playlistManager->getViewModel()->playbackQueue().indexOf(id);
            if (queueIdx != -1) m_playlistManager->play(queueIdx);
        }
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
    connect(m_playlistManager->getViewModel(), &QAbstractItemModel::modelReset, this, [this]() {
        QAbstractItemModel* model = songTreeView->model();
        if (!model) return;
        for (int i = 0; i < model->rowCount(); ++i) {
            QModelIndex idx = model->index(i, 0);
            if (model->hasChildren(idx)) {
                songTreeView->setFirstColumnSpanned(i, QModelIndex(), true);
                songTreeView->setExpanded(idx, true);
            }
        }
    });
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
    actRenamePlaylist = new QAction("&Rename playlist");
    actSavePlaylist = new QAction("&Save current playlist");
    actRemovePlaylist = new QAction("Remove current palylist");
    actExit = new QAction("&Exit");
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

    btnPlay = new QPushButton(">");
    btnPause = new QPushButton("||");
    btnStop = new QPushButton(">|");
    btnPrev = new QPushButton("<<");
    btnNext = new QPushButton(">>");
    btnMute = new QPushButton("Mute");
    btnPlay->setFixedSize(25, 25);
    btnPause->setFixedSize(25, 25);
    btnStop->setFixedSize(25, 25);
    btnPrev->setFixedSize(25, 25);
    btnNext->setFixedSize(25, 25);
    btnMute->setFixedSize(25, 25);

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
    bottomToolBar->addWidget(btnPrev);
    bottomToolBar->addWidget(btnNext);
    bottomToolBar->addWidget(sliderPostion);
    bottomToolBar->addWidget(timeProgress);
    bottomToolBar->addWidget(btnMute);
    bottomToolBar->addWidget(sliderVolume);
    addToolBar(Qt::BottomToolBarArea, bottomToolBar);

// Main window
    /// cover & lyrics
    coverSplitter = new QSplitter(Qt::Vertical, this);

    coverImageLabel = new QLabel();
    origin_cover = new QPixmap;
    origin_cover->load("/home/wuzhenhuan/pictures/zhihu-meme.jpg");
    coverImageLabel->setPixmap(*origin_cover);
    resize(800, 600);

    lrcListView = new QListView;
    // @TODO: Lyrics display
    coverSplitter->addWidget(coverImageLabel);
    coverSplitter->addWidget(lrcListView);
    coverSplitter->setChildrenCollapsible(false);
    coverSplitter->setStretchFactor(0, 1);
    coverSplitter->setStretchFactor(1, 1);
    /// playlist & table with splitter
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    playlistTree = new QTreeWidget();
    playlistTree->setHeaderLabel("Playlist");
    playlistTree->setMinimumWidth(120);
    

    songTreeView = new QTreeView();
    songTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    songTreeView->header()->setSectionsMovable(true);

    songTreeView->header()->setFirstSectionMovable(false);
    songTreeView->header()->setMinimumSectionSize(30);
    songTreeView->header()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    songTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    songTreeView->setModel(m_playlistManager->getViewModel());
    songTreeView->setAlternatingRowColors(true);

    mainSplitter->addWidget(playlistTree);
    mainSplitter->addWidget(songTreeView);
    mainSplitter->addWidget(coverSplitter);
    mainSplitter->handle(1)->hide();
    mainSplitter->setStretchFactor(0, 2);
    mainSplitter->setStretchFactor(1, 6);
    mainSplitter->setStretchFactor(2, 3);
    mainSplitter->setChildrenCollapsible(false);
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

        // STUB: sort trigger
        m_playlistManager->getViewModel()->setGrouping(SortType::Album, true);
        songTreeView->expandAll();

    } else {
        qDebug() << "[INFO] playlist filepath is empty!";
    }
}

void MainWindow::onCopyPlaylist() {
    m_playlistManager->copyPlaylist(m_playlistManager->getCurrentPlaylist());
}


void MainWindow::onRenamePlaylist() {
    bool ok;
    QString dst_name = QInputDialog::getText(
        this,
        tr("Rename playlist"),
        tr("Input new name for current playlist:"),
        QLineEdit::Normal,
        tr("Default playlist"),
        &ok
    );
    if (ok) {
        const auto& temp = m_playlistManager->getCurrentPlaylist();
        m_playlistManager->renamePlaylist(m_playlistManager->getCurrentPlaylist(), dst_name);
    } else {
        qDebug() << "[INFO] Cancel renaming for current playlist";
    }
}

void MainWindow::onRemovePlaylist() {
    QMessageBox::StandardButton btn;
    btn = QMessageBox::question(
        this,
        "comfirm",
        "Do you really want to remove this playlist?",
        QMessageBox::Yes | QMessageBox::No
    );
    if (btn == QMessageBox::Yes) {
        m_playlistManager->removePlaylist(m_playlistManager->getCurrentPlaylist());
    } else {
        qDebug() << "Cancel removing current playlist";
    }
}

void MainWindow::onPlayerStateChanged(Player::State state) {
    btnPlay->setEnabled( state != Player::State::PLAYING );
    btnPause->setEnabled( state != Player::State::PAUSED );
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
void MainWindow::onSavePlaylist() {
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

void MainWindow::onTreeContextMenuRequested(const QPoint &pos) {
    QTreeWidgetItem* item = playlistTree->itemAt(pos);
    if (!item) return;

    WPlayListWidgetItem* playlistItem = dynamic_cast<WPlayListWidgetItem*>(item);
    if (!playlistItem) return;

    QUuid playlistId = playlistItem->id();

    QMenu menu(this);
    QAction* actAddTrack = menu.addAction("Add track");
    QAction* actAddFolder = menu.addAction("Add folder");
    QAction* actSave = menu.addAction("Save as");
    QAction* actRename = menu.addAction("Rename");
    QAction* actCopy = menu.addAction("Copy");
    QAction* actRemove = menu.addAction("Remove");
    
    connect(actAddTrack, &QAction::triggered, this, [this, playlistId](){
        if (m_playlistManager->getCurrentPlaylist() != playlistId) {
            m_playlistManager->switchToPlaylist(playlistId);
        }
        this->onAddFile();
    });

    connect(actAddFolder, &QAction::triggered, this, [this, playlistId](){
        if (m_playlistManager->getCurrentPlaylist() != playlistId) {
            m_playlistManager->switchToPlaylist(playlistId);
        }
        this->onAddFolder();
    });

    connect(actSave, &QAction::triggered, this, [this, playlistId](){
        if (m_playlistManager->getCurrentPlaylist() != playlistId) {
            m_playlistManager->switchToPlaylist(playlistId);
        }
        this->onSavePlaylist();
    });
    connect(actRename, &QAction::triggered, this, [this, playlistId, playlistItem](){
        bool ok;
        QString dst_name = QInputDialog::getText(
            this,
            tr("Rename playlist"),
            tr("Input new name"),
            QLineEdit::Normal,
            playlistItem->text(0),
            &ok
        );
        if (ok && !dst_name.isEmpty()) {
            m_playlistManager->renamePlaylist(playlistId, dst_name);
        }
    });

    connect(actCopy, &QAction::triggered, this, [this, playlistId](){
        m_playlistManager->copyPlaylist(playlistId);
    });

    connect(actRemove, &QAction::triggered, this, [this, playlistId](){
        QMessageBox::StandardButton btn = QMessageBox::question(
            this,
            "Confirm",
            "Do you really want to remove this playlist?",
            QMessageBox::Yes | QMessageBox::No
        );
        if (btn == QMessageBox::Yes) {
            m_playlistManager->removePlaylist(playlistId);
        }
    });

    menu.exec(playlistTree->mapToGlobal(pos));
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

void MainWindow::resizeEvent(QResizeEvent *event) {
    if (!origin_cover->isNull()) {
        int dst_width = width() / 3;
        int dst_height = height() / 3;

        QPixmap scaled = origin_cover->scaled(
            dst_width,
            dst_height,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
        coverImageLabel->setFixedSize(scaled.size());
        coverImageLabel->setPixmap(scaled);
    }
    QMainWindow::resizeEvent(event);
}