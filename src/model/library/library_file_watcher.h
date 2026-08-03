#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>

/**
 * @brief 文件监控:QFileSystemWatcher(根目录顶层)+ reconcile 定时扫描。
 *
 * 受 inotify 限制只监控根目录顶层;子目录变化由 reconcile 定时兜底。
 */
class LibraryFileWatcher : public QObject
{
    Q_OBJECT
public:
    explicit LibraryFileWatcher(QObject* parent = nullptr);

    void set_roots(const QStringList& roots);
    void set_reconcile_interval(int seconds);

signals:
    void sgn_root_changed(const QString& root);
    void sgn_reconcile_requested();

private:
    void on_directory_changed(const QString& path);

    QFileSystemWatcher m_watcher;
    QTimer m_reconcile_timer;
};
