#include "WControlBar.h"

#define SLIDER_VOLUME_MIN_WIDTH 100
#define SLIDER_VOLUME_MAX_WIDTH 100

WControlBar::WControlBar(QWidget* parent)
    : QWidget(parent)
{
    btnPlay = new QPushButton(">", this);
    btnPause = new QPushButton("||", this);
    btnStop = new QPushButton(">|", this);
    btnPrev = new QPushButton("<<", this);
    btnNext = new QPushButton(">>", this);
    btnMode = new QPushButton("M", this);
    btnMute = new QPushButton("V", this);
    btnDevices = new QPushButton("D", this);
    btnPlay->setFixedSize(25, 25);
    btnPause->setFixedSize(25, 25);
    btnStop->setFixedSize(25, 25);
    btnPrev->setFixedSize(25, 25);
    btnNext->setFixedSize(25, 25);
    btnMode->setFixedSize(25, 25);
    btnMute->setFixedSize(25, 25);
    btnDevices->setFixedSize(25, 25);

    menuMode = new QMenu(this);
    actInOrder = new QAction("In order", menuMode);
    actLoop = new QAction("Loop", menuMode);
    actShuffle = new QAction("Shuffle", menuMode);
    actOutOfOrderTrack = new QAction("Out of order by track", menuMode);
    actOutOfOrderGroup = new QAction("Out of order by troup", menuMode);

    menuDevices = new QMenu(this);

    /// Position Bar: position/Duration
    sliderPostion = new QSlider(Qt::Horizontal, this);
    sliderPostion->setRange(0, 100);
    /// bar's time progress
    timeProgress = new WTimeProgress(this);
    sliderVolume = new QSlider(Qt::Horizontal, this);
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
    hbMain->addWidget(btnDevices);
    hbMain->addWidget(btnMode);
    hbMain->addWidget(btnMute);
    hbMain->addWidget(sliderVolume);

    for (QAction* action : {actInOrder, actLoop, actShuffle, actOutOfOrderTrack, actOutOfOrderGroup}) {
        action->setCheckable(true);
        menuMode->addAction(action);

        connect(action, &QAction::toggled, this, [this](bool checked) {
            QAction* act = qobject_cast<QAction*>(sender());
            if (act) {
                act->setChecked(true);
            }
        });
    }
    
    this->setLayout(hbMain);


    actGroup = new QActionGroup(this);
    actGroup->setExclusive(true);
    actGroup->addAction(actInOrder);
    actGroup->addAction(actLoop);
    actGroup->addAction(actShuffle);
    actGroup->addAction(actOutOfOrderTrack);
    actGroup->addAction(actOutOfOrderGroup);

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
    connect(btnDevices, &QPushButton::clicked, this, [this](){
        QPoint pos = btnDevices->mapToGlobal(QPoint(0, btnDevices->height()));
        menuDevices->exec(pos);
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

void WControlBar::setPlayMode(PlayMode mode) {
    for (QAction* action : {actInOrder, actLoop, actShuffle, actOutOfOrderTrack, actOutOfOrderGroup}) {
        action->setCheckable(true);
        action->setIconText("");
    }
    if (mode == PlayMode::in_order) {
        actInOrder->setChecked(true);
    } else if (mode == PlayMode::loop) {
        actLoop->setChecked(true);
    } else if (mode == PlayMode::shuffle)  {
        actShuffle->setChecked(true);
    } else if (mode == PlayMode::out_of_order_group) {
        actOutOfOrderGroup->setChecked(true);
    } else if (mode == PlayMode::out_of_order_track) {
        actOutOfOrderTrack->setChecked(true);
    }
}

void WControlBar::setDevice(const QList<QAudioDevice>& devices, const QByteArray& current_id) {
    m_devices = devices;

    menuDevices->clear();
    auto* group = new QActionGroup(menuDevices);
    group->setExclusive(true);
    for (const auto& dev : m_devices) {
        QAction* act = menuDevices->addAction(dev.description());
        act->setCheckable(true);
        act->setChecked(dev.id() == current_id);
        act->setData(dev.id());
        group->addAction(act);

        connect(act, &QAction::triggered, this, [this, act]() {
            emit sgnSelectDeviceId(act->data().toByteArray());
        });
    }
}