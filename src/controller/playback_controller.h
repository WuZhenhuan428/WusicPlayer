#pragma once

#include "core/config_manager/i_configurable.h"
#include "core/player/player.h"
#include "plugin/eq_types.h"

#include <QAudioDevice>
#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>
#include <memory>

class PlaybackController : public QObject, public IConfigurable
{
    Q_OBJECT
public:
    explicit PlaybackController(Player* player, QObject* parent = nullptr);
    ~PlaybackController();

    void play();
    void pause();
    void stop();
    PlayingState state();
    void set_position(qint64 pos_ms);
    qint64 position();
    void set_volume(int percent);
    /// 插件路径: 应用任意 band 的 EQ 配置(enabled 取自 cfg.enabled)
    void set_eq_config(EqConfig cfg);
    EqConfig eq_config() const;
    /// 当前选中的 EQ 插件 id(持久化)
    QString eq_plugin_id() const;
    void set_eq_plugin_id(const QString& id);
    void set_mute(bool mute_on);
    bool is_mute();
    void flip_mute();
    void read(QString filepath);

    void set_device(QAudioDevice dev);
    void set_device_by_id(QByteArray id);
    QList<QAudioDevice> available_devices();
    QByteArray current_device_id();

    int last_position_ms() const;
    bool last_was_playing() const;

    // config S/L interface implement
    void load_from_json(const QJsonObject& json) override;
    QJsonObject save_to_json() override;
    QString config_sub_key() const override;

signals:
    void sgn_position_changed(qint64 pos_ms);
    void sgn_duration_changed(qint64 dur_ms);
    void sgn_playback_state_changed(PlayingState state);
    void sgn_playback_natural_end();
    void sgn_devices_changed(QList<QAudioDevice> devs, QByteArray id);
    void sgn_volume_changed(int percent);
    void sgn_mute_changed(bool muted);

private:
    Player* m_player;

    bool m_is_muted;
    int m_last_position_ms  = 0;
    bool m_last_was_playing = false;

    QString m_eq_plugin_id; // 选中的 EQ 插件 id(持久化)
    std::shared_ptr<const EqConfig> m_eq_config_cache;
};
