#include "MainWindow.h"
#include <QHeaderView>
#include <QTimer>

#include "src/playlist/playlist_definitions.h"
#include "src/playlist/playlist_widgets.h"
#include "include/audio.h"

#define DEFAULT_COVER_IMAGE_PATH "/home/wuzhenhuan/pictures/zhihu-meme.jpg"

MainWindow::MainWindow(Player* player, QWidget *parent)
    : m_player(player), QMainWindow(parent)
{
    m_playlistManager = new PlaylistManager(this);

    // [Fix] Initialize view with default sort rules if needed, 
    // or trigger a rebuild if playlist already exists
    SortRule defaultRule;
    defaultRule.type = SortType::album; // Or whatever default
    m_playlistManager->getViewModel()->setSingleGrouping(defaultRule);
    
    this->setMinimumSize(1280, 720);
    this->initUI();
    this->initConnection();
}

MainWindow::~MainWindow() {}

void MainWindow::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);
    if (m_cacheLoadScheduled) {
        return;
    }
    m_cacheLoadScheduled = true;
    QTimer::singleShot(0, m_playlistManager, &PlaylistManager::loadCacheAfterShown);
}

void MainWindow::initConnection()
{
    // WControlBar::controlBar: play | pause | stop | next | prev
    connect(controlBar, &WControlBar::sgnBtnPlayClicked, m_player, &Player::play);
    connect(controlBar, &WControlBar::sgnBtnPauseClicked, m_player, &Player::pause);
    connect(controlBar, &WControlBar::sgnBtnStopClicked, m_player, &Player::stop);
    connect(controlBar, &WControlBar::sgnBtnNextClicked, this, [this](){
        QString next_track = m_playlistManager->nextTrack();
        if (!next_track.isEmpty()) {
            emit sgnFilepathChanged(next_track);
        }
    });
    connect(controlBar, &WControlBar::sgnBtnPrevClicked, this, [this](){
        QString prev_track = m_playlistManager->prevTrack();
        if (!prev_track.isEmpty()) {
            emit sgnFilepathChanged(prev_track);
        }
    });

    connect(controlBar, &WControlBar::sgnSliderPositionReleased, this, [this](int percent) {
        m_player->setPosition(percent * 1000);
    });
    connect(controlBar, &WControlBar::sgnSliderVolumeReleased, m_player, &Player::setVolume);
    connect(controlBar, &WControlBar::sgnSliderVolumeMoved, m_player, &Player::setVolume);
    connect(controlBar, &WControlBar::sgnBtnMuteClicked, m_player, [this](){
        m_player->flipMute();
        // then switch icon
    });

    connect(controlBar, &WControlBar::sgnInOrder, this, [this]() {m_playlistManager->getViewModel()->setPlayMode(PlayMode::in_order);});
    connect(controlBar, &WControlBar::sgnLoop, this, [this]() {m_playlistManager->getViewModel()->setPlayMode(PlayMode::loop);});
    connect(controlBar, &WControlBar::sgnShuffle, this, [this]() {m_playlistManager->getViewModel()->setPlayMode(PlayMode::shuffle);});
    connect(controlBar, &WControlBar::sgnOutOfOrderTrack, this, [this]() {m_playlistManager->getViewModel()->setPlayMode(PlayMode::out_of_order_track);});
    connect(controlBar, &WControlBar::sgnOutOfOrderGroup, this, [this]() {m_playlistManager->getViewModel()->setPlayMode(PlayMode::out_of_order_group);});

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
    connect(actSetSortRule, &QAction::triggered, this, [this](){
        WSortTypeSetDialog dialog;
        if (dialog.exec() == QDialog::Accepted) {
            QString input = dialog.getText();
            m_playlistManager->getViewModel()->setSortExpression(input);
        }
    });
    connect(actInsertColumn, &QAction::triggered, this, [this](){
        WInsertColumnDialog dialog;
        int maxIndex = m_playlistManager->getViewModel()->getColumns().size();
        dialog.setMaxIndex(maxIndex);
        dialog.setIndex(1);
        int result = dialog.exec();
        if (result == QDialog::Accepted) {
            TableColumn column = dialog.getRule();
            int index = dialog.index();
            m_playlistManager->getViewModel()->insertColumn(index, column);
        }
    });
    connect(actRemoveColumn, &QAction::triggered, this, [this](){
        WColumnIndexDialog dialog(tr("Remove column"), tr("Input the column index except 0"), this);
        int maxIndex = m_playlistManager->getViewModel()->getColumns().size() - 1;
        dialog.setMaxIndex(maxIndex);
        dialog.setIndex(1);
        if (dialog.exec() == QDialog::Accepted) {
            m_playlistManager->getViewModel()->removeColumn(dialog.index());
        }
    });
    // read file
    connect(this, &MainWindow::sgnFilepathChanged, this, &MainWindow::playTrack);
    connect(this, &MainWindow::sgnLoadPlaylist, m_playlistManager, &PlaylistManager::loadPlaylist);

    connect(m_player->MediaPlayer, &QMediaPlayer::playbackStateChanged, controlBar, &WControlBar::onPlayerStateChanged);
    connect(m_player->MediaPlayer, &QMediaPlayer::mediaStatusChanged, [=](QMediaPlayer::MediaStatus status){
        if (status == QMediaPlayer::MediaStatus::EndOfMedia) {
            QString next_track = m_playlistManager->nextTrack();
            if (!next_track.isEmpty()) {
                emit sgnFilepathChanged(next_track);
            }
        }
    });
    connect(m_player, &Player::positionChanged, controlBar, &WControlBar::updatePosition);
    connect(m_player, &Player::durationChanged, controlBar, &WControlBar::updateDuration);

    // main window: playlist & song table
    connect(m_playlistManager, &PlaylistManager::requestPlay, this, &MainWindow::playTrack);

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
    auto showLoading = [this]() {
        if (++m_loadingCount == 1 && songTreeLoadingLabel) {
            songTreeLoadingLabel->show();
            songTreeLoadingLabel->raise();
        }
    };
    auto hideLoading = [this]() {
        if (m_loadingCount > 0) {
            --m_loadingCount;
        }
        if (m_loadingCount == 0 && songTreeLoadingLabel) {
            songTreeLoadingLabel->hide();
        }
    };

    connect(m_playlistManager, &PlaylistManager::cacheLoadStarted, this, showLoading);
    connect(m_playlistManager, &PlaylistManager::cacheLoadFinished, this, [hideLoading](int) {
        hideLoading();
    });
    connect(m_playlistManager, &PlaylistManager::playlistLoadStarted, this, [showLoading](const QUuid&, int) {
        showLoading();
    });
    connect(m_playlistManager, &PlaylistManager::playlistLoadFinished, this, [hideLoading](const QUuid&) {
        hideLoading();
    });
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
    connect(songTreeViewHeader, &QHeaderView::customContextMenuRequested, this, [this](const QPoint& pos){
        int logical_index = songTreeViewHeader->logicalIndexAt(pos);
        QMenu menu(this);
        QAction* actInsert = menu.addAction("Insert Column Here");
        QAction* actRemove = menu.addAction("Remove This Column");
        connect(actInsert, &QAction::triggered, [this, logical_index](){
            WInsertColumnDialog dialog;
            int maxIndex = m_playlistManager->getViewModel()->getColumns().size();
            dialog.setMaxIndex(maxIndex);
            dialog.setIndex(logical_index);
            if (dialog.exec() == QDialog::Accepted) {
                TableColumn column = dialog.getRule();
                m_playlistManager->getViewModel()->insertColumn(dialog.index(), column);
            }
        });
        connect(actRemove, &QAction::triggered, [this, logical_index](){
            WColumnIndexDialog dialog(tr("Remove column"), tr("Input the column index except 0"), this);
            int maxIndex = m_playlistManager->getViewModel()->getColumns().size() - 1;
            dialog.setMaxIndex(maxIndex);
            dialog.setIndex(logical_index);
            if (dialog.exec() == QDialog::Accepted) {
                m_playlistManager->getViewModel()->removeColumn(dialog.index());
            }
        });
        menu.exec(songTreeViewHeader->mapToGlobal(pos));
    });
}

