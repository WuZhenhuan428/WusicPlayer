#pragma once

#include <QWidget>

#include "wtimeprogress.h"
#include <QPushButton>
#include <QSlider>
#include <QString>
#include <QAction>
#include <QMenu>
#include <QHBoxLayout>
#include <QMediaPlayer>

class WControlBar : public QWidget
{
    Q_OBJECT
public:
    explicit WControlBar(QWidget* parent = nullptr);
    ~WControlBar();

public slots:
    void onPlayerStateChanged(QMediaPlayer::PlaybackState newState);
    void updateDuration(qint64 duration_ms);
    void updatePosition(qint64 position_ms);

signals:
    void sgnBtnPlayClicked();
    void sgnBtnPauseClicked();
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

private:
    QPushButton* btnPlay;
    QPushButton* btnPause;
    QPushButton* btnStop;
    QPushButton* btnNext;
    QPushButton* btnPrev;
    QPushButton* btnMute;
    QPushButton* btnMode;
    QMenu* menuMode;
    QAction* actInOrder;    // 顺序播放
    QAction* actLoop;       // 循环播放
    QAction* actShuffle;    // 随机播放 - 不停止
    QAction* actOutOfOrderTrack; // 乱序播放 - 有最后一首
    QAction* actOutOfOrderGroup;// 组间乱序 / 组内顺序

    /// Progress Bar: Position/Duration
    QSlider* sliderPostion;
    WTimeProgress* timeProgress;
    QSlider* sliderVolume;

    QHBoxLayout* hbMain;

private slots:
};
