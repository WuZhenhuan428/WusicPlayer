#include "core/player/player.h"

#include <QMetaObject>
#include <algorithm>
#include <qmath.h>

#include "core/logger/logger_manager.h"
namespace
{
Logger* logger = LoggerManager::file_logger("player", {"console", "gui"});
}

Player::Player(QObject* parent) :
    QObject(parent), m_player_engine(std::make_unique<PlayerEngine>()), m_min_db(-50.0)
{
    if (!m_player_engine->start_device()) {
        return;
    }

    m_player_engine->set_playback_finished_callback([this](PlayerEngine::StopReason reason) {
        QMetaObject::invokeMethod(
            this,
            [this, reason]() {
                if (reason == PlayerEngine::StopReason::NATURAL_EOF) {
                    emit sgn_state_changed(m_player_engine->state());
                    emit sgn_playback_natural_end();
                } /*else ...  */
            },
            Qt::QueuedConnection);
    });
    m_player_engine->set_watchdog();

    refresh_device_cache();

    // 设备热插拔轮询(miniaudio 无变化信号, 每 2s 检测一次)
    m_device_poll_timer = new QTimer(this);
    m_device_poll_timer->setInterval(2000);
    connect(m_device_poll_timer, &QTimer::timeout, this, &Player::poll_devices);
    m_device_poll_timer->start();

    m_position_timer = new QTimer(this);
    m_position_timer->setInterval(100);
    connect(m_position_timer, &QTimer::timeout, this, [this]() {
        if (!m_player_engine) {
            return;
        }
        const PlayingState curr_state = m_player_engine->state();
        if (curr_state == PlayingState::PLAYING || curr_state == PlayingState::PAUSE) {
            emit sgn_position_changed(this->position());
        }
    });
    m_position_timer->start();
}
Player::~Player() {}

PlayingState Player::state() const
{
    if (!m_player_engine) {
        return PlayingState::STOP;
    }
    return const_cast<PlayerEngine*>(m_player_engine.get())->state();
}

void Player::read(const QString& filepath)
{
    if (!m_player_engine || filepath.isEmpty()) {
        return;
    }

    m_loaded_track_path = filepath;

    m_player_engine->set_url(filepath.toStdString());
    m_player_engine->resume();
    const auto meta  = m_player_engine->metadata();
    auto duration_it = meta.find("DURATION_MS");
    if (duration_it != meta.end()) {
        emit sgn_duration_changed(QString::fromStdString(duration_it->second).toLongLong());
    }
    emit sgn_state_changed(m_player_engine->state());
    emit sgn_position_changed(this->position());
}

void Player::play()
{
    if (!m_player_engine) {
        return;
    }

    if (m_player_engine->state() == PlayingState::STOP) {
        if (m_loaded_track_path.isEmpty()) {
            logger->info("play ignored: no loaded track while in STOP state.");
            emit sgn_state_changed(m_player_engine->state());
            emit sgn_position_changed(0);
            return;
        }

        m_player_engine->set_url(m_loaded_track_path.toStdString());
        const auto meta  = m_player_engine->metadata();
        auto duration_it = meta.find("DURATION_MS");
        if (duration_it != meta.end()) {
            emit sgn_duration_changed(QString::fromStdString(duration_it->second).toLongLong());
        }
    }

    m_player_engine->resume();
    emit sgn_state_changed(m_player_engine->state());
}

void Player::pause()
{
    if (!m_player_engine) {
        return;
    }
    m_player_engine->pause();
    emit sgn_state_changed(m_player_engine->state());
}

// sgn_state_changed & sgn_position_changed仅用于改变控制栏按键状态
void Player::stop()
{
    if (!m_player_engine) {
        return;
    }

    emit sgn_state_changed(PlayingState::STOP);
    emit sgn_position_changed(0);

    const PlayingState prev_state = m_player_engine->state();
    if (prev_state == PlayingState::STOP) {
        return;
    }
    m_player_engine->stop();
}

void Player::seek(qint64 pos_ms)
{
    if (!m_player_engine) {
        return;
    }
    m_player_engine->seek((int64_t)pos_ms);
    emit sgn_position_changed(this->position());
}

bool Player::is_muted()
{
    return m_is_mute;
}

void Player::set_mute(bool mute)
{
    if (!m_player_engine || m_is_mute == mute) {
        return;
    }

    if (m_is_mute) { // && mute = off (recover)
        m_player_engine->set_volume(m_old_volume);
    } else { // mute = on (mute)
        m_old_volume = m_player_engine->volume();
        m_player_engine->set_volume(0.0f);
    }

    m_is_mute = mute;
}

void Player::set_volume(float vol)
{
    if (!m_player_engine) {
        return;
    }

    const double normalized = std::clamp(static_cast<double>(vol) / 100.0, 0.0, 1.0);
    double audio_gain       = this->map_slider_to_volume(normalized, m_min_db);
    m_player_engine->set_volume((float)audio_gain);

    if (!m_is_mute) {
        m_old_volume = static_cast<float>(audio_gain);
    }
}

double Player::map_slider_to_volume(double value, double min_db)
{
    if (value <= 0.0) {
        return 0.0;
    }

    if (value >= 1.0) {
        return 1.0;
    }

    double db = min_db + (0.0 - min_db) * value;
    return qPow(10.0, db / 20.0);
}

