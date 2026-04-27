#pragma once

#include "player_engine.h"

#include <QObject>
#include <QTimer>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QByteArray>
#include <QList>
#include <QString>
#include <memory>

using PlayingState = PlayerEngine::PlayingState;

class Player : public QObject
{
    Q_OBJECT
public:
    enum class PlayerActions {
        ERROR = -1, // err or ignore
        NATURE_END,
        MANUAL_STOP
    };

    explicit Player(QObject *parent = nullptr);
    ~Player();

    PlayingState state() const;

    void read(const QString& filepath);
    void play();
    void pause();
    void stop();
    bool muted();
    void seek(qint64 pos_ms);
    void setMute(bool mute);
    void setVolume(float vol);
    qint64 position();
    void setOutputDevice(const QAudioDevice& device);
    void setOutputDeviceById(const QByteArray& id);
    QList<QAudioDevice> devices() const;
    QAudioDevice currentOutputDevice() const;

signals:
    void stateChanged(PlayingState state);
    void positionChanged(qint64 ms);
    void durationChanged(qint64 ms);
    void sgnPlaybackNatualEnd();
    void deviceChanged(QAudioDevice device);
private:
    std::unique_ptr<PlayerEngine> core = nullptr;
    QMediaDevices* m_media_devices = nullptr;
    QList<QAudioDevice> m_audio_devices;
    QByteArray m_current_output_id;
    QByteArray m_preferred_output_id;
    QString m_loaded_track_path;
    QTimer* m_position_timer = nullptr;
    bool m_is_mute = false;
    float m_old_volume = 1.0f;

    void refreshDeviceCache();
    double m_min_db;
    double mapSliderToVolume(double value, double min_db = -60.0);
};
