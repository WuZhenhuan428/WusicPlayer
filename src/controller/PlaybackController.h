#pragma once

#include "core/types.h"
#include "core/player/player.h"
#include "core/player_types.h"

#include <QObject>
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
    void play();
    void pause();
    void stop();
    PlayingState state();
    void setPosition(qint64 pos_ms);
    qint64 position();
    void setVolume(int percent);
    void setGains(gains_t gains);
    void setMute(bool mute_on);
    bool getMute();
    void flipMute();
    void read(QString filepath);

    void setDevice(QAudioDevice dev);
    void setDeviceById(QByteArray id);
    QList<QAudioDevice> availableDevices();
    QByteArray currentDeviceId(); 

signals:
    void sgnPositionChanged(qint64 pos_ms);
    void sgnDurationChanged(qint64 dur_ms);
    void sgnPlaybackStateChanged(PlayingState state);
    void sgnPlaybackNatualEnd();
    void sgnPlayModeChanged(PlayMode mode);
    void sgnDevicesChanged(QList<QAudioDevice> devs, QByteArray id);

private:
    Player* m_player;
    PlayMode m_playMode;

    bool m_is_muted;
};