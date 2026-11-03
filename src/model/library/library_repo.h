#pragma once

#include "core/search_types.h"
#include "model/library/library_track.h"

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
    ~LibraryRepo() override
    {
        this->close();
    };

    LibraryRepo(const LibraryRepo&)            = delete;
    LibraryRepo& operator=(const LibraryRepo&) = delete;
    LibraryRepo(LibraryRepo&&)                 = delete;
    LibraryRepo& operator=(LibraryRepo&&)      = delete;

    bool open(const QString& db_path);
    void close();
    [[nodiscard]] bool is_open() const;
    [[nodiscard]] QString last_error() const;

    // ---- watched folders ----
    bool add_watched_folder(const QString& path);
    bool remove_watched_folder(const QString& path);
    [[nodiscard]] QStringList watched_folders() const;

    // ---- tracks ----
    bool upsert_track(const LibraryTrack& track);
    bool mark_missing(const QString& normalized_path, bool missing);
    bool remove_track(const QString& normalized_path);
    [[nodiscard]] QVector<LibraryTrack> load_all_tracks() const;
    [[nodiscard]] int track_count() const;

    // FTS5 全文搜索(阶段 5 搜索后端;FTS5 不可用时返回空)
    [[nodiscard]] QVector<LibraryTrack> search(const QString& keyword, SearchQueryMode mode,
                                               int limit = 200) const;

private:
    bool exec_schema();
    [[nodiscard]] bool has_fts5() const;
    [[nodiscard]] static LibraryTrack row_to_track(const QSqlQuery& q);

    QSqlDatabase db_;
    QString conn_name_;
    QString last_error_;
    bool open_ = false;
};
