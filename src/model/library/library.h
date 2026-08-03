#pragma once

#include "library_track.h"

#include <QHash>
#include <QString>

#include <optional>

/**
 * @brief 音乐库内存索引:规范化路径 → 库曲目。
 *
 * 只读查询接口供外部使用;写操作(upsert/remove/mark_missing)仅供 LibraryManager 内部调用。
 * 非线程安全,需在主线程使用。
 */
class Library
{
public:
    // ---- 写(仅 LibraryManager 内部使用) ----
    void upsert(const LibraryTrack& track);              // 新增/更新(按 path 去重)
    void remove_by_path(const QString& normalized_path); // 移除
    bool mark_missing(const QString& normalized_path, bool missing);
    void clear();

    // ---- 只读查询 ----
    std::optional<LibraryTrack> track_by_path(const QString& normalized_path) const;
    std::optional<LibraryTrack> track_by_id(TrackId id) const;
    // 非拥有:返回内部容器的 const 引用,仅本次调用内有效
    const QHash<QString, LibraryTrack>& index() const;
    int track_count() const;

private:
    QHash<QString, LibraryTrack> m_by_path; // path → track
    QHash<TrackId, QString> m_id_to_path;   // track_id → path
};
