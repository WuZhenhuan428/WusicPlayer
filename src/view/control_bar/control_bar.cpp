#include "view/control_bar/control_bar.h"

#include "core/theme/theme_manager.h"
#include <QIcon>
#include <magic_enum/magic_enum.hpp>

#define SLIDER_VOLUME_MIN_WIDTH 100
#define SLIDER_VOLUME_MAX_WIDTH 100

#define ICON_SIZE               25
#define BTN_SIZE                25

/// 根据当前主题的明暗 + 用户图标模式偏好返回正确的图标路径前缀

#include "core/logger/logger_manager.h"
namespace
{
Logger* logger = LoggerManager::file_logger("control_bar", {"console", "gui"});
}

static QString iconPath(const QString& name)
{
    bool dark = ThemeManager::instance().effective_icon_is_dark();
    return QString(":/icons/%1/%2.svg").arg(dark ? "dark" : "light", name);
}

ControlBar::ControlBar(QWidget* parent) : QWidget(parent)
{
    m_btn_play_pause = new QPushButton(this);
    m_btn_play_pause->setIcon(QIcon(iconPath("play")));

    m_btn_stop = new QPushButton(this);
    m_btn_stop->setIcon(QIcon(iconPath("stop")));

    m_btn_prev = new QPushButton(this);
    m_btn_prev->setIcon(QIcon(iconPath("prev")));

    m_btn_next = new QPushButton(this);
    m_btn_next->setIcon(QIcon(iconPath("next")));

    m_btn_devices = new QPushButton(this);
    m_btn_devices->setIcon(QIcon(iconPath("device")));

    // TODO: load correct icons when restore config status
    m_btn_mode = new QPushButton(this);
    m_btn_mode->setIcon(QIcon(iconPath("in_order")));

    m_btn_mute = new QPushButton(this);
    m_btn_mute->setIcon(QIcon(iconPath("volume_3")));

    m_btn_devices->setFixedSize(BTN_SIZE, BTN_SIZE);
    m_btn_play_pause->setFixedSize(BTN_SIZE, BTN_SIZE);
    m_btn_stop->setFixedSize(BTN_SIZE, BTN_SIZE);
    m_btn_prev->setFixedSize(BTN_SIZE, BTN_SIZE);
    m_btn_next->setFixedSize(BTN_SIZE, BTN_SIZE);
    m_btn_mode->setFixedSize(BTN_SIZE, BTN_SIZE);
    m_btn_mute->setFixedSize(BTN_SIZE, BTN_SIZE);

    m_btn_play_pause->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    m_btn_stop->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    m_btn_prev->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    m_btn_next->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    m_btn_devices->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    m_btn_mode->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
    m_btn_mute->setIconSize(QSize(ICON_SIZE, ICON_SIZE));

    m_btn_play_pause->setFlat(true);
    m_btn_stop->setFlat(true);
    m_btn_prev->setFlat(true);
    m_btn_next->setFlat(true);
    m_btn_devices->setFlat(true);
    m_btn_mode->setFlat(true);
    m_btn_mute->setFlat(true);

    m_menu_mode              = new QMenu(this);
    m_act_in_order           = new QAction("In order", m_menu_mode);
    m_act_loop               = new QAction("Loop", m_menu_mode);
    m_act_shuffle            = new QAction("Shuffle", m_menu_mode);
    m_act_out_of_order_track = new QAction("Out of order by track", m_menu_mode);
    m_act_out_of_order_group = new QAction("Out of order by group", m_menu_mode);

    m_act_group              = new QActionGroup(this);
    m_act_group->setExclusive(true);

    m_menu_devices    = new QMenu(this);

    /// Position Bar: position/Duration
    m_slider_position = new QSlider(Qt::Horizontal, this);
    m_slider_position->setRange(0, 100);
    /// bar's time progress
    m_time_progress = new TimeProgress(this);
    m_slider_volume = new QSlider(Qt::Horizontal, this);
    m_slider_volume->setRange(0, 100);
    m_slider_volume->setValue(100);
    m_slider_volume->setMinimumWidth(SLIDER_VOLUME_MIN_WIDTH);
    m_slider_volume->setMaximumWidth(SLIDER_VOLUME_MAX_WIDTH);

    m_hbl_main = new QHBoxLayout(this);
    m_hbl_main->addWidget(m_btn_play_pause);
    m_hbl_main->addWidget(m_btn_stop);
    m_hbl_main->addWidget(m_btn_prev);
    m_hbl_main->addWidget(m_btn_next);
    m_hbl_main->addWidget(m_slider_position);
    m_hbl_main->addWidget(m_time_progress);
    m_hbl_main->addWidget(m_btn_devices);
    m_hbl_main->addWidget(m_btn_mode);
    m_hbl_main->addWidget(m_btn_mute);
    m_hbl_main->addWidget(m_slider_volume);

    for (QAction* action : {m_act_in_order, m_act_loop, m_act_shuffle, m_act_out_of_order_track,
                            m_act_out_of_order_group}) {
        action->setCheckable(true);
        m_menu_mode->addAction(action);
        m_act_group->addAction(action);
    }

    this->setLayout(m_hbl_main);

    connect(m_btn_play_pause, &QPushButton::clicked, this, [this]() {
        if (m_is_playing)
            emit sgnBtnPlayPauseClicked(false);
        else
            emit sgnBtnPlayPauseClicked(true);
    });
    connect(m_btn_stop, &QPushButton::clicked, this, [this]() { emit sgnBtnStopClicked(); });
    connect(m_btn_next, &QPushButton::clicked, this, [this]() { emit sgnBtnNextClicked(); });
    connect(m_btn_prev, &QPushButton::clicked, this, [this]() { emit sgnBtnPrevClicked(); });
    connect(m_btn_mute, &QPushButton::clicked, this, [this]() { emit sgnBtnMuteClicked(); });
    connect(m_btn_mode, &QPushButton::clicked, this, [this]() {
        QPoint pos = m_btn_mode->mapToGlobal(QPoint(0, m_btn_mode->height()));
        m_menu_mode->exec(pos);
    });

    connect(m_act_in_order, &QAction::triggered, this, [this]() { emit sgnInOrder(); });
    connect(m_act_loop, &QAction::triggered, this, [this]() { emit sgnLoop(); });
    connect(m_act_shuffle, &QAction::triggered, this, [this]() { emit sgnShuffle(); });
    connect(m_act_out_of_order_track, &QAction::triggered, this,
            [this]() { emit sgnOutOfOrderTrack(); });
    connect(m_act_out_of_order_group, &QAction::triggered, this,
            [this]() { emit sgnOutOfOrderGroup(); });

    connect(m_slider_volume, &QSlider::valueChanged, this, [this](int value) {
        emit sgnSliderVolumeReleased(value);
        this->update_volume_slider_icon(value);
    });
    connect(m_slider_position, &QSlider::sliderReleased, this,
            [this]() { emit sgnSliderPositionReleased(m_slider_position->value()); });
    connect(m_slider_position, &QSlider::sliderMoved, this,
            [this](int value) { m_time_progress->set_current_time(value); });
    connect(m_btn_devices, &QPushButton::clicked, this, [this]() {
        QPoint pos = m_btn_devices->mapToGlobal(QPoint(0, m_btn_devices->height()));
        m_menu_devices->exec(pos);
    });

    // 图标模式切换后立即刷新所有图标
    connect(&ThemeManager::instance(), &ThemeManager::sgn_theme_changed, this,
            &ControlBar::refresh_all_icons);
}

