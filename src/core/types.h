#pragma once

#include <QObject>
#include <QByteArray>
#include <QUuid>
#include <QHash>
#include <QString>

struct LibraryWidgetStates
{
    QByteArray splitterState;
    QByteArray songTreeViewHeaderState;
};

using trackId = QUuid;
using playlistId = QUuid;

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
    duration,
    filename,
    genre,
    title,
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

struct TableColumn {
    QString headerName;
    SortType sortType;
    // 0 = Not Sorted (e.g. status icon) or Custom
};

static const QHash<QString, SortType> mapStrToSorttype {
    // 标准字段
    {"title",       SortType::title},
    {"artist",      SortType::artist},
    {"album",       SortType::album},
    {"album artist",SortType::album_artist},
    {"album_artist",SortType::album_artist}, // 兼容下划线
    {"genre",       SortType::genre},
    {"composer",    SortType::composer},
    {"year",        SortType::year},
    {"date",        SortType::year},         // 兼容别名
    {"track",       SortType::track_number},
    {"track_number",SortType::track_number},
    {"disc",        SortType::disc_number},
    {"disc_number", SortType::disc_number},
    // 文件属性
    {"filename",    SortType::filename},
    {"path",        SortType::directory},
    {"filepath",    SortType::directory},
    {"directory",   SortType::directory},
    {"folder",      SortType::directory},
    {"bitrate",     SortType::bitrate},
};

enum class PlayMode
{
    in_order = 0,
    loop,
    shuffle,
    out_of_order_track,
    out_of_order_group
};

struct PlaybackQueueSnapshot
{
    QVector<trackId> queue;
    qint64 version = 0;
};