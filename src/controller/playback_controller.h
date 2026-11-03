#pragma once

#include "core/config_manager/i_configurable.h"
#include "core/player/player.h"
#include "plugin/eq_types.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QVector>
#include <memory>

class PlaybackController : public QObject, public IConfigurable
{
    Q_OBJECT
public:
    explicit PlaybackController(Player* player, QObject* parent = nullptr);

    void play();
    void pause();
    void stop();
    PlayingState state();
    void set_position(qint64 pos_ms);
    qint64 position();
    void set_volume(int percent);
    /// 插件路径: 应用任意 band 的 EQ 配置(enabled 取自 cfg.enabled)
    void set_eq_config(EqConfig cfg);
    [[nodiscard]] EqConfig eq_config() const;
    /// 当前选中的 EQ 插件 id(持久化)
    [[nodiscard]] QString eq_plugin_id() const;
    void set_eq_plugin_id(const QString& id);
    void set_mute(bool mute_on);
    bool is_mute();
    void flip_mute();
    void read(const QString& filepath);

    void set_device(const AudioDeviceInfo& dev);
    void set_device_by_id(const QByteArray& id);
    QVector<AudioDeviceInfo> available_devices();
    QByteArray current_device_id();
    /// 是否跟随系统默认播放设备(持久化)
    [[nodiscard]] bool follow_system_device() const;
    void set_follow_system_device(bool on);

    [[nodiscard]] int last_position_ms() const;
    [[nodiscard]] bool last_was_playing() const;

    // config S/L interface implement
    void load_from_json(const QJsonObject& json) override;
    QJsonObject save_to_json() override;
    [[nodiscard]] QString config_sub_key() const override;

signals:
    void sgn_position_changed(qint64 pos_ms);
    void sgn_duration_changed(qint64 dur_ms);
    void sgn_playback_state_changed(PlayingState state);
    void sgn_playback_natural_end();
    void sgn_devices_changed(QVector<AudioDeviceInfo> devs, QByteArray id);
    void sgn_follow_system_changed(bool on);
    void sgn_volume_changed(int percent);
    void sgn_mute_changed(bool muted);

private:
    Player* player_;

    bool is_muted_;
    int last_position_ms_      = 0;
    bool last_was_playing_     = false;
    bool follow_system_device_ = true; // 跟随系统默认设备(持久化)

    QString eq_plugin_id_; // 选中的 EQ 插件 id(持久化)
    std::shared_ptr<const EqConfig> eq_config_cache_;
};