qint64 Player::position() const
{
    if (!m_player_engine) {
        return 0;
    }

    if (m_player_engine->state() == PlayingState::STOP) {
        return 0;
    }

    return (qint64)(m_player_engine->position());
}

void Player::set_output_device(const AudioDeviceInfo& device)
{
    if (!m_player_engine || device.id.isEmpty()) {
        logger->warn("set_output_device ignored. m_player_engine/device invalid.");
        return;
    }

    logger->info("switching output device to {} id='{}'", device.description, device.id);

    const bool ok = m_player_engine->set_output_device_by_name(device.description.toStdString());
    if (!ok) {
        logger->warn("backend switch failed for '{}'", device.description);
        return;
    }

    m_preferred_output_id = device.id;
    refresh_device_cache();
    logger->info("output switch applied. active={} id='{}'", current_output_device().description,
                 current_output_device().id);
    emit sgn_device_changed(current_output_device());
}

void Player::set_output_device_by_id(const QByteArray& id)
{
    if (id.isEmpty()) {
        logger->warn("set_output_device_by_id ignored. empty id.");
        return;
    }

    logger->info("request switch by id='{}'", id);

    for (const auto& dev : m_audio_devices) {
        if (dev.id == id) {
            set_output_device(dev);
            return;
        }
    }

    logger->warn("no matching output device id found: {}", id);
}

QVector<AudioDeviceInfo> Player::devices() const
{
    return m_audio_devices;
}

AudioDeviceInfo Player::current_output_device() const
{
    for (const auto& dev : m_audio_devices) {
        if (dev.id == m_current_output_id) {
            return dev;
        }
    }

    if (!m_audio_devices.isEmpty()) {
        return m_audio_devices.first();
    }

    return {};
}

void Player::refresh_device_cache()
{
    if (!m_player_engine) {
        m_audio_devices.clear();
        m_current_output_id.clear();
        return;
    }

    // 枚举来自 miniaudio(miniaudio 的 ma_context_get_devices)
    const auto names = m_player_engine->output_devices();
    QVector<AudioDeviceInfo> devices;
    devices.reserve(static_cast<qsizetype>(names.size()));
    for (const auto& n : names) {
        AudioDeviceInfo info;
        info.id          = QByteArray(n.data(), static_cast<int>(n.size()));
        info.description = QString::fromUtf8(n.data(), static_cast<int>(n.size()));
        devices.push_back(std::move(info));
    }
    m_audio_devices = std::move(devices);

    if (m_audio_devices.isEmpty()) {
        m_current_output_id.clear();
        logger->warn("refresh_device_cache got empty list.");
        return;
    }

    const std::string active_name = m_player_engine->current_output_device_name();
    for (const auto& dev : m_audio_devices) {
        if (dev.description.toStdString() == active_name) {
            m_current_output_id = dev.id;
            logger->info("active backend device mapped: {}", dev.description);
            return;
        }
    }

    if (!m_preferred_output_id.isEmpty()) {
        for (const auto& dev : m_audio_devices) {
            if (dev.id == m_preferred_output_id) {
                m_current_output_id = dev.id;
                logger->info("use preferred device: {}", dev.description);
                return;
            }
        }
    }

    m_current_output_id = m_audio_devices.first().id;
    logger->info("using first available output device: {}", m_audio_devices.first().description);
}

void Player::poll_devices()
{
    const QVector<AudioDeviceInfo> previous = m_audio_devices;
    this->refresh_device_cache();

    bool changed = (previous.size() != m_audio_devices.size());
    if (!changed) {
        for (qsizetype i = 0; i < m_audio_devices.size(); ++i) {
            if (previous[i].id != m_audio_devices[i].id) {
                changed = true;
                break;
            }
        }
    }
    if (!changed) {
        return;
    }
    this->handle_devices_changed();
}

void Player::handle_devices_changed()
{
    const QByteArray old_id = m_current_output_id;
    logger->info("output devices changed. old_id='{}'", old_id);
    logger->info("refreshed outputs count={} current_id='{}'", m_audio_devices.size(),
                 m_current_output_id);

    if (m_audio_devices.isEmpty()) {
        m_current_output_id.clear();
        logger->warn("no available output devices after hot-plug.");
        emit sgn_device_changed(AudioDeviceInfo{});
        return;
    }

    bool old_still_exists = false;
    for (const auto& dev : m_audio_devices) {
        if (dev.id == old_id) {
            old_still_exists = true;
            break;
        }
    }

    if (!old_still_exists) {
        logger->info("previous output removed. trying fallback strategy.");
        bool preferred_exists = false;
        for (const auto& dev : m_audio_devices) {
            if (!m_preferred_output_id.isEmpty() && dev.id == m_preferred_output_id) {
                preferred_exists = true;
                logger->info("restoring preferred device: {}", dev.description);
                set_output_device(dev);
                break;
            }
        }

        if (!preferred_exists) {
            logger->info("preferred device unavailable. fallback to: {}",
                         m_audio_devices.first().description);
            set_output_device(m_audio_devices.first());
        }
        return;
    }

    logger->info("output device still valid: {}", current_output_device().description);
    emit sgn_device_changed(current_output_device());
}

void Player::set_eq_config(std::shared_ptr<const EqConfig> cfg)
{
    m_player_engine->set_eq_config(std::move(cfg));
}

std::shared_ptr<const EqConfig> Player::eq_config() const
{
    return m_player_engine->eq_config();
}

float Player::volume() const
{
    return m_player_engine->volume();
}
