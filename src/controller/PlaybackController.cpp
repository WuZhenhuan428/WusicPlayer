#include "PlaybackController.h"

#include <QJsonObject>

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
    connect(m_player, &Player::sgnPlaybackNatualEnd, this, [this]() {
        emit sgnPlaybackNatualEnd();
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

void PlaybackController::setGains(gains_t gains){
    if (!m_player) {
        return;
    }
    m_player->setEQ(gains);
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


void PlaybackController::loadFromJson(const QJsonObject &json)
{
    QJsonObject obj = json.value(this->configSubKey()).toObject();
    this->setVolume(obj.value("volume").toInt(100));
    this->setMute(obj.value("muted").toBool());
    m_playMode = static_cast<PlayMode>(obj.value("play_mode").toInt());
    this->setDeviceById(QByteArray::fromBase64(obj.value("last_device").toString().toUtf8()));

    m_last_position_ms = obj.value("last_position_ms").toInt();
    m_last_was_playing = obj.value("last_was_playing").toBool(false);

    // TODO: consider about time sequence
    if (m_player) {
        m_player->seek(m_last_position_ms);
    }
}

QJsonObject PlaybackController::saveToJson()
{
    QJsonObject obj;

    m_last_was_playing = this->state() == PlayingState::PLAYING;
    m_last_position_ms = this->state() != PlayingState::STOP
                            ? m_player->position()
                            : 0;

    obj["volume"] = m_player->volume();
    obj["muted"] = m_is_muted;
    obj["play_mode"] = static_cast<int>(m_playMode);
    obj["last_device"] = QString::fromUtf8(this->currentDeviceId().toBase64());
    obj["last_was_playing"] = m_last_was_playing;
    obj["last_position_ms"] = m_last_position_ms;
    return obj;
}

QString PlaybackController::configSubKey() const
{
    return "playback";
}

int PlaybackController::lastPositionMs() const
{
    return m_last_position_ms;
}

bool PlaybackController::lastWasPlaying() const
{
    return m_last_was_playing;
}
