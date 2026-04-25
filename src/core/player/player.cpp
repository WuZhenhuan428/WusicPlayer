#include "player.h"

#include <qmath.h>
#include <QMetaObject>
#include <QDebug>
#include <algorithm>

Player::Player(QObject *parent)
    : QObject(parent),
      core(std::make_unique<PlayerEngine>()),
            m_media_devices(new QMediaDevices(this)),
      m_min_db(-50.0)
{
    if (!core->startDevice()) {
        return;
    }

    core->setPlaybackFinishedCallback([this]() {
        QMetaObject::invokeMethod(this, [this]() {
            if (m_suppress_next_finished_signal) {
                qInfo() << "[AUDIO] suppress playbackFinished emitted by manual stop";
                m_suppress_next_finished_signal = false;
                return;
            }
            emit stateChanged(core->state());
            emit playbackFinished();
        }, Qt::QueuedConnection);
    });
    core->setWatcdog();

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
        if (!core) {
            return;
        }
        const PlayingState curr_state = core->state();
        if (curr_state == PlayingState::PLAYING || curr_state == PlayingState::PAUSE) {
            emit positionChanged(this->position());
        }
    });
    m_position_timer->start();
}
Player::~Player() {}

PlayingState Player::state() const
{
    if (!core) {
        return PlayingState::STOP;
    }
    return const_cast<PlayerEngine*>(core.get())->state();
}


void Player::read(const QString& filepath)
{
    if (!core || filepath.isEmpty()) {
        return;
    }

    m_loaded_track_path = filepath;
    m_suppress_next_finished_signal = false;

    core->setUrl(filepath.toStdString());
    core->resume();
    const auto meta = core->metadata();
    auto duration_it = meta.find("DURATION_MS");
    if (duration_it != meta.end()) {
        emit durationChanged(QString::fromStdString(duration_it->second).toLongLong());
    }
    emit stateChanged(core->state());
    emit positionChanged(this->position());
}

void Player::play()
{
    if (!core) {
        return;
    }

    m_suppress_next_finished_signal = false;

    if (core->state() == PlayingState::STOP) {
        if (m_loaded_track_path.isEmpty()) {
            qInfo() << "[AUDIO] play ignored: no loaded track while in STOP state.";
            emit stateChanged(core->state());
            emit positionChanged(0);
            return;
        }

        core->setUrl(m_loaded_track_path.toStdString());
        const auto meta = core->metadata();
        auto duration_it = meta.find("DURATION_MS");
        if (duration_it != meta.end()) {
            emit durationChanged(QString::fromStdString(duration_it->second).toLongLong());
        }
    }

    core->resume();
    emit stateChanged(core->state());
}

void Player::pause()
{
    if (!core) {
        return;
    }
    core->pause();
    emit stateChanged(core->state());
}

void Player::stop()
{
    if (!core) {
        return;
    }

    const PlayingState prev_state = core->state();
    if (prev_state == PlayingState::STOP) {
        m_suppress_next_finished_signal = false;
        emit stateChanged(PlayingState::STOP);
        emit positionChanged(0);
        return;
    }

    m_suppress_next_finished_signal = true;
    core->stop();
    emit stateChanged(PlayingState::STOP);
    emit positionChanged(0);
}

void Player::seek(qint64 pos_ms)
{
    if (!core) {
        return;
    }
    core->seek((int64_t)pos_ms);
    emit positionChanged(this->position());
}

bool Player::muted()
{
    return m_is_mute;
}

void Player::setMute(bool mute)
{
    if (!core || m_is_mute == mute) {
        return;
    }

    if (m_is_mute) {    // && mute = off (recover)
        core->setVolume(m_old_volume);
    } else { // mute = on (mute)
        m_old_volume = core->volume();
        core->setVolume(0.0f);
    }

    m_is_mute = mute;
}

void Player::setVolume(float vol)
{
    if (!core) {
        return;
    }

    const double normalized = std::clamp(static_cast<double>(vol) / 100.0, 0.0, 1.0);
    double audio_gain = this->mapSliderToVolume(normalized, m_min_db);
    core->setVolume((float)audio_gain);

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
    if (!core) {
        return 0;
    }

    if (core->state() == PlayingState::STOP) {
        return 0;
    }

    return (qint64)(core->position());
}

void Player::setOutputDevice(const QAudioDevice& device)
{
    if (!core || device.isNull()) {
        qWarning() << "[AUDIO] setOutputDevice ignored. core/device invalid.";
        return;
    }

    qInfo() << "[AUDIO] switching output device to" << device.description() << "id=" << device.id();

    const bool ok = core->setOutputDeviceByName(device.description().toStdString());
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
    if (m_audio_devices.isEmpty() || !core) {
        qWarning() << "[AUDIO] refreshDeviceCache got empty list or null core.";
        m_current_output_id.clear();
        return;
    }

    const std::string active_name = core->currentOutputDeviceName();
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