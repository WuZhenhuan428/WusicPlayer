#pragma once

#include "core/types.h"

#include <QString>

/**
 * @brief 现在播放队列项。
 *
 * 与内容来源解耦:同一项可来自媒体库 / 播放列表 / 外部文件。
 * `filepath` 为最终播放依据;各 id 用于身份与定位回源(可组合,不互斥)。
 */
struct QueueItem
{
    TrackId library_track_id;      // 来自媒体库(搜索 / 库控件)
    EntryId playlist_entry_id;     // 来自播放列表条目(可定位回源)
    PlaylistId source_playlist_id; // 来源列表 id
    QString source_label;          // 展示用来源说明(如 "播放列表:摇滚")
    QString filepath;              // 规范化路径(播放依据)
    TrackMetaData meta;            // 元数据快照(入队时解析一次,主面板直接读)

    bool is_library() const
    {
        return !library_track_id.is_null();
    }
    bool is_playlist() const
    {
        return !playlist_entry_id.is_null();
    }
    bool is_external() const
    {
        return !is_library() && !is_playlist();
    }
};
