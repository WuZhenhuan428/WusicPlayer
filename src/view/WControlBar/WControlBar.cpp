#include "WControlBar.h"

#define SLIDER_VOLUME_MIN_WIDTH 100
#define SLIDER_VOLUME_MAX_WIDTH 100

WControlBar::WControlBar(QWidget* parent)
    : QWidget(parent)
{
    m_btn_play = new QPushButton(">", this);
    m_btn_pause = new QPushButton("||", this);
    m_btn_stop = new QPushButton(">|", this);
    m_btn_prev = new QPushButton("<<", this);
    m_btn_next = new QPushButton(">>", this);
    m_btn_mode = new QPushButton("M", this);
    m_btn_mute = new QPushButton("V", this);
    m_btn_devices = new QPushButton("D", this);
    m_btn_play->setFixedSize(25, 25);
    m_btn_pause->setFixedSize(25, 25);
    m_btn_stop->setFixedSize(25, 25);
    m_btn_prev->setFixedSize(25, 25);
    m_btn_next->setFixedSize(25, 25);
    m_btn_mode->setFixedSize(25, 25);
    m_btn_mute->setFixedSize(25, 25);
    m_btn_devices->setFixedSize(25, 25);

    m_menu_mode = new QMenu(this);
    m_act_in_order = new QAction("In order", m_menu_mode);
    m_act_loop = new QAction("Loop", m_menu_mode);
    m_act_shuffle = new QAction("Shuffle", m_menu_mode);
    m_act_out_of_order_track = new QAction("Out of order by track", m_menu_mode);
    m_act_out_of_order_group = new QAction("Out of order by group", m_menu_mode);

    m_act_group = new QActionGroup(this);
    m_act_group->setExclusive(true);

    m_menu_devices = new QMenu(this);

    /// Position Bar: position/Duration
    m_slider_position = new QSlider(Qt::Horizontal, this);
    m_slider_position->setRange(0, 100);
    /// bar's time progress
    m_time_progress = new WTimeProgress(this);
    m_slider_volume = new QSlider(Qt::Horizontal, this);
    m_slider_volume->setRange(0, 100);
    m_slider_volume->setValue(100);
    m_slider_volume->setMinimumWidth(SLIDER_VOLUME_MIN_WIDTH);
    m_slider_volume->setMaximumWidth(SLIDER_VOLUME_MAX_WIDTH);

    m_hbl_main = new QHBoxLayout(this);
    m_hbl_main->addWidget(m_btn_play);
    m_hbl_main->addWidget(m_btn_pause);
    m_hbl_main->addWidget(m_btn_stop);
    m_hbl_main->addWidget(m_btn_prev);
    m_hbl_main->addWidget(m_btn_next);
    m_hbl_main->addWidget(m_slider_position);
    m_hbl_main->addWidget(m_time_progress);
    m_hbl_main->addWidget(m_btn_devices);
    m_hbl_main->addWidget(m_btn_mode);
    m_hbl_main->addWidget(m_btn_mute);
    m_hbl_main->addWidget(m_slider_volume);

    for (QAction* action : {m_act_in_order, m_act_loop, m_act_shuffle, m_act_out_of_order_track, m_act_out_of_order_group}) {
        action->setCheckable(true);
        m_menu_mode->addAction(action);
        m_act_group->addAction(action);
    }
    
    this->setLayout(m_hbl_main);

    connect(m_btn_play, &QPushButton::clicked, this, [this](){emit sgnBtnPlayClicked();});
    connect(m_btn_pause, &QPushButton::clicked, this, [this](){emit sgnBtnPauseClicked();});
    connect(m_btn_stop, &QPushButton::clicked, this, [this](){emit sgnBtnStopClicked();});
    connect(m_btn_next, &QPushButton::clicked, this, [this](){emit sgnBtnNextClicked();});
    connect(m_btn_prev, &QPushButton::clicked, this, [this](){emit sgnBtnPrevClicked();});
    connect(m_btn_mute, &QPushButton::clicked, this, [this](){emit sgnBtnMuteClicked();});
    connect(m_btn_mode, &QPushButton::clicked, this, [this](){
        QPoint pos = m_btn_mode->mapToGlobal(QPoint(0, m_btn_mode->height()));
        m_menu_mode->exec(pos);
    });

    connect(m_act_in_order, &QAction::triggered, this, [this](){emit sgnInOrder();});
    connect(m_act_loop, &QAction::triggered, this, [this](){emit sgnLoop();});
    connect(m_act_shuffle, &QAction::triggered, this, [this](){emit sgnShuffle();});
    connect(m_act_out_of_order_track, &QAction::triggered, this, [this](){emit sgnOutOfOrderTrack();});
    connect(m_act_out_of_order_group, &QAction::triggered, this, [this](){emit sgnOutOfOrderGroup();});

    connect(m_slider_position, &QSlider::sliderReleased, this, [this](){emit sgnSliderPositionReleased(m_slider_position->value());});
    connect(m_slider_volume, &QSlider::sliderReleased, this, [this](){emit sgnSliderVolumeReleased(m_slider_volume->value());});
    connect(m_slider_volume, &QSlider::sliderMoved, this, [this](){emit sgnSliderVolumeMoved(m_slider_volume->value());});
    connect(m_slider_position, &QSlider::sliderMoved, this, [this](int value){
        m_time_progress->setCurrentTime(value);
    });
    connect(m_btn_devices, &QPushButton::clicked, this, [this](){
        QPoint pos = m_btn_devices->mapToGlobal(QPoint(0, m_btn_devices->height()));
        m_menu_devices->exec(pos);
    });
}