ControlBar::~ControlBar() {}

void ControlBar::refresh_all_icons()
{
    m_btn_stop->setIcon(QIcon(iconPath("stop")));
    m_btn_prev->setIcon(QIcon(iconPath("prev")));
    m_btn_next->setIcon(QIcon(iconPath("next")));
    m_btn_devices->setIcon(QIcon(iconPath("device")));
    m_btn_play_pause->setIcon(QIcon(iconPath(m_is_playing ? "pause" : "play")));
    m_btn_mode->setIcon(QIcon(iconPath(m_current_mode_icon)));
    if (m_is_muted) {
        m_btn_mute->setIcon(QIcon(iconPath("volume_x")));
    } else {
        update_volume_slider_icon(m_current_volume_pct);
    }
}

void ControlBar::update_button_status(PlayerEngine::PlayingState new_state)
{
    // set icon here
    logger->debug("[ControlBar] update new state: {}", magic_enum::enum_name(new_state));
    if (new_state != PlayerEngine::PlayingState::PLAYING) {
        m_is_playing = false;
        m_btn_play_pause->setIcon(QIcon(iconPath("play")));
    } else {
        m_is_playing = true;
        m_btn_play_pause->setIcon(QIcon(iconPath("pause")));
    }
}

void ControlBar::update_duration(qint64 duration_ms)
{
    qint64 duration_s = duration_ms / 1000;
    m_slider_position->setRange(0, duration_s);
    m_time_progress->set_total_time(duration_s);
}

