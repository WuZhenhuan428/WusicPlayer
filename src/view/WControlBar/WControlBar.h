#pragma once

#include <QWidget>

#include "core/types.h"

#include "wtimeprogress.h"
#include <QPushButton>
#include <QSlider>
#include <QString>
#include <QAction>
#include <QMenu>
#include <QHBoxLayout>
#include <QActionGroup>
#include <QAudioDevice>
#include <QList>

#include "core/player/player_engine.h"

class WControlBar : public QWidget
{
    Q_OBJECT
public:
    explicit WControlBar(QWidget* parent = nullptr);
    ~WControlBar();
    void setPlayMode(PlayMode mode);
    void setDevice(const QList<QAudioDevice>& devices, const QByteArray& current_id);
    QSlider* getProgressSlider() const;
    QSlider* getVolumeSlider() const;

public slots:
    void updateButtonStatus(PlayerEngine::PlayingState new_state);
    void updateDuration(qint64 duration_ms);
    void updatePosition(qint64 position_ms);
    void updateVolumeSlider(int percent);
    void updateMuteButton(bool muted);
    void refreshAllIcons();  // 图标模式切换后刷新所有图标

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
    void updateVolumeSliderIcon(int volume_by_percent);
    void updateModeIcon(QString icon_url);

private:
    QPushButton* m_btn_play_pause;
    QPushButton* m_btn_stop;
    QPushButton* m_btn_next;
    QPushButton* m_btn_prev;
    QPushButton* m_btn_mute;
    QPushButton* m_btn_mode;
    QPushButton* m_btn_devices;
    QMenu* m_menu_mode;
    QAction* m_act_in_order;    // 顺序播放
    QAction* m_act_loop;       // 循环播放
    QAction* m_act_shuffle;    // 随机播放 - 不停止
    QAction* m_act_out_of_order_track; // 乱序播放 - 有最后一首
    QAction* m_act_out_of_order_group;// 组间乱序 / 组内顺序
    QActionGroup* m_act_group;     // exclusive group -> show available icon

    /// Progress Bar: Position/Duration
    QSlider* m_slider_position;
    WTimeProgress* m_time_progress;
    QSlider* m_slider_volume;

    QHBoxLayout* m_hbl_main;

    QMenu* m_menu_devices;
    QList<QAudioDevice> m_devices;

    bool m_is_playing = false;
    QString m_current_mode_icon = QStringLiteral("in_order");  // 当前播放模式图标名
    int m_current_volume_pct = 100;   // 当前音量百分比，用于刷新音量图标
    bool m_is_muted = false;          // 当前静音状态
};