WControlBar::~WControlBar() {}

void WControlBar::onPlayerStateChanged(QMediaPlayer::PlaybackState newState) {
    m_btn_play->setEnabled( newState != QMediaPlayer::PlaybackState::PlayingState );
    m_btn_pause->setEnabled( newState != QMediaPlayer::PlaybackState::PausedState );
}

void WControlBar::updateDuration(qint64 duration_ms) {
    qint64 duration_s = duration_ms / 1000;
    m_slider_position->setRange(0, duration_s);
    m_time_progress->setTotalTime(duration_s);
}

void WControlBar::updatePosition(qint64 position_ms) {
    if (!m_slider_position->isSliderDown())
    {
        m_slider_position->setValue(position_ms/1000);
        m_time_progress->setCurrentTime(position_ms/1000);
    }
}

void WControlBar::setPlayMode(PlayMode mode) {
    for (QAction* action : {m_act_in_order, m_act_loop, m_act_shuffle, m_act_out_of_order_track, m_act_out_of_order_group}) {
        action->setCheckable(true);
        action->setIconText("");
    }
    if (mode == PlayMode::in_order) {
        m_act_in_order->setChecked(true);
    } else if (mode == PlayMode::loop) {
        m_act_loop->setChecked(true);
    } else if (mode == PlayMode::shuffle)  {
        m_act_shuffle->setChecked(true);
    } else if (mode == PlayMode::out_of_order_group) {
        m_act_out_of_order_group->setChecked(true);
    } else if (mode == PlayMode::out_of_order_track) {
        m_act_out_of_order_track->setChecked(true);
    }
}

void WControlBar::setDevice(const QList<QAudioDevice>& devices, const QByteArray& current_id) {
    m_devices = devices;

    m_menu_devices->clear();
    auto* group = new QActionGroup(m_menu_devices);
    group->setExclusive(true);
    for (const auto& dev : m_devices) {
        QAction* act = m_menu_devices->addAction(dev.description());
        act->setCheckable(true);
        act->setChecked(dev.id() == current_id);
        act->setData(dev.id());
        group->addAction(act);

        connect(act, &QAction::triggered, this, [this, act]() {
            emit sgnSelectDeviceId(act->data().toByteArray());
        });
    }
}

QSlider* WControlBar::getProgressSlider() const {
    return m_slider_position;
};

QSlider* WControlBar::getVolumeSlider() const {
    return m_slider_volume;
};