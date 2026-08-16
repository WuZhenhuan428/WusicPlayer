#pragma once

#include "core/types.h"

#include <QString>

/**
 * @brief 库曲目实体:音乐库中的文件级条目。
 *
 * 与播放列表条目 `Track` 区分:库曲目以文件为粒度,由 `TrackId` 标识;
 * 阶段 3 中播放列表条目通过 `Track::library_track_id` 引用库曲目。
 */
struct LibraryTrack
{
    TrackId track_id; // 库级身份(首次扫描分配,全局唯一)
    QString filepath; // 规范化路径(唯一,去重键)
    qint64 file_size = 0;
    qint64 mtime     = 0; // 秒级,增量检测
    int duration_ms  = 0;
    bool missing     = false; // 文件缺失标记
    TrackMetaData meta;

    // 由规范化路径构造(分配新身份,用于新增条目)
    static LibraryTrack from_path(const QString& normalized_path)
    {
        LibraryTrack t;
        t.track_id = TrackId::create_uuid();
        t.filepath = normalized_path;
        return t;
    }
};
