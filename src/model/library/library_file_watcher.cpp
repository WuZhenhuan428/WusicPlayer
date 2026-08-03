#include "library_file_watcher.h"

LibraryFileWatcher::LibraryFileWatcher(QObject* parent) : QObject(parent)
{
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this,
            &LibraryFileWatcher::on_directory_changed);

    m_reconcile_timer.setInterval(30 * 60 * 1000); // 默认 30 分钟
    connect(&m_reconcile_timer, &QTimer::timeout, this,
            &LibraryFileWatcher::sgn_reconcile_requested);
    m_reconcile_timer.start();
}

void LibraryFileWatcher::set_roots(const QStringList& roots)
{
    if (!m_watcher.directories().isEmpty()) {
        m_watcher.removePaths(m_watcher.directories());
    }
    if (!roots.isEmpty()) {
        m_watcher.addPaths(roots);
    }
}

void LibraryFileWatcher::set_reconcile_interval(int seconds)
{
    m_reconcile_timer.setInterval(seconds * 1000);
}

void LibraryFileWatcher::on_directory_changed(const QString& path)
{
    emit sgn_root_changed(path);
}
