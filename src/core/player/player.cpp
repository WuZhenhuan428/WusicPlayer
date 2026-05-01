#include "player.h"

#include <qmath.h>
#include <QMetaObject>
#include <QDebug>
#include <algorithm>

Player::Player(QObject *parent)
    : QObject(parent),
      m_player_engine(std::make_unique<PlayerEngine>()),
            m_media_devices(new QMediaDevices(this)),
      m_min_db(-50.0)
{
    if (!m_player_engine->startDevice()) {
        return;
    }

    m_player_engine->setPlaybackFinishedCallback([this](PlayerEngine::StopReason reason) {
        QMetaObject::invokeMethod(this, [this, reason]() {
            if (reason == PlayerEngine::StopReason::NATURAL_EOF) {
                emit stateChanged(m_player_engine->state());
                emit sgnPlaybackNatualEnd();
            } /*else ...  */
        }, Qt::QueuedConnection);
    });
    m_player_engine->setWatcdog();

    refreshDeviceCache();
    connect(m_media_devices, &QMediaDevices::audioOutputsChanged, this, [this]() {
        const QByteArray old_id = m_current_output_id;
        qInfo() << "[AUDIO] audioOutputsChanged triggered. old_id=" << old_id;
        refreshDeviceCache();
        qInfo() << "[AUDIO] refreshed outputs count=" << m_audio_devices.size()
                << "current_id=" << m_current_output_id;

        if (m_audio_devices.isEmpty()) {
            m_current_output_id.clear();
            qWarning() << "[AUDIO] no available output devices after hot-plug.";
            emit deviceChanged(QAudioDevice());
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
            qInfo() << "[AUDIO] previous output removed. trying fallback strategy.";
            bool preferred_exists = false;
            for (const auto& dev : m_audio_devices) {
                if (!m_preferred_output_id.isEmpty() && dev.id() == m_preferred_output_id) {
                    preferred_exists = true;
                    qInfo() << "[AUDIO] restoring preferred device:" << dev.description();
                    setOutputDevice(dev);
                    break;
                }
            }

            if (!preferred_exists) {
                qInfo() << "[AUDIO] preferred device unavailable. fallback to:" << m_audio_devices.first().description();
                setOutputDevice(m_audio_devices.first());
            }
            return;
        }

        qInfo() << "[AUDIO] output device still valid:" << currentOutputDevice().description();
        emit deviceChanged(currentOutputDevice());
    });

    m_position_timer = new QTimer(this);
    m_position_timer->setInterval(100);
    connect(m_position_timer, &QTimer::timeout, this, [this]() {
        if (!m_player_engine) {
            return;
        }
        const PlayingState curr_state = m_player_engine->state();
        if (curr_state == PlayingState::PLAYING || curr_state == PlayingState::PAUSE) {
            emit positionChanged(this->position());
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

    m_player_engine->setUrl(filepath.toStdString());
    m_player_engine->resume();
    const auto meta = m_player_engine->metadata();
    auto duration_it = meta.find("DURATION_MS");
    if (duration_it != meta.end()) {
        emit durationChanged(QString::fromStdString(duration_it->second).toLongLong());
    }
    emit stateChanged(m_player_engine->state());
    emit positionChanged(this->position());
}

void Player::play()
{
    if (!m_player_engine) {
        return;
    }

    if (m_player_engine->state() == PlayingState::STOP) {
        if (m_loaded_track_path.isEmpty()) {
            qInfo() << "[AUDIO] play ignored: no loaded track while in STOP state.";
            emit stateChanged(m_player_engine->state());
            emit positionChanged(0);
            return;
        }

        m_player_engine->setUrl(m_loaded_track_path.toStdString());
        const auto meta = m_player_engine->metadata();
        auto duration_it = meta.find("DURATION_MS");
        if (duration_it != meta.end()) {
            emit durationChanged(QString::fromStdString(duration_it->second).toLongLong());
        }
    }

    m_player_engine->resume();
    emit stateChanged(m_player_engine->state());
}

void Player::pause()
{
    if (!m_player_engine) {
        return;
    }
    m_player_engine->pause();
    emit stateChanged(m_player_engine->state());
}

// stateChanged & positionChanged仅用于改变控制栏按键状态
void Player::stop()
{
    if (!m_player_engine) {
        return;
    }

    emit stateChanged(PlayingState::STOP);
    emit positionChanged(0);

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
    emit positionChanged(this->position());
}

bool Player::muted()
{
    return m_is_mute;
}

void Player::setMute(bool mute)
{
    if (!m_player_engine || m_is_mute == mute) {
        return;
    }

    if (m_is_mute) {    // && mute = off (recover)
        m_player_engine->setVolume(m_old_volume);
    } else { // mute = on (mute)
        m_old_volume = m_player_engine->volume();
        m_player_engine->setVolume(0.0f);
    }

    m_is_mute = mute;
}

void Player::setVolume(float vol)
{
    if (!m_player_engine) {
        return;
    }

    const double normalized = std::clamp(static_cast<double>(vol) / 100.0, 0.0, 1.0);
    double audio_gain = this->mapSliderToVolume(normalized, m_min_db);
    m_player_engine->setVolume((float)audio_gain);

    if (!m_is_mute) {
        m_old_volume = static_cast<float>(audio_gain);
    }
}


double Player::mapSliderToVolume(double value, double min_db)
{
    if (value <= 0.0) {
        return 0.0;
    }

    if (value >= 1.0) {
        return 1.0;
    }

    double db = min_db + (0.0 - min_db) * value;
    return qPow(10.0, db/20.0);
}

qint64 Player::position()
{
    if (!m_player_engine) {
        return 0;
    }

    if (m_player_engine->state() == PlayingState::STOP) {
        return 0;
    }

    return (qint64)(m_player_engine->position());
}

void Player::setOutputDevice(const QAudioDevice& device)
{
    if (!m_player_engine || device.isNull()) {
        qWarning() << "[AUDIO] setOutputDevice ignored. m_player_engine/device invalid.";
        return;
    }

    qInfo() << "[AUDIO] switching output device to" << device.description() << "id=" << device.id();

    const bool ok = m_player_engine->setOutputDeviceByName(device.description().toStdString());
    if (!ok) {
        qWarning() << "[AUDIO] backend switch failed for" << device.description();
        return;
    }

    m_preferred_output_id = device.id();
    refreshDeviceCache();
    qInfo() << "[AUDIO] output switch applied. active=" << currentOutputDevice().description()
            << "id=" << currentOutputDevice().id();
    emit deviceChanged(currentOutputDevice());
}

void Player::setOutputDeviceById(const QByteArray& id)
{
    if (id.isEmpty()) {
        qWarning() << "[AUDIO] setOutputDeviceById ignored. empty id.";
        return;
    }

    qInfo() << "[AUDIO] request switch by id=" << id;

    for (const auto& dev : m_audio_devices) {
        if (dev.id() == id) {
            setOutputDevice(dev);
            return;
        }
    }

    qWarning() << "[AUDIO] no matching output device id found:" << id;
}

QList<QAudioDevice> Player::devices() const
{
    return m_audio_devices;
}

QAudioDevice Player::currentOutputDevice() const
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

void Player::refreshDeviceCache()
{
    m_audio_devices = QMediaDevices::audioOutputs();
    if (m_audio_devices.isEmpty() || !m_player_engine) {
        qWarning() << "[AUDIO] refreshDeviceCache got empty list or null m_player_engine.";
        m_current_output_id.clear();
        return;
    }

    const std::string active_name = m_player_engine->currentOutputDeviceName();
    for (const auto& dev : m_audio_devices) {
        if (dev.description().toStdString() == active_name) {
            m_current_output_id = dev.id();
            qInfo() << "[AUDIO] active backend device mapped to Qt device:" << dev.description();
            return;
        }
    }

    if (!m_preferred_output_id.isEmpty()) {
        for (const auto& dev : m_audio_devices) {
            if (dev.id() == m_preferred_output_id) {
                m_current_output_id = dev.id();
                qInfo() << "[AUDIO] backend device not in Qt list. use preferred Qt device:" << dev.description();
                return;
            }
        }
    }

    m_current_output_id = m_audio_devices.first().id();
    qInfo() << "[AUDIO] using first available Qt output device:" << m_audio_devices.first().description();
}

void Player::setEQ(gains_t gains)
{
    m_player_engine->setEQ(gains);
}
