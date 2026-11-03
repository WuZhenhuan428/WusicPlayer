#include "controller/playback_controller.h"

#include <QJsonArray>
#include <QJsonObject>
#include <numbers>

PlaybackController::PlaybackController(Player* player, QObject* parent) :
    QObject(parent), player_(player), is_muted_(false)
{
    if (!player) {
        return;
    }
    // broadcast Player signals
    connect(player_, &Player::sgn_position_changed, this,
            [this](qint64 pos_ms) { emit sgn_position_changed(pos_ms); });
    connect(player_, &Player::sgn_duration_changed, this,
            [this](qint64 dur_ms) { emit sgn_duration_changed(dur_ms); });
    connect(player_, &Player::sgn_state_changed, this,
            [this](PlayingState state) { emit sgn_playback_state_changed(state); });
    connect(player_, &Player::sgn_playback_natural_end, this,
            [this]() { emit sgn_playback_natural_end(); });
    connect(player_, &Player::sgn_device_changed, this, [this](const AudioDeviceInfo& device) {
        emit sgn_devices_changed(this->available_devices(), device.id);
    });
    connect(player_, &Player::sgn_follow_system_changed, this,
            &PlaybackController::sgn_follow_system_changed);

    emit sgn_devices_changed(this->available_devices(), this->current_device_id());
}

bool PlaybackController::follow_system_device() const
{
    return follow_system_device_;
}

void PlaybackController::set_follow_system_device(bool on)
{
    follow_system_device_ = on;
    if (player_) {
        player_->set_follow_system_device(on);
    }
}

void PlaybackController::play()
{
    if (!player_) {
        return;
    }
    player_->play();
}

void PlaybackController::pause()
{
    if (!player_) {
        return;
    }
    player_->pause();
}

void PlaybackController::stop()
{
    if (!player_) {
        return;
    }
    player_->stop();
}

PlayingState PlaybackController::state()
{
    if (!player_) {
        return PlayingState::Stop;
    }
    return player_->state();
}

void PlaybackController::set_position(qint64 pos_ms)
{
    if (!player_) {
        return;
    }
    player_->seek(pos_ms);
}

qint64 PlaybackController::position()
{
    if (!player_) {
        return 0;
    }
    return player_->position();
}

void PlaybackController::set_volume(int percent)
{
    if (!player_) {
        return;
    }
    player_->set_volume(static_cast<float>(percent));
    emit sgn_volume_changed(percent);
}

void PlaybackController::set_eq_config(EqConfig cfg)
{
    if (!player_) {
        return;
    }
    eq_config_cache_ = std::make_shared<const EqConfig>(std::move(cfg));
    player_->set_eq_config(eq_config_cache_);
}

EqConfig PlaybackController::eq_config() const
{
    if (eq_config_cache_) {
        return *eq_config_cache_;
    }
    // 回退: 从播放链路取当前生效配置(如首次打开 EQ 窗口时)
    if (player_) {
        auto cur = player_->eq_config();
        if (cur) {
            return *cur;
        }
    }
    return EqConfig{};
}

QString PlaybackController::eq_plugin_id() const
{
    return eq_plugin_id_;
}

void PlaybackController::set_eq_plugin_id(const QString& id)
{
    eq_plugin_id_ = id;
}

void PlaybackController::read(const QString& filepath)
{
    if (!player_) {
        return;
    }
    player_->read(filepath);
}

void PlaybackController::set_mute(bool mute_on)
{
    if (player_) {
        player_->set_mute(mute_on);
        is_muted_ = mute_on;
        emit sgn_mute_changed(mute_on);
    }
}

bool PlaybackController::is_mute()
{
    if (player_) {
        return player_->is_muted();
    }
    return false;
}

void PlaybackController::flip_mute()
{
    this->set_mute(!is_muted_);
}

void PlaybackController::set_device(const AudioDeviceInfo& dev)
{
    if (!player_) {
        return;
    }
    player_->set_output_device(dev);
    emit sgn_devices_changed(this->available_devices(), this->current_device_id());
}

