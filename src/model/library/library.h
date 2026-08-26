#pragma once

#include "model/library/library_track.h"

#include <QHash>
#include <QString>

#include <memory>

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
    // 返回共享引用(权威副本由本容器持有, 零拷贝); nullptr 表示不存在
    std::shared_ptr<const LibraryTrack> track_by_path(const QString& normalized_path) const;
    std::shared_ptr<const LibraryTrack> track_by_id(TrackId id) const;
    // 非拥有:返回内部容器的 const 引用,仅本次调用内有效
    const QHash<QString, std::shared_ptr<const LibraryTrack>>& index() const;
    int track_count() const;

private:
    QHash<QString, std::shared_ptr<const LibraryTrack>> m_by_path; // path → track(共享)
    QHash<TrackId, QString> m_id_to_path;                          // track_id → path
};
