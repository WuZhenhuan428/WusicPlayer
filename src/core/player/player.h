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
        Error = -1, // err or ignore
        NaturalEnd,
        ManualStop,
    };

    explicit Player(QObject* parent = nullptr);

    [[nodiscard]] PlayingState state() const;

    void read(const QString& filepath);
    void play();
    void pause();
    void stop();
    [[nodiscard]] bool is_muted() const;
    void seek(qint64 pos_ms);
    void set_mute(bool mute);
    void set_volume(float vol);
    [[nodiscard]] float volume() const;
    /// 插件路径: 任意 band 的 EQ 配置
    void set_eq_config(std::shared_ptr<const EqConfig> cfg);
    [[nodiscard]] std::shared_ptr<const EqConfig> eq_config() const;

    // return millisecond
    [[nodiscard]] qint64 position() const;

    void set_output_device(const AudioDeviceInfo& device);
    void set_output_device_by_id(const QByteArray& id);
    [[nodiscard]] QVector<AudioDeviceInfo> devices() const;
    [[nodiscard]] AudioDeviceInfo current_output_device() const;

    /// 是否跟随系统默认播放设备
    [[nodiscard]] bool follow_system_device() const;
    void set_follow_system_device(bool on);

signals:
    void sgn_state_changed(PlayingState state);
    void sgn_position_changed(qint64 ms);
    void sgn_duration_changed(qint64 ms);
    void sgn_playback_natural_end();
    void sgn_device_changed(AudioDeviceInfo device);
    void sgn_follow_system_changed(bool on);

private:
    void refresh_device_cache();
    void poll_devices();           // 设备热插拔轮询(miniaudio 无变化信号)
    void handle_devices_changed(); // 设备列表变化后的跟随/静音/通知
    // 应用输出设备; manual=true 表示用户手动切换(退出跟随)
    void apply_output_device(const AudioDeviceInfo& device, bool manual);
    [[nodiscard]] const AudioDeviceInfo* find_default_device() const;

    std::unique_ptr<PlayerEngine> player_engine_ = nullptr;
    QVector<AudioDeviceInfo> audio_devices_;
    QByteArray current_output_id_;
    QByteArray preferred_output_id_; // 用户固定选择的设备(非跟随)
    QByteArray default_device_id_;   // 当前系统默认设备
    QString loaded_track_path_;
    QTimer* position_timer_    = nullptr;
    QTimer* device_poll_timer_ = nullptr;
    bool is_mute_              = false;
    bool follow_system_device_ = true;  // 默认跟随系统默认设备
    bool mute_due_device_loss_ = false; // 因设备丢失而静音(等待恢复)
    float old_volume_          = 1.0f;

    double min_db_;
    static double map_slider_to_volume(double value, double min_db = -60.0);
};
