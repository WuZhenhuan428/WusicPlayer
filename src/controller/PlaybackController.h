#pragma once

#include "../../src/core/types.h"
#include "../../src/player/player.h"

#include <QObject>
#include <QMediaPlayer>
#include <QString>

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

signals:
    void sgnMediaStateChanged(QMediaPlayer::MediaStatus state);
    void sgnPositionChanged(qint64 pos_ms);
    void sgnDurationChanged(qint64 dur_ms);
    void sgnPlaybackStateChanged(QMediaPlayer::PlaybackState new_state);
    void sgnPlayModeChanged(PlayMode mode);

private:
    Player* m_player;
    PlayMode m_playMode;
};