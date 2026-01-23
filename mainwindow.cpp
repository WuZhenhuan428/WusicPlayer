#include "mainwindow.h"

#define SLIDER_VOLUME_MIN_WIDTH 80
#define SLIDER_VOLUME_MAX_WIDTH 80

MainWindow::MainWindow(Player* player, QWidget *parent)
    : m_player(player), QMainWindow(parent)
{
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

    // Menu
    /// Select a file & get filepath
    connect(actOpenFile, &QAction::triggered, this, &MainWindow::onOpenFile);
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
    connect(this, &MainWindow::filepathChanged, m_player, &Player::read);

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
    // connect(playlistTree, &QTreeWidget::currentItemChanged, this, );
}

void MainWindow::initUI()
{
// MenuBar
    mainMenuBar = new QMenuBar;

    menuFile = new QMenu("&File");
    actOpenFile = new QAction("&Open");
    actExit = new QAction("&Exit");
    menuFile->addAction(actOpenFile);
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
    coverSplitter->addWidget(coverImageLabel);
    coverSplitter->addWidget(lrcListView);
    coverSplitter->setStretchFactor(0, 1);
    coverSplitter->setStretchFactor(1, 1);
    /// list & table with splitter
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    playlistTree = new QTreeWidget();
    playlistTree->setHeaderLabel("Playlist");
    QTreeWidgetItem* favorites = new QTreeWidgetItem(playlistTree, QStringList() << "MyFavorite");
    QTreeWidgetItem* recently = new QTreeWidgetItem(playlistTree, QStringList() << "Recently");
    songTableView = new QTableView();

    mainSplitter->addWidget(playlistTree);
    mainSplitter->addWidget(songTableView);
    mainSplitter->addWidget(coverSplitter);
    mainSplitter->setStretchFactor(0, 2);
    mainSplitter->setStretchFactor(1, 6);
    mainSplitter->setStretchFactor(2, 3);
    setCentralWidget(mainSplitter);
}

void MainWindow::onOpenFile()
{
    QString filepath = QFileDialog::getOpenFileName(
        this,
        tr("Open Audio File"),
        QString(),
        tr("*.mp3 *.wav *.flac")
    );

    if (!filepath.isEmpty()) {
        emit filepathChanged(filepath);
    }
    else {
        qDebug() << "[INFO] filepath is empty.";
    }
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
