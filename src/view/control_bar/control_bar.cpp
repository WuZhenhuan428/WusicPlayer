#include "view/control_bar/control_bar.h"

#include "core/theme/theme_manager.h"
#include <QIcon>
#include <magic_enum/magic_enum.hpp>

static constexpr int kSliderVolumeMinWidth = 100;
static constexpr int kSliderVolumeMaxWidth = 100;
static constexpr int kIconSize             = 25;
static constexpr int kBtnSize              = 25;

/// 根据当前主题的明暗 + 用户图标模式偏好返回正确的图标路径前缀

#include "core/logger/logger_manager.h"
namespace
{
// NOLINTNEXTLINE
Logger* logger = LoggerManager::file_logger("control_bar", {"console", "gui"});
} // namespace

static QString icon_path(const QString& name)
{
    bool dark = ThemeManager::instance().effective_icon_is_dark();
    return QString(":/icons/%1/%2.svg").arg(dark ? "dark" : "light", name);
}

ControlBar::ControlBar(QWidget* parent) : QWidget(parent)
{
    btn_play_pause_ = new QPushButton(this);
    btn_play_pause_->setIcon(QIcon(icon_path("play")));

    btn_stop_ = new QPushButton(this);
    btn_stop_->setIcon(QIcon(icon_path("stop")));

    btn_prev_ = new QPushButton(this);
    btn_prev_->setIcon(QIcon(icon_path("prev")));

    btn_next_ = new QPushButton(this);
    btn_next_->setIcon(QIcon(icon_path("next")));

    btn_devices_ = new QPushButton(this);
    btn_devices_->setIcon(QIcon(icon_path("device")));

    // TODO: load correct icons when restore config status
    btn_mode_ = new QPushButton(this);
    btn_mode_->setIcon(QIcon(icon_path("in_order")));

    btn_mute_ = new QPushButton(this);
    btn_mute_->setIcon(QIcon(icon_path("volume_3")));

    btn_devices_->setFixedSize(kBtnSize, kBtnSize);
    btn_play_pause_->setFixedSize(kBtnSize, kBtnSize);
    btn_stop_->setFixedSize(kBtnSize, kBtnSize);
    btn_prev_->setFixedSize(kBtnSize, kBtnSize);
    btn_next_->setFixedSize(kBtnSize, kBtnSize);
    btn_mode_->setFixedSize(kBtnSize, kBtnSize);
    btn_mute_->setFixedSize(kBtnSize, kBtnSize);

    btn_play_pause_->setIconSize(QSize(kIconSize, kIconSize));
    btn_stop_->setIconSize(QSize(kIconSize, kIconSize));
    btn_prev_->setIconSize(QSize(kIconSize, kIconSize));
    btn_next_->setIconSize(QSize(kIconSize, kIconSize));
    btn_devices_->setIconSize(QSize(kIconSize, kIconSize));
    btn_mode_->setIconSize(QSize(kIconSize, kIconSize));
    btn_mute_->setIconSize(QSize(kIconSize, kIconSize));

    btn_play_pause_->setFlat(true);
    btn_stop_->setFlat(true);
    btn_prev_->setFlat(true);
    btn_next_->setFlat(true);
    btn_devices_->setFlat(true);
    btn_mode_->setFlat(true);
    btn_mute_->setFlat(true);

    menu_mode_              = new QMenu(this);
    act_in_order_           = new QAction("In order", menu_mode_);
    act_loop_               = new QAction("Loop", menu_mode_);
    act_shuffle_            = new QAction("Shuffle", menu_mode_);
    act_out_of_order_track_ = new QAction("Out of order by track", menu_mode_);
    act_out_of_order_group_ = new QAction("Out of order by group", menu_mode_);

    act_group_              = new QActionGroup(this);
    act_group_->setExclusive(true);

    menu_devices_    = new QMenu(this);

    /// Position Bar: position/Duration
    slider_position_ = new QSlider(Qt::Horizontal, this);
    slider_position_->setRange(0, 100);
    /// bar's time progress
    time_progress_ = new TimeProgress(this);
    slider_volume_ = new QSlider(Qt::Horizontal, this);
    slider_volume_->setRange(0, 100);
    slider_volume_->setValue(100);
    slider_volume_->setMinimumWidth(kSliderVolumeMinWidth);
    slider_volume_->setMaximumWidth(kSliderVolumeMaxWidth);

    hbl_main_ = new QHBoxLayout(this);
    hbl_main_->addWidget(btn_play_pause_);
    hbl_main_->addWidget(btn_stop_);
    hbl_main_->addWidget(btn_prev_);
    hbl_main_->addWidget(btn_next_);
    hbl_main_->addWidget(slider_position_);
    hbl_main_->addWidget(time_progress_);
    hbl_main_->addWidget(btn_devices_);
    hbl_main_->addWidget(btn_mode_);
    hbl_main_->addWidget(btn_mute_);
    hbl_main_->addWidget(slider_volume_);

    for (QAction* action : {
             act_in_order_,
             act_loop_,
             act_shuffle_,
             act_out_of_order_track_,
             act_out_of_order_group_,
         }) {
        action->setCheckable(true);
        menu_mode_->addAction(action);
        act_group_->addAction(action);
    }

    this->setLayout(hbl_main_);

    connect(btn_play_pause_, &QPushButton::clicked, this, [this]() {
        if (is_playing_) {
            emit sgn_btn_play_pause_clicked(false);
        } else {
            emit sgn_btn_play_pause_clicked(true);
        }
    });
    connect(btn_stop_, &QPushButton::clicked, this, [this]() { emit sgn_btn_stop_clicked(); });
    connect(btn_next_, &QPushButton::clicked, this, [this]() { emit sgn_btn_next_clicked(); });
    connect(btn_prev_, &QPushButton::clicked, this, [this]() { emit sgn_btn_prev_clicked(); });
    connect(btn_mute_, &QPushButton::clicked, this, [this]() { emit sgn_btn_mute_clicked(); });
    connect(btn_mode_, &QPushButton::clicked, this, [this]() {
        QPoint pos = btn_mode_->mapToGlobal(QPoint(0, btn_mode_->height()));
        menu_mode_->exec(pos);
    });

    connect(act_in_order_, &QAction::triggered, this, [this]() { emit sgn_in_order(); });
    connect(act_loop_, &QAction::triggered, this, [this]() { emit sgn_loop(); });
    connect(act_shuffle_, &QAction::triggered, this, [this]() { emit sgn_shuffle(); });
    connect(act_out_of_order_track_, &QAction::triggered, this,
            [this]() { emit sgn_out_of_order_by_track(); });
    connect(act_out_of_order_group_, &QAction::triggered, this,
            [this]() { emit sgn_out_of_order_by_group(); });

    connect(slider_volume_, &QSlider::valueChanged, this, [this](int value) {
        emit sgn_slider_volume_released(value);
        this->update_volume_slider_icon(value);
    });
    connect(slider_position_, &QSlider::sliderReleased, this,
            [this]() { emit sgn_slider_position_released(slider_position_->value()); });
    connect(slider_position_, &QSlider::sliderMoved, this,
            [this](int value) { time_progress_->set_current_time(value); });
    connect(btn_devices_, &QPushButton::clicked, this, [this]() {
        QPoint pos = btn_devices_->mapToGlobal(QPoint(0, btn_devices_->height()));
        menu_devices_->exec(pos);
    });

    // 图标模式切换后立即刷新所有图标
    connect(&ThemeManager::instance(), &ThemeManager::sgn_theme_changed, this,
            &ControlBar::refresh_all_icons);
}