void ControlBar::update_position(qint64 position_ms)
{
    if (!m_slider_position->isSliderDown()) {
        m_slider_position->setValue(position_ms / 1000);
        m_time_progress->set_current_time(position_ms / 1000);
    }
}

void ControlBar::update_volume_slider(int percent)
{
    if (!m_slider_volume->isSliderDown()) {
        m_slider_volume->setValue(percent);
    }
}

void ControlBar::update_mute_button(bool muted)
{
    m_is_muted = muted;
    if (muted) {
        m_btn_mute->setIcon(QIcon(iconPath("volume_x")));
    } else {
        this->update_volume_slider_icon(m_slider_volume->value());
    }
}

void ControlBar::set_play_mode(PlayMode mode)
{
    for (QAction* action : {m_act_in_order, m_act_loop, m_act_shuffle, m_act_out_of_order_track,
                            m_act_out_of_order_group}) {
        action->setCheckable(true);
        action->setIconText("");
    }
    if (mode == PlayMode::in_order) {
        m_act_in_order->setChecked(true);
        m_current_mode_icon = QStringLiteral("in_order");
        this->update_mode_icon(iconPath("in_order"));
    } else if (mode == PlayMode::loop) {
        m_act_loop->setChecked(true);
        m_current_mode_icon = QStringLiteral("loop");
        this->update_mode_icon(iconPath("loop"));
    } else if (mode == PlayMode::shuffle) {
        m_act_shuffle->setChecked(true);
        m_current_mode_icon = QStringLiteral("shuffle");
        this->update_mode_icon(iconPath("shuffle"));
    } else if (mode == PlayMode::out_of_order_group) {
        m_act_out_of_order_group->setChecked(true);
        m_current_mode_icon = QStringLiteral("out_of_order_group");
        this->update_mode_icon(iconPath("out_of_order_group"));
    } else if (mode == PlayMode::out_of_order_track) {
        m_act_out_of_order_track->setChecked(true);
        m_current_mode_icon = QStringLiteral("out_of_order_track");
        this->update_mode_icon(iconPath("out_of_order_track"));
    }
}

void ControlBar::set_device(const QList<QAudioDevice>& devices, const QByteArray& current_id)
{
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

        connect(act, &QAction::triggered, this,
                [this, act]() { emit sgnSelectDeviceId(act->data().toByteArray()); });
    }
}

QSlider* ControlBar::get_progress_slider() const
{
    return m_slider_position;
};

QSlider* ControlBar::get_volume_slider() const
{
    return m_slider_volume;
};

/* in player, set_mute == set_volume(0), so when update slider, the mute
   status will be covered, and there is NO NEED to save mute status */
void ControlBar::update_volume_slider_icon(int volume_by_percent)
{
    m_current_volume_pct = volume_by_percent;
    if (volume_by_percent > 66) {
        m_btn_mute->setIcon(QIcon(iconPath("volume_3")));
    } else if (volume_by_percent > 33) {
        m_btn_mute->setIcon(QIcon(iconPath("volume_2")));
    } else if (volume_by_percent > 0) {
        m_btn_mute->setIcon(QIcon(iconPath("volume_1")));
    } else {
        m_btn_mute->setIcon(QIcon(iconPath("volume_0")));
    }
}

void ControlBar::update_mode_icon(QString icon_url)
{
    QIcon icon(icon_url);
    m_btn_mode->setIcon(icon);
}
