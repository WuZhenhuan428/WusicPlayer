#pragma once

#include <QString>
#include <QUuid>

enum class SortType
{
    not_sorted = 0,
    album,
    album_artist,
    artist,
    bitrate,
    composer,
    directory,
    disc_number,
    filename,
    genre,
    // labels,
    title,
    // title_with_disc_track,
    track_number,
    year
};

struct SortRule
{
    SortType type;
    Qt::SortOrder order = Qt::AscendingOrder;
};

struct TrackMetaData
{
    // cover analysis separately
    QString album;
    QString album_artist;
    QString artist;
    int     bitrate;
    QString comment;
    QString composer;
    QString date;
    int     disc_number = 0;    // 2/3 -> disck_number / disc_total
    int     disc_total = 0;
    int     duration_s;
    QString encoder;
    QString filepath;
    QString filename;
    QString genre;
    QString lyrics;
    int     start_at;
    QString title;
    int     track_number;
    int     year = 0;

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
