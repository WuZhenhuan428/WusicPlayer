#pragma once

#include "core/player/audio_device_info.h"
#include "core/player/player_engine.h"
#include "core/types.h"
#include "view/control_bar/time_progress.h"

#include <QAction>
#include <QActionGroup>
#include <QHBoxLayout>
#include <QMenu>
#include <QPushButton>
#include <QSlider>
#include <QString>
#include <QWidget>

class ControlBar : public QWidget
{
    Q_OBJECT
public:
    explicit ControlBar(QWidget* parent = nullptr);
    void set_play_mode(PlayMode mode);
    void set_device(const QVector<AudioDeviceInfo>& devices, const QByteArray& current_id);
    // 同步“跟随系统设备”状态(不触发信号, 供外部状态回灌)
    void set_follow_system(bool on);
    [[nodiscard]] QSlider* get_progress_slider() const;
    [[nodiscard]] QSlider* get_volume_slider() const;

public slots:
    void update_button_status(PlayerEngine::PlayingState new_state);
    void update_duration(qint64 duration_ms);
    void update_position(qint64 position_ms);
    void update_volume_slider(int percent);
    void update_mute_button(bool muted);
    void refresh_all_icons(); // 图标模式切换后刷新所有图标

signals:
    void sgn_btn_play_pause_clicked(bool is_request_play);
    void sgn_btn_stop_clicked();
    void sgn_btn_next_clicked();
    void sgn_btn_prev_clicked();
    void sgn_btn_mute_clicked();
    void sgn_slider_position_released(int percent);
    void sgn_slider_volume_released(int percent);
    void sgn_slider_volume_moved(int percent);
    void sgn_in_order();
    void sgn_loop();
    void sgn_shuffle();
    void sgn_out_of_order_by_track();
    void sgn_out_of_order_by_group();
    void sgn_select_device_id(QByteArray id);
    void sgn_follow_system_toggled(bool on);

private:
    void update_volume_slider_icon(int volume_by_percent);
    void update_mode_icon(const QString& icon_url);

private:
    QPushButton* btn_play_pause_;
    QPushButton* btn_stop_;
    QPushButton* btn_next_;
    QPushButton* btn_prev_;
    QPushButton* btn_mute_;
    QPushButton* btn_mode_;
    QPushButton* btn_devices_;
    QMenu* menu_mode_;
    QAction* act_in_order_;           // 顺序播放
    QAction* act_loop_;               // 循环播放
    QAction* act_shuffle_;            // 随机播放 - 不停止
    QAction* act_out_of_order_track_; // 乱序播放 - 有最后一首
    QAction* act_out_of_order_group_; // 组间乱序 / 组内顺序
    QActionGroup* act_group_;         // exclusive group -> show available icon

    /// Progress Bar: Position/Duration
    QSlider* slider_position_;
    TimeProgress* time_progress_;
    QSlider* slider_volume_;

    QHBoxLayout* hbl_main_;

    QMenu* menu_devices_;
    QAction* act_follow_system_ = nullptr; // 跟随系统设备
    bool follow_system_         = true;    // 当前跟随状态
    QVector<AudioDeviceInfo> devices_;

    bool is_playing_           = false;
    QString current_mode_icon_ = QStringLiteral("in_order"); // 当前播放模式图标名
    int current_volume_pct_    = 100;                        // 当前音量百分比，用于刷新音量图标
    bool is_muted_             = false;                      // 当前静音状态
};
