#pragma once

#include "core/search_types.h"
#include "library_track.h"

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QVector>

/**
 * @brief SQLite 持久化:schema 管理、tracks / watched_folders CRUD、FTS5 同步。
 *
 * 纯持久化,不包含业务逻辑;必须与 LibraryManager 在同一线程(主线程)使用。
 */
class LibraryRepo : public QObject
{
    Q_OBJECT
public:
    explicit LibraryRepo(QObject* parent = nullptr);
    ~LibraryRepo() override;

    bool open(const QString& db_path);
    void close();
    bool is_open() const;
    QString last_error() const;

    // ---- watched folders ----
    bool add_watched_folder(const QString& path);
    bool remove_watched_folder(const QString& path);
    QStringList watched_folders() const;

    // ---- tracks ----
    bool upsert_track(const LibraryTrack& track);
    bool mark_missing(const QString& normalized_path, bool missing);
    bool remove_track(const QString& normalized_path);
    QVector<LibraryTrack> load_all_tracks() const;
    int track_count() const;

    // FTS5 全文搜索(阶段 5 搜索后端;FTS5 不可用时返回空)
    QVector<LibraryTrack> search(const QString& keyword, SearchQueryMode mode,
                                 int limit = 200) const;

private:
    bool exec_schema();
    bool has_fts5() const;
    LibraryTrack row_to_track(const QSqlQuery& q) const;

    QSqlDatabase m_db;
    QString m_conn_name;
    QString m_last_error;
    bool m_open = false;
};
