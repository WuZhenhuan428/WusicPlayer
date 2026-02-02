#include "player.h"

Player::Player(QObject *parent)
    : QObject{parent}
    , MediaPlayer(new QMediaPlayer)
    , AudioOutput(new QAudioOutput)
    , m_state(Player::State::IDLE)
{
    setDevice();
    initConnections();
}

Player::~Player() {}

void Player::initConnections() {
    connect(MediaPlayer, &QMediaPlayer::positionChanged, this, &Player::positionChanged);
    connect(MediaPlayer, &QMediaPlayer::durationChanged, this, &Player::durationChanged);
}

void Player::read(const QString& filepath) {
    openFile(filepath);     // set source
    switch (m_state)
    {
    case State::IDLE:
    case State::PAUSED:
    case State::STOPPED:
    case State::PLAYING:
        setState(State::STOPPED);
        setState(State::PLAYING);
        break;
    }
}

void Player::play() {
    switch (m_state)
    {
    case State::IDLE:
    case State::PAUSED:
    case State::STOPPED:
        setState(State::PLAYING); break;
    case State::PLAYING: break; // ignore
    }
    qDebug() << "[INFO] Current state: " << m_state;
}

void Player::pause() {
    if (m_state == State::PLAYING) {
        setState(State::PAUSED);
    }
    qDebug() << "[INFO] Current state: " << m_state;
}

void Player::stop() {
    if (m_state != State::IDLE) {
        setState(State::STOPPED);
    }
    qDebug() << "[INFO] Current state: " << m_state;
}

void Player::openFile(const QString& filepath) {
    MediaPlayer->setSource(QUrl::fromLocalFile(filepath));
    qDebug() << "[INFO] Open file path: " << filepath;
}

void Player::setState(State newState)
{
    if (m_state == newState) {
        return;
    }
    m_state = newState;
    emit stateChanged(m_state);

    switch (m_state)
    {
    case State::IDLE: break;
    case State::PAUSED: MediaPlayer->pause(); break;
    case State::STOPPED: MediaPlayer->stop(); break;
    case State::PLAYING: MediaPlayer->play(); break;
    }
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
        qDebug() << "[INFO] Mute:" << "off";
    } else {
        qDebug() << "[INFO] Mute:" << "on";
    }
}

void Player::setVolume(qint64 volume) {
    double audioGain = mapSliderToVolume(volume, -50.0);
    this->AudioOutput->setVolume(audioGain);
    // qDebug() << "[INFO] Volume gain / percent = " << audioGain << ", " << volume;
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