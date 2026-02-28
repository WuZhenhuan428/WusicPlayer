#include "player.h"

#include <QUrl>

Player::Player(QObject *parent)
    : QObject{parent},
    m_mediaPlayer(new QMediaPlayer(this)),
    m_audioOutput(new QAudioOutput(this)),
    m_mediaDevices(new QMediaDevices(this))
{
    setDevice();
    connect(m_mediaPlayer, &QMediaPlayer::positionChanged, this, &Player::positionChanged);
    connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, &Player::durationChanged);
    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &Player::onPlaybackStateChanged);

    connect(m_mediaDevices, &QMediaDevices::audioOutputsChanged, 
            this, &Player::onAudioOutputchanged);
}

Player::~Player() {
    if (m_mediaPlayer) {
        m_mediaPlayer->stop();
    }
}


void Player::read(const QString& filepath) {
    openFile(filepath);
    m_mediaPlayer->play();
}

void Player::play() {
    m_mediaPlayer->play();
}

void Player::pause() {
    m_mediaPlayer->pause();
}

void Player::stop() {
    m_mediaPlayer->stop();
}

void Player::openFile(const QString& filepath) {
    m_mediaPlayer->setSource(QUrl::fromLocalFile(filepath));
}

QMediaPlayer* Player::getMediaPlayer() const {
    return this->m_mediaPlayer;
}

Player::State Player::state() const {
    return const_cast<Player*>(this)->mapPlaybackState(m_mediaPlayer->playbackState());
}

Player::State Player::mapPlaybackState(QMediaPlayer::PlaybackState state) {
    switch (state) {
        case QMediaPlayer::PlaybackState::PlayingState:
            return Player::State::PLAYING;
        case QMediaPlayer::PlaybackState::PausedState:
            return Player::State::PAUSED;
        case QMediaPlayer::PlaybackState::StoppedState:
            return Player::State::STOPPED;
        default:
            return Player::State::IDLE;
    }
}

void Player::onPlaybackStateChanged(QMediaPlayer::PlaybackState state) {
    emit stateChanged(mapPlaybackState(state));
}

void Player::setDevice() {
    QAudioDevice curr_device = QMediaDevices::defaultAudioOutput();
    qDebug() << "[INFO] Output device:" << curr_device.description();
    this->m_audioOutput->setDevice(curr_device);
    this->m_mediaPlayer->setAudioOutput(m_audioOutput);
}

void Player::setOutputDevice(const QAudioDevice& device) {
    if (device.isNull()) return;
    m_audioOutput->setDevice(device);
    m_prefferedOutputId = device.id();
    qDebug() << "[INFO] Switch to device:" << device.description();
}

QAudioDevice Player::currentOutputDevice() const {
    return m_audioOutput ? m_audioOutput->device() : QAudioDevice();
}

QList<QAudioDevice> Player::devices() const {
    return QMediaDevices::audioOutputs();
}

void Player::onAudioOutputchanged() {
    const auto outputs = QMediaDevices::audioOutputs();
    if (outputs.isEmpty()) {
        qWarning() << "[WARNING] No audio output device available.";
        return;
    }

    // is current device still available
    const QByteArray curr_id = m_audioOutput->device().id();
    bool curr_still_exists = false;
    for (const auto& device : outputs) {
        if (device.id() == curr_id) {
            curr_still_exists = true;
            break;
        }
    }

    if (curr_still_exists) return;

    // if current device failed
    for (const auto& device : outputs) {
        if (!m_prefferedOutputId.isEmpty() && device.id() == m_prefferedOutputId) {
            m_audioOutput->setDevice(device);
            qDebug() << "[INFO] Restored preffered output device: " << device.description();
            return;
        }
    }

    const QAudioDevice fallback = QMediaDevices::defaultAudioOutput();
    m_audioOutput->setDevice(fallback);
    qDebug() << "[AUDIO] Fallback to default output:" << fallback.description();
}


void Player::setPosition(qint64 position) {
    m_mediaPlayer->setPosition(position);
}

void Player::flipMute() {
    bool muteState = this->m_audioOutput->isMuted();
    this->m_audioOutput->setMuted(!muteState);
    if (1 == muteState) {
        qDebug() << "[INFO] Mute: off";
    } else {
        qDebug() << "[INFO] Mute: on";
    }
}

void Player::setVolume(qint64 volume) {
    double audioGain = mapSliderToVolume(volume, -50.0);
    this->m_audioOutput->setVolume(audioGain);
}

/**
 * 分贝映射
 * 滑块线性值映射位分贝数值
 * @param value: 滑块输入值（0-100）
 * @param minDb: 最小分贝值
 * @return : 音频增益0.0 - 1.0
 */
double Player::mapSliderToVolume(qint64 value, double minDb) {
    if (value <= 0.0) {
        return 0.0;
    }
    double x = static_cast<double>(value) / 100.0;
    double db = minDb + (0.0 - minDb) * x;
    return qPow(10.0, db/20.0);
};
