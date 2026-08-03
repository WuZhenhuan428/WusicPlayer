#pragma once

#include "core/player_types.h"
#include "core/player/player_engine.h"

#include <QAudioDevice>
#include <QByteArray>
#include <QList>
#include <QMediaDevices>
#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>

using PlayingState = PlayerEngine::PlayingState;

class Player : public QObject
{
    Q_OBJECT
public:
    enum class PlayerActions
    {
        ERROR = -1, // err or ignore
        NATURE_END,
        MANUAL_STOP
    };

    explicit Player(QObject* parent = nullptr);
    ~Player();

    PlayingState state() const;

    void read(const QString& filepath);
    void play();
    void pause();
    void stop();
    bool is_muted();
    void seek(qint64 pos_ms);
    void set_mute(bool mute);
    void set_volume(float vol);
    float volume() const;
    void set_eq(gains_t gains);
    const gains_t gains() const;

    // return millisecond
    qint64 position() const;

    void set_output_device(const QAudioDevice& device);
    void set_output_device_by_id(const QByteArray& id);
    QList<QAudioDevice> devices() const;
    QAudioDevice current_output_device() const;

signals:
    void sgn_state_changed(PlayingState state);
    void sgn_position_changed(qint64 ms);
    void sgn_duration_changed(qint64 ms);
    void sgn_playback_natural_end();
    void sgn_device_changed(QAudioDevice device);

private:
    std::unique_ptr<PlayerEngine> m_player_engine = nullptr;
    QMediaDevices* m_media_devices                = nullptr;
    QList<QAudioDevice> m_audio_devices;
    QByteArray m_current_output_id;
    QByteArray m_preferred_output_id;
    QString m_loaded_track_path;
    QTimer* m_position_timer = nullptr;
    bool m_is_mute           = false;
    float m_old_volume       = 1.0f;

    void refresh_device_cache();
    double m_min_db;
    double map_slider_to_volume(double value, double min_db = -60.0);
};
