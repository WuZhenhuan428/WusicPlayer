#include "library_repo.h"

#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "core/logger/logger_manager.h"
namespace
{
Logger* logger = LoggerManager::file_logger("library_repo", {"console", "gui"});
}

namespace
{
constexpr const char* kCreateWatchedFolders =
    "CREATE TABLE IF NOT EXISTS watched_folders (path TEXT PRIMARY KEY)";

constexpr const char* kCreateTracks = "CREATE TABLE IF NOT EXISTS tracks ("
                                      "  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
                                      "  track_id     TEXT NOT NULL UNIQUE,"
                                      "  path         TEXT NOT NULL UNIQUE,"
                                      "  file_size    INTEGER NOT NULL DEFAULT 0,"
                                      "  mtime        INTEGER NOT NULL DEFAULT 0,"
                                      "  duration_ms  INTEGER NOT NULL DEFAULT 0,"
                                      "  missing      INTEGER NOT NULL DEFAULT 0,"
                                      "  artist       TEXT DEFAULT '',"
                                      "  title        TEXT DEFAULT '',"
                                      "  album        TEXT DEFAULT '',"
                                      "  album_artist TEXT DEFAULT '',"
                                      "  genre        TEXT DEFAULT '',"
                                      "  composer     TEXT DEFAULT '',"
                                      "  year         INTEGER DEFAULT 0,"
                                      "  track_number INTEGER DEFAULT 0,"
                                      "  disc_number  INTEGER DEFAULT 0,"
                                      "  disc_total   INTEGER DEFAULT 0,"
                                      "  bitrate      INTEGER DEFAULT 0,"
                                      "  comment      TEXT DEFAULT '',"
                                      "  lyrics       TEXT DEFAULT '',"
                                      "  encoder      TEXT DEFAULT '',"
                                      "  date         TEXT DEFAULT '')";

constexpr const char* kCreateIndexArtist =
    "CREATE INDEX IF NOT EXISTS idx_tracks_artist ON tracks(artist)";
constexpr const char* kCreateIndexTitle =
    "CREATE INDEX IF NOT EXISTS idx_tracks_title ON tracks(title)";
constexpr const char* kCreateIndexAlbum =
    "CREATE INDEX IF NOT EXISTS idx_tracks_album ON tracks(album)";
constexpr const char* kCreateIndexMissing =
    "CREATE INDEX IF NOT EXISTS idx_tracks_missing ON tracks(missing)";

constexpr const char* kCreateFts = "CREATE VIRTUAL TABLE IF NOT EXISTS tracks_fts USING fts5("
                                   "  title, artist, album, album_artist, genre,"
                                   "  content='tracks', content_rowid='id')";

constexpr const char* kTriggerAi =
    "CREATE TRIGGER IF NOT EXISTS tracks_ai AFTER INSERT ON tracks BEGIN"
    "  INSERT INTO tracks_fts(rowid, title, artist, album, album_artist, genre)"
    "  VALUES (new.id, new.title, new.artist, new.album, new.album_artist, new.genre);"
    "END";

constexpr const char* kTriggerAd =
    "CREATE TRIGGER IF NOT EXISTS tracks_ad AFTER DELETE ON tracks BEGIN"
    "  INSERT INTO tracks_fts(tracks_fts, rowid, title, artist, album, album_artist, genre)"
    "  VALUES ('delete', old.id, old.title, old.artist, old.album, old.album_artist, old.genre);"
    "END";

constexpr const char* kTriggerAu =
    "CREATE TRIGGER IF NOT EXISTS tracks_au AFTER UPDATE ON tracks BEGIN"
    "  INSERT INTO tracks_fts(tracks_fts, rowid, title, artist, album, album_artist, genre)"
    "  VALUES ('delete', old.id, old.title, old.artist, old.album, old.album_artist, old.genre);"
    "  INSERT INTO tracks_fts(rowid, title, artist, album, album_artist, genre)"
    "  VALUES (new.id, new.title, new.artist, new.album, new.album_artist, new.genre);"
    "END";
} // namespace

LibraryRepo::LibraryRepo(QObject* parent) : QObject(parent)
{
    static int s_conn_counter = 0;
    m_conn_name               = QStringLiteral("wusic_library_%1").arg(++s_conn_counter);
}

LibraryRepo::~LibraryRepo()
{
    close();
}

bool LibraryRepo::open(const QString& db_path)
{
    if (m_open) {
        return true;
    }
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_conn_name);
    m_db.setDatabaseName(db_path);
    if (!m_db.open()) {
        m_last_error = m_db.lastError().text();
        logger->warn("[LIBRARY] cannot open database: {} {}", db_path, m_last_error);
        return false;
    }
    m_open = true;
    return exec_schema();
}

void LibraryRepo::close()
{
    if (!m_open) {
        return;
    }
    m_db.close();
    m_db = QSqlDatabase(); // 释放连接引用后再移除
    QSqlDatabase::removeDatabase(m_conn_name);
    m_open = false;
}

bool LibraryRepo::is_open() const
{
    return m_open;
}

