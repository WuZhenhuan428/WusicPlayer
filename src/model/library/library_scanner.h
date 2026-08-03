#pragma once

#include "library_track.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

// 文件时间戳快照:用于增量检测(size + mtime),并携带既有库身份
struct FileStamp
{
    qint64 size  = 0;
    qint64 mtime = 0;
    TrackId track_id; // 已知路径对应的库身份(modified 时保留,避免身份漂移)
};
using LibrarySnapshot = QHash<QString, FileStamp>;

// 扫描结果:单个文件的变化分类
enum class TrackChange
{
    added,    // 新增(快照无此路径)
    modified, // 已修改(大小或 mtime 变化,保留原 track_id)
    missing   // 快照有但磁盘消失(标记 missing,不删除)
};

struct TrackUpdate
{
    TrackChange change;
    QString path;       // 规范化路径(missing 时有效)
    LibraryTrack track; // added/modified 时有效
};

Q_DECLARE_METATYPE(TrackUpdate)
Q_DECLARE_METATYPE(QVector<TrackUpdate>)

/**
 * @brief 扫描引擎:在 worker 线程执行,只读文件系统,不碰数据库。
 *
 * 阶段 1 递归收集全部音频文件计算总数;阶段 2 与快照比对、分类、解析标签,
 * 分批通过信号回报结果;阶段 3 检测缺失文件。
 */
class LibraryScanner : public QObject
{
    Q_OBJECT
public:
    explicit LibraryScanner(QObject* parent = nullptr);

    void start_scan(const QStringList& roots, const LibrarySnapshot& snapshot);

signals:
    void sgn_progress(int processed, int total);
    void sgn_batch_ready(QVector<TrackUpdate> batch);
    void sgn_finished();
};
