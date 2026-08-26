#include "controller/playback_controller.h"

#include <QJsonArray>
#include <QJsonObject>

PlaybackController::PlaybackController(Player* player, QObject* parent) :
    QObject(parent), m_player(player), m_is_muted(false)
{
    if (!player) {
        return;
    }
    // broadcast Player signals
    connect(m_player, &Player::sgn_position_changed, this,
            [this](qint64 pos_ms) { emit sgn_position_changed(pos_ms); });
    connect(m_player, &Player::sgn_duration_changed, this,
            [this](qint64 dur_ms) { emit sgn_duration_changed(dur_ms); });
    connect(m_player, &Player::sgn_state_changed, this,
            [this](PlayingState state) { emit sgn_playback_state_changed(state); });
    connect(m_player, &Player::sgn_playback_natural_end, this,
            [this]() { emit sgn_playback_natural_end(); });
    connect(m_player, &Player::sgn_device_changed, this, [this](AudioDeviceInfo device) {
        emit sgn_devices_changed(this->available_devices(), device.id);
    });

    emit sgn_devices_changed(this->available_devices(), this->current_device_id());
}

PlaybackController::~PlaybackController() {}

void PlaybackController::play()
{
    if (!m_player) {
        return;
    }
    m_player->play();
}

void PlaybackController::pause()
{
    if (!m_player) {
        return;
    }
    m_player->pause();
}

void PlaybackController::stop()
{
    if (!m_player) {
        return;
    }
    m_player->stop();
}

PlayingState PlaybackController::state()
{
    if (!m_player) {
        return PlayingState::STOP;
    }
    return m_player->state();
}

void PlaybackController::set_position(qint64 pos_ms)
{
    if (!m_player) {
        return;
    }
    m_player->seek(pos_ms);
}

qint64 PlaybackController::position()
{
    if (!m_player) {
        return 0;
    }
    return m_player->position();
}

void PlaybackController::set_volume(int percent)
{
    if (!m_player) {
        return;
    }
    m_player->set_volume(percent);
    emit sgn_volume_changed(percent);
}

void PlaybackController::set_eq_config(EqConfig cfg)
{
    if (!m_player) {
        return;
    }
    m_eq_config_cache = std::make_shared<const EqConfig>(std::move(cfg));
    m_player->set_eq_config(m_eq_config_cache);
}

EqConfig PlaybackController::eq_config() const
{
    if (m_eq_config_cache) {
        return *m_eq_config_cache;
    }
    // 回退: 从播放链路取当前生效配置(如首次打开 EQ 窗口时)
    if (m_player) {
        auto cur = m_player->eq_config();
        if (cur) {
            return *cur;
        }
    }
    return EqConfig{};
}

QString PlaybackController::eq_plugin_id() const
{
    return m_eq_plugin_id;
}

void PlaybackController::set_eq_plugin_id(const QString& id)
{
    m_eq_plugin_id = id;
}

void PlaybackController::read(QString filepath)
{
    if (!m_player) {
        return;
    }
    m_player->read(filepath);
}

void PlaybackController::set_mute(bool mute_on)
{
    if (m_player) {
        m_player->set_mute(mute_on);
        m_is_muted = mute_on;
        emit sgn_mute_changed(mute_on);
    }
}

bool PlaybackController::is_mute()
{
    if (m_player) {
        return m_player->is_muted();
    }
    return false;
}

void PlaybackController::flip_mute()
{
    this->set_mute(!m_is_muted);
}

void PlaybackController::set_device(const AudioDeviceInfo& dev)
{
    if (!m_player) {
        return;
    }
    m_player->set_output_device(dev);
    emit sgn_devices_changed(this->available_devices(), this->current_device_id());
}

void PlaybackController::set_device_by_id(QByteArray id)
{
    if (!m_player) {
        return;
    }
    m_player->set_output_device_by_id(id);
    emit sgn_devices_changed(this->available_devices(), this->current_device_id());
}

QVector<AudioDeviceInfo> PlaybackController::available_devices()
{
    if (!m_player) {
        return {};
    }
    return m_player->devices();
}

QByteArray PlaybackController::current_device_id()
{
    if (!m_player) {
        return {};
    }
    return m_player->current_output_device().id;
}

void PlaybackController::load_from_json(const QJsonObject& json)
{
    QJsonObject obj      = json.value(this->config_sub_key()).toObject();
    double volume_double = obj.value("volume").toDouble();
    this->set_volume(static_cast<int>(volume_double * 100));
    this->set_mute(obj.value("muted").toBool());
    this->set_device_by_id(QByteArray::fromBase64(obj.value("last_device").toString().toUtf8()));

    m_last_position_ms       = obj.value("last_position_ms").toInt();
    m_last_was_playing       = obj.value("last_was_playing").toBool(false);

    // Restore EQ state (EqConfig 驱动)
    const QJsonObject eq_obj = obj.value("eq").toObject();
    m_eq_plugin_id           = eq_obj.value("plugin_id").toString();
    if (m_player && eq_obj.contains("enabled")) {
        auto cfg               = std::make_shared<EqConfig>();
        cfg->enabled           = eq_obj.value("enabled").toBool(false);
        const QJsonArray bands = eq_obj.value("bands").toArray();
        for (const auto& b : bands) {
            const QJsonObject bo = b.toObject();
            EqBand band;
            band.type    = static_cast<EqFilterType>(bo.value("type").toInt(0));
            band.freq    = bo.value("freq").toDouble(1000.0);
            band.q       = bo.value("q").toDouble(1.414);
            band.gain_db = static_cast<float>(bo.value("gain_db").toDouble(0.0));
            cfg->bands.push_back(band);
        }
        m_eq_config_cache = cfg;
        m_player->set_eq_config(cfg);
    }

    // TODO: consider about time sequence
    if (m_player) {
        m_player->seek(m_last_position_ms);
    }
}

QJsonObject PlaybackController::save_to_json()
{
    QJsonObject obj;

    m_last_was_playing      = this->state() == PlayingState::PLAYING;
    m_last_position_ms      = this->state() != PlayingState::STOP ? m_player->position() : 0;

    obj["volume"]           = m_player->volume();
    obj["muted"]            = m_is_muted;
    obj["last_device"]      = QString::fromUtf8(this->current_device_id().toBase64());
    obj["last_was_playing"] = m_last_was_playing;
    obj["last_position_ms"] = m_last_position_ms;

    // EQ 配置(插件路径)
    QJsonObject eq_obj;
    eq_obj["plugin_id"] = m_eq_plugin_id;
    if (m_eq_config_cache) {
        eq_obj["enabled"] = m_eq_config_cache->enabled;
        QJsonArray bands;
        for (const EqBand& band : m_eq_config_cache->bands) {
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
    return m_last_position_ms;
}

bool PlaybackController::last_was_playing() const
{
    return m_last_was_playing;
}