QString LibraryRepo::last_error() const
{
    return m_last_error;
}

bool LibraryRepo::exec_schema()
{
    const QStringList base = {kCreateWatchedFolders, kCreateTracks,     kCreateIndexArtist,
                              kCreateIndexTitle,     kCreateIndexAlbum, kCreateIndexMissing};
    for (const auto& sql : base) {
        QSqlQuery q(m_db);
        if (!q.exec(sql)) {
            m_last_error = q.lastError().text();
            logger->warn("[LIBRARY] schema exec failed: {}", m_last_error);
            return false;
        }
    }
    if (has_fts5()) {
        const QStringList fts = {kCreateFts, kTriggerAi, kTriggerAd, kTriggerAu};
        for (const auto& sql : fts) {
            QSqlQuery q(m_db);
            if (!q.exec(sql)) {
                m_last_error = q.lastError().text();
                logger->warn("[LIBRARY] FTS5 setup failed: {}", m_last_error);
                return false;
            }
        }
    } else {
        logger->warn("[LIBRARY] FTS5 not available in bundled SQLite; search disabled.");
    }
    return true;
}

bool LibraryRepo::has_fts5() const
{
    QSqlQuery q(m_db);
    if (!q.exec("SELECT sqlite_compileoption_used('ENABLE_FTS5')")) {
        return false;
    }
    return q.next() && q.value(0).toInt() == 1;
}

bool LibraryRepo::add_watched_folder(const QString& path)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT OR IGNORE INTO watched_folders(path) VALUES (?)");
    q.addBindValue(path);
    return q.exec();
}

bool LibraryRepo::remove_watched_folder(const QString& path)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM watched_folders WHERE path=?");
    q.addBindValue(path);
    return q.exec();
}

QStringList LibraryRepo::watched_folders() const
{
    QStringList result;
    QSqlQuery q(m_db);
    if (q.exec("SELECT path FROM watched_folders ORDER BY path")) {
        while (q.next()) {
            result << q.value(0).toString();
        }
    }
    return result;
}

bool LibraryRepo::upsert_track(const LibraryTrack& track)
{
    if (!m_open) {
        return false;
    }
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT INTO tracks (track_id, path, file_size, mtime, duration_ms, missing,"
        "  artist, title, album, album_artist, genre, composer, year, track_number,"
        "  disc_number, disc_total, bitrate, comment, lyrics, encoder, date)"
        " VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
        " ON CONFLICT(path) DO UPDATE SET"
        "  track_id=excluded.track_id, file_size=excluded.file_size, mtime=excluded.mtime,"
        "  duration_ms=excluded.duration_ms, missing=excluded.missing,"
        "  artist=excluded.artist, title=excluded.title, album=excluded.album,"
        "  album_artist=excluded.album_artist, genre=excluded.genre, composer=excluded.composer,"
        "  year=excluded.year, track_number=excluded.track_number,"
        "  disc_number=excluded.disc_number, disc_total=excluded.disc_total,"
        "  bitrate=excluded.bitrate, comment=excluded.comment, lyrics=excluded.lyrics,"
        "  encoder=excluded.encoder, date=excluded.date");
    const auto& m = track.meta;
    q.addBindValue(track.track_id.toString(QUuid::WithoutBraces));
    q.addBindValue(track.filepath);
    q.addBindValue(track.file_size);
    q.addBindValue(track.mtime);
    q.addBindValue(track.duration_ms);
    q.addBindValue(track.missing ? 1 : 0);
    q.addBindValue(m.artist);
    q.addBindValue(m.title);
    q.addBindValue(m.album);
    q.addBindValue(m.album_artist);
    q.addBindValue(m.genre);
    q.addBindValue(m.composer);
    q.addBindValue(m.year);
    q.addBindValue(m.track_number);
    q.addBindValue(m.disc_number);
    q.addBindValue(m.disc_total);
    q.addBindValue(m.bitrate);
    q.addBindValue(m.comment);
    q.addBindValue(m.lyrics);
    q.addBindValue(m.encoder);
    q.addBindValue(m.date);
    if (!q.exec()) {
        m_last_error = q.lastError().text();
        logger->warn("[LIBRARY] upsert_track failed: {}", m_last_error);
        return false;
    }
    return true;
}

bool LibraryRepo::mark_missing(const QString& normalized_path, bool missing)
{
    QSqlQuery q(m_db);
    q.prepare("UPDATE tracks SET missing=? WHERE path=?");
    q.addBindValue(missing ? 1 : 0);
    q.addBindValue(normalized_path);
    return q.exec();
}

bool LibraryRepo::remove_track(const QString& normalized_path)
{
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM tracks WHERE path=?");
    q.addBindValue(normalized_path);
    return q.exec();
}

QVector<LibraryTrack> LibraryRepo::load_all_tracks() const
{
    QVector<LibraryTrack> result;
    QSqlQuery q(m_db);
    if (!q.exec("SELECT * FROM tracks")) {
        return result;
    }
    while (q.next()) {
        result.append(row_to_track(q));
    }
    return result;
}

