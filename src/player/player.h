#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>
#include <QDebug>
#include <QtMultimedia/QtMultimedia>
#include <QMediaPlayer>
#include <QUrl>
#include <QMediaDevices>
#include <QMediaMetaData>
#include <QTimer>
#include <qmath.h>

// Class Declaration
class Player;
enum class State;


// Member Declaration
class Player : public QObject
{
    Q_OBJECT

public:
    // local class
    /// play state FSM
    enum class State
    {
        IDLE = 0,
        PLAYING,
        PAUSED,
        STOPPED
    };
    Q_ENUM(State);

    /// constructor & destructor
    explicit Player(QObject *parent = nullptr);
    ~Player();

    /// local member class
    QMediaPlayer* MediaPlayer;
    State state() const;

public slots:
    void read(const QString& filepath);
    void play();
    void pause();
    void stop();
    void setPosition(qint64 position);
    void flipMute();
    void setVolume(qint64 volume);

private:
    QAudioOutput* AudioOutput;
    double m_minDb;
    void initConnections();
    void setDevice();
    void openFile(const QString& filepath);
    double mapSliderToVolume(qint64 value, double minDb = -60.0);
    Player::State mapPlaybackState(QMediaPlayer::PlaybackState state);

private slots:
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);

signals:
    // Drive UI display behaviors through signal from backend
    void stateChanged(Player::State state);
    /// time info, slide bar
    void positionChanged(qint64 ms);
    void durationChanged(qint64 ms);
};

#endif // PLAYER_H
