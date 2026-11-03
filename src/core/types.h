#pragma once

#include "core/utils/path.hpp"

#include <QByteArray>
#include <QFileInfo>
#include <QHash>
#include <QObject>
#include <QString>
#include <QUuid>

template <typename Tag>
class Id
{
public:
    Id() = default; // null id
    explicit Id(const QUuid& uuid) : uuid_(uuid) {}
    explicit Id(QAnyStringView string) : uuid_(QUuid(string)) {}
    static Id create_uuid()
    {
        return Id(QUuid::createUuid());
    }

    [[nodiscard]] QUuid to_uuid() const
    {
        return uuid_;
    }

    [[nodiscard]] QString to_string() const
    {
        return uuid_.toString();
    }

    [[nodiscard]] QString to_string_without_brace() const
    {
        return uuid_.toString(QUuid::WithoutBraces);
    }

    [[nodiscard]] bool is_null() const
    {
        return uuid_.isNull();
    }

    friend bool operator==(const Id& a, const Id& b)
    {
        return a.uuid_ == b.uuid_;
    }

    friend bool operator!=(const Id& a, const Id& b)
    {
        return a.uuid_ != b.uuid_;
    }

    template <typename ToTag>
    Id<ToTag> to_tag_of() const
    {
        return Id<ToTag>(uuid_);
    }

    static Id<Tag> from_string(QAnyStringView string)
    {
        return Id<Tag>(string);
    }

private:
    QUuid uuid_;
};

struct EntryIdTag
{};
struct PlaylistIdTag
{};
struct TrackIdTag
{};

using EntryId    = Id<EntryIdTag>;    // 播放列表条目身份:播放列表内唯一
using PlaylistId = Id<PlaylistIdTag>; // 播放列表身份
using TrackId    = Id<TrackIdTag>;    // 库级曲目身份:首次入库时由库分配(阶段 2 落地)

template <typename Tag>
inline size_t qHash(const Id<Tag>& id, size_t seed = 0) noexcept
{
    return qHash(id.to_uuid(), seed);
}

Q_DECLARE_METATYPE(EntryId)
Q_DECLARE_METATYPE(PlaylistId)
Q_DECLARE_METATYPE(TrackId)

namespace wusic
{
// 拖拽 MIME 类型:库曲目拖拽(TrackId JSON 数组)
inline constexpr char kLibraryTracksMime[]   = "application/x-wusic-library-tracks";
// 拖拽 MIME 类型:播放列表条目拖拽(JSON: {src, ids:[EntryId]});列表→列表
inline constexpr char kPlaylistEntriesMime[] = "application/x-wusic-playlist-entries";
} // namespace wusic

enum class SortType
{
    NotSorted = 0,
    Album,
    AlbumArtist,
    Artist,
    Bitrate,
    Composer,
    Directory,
    DiscNumber,
    Duration,
    Filename,
    Genre,
    Title,
    TrackNumber,
    Year,
};

struct SortRule
{
    SortType type       = {};
    Qt::SortOrder order = Qt::AscendingOrder;
};

struct TrackMetaData
{
    // cover analysis separately
    QString album;
    QString album_artist;
    QString artist;
    int bitrate = 0;
    QString comment;
    QString composer;
    QString date;
    int disc_number = 0; // e.g. `2/3` means `disc_number / disc_total`
    int disc_total  = 0;
    int duration_s  = 0;
    QString encoder;
    QString filepath;
    QString filename;
    QString genre;
    QString lyrics;
    int start_at = 0;
    QString title;
    int track_number                            = 0;
    int year                                    = 0;

    bool is_valid                               = false;

    // 用于判断元数据是否真正发生变化(库刷新/播放列表刷新去重);
    // 注意: 依赖上方所有标量字段的默认初始化, 否则比较结果不确定
    bool operator==(const TrackMetaData&) const = default;
};

enum class TrackSource
{
    External, // 外部条目:文件不在库中,元数据内联(当前所有条目的状态)
    Library,  // 库条目:引用库中曲目(阶段 3 后出现)
};

/**
 * @brief 曲目条目。音乐库与播放列表共用。
 *
 * - 库条目(source=library):entry_id 指向库级 TrackId,元数据由库维护
 * - 外部条目(source=external):文件不在库中,元数据内联
 */
struct Track
{
    EntryId entry_id = EntryId::create_uuid(); // 条目身份(播放列表内唯一)
    TrackId library_track_id;                  // 库级身份;外部条目为空
    TrackSource source = TrackSource::External;
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
        t.meta.is_valid = false;
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
        t.meta.is_valid = false;
        return t;
    }
};

struct TableColumn
{
    QString header_name;
    SortType sort_type;
    // 0 = Not Sorted (e.g. status icon) or Custom
};

static const QHash<QString, SortType> gMapStrToSorttype{
    // 标准字段
    {"title", SortType::Title},
    {"artist", SortType::Artist},
    {"album", SortType::Album},
    {"album artist", SortType::AlbumArtist},
    {"album_artist", SortType::AlbumArtist}, // 兼容下划线
    {"genre", SortType::Genre},
    {"composer", SortType::Composer},
    {"year", SortType::Year},
    {"date", SortType::Year}, // 兼容别名
    {"track", SortType::TrackNumber},
    {"track_number", SortType::TrackNumber},
    {"disc", SortType::DiscNumber},
    {"disc_number", SortType::DiscNumber},
    // 文件属性
    {"filename", SortType::Filename},
    {"path", SortType::Directory},
    {"filepath", SortType::Directory},
    {"directory", SortType::Directory},
    {"folder", SortType::Directory},
    {"bitrate", SortType::Bitrate},
};

enum class PlayMode
{
    InOrder = 0,
    Loop,
    Shuffle,
    OutOfOrderByTrack,
    OutOfOrderByGroup,
};

/**
 * @brief 向播放列表添加未入库文件时的解析策略。
 *
 * ByOperation: 按操作类型(文件夹→同步入库,单文件→外部文件)
 * ImportToLibrary: 未命中时把文件(父目录)注册到库,扫描后升级为库引用
 * KeepExternal: 保持外部文件(不强制入库)
 * AlwaysAsk: 每次询问(由 controller/UI 弹窗展开为 import/external)
 */
enum class AddFilePolicy
{
    ByOperation = 0,
    ImportToLibrary,
    KeepExternal,
    AlwaysAsk,
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
    TwoLine,
};

enum class AlignMode
{
    Left,
    Middle,
    Right,
};
