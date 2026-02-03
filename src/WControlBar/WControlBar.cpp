#include "WControlBar.h"

#define SLIDER_VOLUME_MIN_WIDTH 100
#define SLIDER_VOLUME_MAX_WIDTH 100

WControlBar::WControlBar(QWidget* parent) : QWidget(parent) {
    btnPlay = new QPushButton(">");
    btnPause = new QPushButton("||");
    btnStop = new QPushButton(">|");
    btnPrev = new QPushButton("<<");
    btnNext = new QPushButton(">>");
    btnMute = new QPushButton("M");
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

    hbMain = new QHBoxLayout(this);
    hbMain->addWidget(btnPlay);
    hbMain->addWidget(btnPause);
    hbMain->addWidget(btnStop);
    hbMain->addWidget(btnPrev);
    hbMain->addWidget(btnNext);
    hbMain->addWidget(sliderPostion);
    hbMain->addWidget(timeProgress);
    hbMain->addWidget(btnMute);
    hbMain->addWidget(sliderVolume);

    this->setLayout(hbMain);

    connect(btnPlay, &QPushButton::clicked, this, [this](){emit sgnBtnPlayClicked();});
    connect(btnPause, &QPushButton::clicked, this, [this](){emit sgnBtnPauseClicked();});
    connect(btnStop, &QPushButton::clicked, this, [this](){emit sgnBtnStopClicked();});
    connect(btnNext, &QPushButton::clicked, this, [this](){emit sgnBtnNextClicked();});
    connect(btnPrev, &QPushButton::clicked, this, [this](){emit sgnBtnPrevClicked();});
    connect(btnMute, &QPushButton::clicked, this, [this](){emit sgnBtnMuteClicked();});

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