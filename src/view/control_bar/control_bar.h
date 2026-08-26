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
    ~ControlBar();
    void set_play_mode(PlayMode mode);
    void set_device(const QVector<AudioDeviceInfo>& devices, const QByteArray& current_id);
    QSlider* get_progress_slider() const;
    QSlider* get_volume_slider() const;

public slots:
    void update_button_status(PlayerEngine::PlayingState new_state);
    void update_duration(qint64 duration_ms);
    void update_position(qint64 position_ms);
    void update_volume_slider(int percent);
    void update_mute_button(bool muted);
    void refresh_all_icons(); // 图标模式切换后刷新所有图标

signals:
    void sgnBtnPlayPauseClicked(bool is_request_play);
    void sgnBtnStopClicked();
    void sgnBtnNextClicked();
    void sgnBtnPrevClicked();
    void sgnBtnMuteClicked();
    void sgnSliderPositionReleased(int percent);
    void sgnSliderVolumeReleased(int percent);
    void sgnSliderVolumeMoved(int percent);
    void sgnInOrder();
    void sgnLoop();
    void sgnShuffle();
    void sgnOutOfOrderTrack();
    void sgnOutOfOrderGroup();
    void sgnSelectDeviceId(QByteArray id);

private:
    void update_volume_slider_icon(int volume_by_percent);
    void update_mode_icon(QString icon_url);

private:
    QPushButton* m_btn_play_pause;
    QPushButton* m_btn_stop;
    QPushButton* m_btn_next;
    QPushButton* m_btn_prev;
    QPushButton* m_btn_mute;
    QPushButton* m_btn_mode;
    QPushButton* m_btn_devices;
    QMenu* m_menu_mode;
    QAction* m_act_in_order;           // 顺序播放
    QAction* m_act_loop;               // 循环播放
    QAction* m_act_shuffle;            // 随机播放 - 不停止
    QAction* m_act_out_of_order_track; // 乱序播放 - 有最后一首
    QAction* m_act_out_of_order_group; // 组间乱序 / 组内顺序
    QActionGroup* m_act_group;         // exclusive group -> show available icon

    /// Progress Bar: Position/Duration
    QSlider* m_slider_position;
    TimeProgress* m_time_progress;
    QSlider* m_slider_volume;

    QHBoxLayout* m_hbl_main;

    QMenu* m_menu_devices;
    QVector<AudioDeviceInfo> m_devices;

    bool m_is_playing           = false;
    QString m_current_mode_icon = QStringLiteral("in_order"); // 当前播放模式图标名
    int m_current_volume_pct    = 100;                        // 当前音量百分比，用于刷新音量图标
    bool m_is_muted             = false;                      // 当前静音状态
};
