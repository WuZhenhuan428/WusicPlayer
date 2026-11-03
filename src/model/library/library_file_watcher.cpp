#include "library_file_watcher.h"

LibraryFileWatcher::LibraryFileWatcher(QObject* parent) : QObject(parent)
{
    connect(&watcher_, &QFileSystemWatcher::directoryChanged, this,
            &LibraryFileWatcher::on_directory_changed);

    reconcile_timer_.setInterval(30 * 60 * 1000); // 默认 30 分钟
    connect(&reconcile_timer_, &QTimer::timeout, this,
            &LibraryFileWatcher::sgn_reconcile_requested);
    reconcile_timer_.start();
}

void LibraryFileWatcher::set_roots(const QStringList& roots)
{
    if (!watcher_.directories().isEmpty()) {
        watcher_.removePaths(watcher_.directories());
    }
    if (!roots.isEmpty()) {
        watcher_.addPaths(roots);
    }
}

void LibraryFileWatcher::set_reconcile_interval(int seconds)
{
    reconcile_timer_.setInterval(seconds * 1000);
}

void LibraryFileWatcher::on_directory_changed(const QString& path)
{
    emit sgn_root_changed(path);
}
