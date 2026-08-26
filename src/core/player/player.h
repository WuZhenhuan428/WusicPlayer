#pragma once

#include "core/player/audio_device_info.h"
#include "core/player/player_engine.h"

#include <QByteArray>
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
    /// 插件路径: 任意 band 的 EQ 配置
    void set_eq_config(std::shared_ptr<const EqConfig> cfg);
    std::shared_ptr<const EqConfig> eq_config() const;

    // return millisecond
    qint64 position() const;

    void set_output_device(const AudioDeviceInfo& device);
    void set_output_device_by_id(const QByteArray& id);
    QVector<AudioDeviceInfo> devices() const;
    AudioDeviceInfo current_output_device() const;

signals:
    void sgn_state_changed(PlayingState state);
    void sgn_position_changed(qint64 ms);
    void sgn_duration_changed(qint64 ms);
    void sgn_playback_natural_end();
    void sgn_device_changed(AudioDeviceInfo device);

private:
    void refresh_device_cache();
    void poll_devices();           // 设备热插拔轮询(miniaudio 无变化信号)
    void handle_devices_changed(); // 设备列表变化后的 fallback/通知

    std::unique_ptr<PlayerEngine> m_player_engine = nullptr;
    QVector<AudioDeviceInfo> m_audio_devices;
    QByteArray m_current_output_id;
    QByteArray m_preferred_output_id;
    QString m_loaded_track_path;
    QTimer* m_position_timer    = nullptr;
    QTimer* m_device_poll_timer = nullptr;
    bool m_is_mute              = false;
    float m_old_volume          = 1.0f;

    double m_min_db;
    double map_slider_to_volume(double value, double min_db = -60.0);
};