int LibraryRepo::track_count() const
{
    QSqlQuery q(m_db);
    if (!q.exec("SELECT COUNT(*) FROM tracks")) {
        return 0;
    }
    return q.next() ? q.value(0).toInt() : 0;
}

namespace
{
// 按空白(含全角空格等)切分搜索词;QChar::isSpace 覆盖 U+3000
QStringList split_search_tokens(const QString& text)
{
    QStringList tokens;
    QString cur;
    for (const QChar& c : text) {
        if (c.isSpace()) {
            if (!cur.isEmpty()) {
                tokens.push_back(cur);
                cur.clear();
            }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.isEmpty()) {
        tokens.push_back(cur);
    }
    return tokens;
}
} // namespace

QVector<LibraryTrack> LibraryRepo::search(const QString& keyword,
                                          [[maybe_unused]] SearchQueryMode mode, int limit) const
{
    QVector<LibraryTrack> result;
    const QString keyword_trimmed = keyword.trimmed();
    if (keyword_trimmed.isEmpty() || !has_fts5()) {
        return result;
    }
    const QStringList tokens = split_search_tokens(keyword_trimmed);
    if (tokens.isEmpty()) {
        return result;
    }

    // 1) FTS5 前缀优先:每个 token 加 * 并 AND(unicode61 下 CJK 连续为单 token,前缀命中开头)
    QStringList prefix_terms;
    for (const QString& tok : tokens) {
        QString e = tok;
        e.replace('"', "\"\"");
        prefix_terms.push_back(QStringLiteral("\"%1\"*").arg(e));
    }
    QSqlQuery q(m_db);
    q.prepare("SELECT t.* FROM tracks_fts f JOIN tracks t ON t.id = f.rowid"
              " WHERE tracks_fts MATCH ? ORDER BY bm25(tracks_fts) LIMIT ?");
    q.addBindValue(prefix_terms.join(QLatin1Char(' ')));
    q.addBindValue(limit);
    if (q.exec()) {
        while (q.next()) {
            result.append(row_to_track(q));
        }
    }
    // FTS5 命中即返回(前缀已满足多数场景)
    if (!result.isEmpty()) {
        return result;
    }

    // 2) LIKE 子串兜底:每个 token 在任一文本列子串命中(token 间 AND,列间 OR)
    QStringList where_clauses;
    QStringList like_binds;
    for (const QString& tok : tokens) {
        QString like = tok;
        like.replace('\\', "\\\\").replace('%', "\\%").replace('_', "\\_");
        const QString pattern = QStringLiteral("%%1%").arg(like);
        QStringList col_clauses;
        for (const char* col : {"title", "artist", "album", "album_artist", "genre"}) {
            col_clauses.push_back(QStringLiteral("%1 LIKE ? ESCAPE '\\'").arg(col));
            like_binds.push_back(pattern);
        }
        where_clauses.push_back(QStringLiteral("(%1)").arg(col_clauses.join(" OR ")));
    }
    QSqlQuery q2(m_db);
    q2.prepare(QStringLiteral("SELECT t.* FROM tracks t WHERE %1 ORDER BY t.title LIMIT ?")
                   .arg(where_clauses.join(" AND ")));
    for (const QString& pattern : like_binds) {
        q2.addBindValue(pattern);
    }
    q2.addBindValue(limit);
    if (q2.exec()) {
        while (q2.next()) {
            result.append(row_to_track(q2));
        }
    }
    return result;
}

LibraryTrack LibraryRepo::row_to_track(const QSqlQuery& q) const
{
    LibraryTrack t;
    t.track_id          = TrackId(q.value("track_id").toString());
    t.filepath          = q.value("path").toString();
    t.file_size         = q.value("file_size").toLongLong();
    t.mtime             = q.value("mtime").toLongLong();
    t.duration_ms       = q.value("duration_ms").toInt();
    t.missing           = q.value("missing").toInt() != 0;
    t.meta.artist       = q.value("artist").toString();
    t.meta.title        = q.value("title").toString();
    t.meta.album        = q.value("album").toString();
    t.meta.album_artist = q.value("album_artist").toString();
    t.meta.genre        = q.value("genre").toString();
    t.meta.composer     = q.value("composer").toString();
    t.meta.year         = q.value("year").toInt();
    t.meta.track_number = q.value("track_number").toInt();
    t.meta.disc_number  = q.value("disc_number").toInt();
    t.meta.disc_total   = q.value("disc_total").toInt();
    t.meta.bitrate      = q.value("bitrate").toInt();
    t.meta.comment      = q.value("comment").toString();
    t.meta.lyrics       = q.value("lyrics").toString();
    t.meta.encoder      = q.value("encoder").toString();
    t.meta.date         = q.value("date").toString();
    t.meta.filepath     = t.filepath;
    t.meta.filename     = QFileInfo(t.filepath).fileName();
    t.meta.duration_s   = t.duration_ms / 1000;
    t.meta.start_at     = 0;
    t.meta.isValid      = true;
    return t;
}
