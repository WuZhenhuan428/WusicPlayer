#include "player.h"

#include <QUrl>

Player::Player(QObject *parent)
    : QObject{parent},
    m_audio_output(new QAudioOutput(this)),
    m_media_devices(new QMediaDevices(this)),
    m_media_player(new QMediaPlayer(this))
{
    setDevice();
    connect(m_media_player, &QMediaPlayer::positionChanged, this, &Player::positionChanged);
    connect(m_media_player, &QMediaPlayer::durationChanged, this, &Player::durationChanged);
    connect(m_media_player, &QMediaPlayer::playbackStateChanged, this, &Player::onPlaybackStateChanged);

    connect(m_media_devices, &QMediaDevices::audioOutputsChanged, this, &Player::onAudioOutputchanged);
}

Player::~Player() {
    if (m_media_player) {
        m_media_player->stop();
    }
}


void Player::read(const QString& filepath) {
    openFile(filepath);
    m_media_player->play();
}

void Player::play() {
    m_media_player->play();
}

void Player::pause() {
    m_media_player->pause();
}

void Player::stop() {
    m_media_player->stop();
}

void Player::openFile(const QString& filepath) {
    m_media_player->setSource(QUrl::fromLocalFile(filepath));
}

QMediaPlayer* Player::getMediaPlayer() const {
    return this->m_media_player;
}

Player::State Player::state() const {
    return const_cast<Player*>(this)->mapPlaybackState(m_media_player->playbackState());
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
    this->m_audio_output->setDevice(curr_device);
    this->m_media_player->setAudioOutput(m_audio_output);
}

void Player::setOutputDevice(const QAudioDevice& device) {
    if (device.isNull()) return;
    m_audio_output->setDevice(device);
    m_preffered_output_id = device.id();
    qDebug() << "[INFO] Switch to device:" << device.description();
}

void Player::setOutputDeviceById(const QByteArray& id) {
    auto devs = this->devices();
    for (auto dev : devs) {
        if (dev.id() == id) {
            this->setOutputDevice(dev);
            return;
        }
    }
    qDebug() << "[INFO] invalid device id:" << id;
}

QAudioDevice Player::currentOutputDevice() const {
    return m_audio_output ? m_audio_output->device() : QAudioDevice();
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
    const QByteArray curr_id = m_audio_output->device().id();
    bool curr_still_exists = false;
    for (const auto& device : outputs) {
        if (device.id() == curr_id) {
            curr_still_exists = true;
            break;
        }
    }

    if (curr_still_exists) {
        emit deviceChanged(m_audio_output->device());
        return;
    }

    // if current device failed
    for (const auto& device : outputs) {
        if (!m_preffered_output_id.isEmpty() && device.id() == m_preffered_output_id) {
            m_audio_output->setDevice(device);
            emit deviceChanged(device);
            qDebug() << "[INFO] Restored preffered output device: " << device.description();
            return;
        }
    }

    const QAudioDevice fallback = QMediaDevices::defaultAudioOutput();
    m_audio_output->setDevice(fallback);
    emit deviceChanged(fallback);
    qDebug() << "[AUDIO] Fallback to default output:" << fallback.description();
}


void Player::setPosition(qint64 position) {
    m_media_player->setPosition(position);
}

void Player::flipMute() {
    bool is_mute = this->m_audio_output->isMuted();
    this->m_audio_output->setMuted(!is_mute);
    if (1 == is_mute) {
        qDebug() << "[INFO] Mute: off";
    } else {
        qDebug() << "[INFO] Mute: on";
    }
}

void Player::setVolume(qint64 volume) {
    double audio_gain = mapSliderToVolume(volume, -50.0);
    this->m_audio_output->setVolume(audio_gain);
}

/**
 * 分贝映射
 * 滑块线性值映射位分贝数值
 * @param value: 滑块输入值（0-100）
 * @param minDb: 最小分贝值
 * @return : 音频增益0.0 - 1.0
 */
double Player::mapSliderToVolume(qint64 value, double min_db) {
    if (value <= 0.0) {
        return 0.0;
    }
    double x = static_cast<double>(value) / 100.0;
    double db = min_db + (0.0 - min_db) * x;
    return qPow(10.0, db/20.0);
};
