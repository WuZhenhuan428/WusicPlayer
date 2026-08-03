#pragma once

#include "core/search_types.h"
#include "model/library/library.h"
#include "model/library/library_file_watcher.h"
#include "model/library/library_repo.h"
#include "model/library/library_scanner.h"
#include "model/library/library_track.h"

#include <QObject>
#include <QThread>

#include <memory>
#include <optional>

/**
 * @brief 音乐库门面:根目录管理、扫描调度、内存/SQLite 同步、对外信号。
 *
 * 只读查询走内存索引;搜索走 FTS5;扫描在 worker 线程执行(Scanner 不碰数据库)。
 */
class LibraryManager : public QObject
{
    Q_OBJECT
public:
    explicit LibraryManager(QObject* parent = nullptr);
    ~LibraryManager() override;

    // 初始化:打开/创建数据库并载入既有曲目,恢复被监控目录
    bool initialize(const QString& db_path);

    // ---- 根目录 ----
    void add_watched_folder(const QString& path);
    void remove_watched_folder(const QString& path);
    QStringList watched_folders() const;

    // ---- 只读查询 ----
    std::optional<LibraryTrack> track_by_id(TrackId id) const;
    std::optional<LibraryTrack> track_by_path(const QString& normalized_path) const;
    // 非拥有:返回内部容器的 const 引用,仅本次调用内有效
    const QHash<QString, LibraryTrack>& index() const;
    int track_count() const;
    // FTS5 全文搜索(走 SQLite,阶段 5 搜索后端)
    QVector<LibraryTrack> search(const QString& keyword, SearchQueryMode mode,
                                 int limit = 200) const;

    // ---- 扫描 ----
    void start_scan();                     // 全量 reconcile
    void scan_folder(const QString& path); // 单根目录

signals:
    void sgn_scan_progress(int processed, int total);
    void sgn_scan_finished();
    void sgn_library_changed();

private:
    void apply_batch(const QVector<TrackUpdate>& batch);
    void on_scan_finished();
    LibrarySnapshot make_snapshot() const;

    std::unique_ptr<Library> m_library;
    std::unique_ptr<LibraryRepo> m_repo;
    LibraryScanner* m_scanner = nullptr; // 移入 worker 线程;线程结束时 deleteLater
    std::unique_ptr<LibraryFileWatcher> m_watcher;
    QThread* m_worker = nullptr;
};