void ControlBar::refresh_all_icons()
{
    btn_stop_->setIcon(QIcon(icon_path("stop")));
    btn_prev_->setIcon(QIcon(icon_path("prev")));
    btn_next_->setIcon(QIcon(icon_path("next")));
    btn_devices_->setIcon(QIcon(icon_path("device")));
    btn_play_pause_->setIcon(QIcon(icon_path(is_playing_ ? "pause" : "play")));
    btn_mode_->setIcon(QIcon(icon_path(current_mode_icon_)));
    if (is_muted_) {
        btn_mute_->setIcon(QIcon(icon_path("volume_x")));
    } else {
        update_volume_slider_icon(current_volume_pct_);
    }
}

void ControlBar::update_button_status(PlayerEngine::PlayingState new_state)
{
    // set icon here
    logger->debug("update new state: {}", magic_enum::enum_name(new_state));
    if (new_state != PlayerEngine::PlayingState::Playing) {
        is_playing_ = false;
        btn_play_pause_->setIcon(QIcon(icon_path("play")));
    } else {
        is_playing_ = true;
        btn_play_pause_->setIcon(QIcon(icon_path("pause")));
    }
}

void ControlBar::update_duration(qint64 duration_ms)
{
    qint64 duration_s = duration_ms / 1000;
    slider_position_->setRange(0, static_cast<int>(duration_s));
    time_progress_->set_total_time(duration_s);
}

