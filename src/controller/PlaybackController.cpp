#include "PlaybackController.h"

PlaybackController::PlaybackController(Player* player, QObject* parent)
    : QObject(parent),
      m_player(player),
      m_playMode(PlayMode::in_order),
      m_is_muted(false)
{
    if (!player) {
        return;
    }
    // broadcast Player signals
    connect(m_player, &Player::positionChanged, this, [this](qint64 pos_ms) {
        emit sgnPositionChanged(pos_ms);
    });
    connect(m_player, &Player::durationChanged, this, [this](qint64 dur_ms) {
        emit sgnDurationChanged(dur_ms);
    });
    connect(m_player, &Player::stateChanged, this, [this](PlayingState state) {
        emit sgnPlaybackStateChanged(state);
    });
    connect(m_player, &Player::playbackFinished, this, [this]() {
        emit sgnPlaybackFinished();
    });
    connect(m_player, &Player::deviceChanged, this, [this](QAudioDevice device) {
        emit sgnDevicesChanged(this->availableDevices(), device.id());
    });

    emit sgnDevicesChanged(this->availableDevices(), this->currentDeviceId());
}

PlaybackController::~PlaybackController() {}


void PlaybackController::setPlayMode(PlayMode mode) {
    m_playMode = mode;
    emit sgnPlayModeChanged(mode);
}

PlayMode PlaybackController::playMode() {
    return m_playMode;
}

void PlaybackController::play() {
    if (!m_player) {
        return;
    }
    m_player->play();
}

void PlaybackController::pause() {
    if (!m_player) {
        return;
    }
    m_player->pause();
}

void PlaybackController::stop() {
    if (!m_player) {
        return;
    }
    m_player->stop();
}

PlayingState PlaybackController::state() {
    if (!m_player) {
        return PlayingState::STOP;
    }
    return m_player->state();
}

void PlaybackController::setPosition(qint64 pos_ms) {
    if (!m_player) {
        return;
    }
    m_player->seek(pos_ms);
}

qint64 PlaybackController::position() {
    if (!m_player) {
        return 0;
    }
    return m_player->position();
}

void PlaybackController::setVolume(int percent) {
    if (!m_player) {
        return;
    }
    m_player->setVolume(percent);
}

void PlaybackController::read(QString filepath) {
    if (!m_player) {
        return;
    }
    m_player->read(filepath);
}

void PlaybackController::setMute(bool mute_on) {
    if (m_player) {
        m_player->setMute(mute_on);
        m_is_muted = mute_on;
    }
}

bool PlaybackController::getMute() {
    if (m_player) {
        return m_player->muted();
    }
    return false;
}

void PlaybackController::flipMute()
{
    this->setMute(!m_is_muted);
}

void PlaybackController::setDevice(QAudioDevice dev) {
    if (!m_player) {
        return;
    }
    m_player->setOutputDevice(dev);
    emit sgnDevicesChanged(this->availableDevices(), this->currentDeviceId());
}

void PlaybackController::setDeviceById(QByteArray id) {
    if (!m_player) {
        return;
    }
    m_player->setOutputDeviceById(id);
    emit sgnDevicesChanged(this->availableDevices(), this->currentDeviceId());
}

QList<QAudioDevice> PlaybackController::availableDevices() {
    if (!m_player) {
        return {};
    }
    return m_player->devices();
}

QByteArray PlaybackController::currentDeviceId() {
    if (!m_player) {
        return {};
    }
    return m_player->currentOutputDevice().id();
}