void MainWindow::initUI()
{
// Global config
    this->setContextMenuPolicy(Qt::NoContextMenu);
 
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

    menuView = new QMenu("&View");
    actSetSortRule = new QAction("Set sort rule (&R)");
    actInsertColumn = new QAction("Insert a column (&I)");
    actRemoveColumn = new QAction("Remove a column (&R)");
    menuView->addAction(actSetSortRule);
    menuView->addAction(actInsertColumn);
    menuView->addAction(actRemoveColumn);
    mainMenuBar->addMenu(menuView);

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
    controlBar = new WControlBar;
    bottomToolBar->addWidget(controlBar);
    addToolBar(Qt::BottomToolBarArea, bottomToolBar);

// Main window
    /// cover & lyrics
    coverSplitter = new QSplitter(Qt::Vertical, this);

    coverImageLabel = new QLabel();
    coverImageLabel->setAlignment(Qt::AlignCenter);
    origin_cover = new QPixmap;
    origin_cover->load(DEFAULT_COVER_IMAGE_PATH);
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
    songTreeView->setSortingEnabled(true);
    songTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    songTreeView->setModel(m_playlistManager->getViewModel());
    songTreeView->setAlternatingRowColors(true);
    songTreeViewHeader = songTreeView->header();
    songTreeViewHeader->setSectionResizeMode(0, QHeaderView::Interactive);
    songTreeViewHeader->setSectionsMovable(true);
    songTreeViewHeader->setFirstSectionMovable(true);
    songTreeViewHeader->setMinimumSectionSize(30);
    songTreeViewHeader->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    songTreeViewHeader->setContextMenuPolicy(Qt::CustomContextMenu);
    songTreeView->setHeaderHidden(false);
    songTreeViewHeader->setVisible(true);

    songTreeLoadingLabel = new QLabel(songTreeView->viewport());
    songTreeLoadingLabel->setText("Loading...");
    songTreeLoadingLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    songTreeLoadingLabel->move(6, 4);
    songTreeLoadingLabel->hide();

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
        SortRule rule;
        rule.type = SortType::album;
        m_playlistManager->getViewModel()->setSingleGrouping(rule);
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


// @TODO: set default type name
void MainWindow::onSavePlaylist() {
    QFileDialog dialog(this);
    dialog.setWindowTitle("Save playlist file");
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    QStringList filters;
    filters.append("All files (*)");
    filters.append("WusicPlayer playlist (*.wcpl)");
    dialog.setNameFilters(filters);

    if (dialog.exec()) {
        QString filename = dialog.selectedFiles().first();
        QString selected_filter = dialog.selectedNameFilter();

        if (!filename.contains(".")) {
            if (selected_filter.contains("*.wcpl")) {
                filename += ".wcpl";
            } // else if ...
        }
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
    updateCoverScale();
    QMainWindow::resizeEvent(event);
}

void MainWindow::playTrack(const QString& filepath) {
    m_player->read(filepath);
    loadCover(filepath);
}

void MainWindow::loadCover(const QString& filepath) {
    QPixmap pix = Audio::parse_cover_to_qpixmap(filepath.toStdString());
    if (!pix.isNull()) {
        *origin_cover = pix;
    } else {
        origin_cover->load(DEFAULT_COVER_IMAGE_PATH);
    }
    updateCoverScale();
}

void MainWindow::updateCoverScale() {
    if (!origin_cover || origin_cover->isNull()) return;
    int dst_side_lenth = this->geometry().width() / 4;
    QPixmap scaled = origin_cover->scaled(
        dst_side_lenth,
        dst_side_lenth,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );
    coverImageLabel->setFixedSize(dst_side_lenth, dst_side_lenth);
    coverImageLabel->setPixmap(scaled);
}
