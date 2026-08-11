#include "core/player/player.h"

#include "core/logger/log.h"
#include <QMetaObject>
#include <algorithm>
#include <qmath.h>

WUSIC_LOG_MODULE(player)

Player::Player(QObject* parent) :
    QObject(parent), m_player_engine(std::make_unique<PlayerEngine>()),
    m_media_devices(new QMediaDevices(this)), m_min_db(-50.0)
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
    connect(m_media_devices, &QMediaDevices::audioOutputsChanged, this, [this]() {
        const QByteArray old_id = m_current_output_id;
        WUSIC_LOG(player, info, "[AUDIO] audioOutputsChanged triggered. old_id={}",
                QString::fromUtf8(old_id));
        refresh_device_cache();
        WUSIC_LOG(player, info, "[AUDIO] refreshed outputs count={} current_id={}",
                m_audio_devices.size(), QString::fromUtf8(m_current_output_id));

        if (m_audio_devices.isEmpty()) {
            m_current_output_id.clear();
            WUSIC_LOG(player, warn, "[AUDIO] no available output devices after hot-plug.");
            emit sgn_device_changed(QAudioDevice());
            return;
        }

        bool old_still_exists = false;
        for (const auto& dev : m_audio_devices) {
            if (dev.id() == old_id) {
                old_still_exists = true;
                break;
            }
        }

        if (!old_still_exists) {
            WUSIC_LOG(player, info, "[AUDIO] previous output removed. trying fallback strategy.");
            bool preferred_exists = false;
            for (const auto& dev : m_audio_devices) {
                if (!m_preferred_output_id.isEmpty() && dev.id() == m_preferred_output_id) {
                    preferred_exists = true;
                    WUSIC_LOG(player, info, "[AUDIO] restoring preferred device: {}", dev.description());
                    set_output_device(dev);
                    break;
                }
            }

            if (!preferred_exists) {
                WUSIC_LOG(player, info, "[AUDIO] preferred device unavailable. fallback to: {}",
                        m_audio_devices.first().description());
                set_output_device(m_audio_devices.first());
            }
            return;
        }

        WUSIC_LOG(player, info, "[AUDIO] output device still valid: {}",
                current_output_device().description());
        emit sgn_device_changed(current_output_device());
    });

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
            WUSIC_LOG(player, info, "[AUDIO] play ignored: no loaded track while in STOP state.");
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

void Player::set_output_device(const QAudioDevice& device)
{
    if (!m_player_engine || device.isNull()) {
        WUSIC_LOG(player, warn, "[AUDIO] set_output_device ignored. m_player_engine/device invalid.");
        return;
    }

    WUSIC_LOG(player, info, "[AUDIO] switching output device to {} id={}",
            device.description(), QString::fromUtf8(device.id()));

    const bool ok = m_player_engine->set_output_device_by_name(device.description().toStdString());
    if (!ok) {
        WUSIC_LOG(player, warn, "[AUDIO] backend switch failed for {}", device.description());
        return;
    }

    m_preferred_output_id = device.id();
    refresh_device_cache();
    WUSIC_LOG(player, info, "[AUDIO] output switch applied. active={} id={}",
            current_output_device().description(),
            QString::fromUtf8(current_output_device().id()));
    emit sgn_device_changed(current_output_device());
}

void Player::set_output_device_by_id(const QByteArray& id)
{
    if (id.isEmpty()) {
        WUSIC_LOG(player, warn, "[AUDIO] set_output_device_by_id ignored. empty id.");
        return;
    }

    WUSIC_LOG(player, info, "[AUDIO] request switch by id={}", QString::fromUtf8(id));

    for (const auto& dev : m_audio_devices) {
        if (dev.id() == id) {
            set_output_device(dev);
            return;
        }
    }

    WUSIC_LOG(player, warn, "[AUDIO] no matching output device id found: {}",
                QString::fromUtf8(id));
}

QList<QAudioDevice> Player::devices() const
{
    return m_audio_devices;
}

QAudioDevice Player::current_output_device() const
{
    for (const auto& dev : m_audio_devices) {
        if (dev.id() == m_current_output_id) {
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
    m_audio_devices = QMediaDevices::audioOutputs();
    if (m_audio_devices.isEmpty() || !m_player_engine) {
        WUSIC_LOG(player, warn, "[AUDIO] refresh_device_cache got empty list or null m_player_engine.");
        m_current_output_id.clear();
        return;
    }

    const std::string active_name = m_player_engine->current_output_device_name();
    for (const auto& dev : m_audio_devices) {
        if (dev.description().toStdString() == active_name) {
            m_current_output_id = dev.id();
            WUSIC_LOG(player, info, "[AUDIO] active backend device mapped to Qt device: {}",
                dev.description());
            return;
        }
    }

    if (!m_preferred_output_id.isEmpty()) {
        for (const auto& dev : m_audio_devices) {
            if (dev.id() == m_preferred_output_id) {
                m_current_output_id = dev.id();
                WUSIC_LOG(player, info, "[AUDIO] backend device not in Qt list. use preferred Qt device: {}",
                        dev.description());
                return;
            }
        }
    }

    m_current_output_id = m_audio_devices.first().id();
    WUSIC_LOG(player, info, "[AUDIO] using first available Qt output device: {}",
            m_audio_devices.first().description());
}

void Player::set_eq(gains_t gains)
{
    m_player_engine->set_eq(gains);
}

float Player::volume() const
{
    return m_player_engine->volume();
}

const gains_t Player::gains() const
{
    return m_player_engine->gains();
}