void ControlBar::update_position(qint64 position_ms)
{
    if (!slider_position_->isSliderDown()) {
        slider_position_->setValue(static_cast<int>(position_ms) / 1000);
        time_progress_->set_current_time(position_ms / 1000);
    }
}

void ControlBar::update_volume_slider(int percent)
{
    if (!slider_volume_->isSliderDown()) {
        slider_volume_->setValue(percent);
    }
}

void ControlBar::update_mute_button(bool muted)
{
    is_muted_ = muted;
    if (muted) {
        btn_mute_->setIcon(QIcon(icon_path("volume_x")));
    } else {
        this->update_volume_slider_icon(slider_volume_->value());
    }
}

void ControlBar::set_play_mode(PlayMode mode)
{
    for (QAction* action : {
             act_in_order_,
             act_loop_,
             act_shuffle_,
             act_out_of_order_track_,
             act_out_of_order_group_,
         }) {
        action->setCheckable(true);
        action->setIconText("");
    }
    if (mode == PlayMode::InOrder) {
        act_in_order_->setChecked(true);
        current_mode_icon_ = QStringLiteral("in_order");
        this->update_mode_icon(icon_path("in_order"));
    } else if (mode == PlayMode::Loop) {
        act_loop_->setChecked(true);
        current_mode_icon_ = QStringLiteral("loop");
        this->update_mode_icon(icon_path("loop"));
    } else if (mode == PlayMode::Shuffle) {
        act_shuffle_->setChecked(true);
        current_mode_icon_ = QStringLiteral("shuffle");
        this->update_mode_icon(icon_path("shuffle"));
    } else if (mode == PlayMode::OutOfOrderByGroup) {
        act_out_of_order_group_->setChecked(true);
        current_mode_icon_ = QStringLiteral("out_of_order_group");
        this->update_mode_icon(icon_path("out_of_order_group"));
    } else if (mode == PlayMode::OutOfOrderByTrack) {
        act_out_of_order_track_->setChecked(true);
        current_mode_icon_ = QStringLiteral("out_of_order_track");
        this->update_mode_icon(icon_path("out_of_order_track"));
    }
}

void ControlBar::set_device(const QVector<AudioDeviceInfo>& devices, const QByteArray& current_id)
{
    devices_ = devices;

    menu_devices_->clear();

    // “跟随系统设备”选项(勾选 = 跟随系统默认; 取消 = 固定所选设备)
    act_follow_system_ = menu_devices_->addAction(tr("Follow System Device"));
    act_follow_system_->setCheckable(true);
    act_follow_system_->setChecked(follow_system_);
    connect(act_follow_system_, &QAction::toggled, this, &ControlBar::sgn_follow_system_toggled);
    menu_devices_->addSeparator();

    auto* group = new QActionGroup(menu_devices_);
    group->setExclusive(true);
    for (const auto& dev : devices_) {
        QAction* act = menu_devices_->addAction(dev.description);
        act->setCheckable(true);
        act->setChecked(dev.id == current_id);
        act->setData(dev.id);
        group->addAction(act);

        connect(act, &QAction::triggered, this,
                [this, act]() { emit sgn_select_device_id(act->data().toByteArray()); });
    }
}

void ControlBar::set_follow_system(bool on)
{
    follow_system_ = on;
    if (act_follow_system_) {
        QSignalBlocker blocker(act_follow_system_);
        act_follow_system_->setChecked(on);
    }
}

QSlider* ControlBar::get_progress_slider() const
{
    return slider_position_;
};

QSlider* ControlBar::get_volume_slider() const
{
    return slider_volume_;
};

/* in player, set_mute == set_volume(0), so when update slider, the mute
   status will be covered, and there is NO NEED to save mute status */
void ControlBar::update_volume_slider_icon(int volume_by_percent)
{
    current_volume_pct_ = volume_by_percent;
    if (volume_by_percent > 66) {
        btn_mute_->setIcon(QIcon(icon_path("volume_3")));
    } else if (volume_by_percent > 33) {
        btn_mute_->setIcon(QIcon(icon_path("volume_2")));
    } else if (volume_by_percent > 0) {
        btn_mute_->setIcon(QIcon(icon_path("volume_1")));
    } else {
        btn_mute_->setIcon(QIcon(icon_path("volume_0")));
    }
}

void ControlBar::update_mode_icon(const QString& icon_url)
{
    QIcon icon(icon_url);
    btn_mode_->setIcon(icon);
}
