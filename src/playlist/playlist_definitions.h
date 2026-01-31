#pragma once

#include <QString>

enum class SortType
{
    PlaceHolder = 0,
    Album,
    AlbumArtist,
    Artist,
    Bitrate,
    Bpm,
    Comment,
    Composer,
    Directory,
    DiscNumber,
    Filename,
    Filesize,
    Genre,
    Labels,
    Duration,     // seconds
    SampleRate,
    Source,
    Title,
    TitleWithTrackNum,
    TrackNumber,
    Year
};

struct SortRule
{
    SortType column;
    Qt::SortOrder order = Qt::AscendingOrder;
};

struct TrackMetaData
{
    QString filepath;

    QString artist;
    QString album;
    QString album_artist;
    QString lyrics;
    QString composer;
    QString date;
    int disc_number = 0;    // 2/3 -> disck_number / disc_total
    int disc_total = 0;
    QString comment;
    QString genre;
    int year = 0;
    QString encoder;
    QString title;
    int track_number;
    int duration_s;
    int start_at;
    int bitrate;
    // cover
    bool isValid = false;
};

enum class PlayMode
{
    straight = 0,
    circulation,
    shuffle_track,
    shuffle_album,
    out_of_order_track,
    out_of_order_album
};