void PlaybackController::set_device_by_id(const QByteArray& id)
{
    if (!player_) {
        return;
    }
    player_->set_output_device_by_id(id);
    emit sgn_devices_changed(this->available_devices(), this->current_device_id());
}

QVector<AudioDeviceInfo> PlaybackController::available_devices()
{
    if (!player_) {
        return {};
    }
    return player_->devices();
}

QByteArray PlaybackController::current_device_id()
{
    if (!player_) {
        return {};
    }
    return player_->current_output_device().id;
}

void PlaybackController::load_from_json(const QJsonObject& json)
{
    QJsonObject obj      = json.value(this->config_sub_key()).toObject();
    double volume_double = obj.value("volume").toDouble();
    this->set_volume(static_cast<int>(volume_double * 100));
    this->set_mute(obj.value("muted").toBool());

    // 设备: 默认跟随系统; 仅当用户明确关闭跟随时恢复固定设备
    const bool follow = obj.value("follow_system_device").toBool(true);
    this->set_follow_system_device(follow);
    if (!follow) {
        this->set_device_by_id(
            QByteArray::fromBase64(obj.value("last_device").toString().toUtf8()));
    }

    last_position_ms_        = obj.value("last_position_ms").toInt();
    last_was_playing_        = obj.value("last_was_playing").toBool(false);

    // Restore EQ state (EqConfig 驱动)
    const QJsonObject eq_obj = obj.value("eq").toObject();
    eq_plugin_id_            = eq_obj.value("plugin_id").toString();
    if (player_ && eq_obj.contains("enabled")) {
        auto cfg               = std::make_shared<EqConfig>();
        cfg->enabled           = eq_obj.value("enabled").toBool(false);
        const QJsonArray bands = eq_obj.value("bands").toArray();
        for (const auto& b : bands) {
            const QJsonObject bo = b.toObject();
            EqBand band;
            band.type    = static_cast<EqFilterType>(bo.value("type").toInt(0));
            band.freq    = bo.value("freq").toDouble(1000.0);
            band.q       = bo.value("q").toDouble(std::numbers::sqrt2);
            band.gain_db = static_cast<float>(bo.value("gain_db").toDouble(0.0));
            cfg->bands.push_back(band);
        }
        eq_config_cache_ = cfg;
        player_->set_eq_config(cfg);
    }

    // TODO: consider about time sequence
    if (player_) {
        player_->seek(last_position_ms_);
    }
}

QJsonObject PlaybackController::save_to_json()
{
    QJsonObject obj;

    last_was_playing_ = this->state() == PlayingState::Playing;
    last_position_ms_ =
        this->state() != PlayingState::Stop ? static_cast<int>(player_->position()) : 0;

    obj["volume"]               = player_->volume();
    obj["muted"]                = is_muted_;
    obj["last_device"]          = QString::fromUtf8(this->current_device_id().toBase64());
    obj["follow_system_device"] = follow_system_device_;
    obj["last_was_playing"]     = last_was_playing_;
    obj["last_position_ms"]     = last_position_ms_;

    // EQ 配置(插件路径)
    QJsonObject eq_obj;
    eq_obj["plugin_id"] = eq_plugin_id_;
    if (eq_config_cache_) {
        eq_obj["enabled"] = eq_config_cache_->enabled;
        QJsonArray bands;
        for (const EqBand& band : eq_config_cache_->bands) {
            QJsonObject bo;
            bo["type"]    = static_cast<int>(band.type);
            bo["freq"]    = band.freq;
            bo["q"]       = band.q;
            bo["gain_db"] = band.gain_db;
            bands.append(bo);
        }
        eq_obj["bands"] = bands;
    } else {
        eq_obj["enabled"] = false;
        eq_obj["bands"]   = QJsonArray();
    }
    obj["eq"] = eq_obj;

    return obj;
}

QString PlaybackController::config_sub_key() const
{
    return "playback";
}

int PlaybackController::last_position_ms() const
{
    return last_position_ms_;
}

bool PlaybackController::last_was_playing() const
{
    return last_was_playing_;
}
