#include "PlaybackController.h"

PlaybackController::PlaybackController(Player* player, QObject* parent)
    : m_player(player), QObject(parent)
{
    m_playMode = PlayMode::in_order;
    if (!player) {
        return;
    }
    // broadcast Player signals
    connect(m_player->getMediaPlayer(), &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        emit sgnMediaStateChanged(status);
    });
    connect(m_player, &Player::positionChanged, this, [this](qint64 pos_ms) {
        emit sgnPositionChanged(pos_ms);
    });
    connect(m_player, &Player::durationChanged, this, [this](qint64 dur_ms) {
        emit sgnDurationChanged(dur_ms);
    });
    connect(m_player->getMediaPlayer(), &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState new_state) {
        emit sgnPlaybackStateChanged(new_state);
    });
    connect(m_player, &Player::deviceChanged, this, [this](QAudioDevice device) {
        emit sgnDevicesChanged(this->availableDevices(), device.id());
    });
}

PlaybackController::~PlaybackController() {}


void PlaybackController::setPlayMode(PlayMode mode) {
    m_playMode = mode;
    emit sgnPlayModeChanged(mode);
}

PlayMode PlaybackController::playMode() {
    return m_playMode;
}

void PlaybackController::flipMute() {
    m_player->flipMute();
}

void PlaybackController::play() {
    m_player->play();
}

void PlaybackController::pause() {
    m_player->pause();
}

void PlaybackController::stop() {
    m_player->stop();
}

void PlaybackController::setPosition(qint64 pos_ms) {
    m_player->setPosition(pos_ms);
}

qint64 PlaybackController::position() {
    return m_player->getMediaPlayer()->position();
}

void PlaybackController::setVolume(int percent) {
    m_player->setVolume(percent);
}

void PlaybackController::read(QString filepath) {
    m_player->read(filepath);
}

void PlaybackController::setMute(bool mute_on) {
    bool now_muted;
    if (m_player->getMediaPlayer() && m_player->getMediaPlayer()->audioOutput()) {
        now_muted = m_player->getMediaPlayer()->audioOutput()->isMuted();
    }
    if (mute_on != now_muted) {
        flipMute();
    }
}

bool PlaybackController::getMute() {
    if (m_player->getMediaPlayer() && m_player->getMediaPlayer()->audioOutput()) {
        return m_player->getMediaPlayer()->audioOutput()->isMuted();
    }
    return false;
}

const QMediaPlayer* PlaybackController::getMediaPlayer() const {
    return m_player->getMediaPlayer();
}

void PlaybackController::setDevice(QAudioDevice dev) {
    m_player->setOutputDevice(dev);
    emit sgnDevicesChanged(this->availableDevices(), dev.id());
}

void PlaybackController::setDeviceById(QByteArray id) {
    m_player->setOutputDeviceById(id);
    emit sgnDevicesChanged(this->availableDevices(), id);
}

QList<QAudioDevice> PlaybackController::availableDevices() {
    return m_player->devices();
}

QByteArray PlaybackController::currentDeviceId() {
    return m_player->currentOutputDevice().id();
}
