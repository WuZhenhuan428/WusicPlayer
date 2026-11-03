#include "model/library/library_manager.h"

#include "core/utils/path.hpp"

#include <QFileInfo>
#include <QStandardPaths>

#include "core/logger/logger_manager.h"
namespace
{
// NOLINTNEXTLINE
Logger* logger = LoggerManager::file_logger("library", {"console", "gui"});
} // namespace

LibraryManager::LibraryManager(QObject* parent) : QObject(parent)
{
    qRegisterMetaType<TrackUpdate>("TrackUpdate");
    qRegisterMetaType<QVector<TrackUpdate>>("QVector<TrackUpdate>");

    library_ = std::make_unique<Library>();
    repo_    = std::make_unique<LibraryRepo>();

    // 扫描器移入独立 worker 线程;线程结束时 deleteLater
    scanner_ = new LibraryScanner;
    worker_  = new QThread(this);
    scanner_->moveToThread(worker_);
    connect(worker_, &QThread::finished, scanner_, &QObject::deleteLater);
    worker_->start();

    connect(scanner_, &LibraryScanner::sgn_progress, this, &LibraryManager::sgn_scan_progress);
    connect(scanner_, &LibraryScanner::sgn_batch_ready, this, &LibraryManager::apply_batch);
    connect(scanner_, &LibraryScanner::sgn_finished, this, &LibraryManager::on_scan_finished);

    watcher_ = std::make_unique<LibraryFileWatcher>();
    connect(watcher_.get(), &LibraryFileWatcher::sgn_root_changed, this,
            &LibraryManager::scan_folder);
    connect(watcher_.get(), &LibraryFileWatcher::sgn_reconcile_requested, this,
            &LibraryManager::start_scan);
}

LibraryManager::~LibraryManager()
{
    if (worker_) {
        worker_->quit();
        worker_->wait();
    }
}

bool LibraryManager::initialize(const QString& db_path)
{
    if (!repo_->open(db_path)) {
        logger->warn("[LIBRARY] initialize failed: {}", repo_->last_error());
        return false;
    }
    // 载入既有曲目到内存索引
    const auto all = repo_->load_all_tracks();
    for (const auto& t : all) {
        library_->upsert(t);
    }
    // 恢复被监控目录
    watcher_->set_roots(repo_->watched_folders());
    return true;
}

void LibraryManager::add_watched_folder(const QString& path)
{
    const QString norm = utils::path::normalize_path(path);
    if (!QFileInfo::exists(norm)) {

        logger->warn("[LIBRARY] add_watched_folder: not exists: {}", norm);
        return;
    }
    repo_->add_watched_folder(norm);
    watcher_->set_roots(repo_->watched_folders());
    scan_folder(norm);
}

void LibraryManager::remove_watched_folder(const QString& path)
{
    const QString norm = utils::path::normalize_path(path);
    repo_->remove_watched_folder(norm);
    watcher_->set_roots(repo_->watched_folders());
    // 注意:不删除该目录下的曲目(播放列表可能仍引用,阶段 3 后由引用关系决定)
}

QStringList LibraryManager::watched_folders() const
{
    return repo_->watched_folders();
}

std::shared_ptr<const LibraryTrack> LibraryManager::track_by_id(TrackId id) const
{
    return library_->track_by_id(id);
}

std::shared_ptr<const LibraryTrack>
LibraryManager::track_by_path(const QString& normalized_path) const
{
    return library_->track_by_path(normalized_path);
}

const QHash<QString, std::shared_ptr<const LibraryTrack>>& LibraryManager::index() const
{
    return library_->index();
}

int LibraryManager::track_count() const
{
    return library_->track_count();
}

QVector<std::shared_ptr<const LibraryTrack>>
LibraryManager::search(const QString& keyword, SearchQueryMode mode, int limit) const
{
    // SQLite 行 → 值 → 共享引用(避免浏览模型等持有整份副本)
    const QVector<LibraryTrack> rows = repo_->search(keyword, mode, limit);
    QVector<std::shared_ptr<const LibraryTrack>> result;
    result.reserve(rows.size());
    for (const LibraryTrack& t : rows) {
        result.append(std::make_shared<const LibraryTrack>(t));
    }
    return result;
}

void LibraryManager::start_scan()
{
    const QStringList roots = repo_->watched_folders();
    if (roots.isEmpty()) {
        emit sgn_scan_progress(0, 0);
        emit sgn_scan_finished();
        return;
    }
    const LibrarySnapshot snapshot = make_snapshot();
    LibraryScanner* scanner        = scanner_;
    QMetaObject::invokeMethod(
        scanner, [roots, snapshot, scanner]() { scanner->start_scan(roots, snapshot); },
        Qt::QueuedConnection);
}

void LibraryManager::scan_folder(const QString& path)
{
    const QString norm = utils::path::normalize_path(path);
    QStringList roots{norm};
    const LibrarySnapshot snapshot = make_snapshot();
    LibraryScanner* scanner        = scanner_;
    QMetaObject::invokeMethod(
        scanner, [roots, snapshot, scanner]() { scanner->start_scan(roots, snapshot); },
        Qt::QueuedConnection);
}

void LibraryManager::apply_batch(const QVector<TrackUpdate>& batch)
{
    for (const auto& update : batch) {
        switch (update.change) {
        case TrackChange::Added:
        case TrackChange::Modified: {
            // 仅有内容变化才算"变化": 扫描器在文件 mtime/size 变化时就会上报,
            // 但元数据可能解析结果完全相同(如仅 touch 文件), 此时不应惊动订阅者。
            const auto cur          = library_->track_by_path(update.track.filepath);
            const bool meta_changed = (!cur || !(cur->meta == update.track.meta));
            const bool was_missing  = (cur && cur->missing);
            library_->upsert(update.track);
            repo_->upsert_track(update.track);
            if (meta_changed || was_missing) {
                ++scan_change_count_;
            }
            break;
        }
        case TrackChange::Missing: {
            // 缺失文件会在每次扫描中重复上报 → 仅当缺失状态真正翻转时计数
            const auto cur = library_->track_by_path(update.path);
            if (cur && !cur->missing) {
                library_->mark_missing(update.path, true);
                repo_->mark_missing(update.path, true);
                ++scan_change_count_;
            }
            break;
        }
        }
    }
}

void LibraryManager::on_scan_finished()
{
    emit sgn_scan_finished();
    // 无真实变化时不广播: 避免订阅者(播放列表/浏览视图)做无意义的全量重建
    if (scan_change_count_ > 0) {
        scan_change_count_ = 0;
        emit sgn_library_changed();
    }
}

LibrarySnapshot LibraryManager::make_snapshot() const
{
    LibrarySnapshot snap;
    const auto& idx = library_->index();
    for (auto it = idx.constBegin(); it != idx.constEnd(); ++it) {
        const auto& t = it.value();
        snap.insert(t->filepath, FileStamp{t->file_size, t->mtime, t->track_id});
    }
    return snap;
}
