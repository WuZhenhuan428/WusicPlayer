#include "player.h"

Player::Player(QObject *parent)
    : QObject{parent},
    MediaPlayer(new QMediaPlayer),
    AudioOutput(new QAudioOutput)
{
    setDevice();
    initConnections();
}

Player::~Player() {}

void Player::initConnections() {
    connect(MediaPlayer, &QMediaPlayer::positionChanged, this, &Player::positionChanged);
    connect(MediaPlayer, &QMediaPlayer::durationChanged, this, &Player::durationChanged);
    connect(MediaPlayer, &QMediaPlayer::playbackStateChanged, this, &Player::onPlaybackStateChanged);
}

void Player::read(const QString& filepath) {
    openFile(filepath);
    MediaPlayer->play();
}

void Player::play() {
    MediaPlayer->play();
}

void Player::pause() {
    MediaPlayer->pause();
}

void Player::stop() {
    MediaPlayer->stop();
}

void Player::openFile(const QString& filepath) {
    MediaPlayer->setSource(QUrl::fromLocalFile(filepath));
}

Player::State Player::state() const {
    return const_cast<Player*>(this)->mapPlaybackState(MediaPlayer->playbackState());
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
    QAudioDevice CurrDevice = QMediaDevices::defaultAudioOutput();
    qDebug() << "[INFO] Output device:" << CurrDevice.description();
    this->AudioOutput->setDevice(CurrDevice);
    this->MediaPlayer->setAudioOutput(AudioOutput);
}


void Player::setPosition(qint64 position) {
    MediaPlayer->setPosition(position);
}

void Player::flipMute() {
    bool muteState = this->AudioOutput->isMuted();
    this->AudioOutput->setMuted(!muteState);
    if (1 == muteState) {
        qDebug() << "[INFO] Mute: off";
    } else {
        qDebug() << "[INFO] Mute: on";
    }
}

void Player::setVolume(qint64 volume) {
    double audioGain = mapSliderToVolume(volume, -50.0);
    this->AudioOutput->setVolume(audioGain);
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