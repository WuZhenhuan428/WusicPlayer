#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>
#include <QDebug>
#include <QtMultimedia>
#include <QMediaPlayer>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QByteArray>

#include <qmath.h>

// Class Declaration
class Player;
enum class State;


// Member Declaration
class Player : public QObject
{
    Q_OBJECT
public:
    enum class State    // play state FSM
    {
        IDLE = 0,
        PLAYING,
        PAUSED,
        STOPPED
    };

    explicit Player(QObject *parent = nullptr);
    ~Player();

public:
    QMediaPlayer* getMediaPlayer() const;
    State state() const;
    QAudioDevice currentOutputDevice() const;
    QList<QAudioDevice> devices() const;

    void read(const QString& filepath);
    void play();
    void pause();
    void stop();
    void setPosition(qint64 position);
    void flipMute();
    void setVolume(qint64 volume);

    void setOutputDevice(const QAudioDevice& device);   // reserved interface
    void setOutputDeviceById(const QByteArray& id);

signals:
    void stateChanged(Player::State state);
    void positionChanged(qint64 ms);
    void durationChanged(qint64 ms);
    void deviceChanged(QAudioDevice device);

private:
    QAudioOutput* m_audio_output;
    QMediaDevices* m_media_devices;
    QByteArray m_preffered_output_id;
    QMediaPlayer* m_media_player;
    
    double m_min_db;
    void setDevice();
    void openFile(const QString& filepath);
    double mapSliderToVolume(qint64 value, double min_db = -60.0);
    Player::State mapPlaybackState(QMediaPlayer::PlaybackState state);

    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onAudioOutputchanged();
};

#endif // PLAYER_H
