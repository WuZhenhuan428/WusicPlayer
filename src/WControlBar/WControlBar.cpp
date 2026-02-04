#include "WControlBar.h"

#define SLIDER_VOLUME_MIN_WIDTH 100
#define SLIDER_VOLUME_MAX_WIDTH 100

WControlBar::WControlBar(QWidget* parent) : QWidget(parent) {
    btnPlay = new QPushButton(">");
    btnPause = new QPushButton("||");
    btnStop = new QPushButton(">|");
    btnPrev = new QPushButton("<<");
    btnNext = new QPushButton(">>");
    btnMode = new QPushButton("M");
    btnMute = new QPushButton("V");
    btnPlay->setFixedSize(25, 25);
    btnPause->setFixedSize(25, 25);
    btnStop->setFixedSize(25, 25);
    btnPrev->setFixedSize(25, 25);
    btnNext->setFixedSize(25, 25);
    btnMode->setFixedSize(25, 25);
    btnMute->setFixedSize(25, 25);

    actInOrder = new QAction("In order");
    actLoop = new QAction("Loop");
    actShuffle = new QAction("Shuffle");
    actOutOfOrderTrack = new QAction("Out of order by track");
    actOutOfOrderGroup = new QAction("Out of order by troup");
    menuMode = new QMenu();
    menuMode->addAction(actInOrder);
    menuMode->addAction(actLoop);
    menuMode->addAction(actShuffle);
    menuMode->addAction(actOutOfOrderTrack);
    menuMode->addAction(actOutOfOrderGroup);

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

    hbMain = new QHBoxLayout(this);
    hbMain->addWidget(btnPlay);
    hbMain->addWidget(btnPause);
    hbMain->addWidget(btnStop);
    hbMain->addWidget(btnPrev);
    hbMain->addWidget(btnNext);
    hbMain->addWidget(sliderPostion);
    hbMain->addWidget(timeProgress);
    hbMain->addWidget(btnMode);
    hbMain->addWidget(btnMute);
    hbMain->addWidget(sliderVolume);

    this->setLayout(hbMain);

    connect(btnPlay, &QPushButton::clicked, this, [this](){emit sgnBtnPlayClicked();});
    connect(btnPause, &QPushButton::clicked, this, [this](){emit sgnBtnPauseClicked();});
    connect(btnStop, &QPushButton::clicked, this, [this](){emit sgnBtnStopClicked();});
    connect(btnNext, &QPushButton::clicked, this, [this](){emit sgnBtnNextClicked();});
    connect(btnPrev, &QPushButton::clicked, this, [this](){emit sgnBtnPrevClicked();});
    connect(btnMute, &QPushButton::clicked, this, [this](){emit sgnBtnMuteClicked();});
    connect(btnMode, &QPushButton::clicked, this, [this](){
        QPoint pos = btnMode->mapToGlobal(QPoint(0, btnMode->height()));
        menuMode->exec(pos);
    });
    connect(actInOrder, &QAction::triggered, this, [this](){emit sgnInOrder();});
    connect(actLoop, &QAction::triggered, this, [this](){emit sgnLoop();});
    connect(actShuffle, &QAction::triggered, this, [this](){emit sgnShuffle();});
    connect(actOutOfOrderTrack, &QAction::triggered, this, [this](){emit sgnOutOfOrderTrack();});
    connect(actOutOfOrderGroup, &QAction::triggered, this, [this](){emit sgnOutOfOrderGroup();});

    connect(sliderPostion, &QSlider::sliderReleased, this, [this](){emit sgnSliderPositionReleased(sliderPostion->value());});
    connect(sliderVolume, &QSlider::sliderReleased, this, [this](){emit sgnSliderVolumeReleased(sliderVolume->value());});
    connect(sliderVolume, &QSlider::sliderMoved, this, [this](){emit sgnSliderVolumeMoved(sliderVolume->value());});
    connect(sliderPostion, &QSlider::sliderMoved, this, [this](int value){
        timeProgress->setCurrentTime(value);
    });
}

WControlBar::~WControlBar() {}

void WControlBar::onPlayerStateChanged(QMediaPlayer::PlaybackState newState) {
    btnPlay->setEnabled( newState != QMediaPlayer::PlaybackState::PlayingState );
    btnPause->setEnabled( newState != QMediaPlayer::PlaybackState::PausedState );
}

void WControlBar::updateDuration(qint64 duration_ms) {
    qint64 duration_s = duration_ms / 1000;
    sliderPostion->setRange(0, duration_s);
    timeProgress->setTotalTime(duration_s);
}

void WControlBar::updatePosition(qint64 position_ms) {
    if (!sliderPostion->isSliderDown())
    {
        sliderPostion->setValue(position_ms/1000);
        timeProgress->setCurrentTime(position_ms/1000);
    }
}