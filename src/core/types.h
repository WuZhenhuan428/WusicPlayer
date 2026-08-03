#pragma once

#include <QByteArray>
#include <QFileInfo>
#include <QHash>
#include <QObject>
#include <QString>
#include <QUuid>

#include "core/utils/path.hpp"

using EntryId    = QUuid; // 播放列表条目身份:播放列表内唯一
using PlaylistId = QUuid; // 播放列表身份
using TrackId    = QUuid; // 库级曲目身份:首次入库时由库分配(阶段 2 落地)

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
    int bitrate;
    QString comment;
    QString composer;
    QString date;
    int disc_number = 0; // e.g. `2/3` means `disc_number / disc_total`
    int disc_total  = 0;
    int duration_s;
    QString encoder;
    QString filepath;
    QString filename;
    QString genre;
    QString lyrics;
    int start_at;
    QString title;
    int track_number;
    int year     = 0;

    bool isValid = false;
};

enum class TrackSource
{
    external, // 外部条目:文件不在库中,元数据内联(当前所有条目的状态)
    library   // 库条目:引用库中曲目(阶段 3 后出现)
};

/**
 * @brief 曲目条目。音乐库与播放列表共用。
 *
 * - 库条目(source=library):entry_id 指向库级 TrackId,元数据由库维护
 * - 外部条目(source=external):文件不在库中,元数据内联
 */
struct Track
{
    EntryId entry_id = EntryId::createUuid(); // 条目身份(播放列表内唯一)
    TrackId library_track_id;                 // 库级身份;外部条目为空
    TrackSource source = TrackSource::external;
    QString filepath; // 规范化路径
    TrackMetaData meta;
    bool missing = false; // 文件缺失标记(阶段 3 使用)

    // 新建外部条目(路径自动规范化)
    static Track from_filepath(const QString& filepath)
    {
        Track t;
        t.filepath      = utils::path::normalize_path(filepath);
        t.meta.filepath = t.filepath;
        t.meta.filename = QFileInfo(t.filepath).fileName();
        t.meta.isValid  = false;
        return t;
    }

    // 从缓存恢复外部条目(指定条目身份)
    static Track from_entry(EntryId eid, const QString& filepath)
    {
        Track t;
        t.entry_id      = eid;
        t.filepath      = utils::path::normalize_path(filepath);
        t.meta.filepath = t.filepath;
        t.meta.filename = QFileInfo(t.filepath).fileName();
        t.meta.isValid  = false;
        return t;
    }
};

struct TableColumn
{
    QString headerName;
    SortType sortType;
    // 0 = Not Sorted (e.g. status icon) or Custom
};

static const QHash<QString, SortType> mapStrToSorttype{
    // 标准字段
    {"title", SortType::title},
    {"artist", SortType::artist},
    {"album", SortType::album},
    {"album artist", SortType::album_artist},
    {"album_artist", SortType::album_artist}, // 兼容下划线
    {"genre", SortType::genre},
    {"composer", SortType::composer},
    {"year", SortType::year},
    {"date", SortType::year}, // 兼容别名
    {"track", SortType::track_number},
    {"track_number", SortType::track_number},
    {"disc", SortType::disc_number},
    {"disc_number", SortType::disc_number},
    // 文件属性
    {"filename", SortType::filename},
    {"path", SortType::directory},
    {"filepath", SortType::directory},
    {"directory", SortType::directory},
    {"folder", SortType::directory},
    {"bitrate", SortType::bitrate},
};

enum class PlayMode
{
    in_order = 0,
    loop,
    shuffle,
    out_of_order_track,
    out_of_order_group
};

/**
 * @brief 向播放列表添加未入库文件时的解析策略。
 *
 * by_operation:按操作类型(文件夹→同步入库,单文件→外部文件)
 * import_to_library:未命中时把文件(父目录)注册到库,扫描后升级为库引用
 * keep_external:保持外部文件(不强制入库)
 * always_ask:每次询问(由 controller/UI 弹窗展开为 import/external)
 */
enum class AddFilePolicy
{
    by_operation = 0,
    import_to_library,
    keep_external,
    always_ask
};

struct PlaybackQueueSnapshot
{
    QVector<EntryId> queue;
    qint64 version = 0;
};

// Desktop lyrics panel attributes
enum class DisplayMode
{
    OneLine,
    TwoLine
};

enum class AlignMode
{
    Left,
    Middle,
    Right
};
