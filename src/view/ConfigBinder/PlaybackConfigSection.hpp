#pragma once
#include "IConfigSection.hpp"
#include "core/types.h"
#include <QByteArray>
#include <QJsonObject>

#include <QPointer>

class PlaybackConfigSection : public IConfigSection
{
public:
    int volume = 100;
    bool muted = false;
    PlayMode play_mode = PlayMode::in_order;
    playlistId last_playlist_id = playlistId();
    trackId last_track_id = trackId();
    int last_position_ms = 0;
    QByteArray last_device = QByteArray();  // QAudioDevice::id()

    QString key() const override {
        return "playback";
    }

    void load(const QJsonObject& root) override {
        QJsonObject obj = root.value(this->key()).toObject();
        volume = obj.value("volume").toInt(100);
        muted = obj.value("muted").toBool(false);
        play_mode = static_cast<PlayMode>(obj.value("play_mode").toInt(0));
        last_playlist_id = playlistId(obj.value("last_playlist_id").toString());
        last_track_id = trackId(obj.value("last_track_id").toString());
        last_position_ms = obj.value("last_position_ms").toInt();
        last_device = QByteArray::fromBase64(obj.value("last_device").toString().toUtf8());
    }

    QJsonObject save() const override {
        QJsonObject obj;
        obj["volume"] = volume;       
        obj["muted"] = muted;
        obj["play_mode"] = static_cast<int>(play_mode);

        obj["last_playlist_id"] = last_playlist_id.toString(QUuid::WithoutBraces);
        obj["last_track_id"] = last_track_id.toString(QUuid::WithoutBraces);
        obj["last_position_ms"] = last_position_ms;
        obj["last_device"] = QString::fromUtf8(last_device.toBase64());
        return obj;
    }
};