#pragma once

#include "../../src/core/types.h"
#include "../../src/player/player.h"

#include <QObject>
#include <QMediaPlayer>
#include <QString>
#include <QByteArray>
#include <QList>
#include <QAudioDevice>

class PlaybackController : public QObject
{
    Q_OBJECT
public:
    explicit PlaybackController(Player* player, QObject* parent = nullptr);
    ~PlaybackController();

    void setPlayMode(PlayMode mode);
    PlayMode playMode();
    void flipMute();
    void play();
    void pause();
    void stop();
    void setPosition(qint64 pos_ms);
    qint64 position();
    void setVolume(int percent);
    void setMute(bool mute_on);
    bool getMute();
    void read(QString filepath);
    const QMediaPlayer* getMediaPlayer() const;
    void setDevice(QAudioDevice dev);
    void setDeviceById(QByteArray id);
    QList<QAudioDevice> availableDevices();
    QByteArray currentDeviceId(); 

signals:
    void sgnMediaStateChanged(QMediaPlayer::MediaStatus state);
    void sgnPositionChanged(qint64 pos_ms);
    void sgnDurationChanged(qint64 dur_ms);
    void sgnPlaybackStateChanged(QMediaPlayer::PlaybackState new_state);
    void sgnPlayModeChanged(PlayMode mode);
    void sgnDevicesChanged(QList<QAudioDevice> devs, QByteArray id);

private:
    Player* m_player;
    PlayMode m_playMode;
};