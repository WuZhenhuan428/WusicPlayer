#include "library_manager.h"

#include "core/utils/path.hpp"

#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>

LibraryManager::LibraryManager(QObject* parent) : QObject(parent)
{
    qRegisterMetaType<TrackUpdate>("TrackUpdate");
    qRegisterMetaType<QVector<TrackUpdate>>("QVector<TrackUpdate>");

    m_library = std::make_unique<Library>();
    m_repo    = std::make_unique<LibraryRepo>();

    // 扫描器移入独立 worker 线程;线程结束时 deleteLater
    m_scanner = new LibraryScanner;
    m_worker  = new QThread(this);
    m_scanner->moveToThread(m_worker);
    connect(m_worker, &QThread::finished, m_scanner, &QObject::deleteLater);
    m_worker->start();

    connect(m_scanner, &LibraryScanner::sgn_progress, this, &LibraryManager::sgn_scan_progress);
    connect(m_scanner, &LibraryScanner::sgn_batch_ready, this, &LibraryManager::apply_batch);
    connect(m_scanner, &LibraryScanner::sgn_finished, this, &LibraryManager::on_scan_finished);

    m_watcher = std::make_unique<LibraryFileWatcher>();
    connect(m_watcher.get(), &LibraryFileWatcher::sgn_root_changed, this,
            &LibraryManager::scan_folder);
    connect(m_watcher.get(), &LibraryFileWatcher::sgn_reconcile_requested, this,
            &LibraryManager::start_scan);
}

LibraryManager::~LibraryManager()
{
    if (m_worker) {
        m_worker->quit();
        m_worker->wait();
    }
}

bool LibraryManager::initialize(const QString& db_path)
{
    if (!m_repo->open(db_path)) {
        qWarning() << "[LIBRARY] initialize failed:" << m_repo->last_error();
        return false;
    }
    // 载入既有曲目到内存索引
    const auto all = m_repo->load_all_tracks();
    for (const auto& t : all) {
        m_library->upsert(t);
    }
    // 恢复被监控目录
    m_watcher->set_roots(m_repo->watched_folders());
    return true;
}

void LibraryManager::add_watched_folder(const QString& path)
{
    const QString norm = utils::path::normalize_path(path);
    if (!QFileInfo::exists(norm)) {
        qWarning() << "[LIBRARY] add_watched_folder: not exists:" << norm;
        return;
    }
    m_repo->add_watched_folder(norm);
    m_watcher->set_roots(m_repo->watched_folders());
    scan_folder(norm);
}

void LibraryManager::remove_watched_folder(const QString& path)
{
    const QString norm = utils::path::normalize_path(path);
    m_repo->remove_watched_folder(norm);
    m_watcher->set_roots(m_repo->watched_folders());
    // 注意:不删除该目录下的曲目(播放列表可能仍引用,阶段 3 后由引用关系决定)
}

QStringList LibraryManager::watched_folders() const
{
    return m_repo->watched_folders();
}

std::optional<LibraryTrack> LibraryManager::track_by_id(TrackId id) const
{
    return m_library->track_by_id(id);
}

std::optional<LibraryTrack> LibraryManager::track_by_path(const QString& normalized_path) const
{
    return m_library->track_by_path(normalized_path);
}

const QHash<QString, LibraryTrack>& LibraryManager::index() const
{
    return m_library->index();
}

int LibraryManager::track_count() const
{
    return m_library->track_count();
}

QVector<LibraryTrack> LibraryManager::search(const QString& keyword, SearchQueryMode mode,
                                             int limit) const
{
    return m_repo->search(keyword, mode, limit);
}

void LibraryManager::start_scan()
{
    const QStringList roots = m_repo->watched_folders();
    if (roots.isEmpty()) {
        emit sgn_scan_progress(0, 0);
        emit sgn_scan_finished();
        return;
    }
    const LibrarySnapshot snapshot = make_snapshot();
    LibraryScanner* scanner        = m_scanner;
    QMetaObject::invokeMethod(
        scanner, [roots, snapshot, scanner]() { scanner->start_scan(roots, snapshot); },
        Qt::QueuedConnection);
}

void LibraryManager::scan_folder(const QString& path)
{
    const QString norm = utils::path::normalize_path(path);
    QStringList roots{norm};
    const LibrarySnapshot snapshot = make_snapshot();
    LibraryScanner* scanner        = m_scanner;
    QMetaObject::invokeMethod(
        scanner, [roots, snapshot, scanner]() { scanner->start_scan(roots, snapshot); },
        Qt::QueuedConnection);
}

void LibraryManager::apply_batch(const QVector<TrackUpdate>& batch)
{
    for (const auto& update : batch) {
        switch (update.change) {
        case TrackChange::added:
        case TrackChange::modified:
            m_library->upsert(update.track);
            m_repo->upsert_track(update.track);
            break;
        case TrackChange::missing:
            m_library->mark_missing(update.path, true);
            m_repo->mark_missing(update.path, true);
            break;
        }
    }
}

void LibraryManager::on_scan_finished()
{
    emit sgn_scan_finished();
    emit sgn_library_changed();
}

LibrarySnapshot LibraryManager::make_snapshot() const
{
    LibrarySnapshot snap;
    const auto& idx = m_library->index();
    for (auto it = idx.constBegin(); it != idx.constEnd(); ++it) {
        const LibraryTrack& t = it.value();
        snap.insert(t.filepath, FileStamp{t.file_size, t.mtime, t.track_id});
    }
    return snap;
}
