#pragma once

#include <QWidget>

#include "wtimeprogress.h"
#include <QPushButton>
#include <QSlider>
#include <QString>
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

private:
    QPushButton* btnPlay;
    QPushButton* btnPause;
    QPushButton* btnStop;
    QPushButton* btnNext;
    QPushButton* btnPrev;
    QPushButton* btnMute;

    /// Progress Bar: Position/Duration
    QSlider* sliderPostion;
    WTimeProgress* timeProgress;
    QSlider* sliderVolume;

    QHBoxLayout* hbMain;

private slots:
};
