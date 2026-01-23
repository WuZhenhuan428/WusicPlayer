#pragma once

#include <QUuid>
#include <QString>

enum class PlayMode
{
    straight = 0,
    circulation,
    shuffle_track,
    shuffle_album,
    out_of_order_track,
    out_of_order_album
};

struct TrackMetaData
{
    QUuid uuid;
    QString filepath;
    QString album;
    QString album_artist;
    QString artist;
    QString composer;
    QString date;
    int disc_number;
    QString encoder;
    QString title;
    int track_number;
    // lyrics
    unsigned int duration_ms;
    unsigned start_at;
    unsigned int bitrate;
    // comment extend to cover / front / back...
};