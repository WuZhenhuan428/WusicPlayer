#include "controller/playback_controller.h"

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
    connect(m_player, &Player::sgn_device_changed, this, [this](QAudioDevice device) {
        emit sgn_devices_changed(this->available_devices(), device.id());
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

void PlaybackController::set_gains(gains_t gains)
{
    if (!m_player) {
        return;
    }
    m_gains_cache = gains;
    m_player->set_eq(gains);
}

void PlaybackController::set_eq_enabled(bool enabled)
{
    m_eq_enabled = enabled;
    if (enabled) {
        m_player->set_eq(m_gains_cache);
    } else {
        m_player->set_eq(gains_t{});
    }
}

void PlaybackController::set_eq_config(EqConfig cfg)
{
    if (!m_player) {
        return;
    }
    m_eq_enabled      = cfg.enabled;
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

bool PlaybackController::is_eq_enabled() const
{
    return m_eq_enabled;
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

void PlaybackController::set_device(QAudioDevice dev)
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

QList<QAudioDevice> PlaybackController::available_devices()
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
    return m_player->current_output_device().id();
}

void PlaybackController::load_from_json(const QJsonObject& json)
{
    QJsonObject obj      = json.value(this->config_sub_key()).toObject();
    double volume_double = obj.value("volume").toDouble();
    this->set_volume(static_cast<int>(volume_double * 100));
    this->set_mute(obj.value("muted").toBool());
    this->set_device_by_id(QByteArray::fromBase64(obj.value("last_device").toString().toUtf8()));

    m_last_position_ms   = obj.value("last_position_ms").toInt();
    m_last_was_playing   = obj.value("last_was_playing").toBool(false);

    // Restore EQ state
    m_eq_enabled         = obj.value("eq_enabled").toBool(false);
    QJsonObject eq_gains = obj.value("eq_gains").toObject();
    if (!eq_gains.isEmpty()) {
        m_gains_cache._31  = static_cast<float>(eq_gains.value("_31").toDouble());
        m_gains_cache._63  = static_cast<float>(eq_gains.value("_63").toDouble());
        m_gains_cache._125 = static_cast<float>(eq_gains.value("_125").toDouble());
        m_gains_cache._250 = static_cast<float>(eq_gains.value("_250").toDouble());
        m_gains_cache._500 = static_cast<float>(eq_gains.value("_500").toDouble());
        m_gains_cache._1k  = static_cast<float>(eq_gains.value("_1k").toDouble());
        m_gains_cache._2k  = static_cast<float>(eq_gains.value("_2k").toDouble());
        m_gains_cache._4k  = static_cast<float>(eq_gains.value("_4k").toDouble());
        m_gains_cache._8k  = static_cast<float>(eq_gains.value("_8k").toDouble());
        m_gains_cache._16k = static_cast<float>(eq_gains.value("_16k").toDouble());
    }
    if (m_eq_enabled && m_player) {
        m_player->set_eq(m_gains_cache);
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

    obj["eq_enabled"]       = m_eq_enabled;
    QJsonObject eq_gains;
    eq_gains["_31"]  = m_gains_cache._31;
    eq_gains["_63"]  = m_gains_cache._63;
    eq_gains["_125"] = m_gains_cache._125;
    eq_gains["_250"] = m_gains_cache._250;
    eq_gains["_500"] = m_gains_cache._500;
    eq_gains["_1k"]  = m_gains_cache._1k;
    eq_gains["_2k"]  = m_gains_cache._2k;
    eq_gains["_4k"]  = m_gains_cache._4k;
    eq_gains["_8k"]  = m_gains_cache._8k;
    eq_gains["_16k"] = m_gains_cache._16k;
    obj["eq_gains"]  = eq_gains;

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

const gains_t PlaybackController::gains() const
{
    return m_player->gains();
}
