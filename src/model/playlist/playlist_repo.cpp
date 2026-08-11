#include "playlist_repo.h"

#include <algorithm>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonValueRef>
#include <QPointer>
#include <QStandardPaths>
#include <QTextStream>
#include <QThread>
#include <QTimer>

#include "core/logger/log.h"

WUSIC_LOG_MODULE(playlist_repo)

static QJsonObject metaToJson(const TrackMetaData& meta)
{
    QJsonObject obj;
    obj["album"]        = meta.album;
    obj["album_artist"] = meta.album_artist;
    obj["artist"]       = meta.artist;
    obj["bitrate"]      = meta.bitrate;
    obj["comment"]      = meta.comment;
    obj["composer"]     = meta.composer;
    obj["date"]         = meta.date;
    obj["disc_number"]  = meta.disc_number;
    obj["disc_total"]   = meta.disc_total;
    obj["duration_s"]   = meta.duration_s;
    obj["encoder"]      = meta.encoder;
    obj["filepath"]     = meta.filepath;
    obj["filename"]     = meta.filename;
    obj["genre"]        = meta.genre;
    obj["lyrics"]       = meta.lyrics;
    obj["start_at"]     = meta.start_at;
    obj["title"]        = meta.title;
    obj["track_number"] = meta.track_number;
    obj["year"]         = meta.year;
    return obj;
}

static void applyJsonToMeta(const QJsonObject& obj, TrackMetaData& meta)
{
    meta.album        = obj.value("album").toString(meta.album);
    meta.album_artist = obj.value("album_artist").toString(meta.album_artist);
    meta.artist       = obj.value("artist").toString(meta.artist);
    meta.bitrate      = obj.value("bitrate").toInt(meta.bitrate);
    meta.comment      = obj.value("comment").toString(meta.comment);
    meta.composer     = obj.value("composer").toString(meta.composer);
    meta.date         = obj.value("date").toString(meta.date);
    meta.disc_number  = obj.value("disc_number").toInt(meta.disc_number);
    meta.disc_total   = obj.value("disc_total").toInt(meta.disc_total);
    meta.duration_s   = obj.value("duration_s").toInt(meta.duration_s);
    meta.encoder      = obj.value("encoder").toString(meta.encoder);
    meta.filepath     = obj.value("filepath").toString(meta.filepath);
    meta.filename     = obj.value("filename").toString(meta.filename);
    meta.genre        = obj.value("genre").toString(meta.genre);
    meta.lyrics       = obj.value("lyrics").toString(meta.lyrics);
    meta.start_at     = obj.value("start_at").toInt(meta.start_at);
    meta.title        = obj.value("title").toString(meta.title);
    meta.track_number = obj.value("track_number").toInt(meta.track_number);
    meta.year         = obj.value("year").toInt(meta.year);
}

static QString resolvePlaylistsCacheDir()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) {
        return QString();
    }

    QDir baseDir(base);
    baseDir.mkpath(".");

    const QString playlists_dir = baseDir.filePath("playlists");
    QDir(playlists_dir).mkpath(".");

    return playlists_dir;
}

PlaylistRepo::PlaylistRepo(QObject* parent) : QObject(parent)
{
    m_cache_dir = resolvePlaylistsCacheDir();
}

PlaylistRepo::~PlaylistRepo() {}

void PlaylistRepo::clear_list()
{
    m_list.clear();
}

void PlaylistRepo::save_list_to_cache(std::shared_ptr<Playlist> playlist)
{
    if (!playlist) {
        return;
    }
    if (m_cache_dir.isEmpty()) {
        WUSIC_LOG(playlist_repo, warn, "[WARNING] Cache dir is empty, skip saving playlist.");
        return;
    }

    QFile file(cache_file_path(playlist->id()));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        WUSIC_LOG(playlist_repo, warn, "[WARNING] Failed to open {} for cache saving.",
                  file.fileName());
        return;
    }
    if (!write_json_playlist(file, playlist)) {
        WUSIC_LOG(playlist_repo, warn, "[WARNING] Failed to write cache file: {}", file.fileName());
    }
}

void PlaylistRepo::load_cache()
{
    emit sgn_cache_load_started();
    load_cache_from_disk();
    emit sgn_cache_load_finished(m_list.size());
}

void PlaylistRepo::load_cache_async()
{
    QPointer<PlaylistRepo> self(this);
    emit sgn_cache_load_started();
    QThread* worker = QThread::create([self]() {
        if (!self) {
            return;
        }
        auto loaded = self->load_cache_from_disk_to_vector();
        if (!self) {
            return;
        }
        QMetaObject::invokeMethod(
            self,
            [self, loaded = std::move(loaded)]() mutable {
                if (!self) {
                    return;
                }
                QVector<std::shared_ptr<Playlist>> lists;
                lists.reserve(loaded.size());
                for (const auto& entry : loaded) {
                    lists.append(entry.first);
                    if (entry.second) {
                        self->save_list_to_cache(entry.first); // 旧格式重写为新格式
                    }
                }
                if (lists.isEmpty()) {
                    emit self->sgn_cache_load_finished(self->m_list.size());
                    return;
                }
                self->m_list += lists;
                emit self->sgn_playlist_changed();
                emit self->sgn_cache_load_finished(self->m_list.size());
            },
            Qt::QueuedConnection);
    });
    QObject::connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

QString PlaylistRepo::cache_file_path(const PlaylistId& pid) const
{
    QDir dir(m_cache_dir);
    return dir.filePath(pid.toString(PlaylistId::WithoutBraces) + ".wcpl");
}

bool PlaylistRepo::write_json_playlist(QIODevice& device,
                                       const std::shared_ptr<Playlist>& playlist) const
{
    if (!playlist) {
        return false;
    }

    QJsonObject root;
    root["schemaVersion"] = kSchemaVersion;
    root["id"]            = playlist->id().toString(QUuid::WithoutBraces);
    root["name"]          = playlist->name();

    QJsonArray tracks;
    const auto& list = playlist->get_tracks();
    for (const auto& track : list) {
        QJsonObject t;
        t["entry_id"]         = track.entry_id.toString(QUuid::WithoutBraces);
        t["library_track_id"] = track.library_track_id.toString(QUuid::WithoutBraces);
        t["source"]           = track.source == TrackSource::library ? "library" : "external";
        t["filepath"]         = track.filepath;
        t["missing"]          = track.missing;
        if (track.meta.isValid) {
            t["meta"] = metaToJson(track.meta);
        }
        tracks.append(t);
    }
    root["tracks"] = tracks;

    QJsonDocument doc(root);
    device.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

bool PlaylistRepo::load_json_playlist(const QByteArray& data, const QString& fallbackName,
                                      std::shared_ptr<Playlist>& out_playlist,
                                      bool* out_legacy_format) const
{
    if (out_legacy_format) {
        *out_legacy_format = false;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    QJsonObject root       = doc.object();
    QJsonValue tracksValue = root.value("tracks");
    if (!tracksValue.isArray()) {
        return false;
    }

    QJsonValue idValue = root.value("id");
    PlaylistId pid     = PlaylistId(idValue.toString());
    if (pid.isNull()) {
        pid = PlaylistId::createUuid();
    }

    QString name = root.value("name").toString(fallbackName);
    out_playlist->new_uuid(pid);
    out_playlist->set_playlist_name(name);

    QJsonArray tracks = tracksValue.toArray();
    for (const auto& item : tracks) {
        if (!item.isObject()) {
            continue;
        }
        QJsonObject obj  = item.toObject();
        QString filepath = obj.value("filepath").toString();
        if (filepath.isEmpty()) {
            continue;
        }
        EntryId tid = EntryId(obj.value("entry_id").toString());
        if (tid.isNull()) {
            // 旧格式回退:阶段 1 前字段名为 "id",语义即条目身份,复用保持身份稳定
            tid = EntryId(obj.value("id").toString());
        }
        if (obj.contains("id") && !obj.contains("entry_id") && out_legacy_format) {
            *out_legacy_format = true;
        }
        Track t  = tid.isNull() ? Track::from_filepath(filepath) : Track::from_entry(tid, filepath);
        t.source = obj.value("source").toString() == "library" ? TrackSource::library
                                                               : TrackSource::external;
        t.library_track_id = TrackId(obj.value("library_track_id").toString());
        t.missing          = obj.value("missing").toBool(false);
        out_playlist->add_track_object(t);

        QJsonValue metaValue = obj.value("meta");
        if (metaValue.isObject()) {
            TrackMetaData meta;
            meta.filepath = filepath;
            meta.filename = QFileInfo(filepath).fileName();
            applyJsonToMeta(metaValue.toObject(), meta);
            meta.isValid = true;
            out_playlist->update_track_meta(t.entry_id, meta);
        }
    }

    return true;
}

void PlaylistRepo::load_cache_from_disk()
{
    const auto loaded = load_cache_from_disk_to_vector();
    QVector<std::shared_ptr<Playlist>> lists;
    lists.reserve(loaded.size());
    for (const auto& entry : loaded) {
        lists.append(entry.first);
        if (entry.second) {
            save_list_to_cache(entry.first); // 旧格式一次性重写为新格式,稳定条目身份
        }
    }
    if (!lists.isEmpty()) {
        m_list += lists;
        emit sgn_playlist_changed();
    }
}

QVector<std::pair<std::shared_ptr<Playlist>, bool>>
PlaylistRepo::load_cache_from_disk_to_vector() const
{
    QVector<std::pair<std::shared_ptr<Playlist>, bool>> loaded;
    if (m_cache_dir.isEmpty()) {
        return loaded;
    }

    QDir dir(m_cache_dir);
    if (!dir.exists()) {
        return loaded;
    }

    QStringList files = dir.entryList(QStringList() << "*.wcpl", QDir::Files);
    if (files.isEmpty()) {
        return loaded;
    }

    loaded.reserve(files.size());

    for (const auto& filename : files) {
        QString filepath = dir.filePath(filename);
        QFile file(filepath);
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }

        QByteArray data = file.readAll();
        auto playlist   = std::make_shared<Playlist>();
        QFileInfo fileInfo(filepath);
        QString fallbackName = fileInfo.baseName();
        bool legacy_format   = false;
        if (!load_json_playlist(data, fallbackName, playlist, &legacy_format)) {
            continue;
        }
        loaded.push_back({playlist, legacy_format});
    }

    return loaded;
}

PlaylistId PlaylistRepo::create_list()
{
    PlaylistId new_id = PlaylistId::createUuid();
    auto new_playlist = std::make_shared<Playlist>();
    new_playlist->new_uuid(new_id);
    QString default_name = QString("New playlist %1").arg(m_list.size() + 1);
    new_playlist->set_playlist_name(default_name);
    m_list.push_back(new_playlist);

    save_list_to_cache(new_playlist);

    emit sgn_playlist_changed();
    return new_id;
}

/* ---- load list from file ---- */
PlaylistId PlaylistRepo::load_list(const QString& filepath)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        WUSIC_LOG(playlist_repo, warn, "[WARNING] Failed to open file for loading: {}", filepath);
        return PlaylistId();
    }

    auto new_playlist = std::make_shared<Playlist>();
    QFileInfo fileInfo(filepath);
    QString fallbackName = fileInfo.baseName();

    QByteArray data      = file.readAll();
    if (!load_json_playlist(data, fallbackName, new_playlist)) {
        // old style: use filename as playlist
        file.seek(0);
        new_playlist->set_playlist_name(fallbackName);

        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                // todo: check wheteher the file path is valid
                new_playlist->add_track(line);
            }
        }
    }

    m_list.push_back(new_playlist);
    emit sgn_playlist_changed();
    WUSIC_LOG(playlist_repo, info, "[INFO] Loading playlist from: {}", filepath);
    return new_playlist->id();
}

PlaylistId PlaylistRepo::load_list_batched(const QString& filepath, int batch_size)
{
    if (batch_size <= 0) {
        batch_size = 500;
    }

    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        WUSIC_LOG(playlist_repo, warn, "[WARNING] Failed to open file for loading: {}", filepath);
        return PlaylistId();
    }

    struct LoadEntry
    {
        PlaylistId id;
        QString filepath;
        bool hasMeta = false;
        TrackMetaData meta;
        TrackSource source = TrackSource::external;
        TrackId library_track_id;
        bool missing = false;
    };

    QVector<LoadEntry> entries;
    entries.reserve(1024);

    auto new_playlist = std::make_shared<Playlist>();
    QFileInfo fileInfo(filepath);
    QString fallbackName = fileInfo.baseName();

    QByteArray data      = file.readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
        QJsonObject root       = doc.object();
        QJsonValue tracksValue = root.value("tracks");

        PlaylistId pid         = PlaylistId(root.value("id").toString());
        if (pid.isNull()) {
            pid = PlaylistId::createUuid();
        }

        QString name = root.value("name").toString(fallbackName);
        new_playlist->new_uuid(pid);
        new_playlist->set_playlist_name(name);

        if (tracksValue.isArray()) {
            QJsonArray tracks = tracksValue.toArray();
            entries.reserve(tracks.size());
            for (const auto& item : tracks) {
                if (!item.isObject()) {
                    continue;
                }
                QJsonObject obj   = item.toObject();
                QString trackPath = obj.value("filepath").toString();
                if (trackPath.isEmpty()) {
                    continue;
                }
                EntryId tid = EntryId(obj.value("entry_id").toString());
                if (tid.isNull()) {
                    // 旧格式回退:阶段 1 前字段名为 "id"
                    tid = EntryId(obj.value("id").toString());
                }
                LoadEntry entry;
                entry.id       = tid;
                entry.filepath = trackPath;
                entry.source   = obj.value("source").toString() == "library" ? TrackSource::library
                                                                             : TrackSource::external;
                entry.library_track_id = TrackId(obj.value("library_track_id").toString());
                entry.missing          = obj.value("missing").toBool(false);
                QJsonValue metaValue   = obj.value("meta");
                if (metaValue.isObject()) {
                    TrackMetaData meta;
                    meta.filepath = trackPath;
                    meta.filename = QFileInfo(trackPath).fileName();
                    applyJsonToMeta(metaValue.toObject(), meta);
                    meta.isValid  = true;
                    entry.hasMeta = true;
                    entry.meta    = meta;
                }
                entries.push_back(entry);
            }
        }
    } else {
        file.seek(0);
        new_playlist->set_playlist_name(fallbackName);

        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                LoadEntry entry;
                entry.filepath = line;
                entries.push_back(entry);
            }
        }
    }

    if (new_playlist->id().isNull()) {
        new_playlist->new_uuid(PlaylistId::createUuid());
    }

    m_list.push_back(new_playlist);
    emit sgn_playlist_changed();

    const PlaylistId pid = new_playlist->id();
    const int totalCount = entries.size();
    emit sgn_playlist_load_started(pid, totalCount);

    auto entriesPtr = std::make_shared<QVector<LoadEntry>>(std::move(entries));
    auto indexPtr   = std::make_shared<int>(0);

    QPointer<PlaylistRepo> self(this);
    std::shared_ptr<Playlist> playlistPtr = new_playlist;

    std::function<void()> processBatch;
    processBatch = [self, playlistPtr, entriesPtr, indexPtr, batch_size, totalCount, pid,
                    &processBatch]() mutable {
        if (!self) {
            return;
        }

        int start = *indexPtr;
        int end   = std::min(start + batch_size, totalCount);
        for (int i = start; i < end; ++i) {
            const LoadEntry& entry = entriesPtr->at(i);
            if (entry.filepath.isEmpty()) {
                continue;
            }
            Track t;
            if (entry.id.isNull()) {
                t = Track::from_filepath(entry.filepath);
            } else {
                t = Track::from_entry(entry.id, entry.filepath);
            }
            t.source           = entry.source;
            t.library_track_id = entry.library_track_id;
            t.missing          = entry.missing;
            playlistPtr->add_track_object(t);
            if (entry.hasMeta) {
                playlistPtr->update_track_meta(t.entry_id, entry.meta);
            }
        }

        *indexPtr = end;
        emit self->playlistBatchLoaded(pid, end, totalCount);

        if (end < totalCount) {
            QTimer::singleShot(0, self, processBatch);
            return;
        }

        self->save_list_to_cache(playlistPtr);
        emit self->sgn_playlist_load_finished(pid);
    };

    QTimer::singleShot(0, this, processBatch);

    WUSIC_LOG(playlist_repo, info, "[INFO] Loading playlist (batched) from: {} total: {}", filepath,
              totalCount);
    return pid;
}

/* ---- save list to file ---- */
void PlaylistRepo::save_list(const PlaylistId& pid, const QString& dst_path)
{
    std::shared_ptr<Playlist> src = find_playlist_by_id(pid);
    if (!src) {
        WUSIC_LOG(playlist_repo, warn, "[WARNING] Playlist ({}) not found, save failed.",
                  pid.toString());
        return;
    }

    QFile file(dst_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        WUSIC_LOG(playlist_repo, warn, "[WARNING] Failed to open file for saving: {}", dst_path);
        return;
    }

    if (!write_json_playlist(file, src)) {
        WUSIC_LOG(playlist_repo, warn, "[WARNING] Failed to write playlist file: {}", dst_path);
        return;
    }

    WUSIC_LOG(playlist_repo, info, "[INFO] Saved playlist to: {}", dst_path);
}

void PlaylistRepo::rename_list(const PlaylistId& pid, const QString& name)
{
    std::shared_ptr<Playlist> src = find_playlist_by_id(pid);
    if (!src) {
        WUSIC_LOG(playlist_repo, warn, "[WARNING] Playlist {} does not exist", pid.toString());
        return;
    }
    src->set_playlist_name(name);
    save_list_to_cache(src);
    emit sgn_playlist_changed();
}

void PlaylistRepo::remove_list(const PlaylistId& pid)
{
    std::shared_ptr<Playlist> src = find_playlist_by_id(pid);
    if (!src) {
        WUSIC_LOG(playlist_repo, warn, "[WARNING] Playlist {} not found", pid.toString());
        return;
    }
    m_list.removeOne(src);
    if (!m_cache_dir.isEmpty()) {
        QFile::remove(cache_file_path(pid));
    }
    emit sgn_playlist_changed();
}

void PlaylistRepo::reorder_lists(const QVector<PlaylistId>& ordered_ids)
{
    if (ordered_ids.size() != m_list.size()) {
        WUSIC_LOG(playlist_repo, warn, "[PlaylistRepo] reorder_lists: size mismatch {} vs {}",
                  ordered_ids.size(), m_list.size());
        return;
    }
    QVector<std::shared_ptr<Playlist>> new_order;
    new_order.reserve(ordered_ids.size());
    for (const PlaylistId& pid : ordered_ids) {
        auto it =
            std::find_if(m_list.begin(), m_list.end(),
                         [&pid](const std::shared_ptr<Playlist>& pl) { return pl->id() == pid; });
        if (it != m_list.end()) {
            new_order.push_back(*it);
        }
    }
    if (new_order.size() != m_list.size()) {
        WUSIC_LOG(playlist_repo, warn, "[PlaylistRepo] reorder_lists: dropped entries, abort");
        return;
    }
    m_list = std::move(new_order);
    emit sgn_playlist_changed();
}

/**
 * @note: this function means "copy-and-paste", but not copy only
 */
void PlaylistRepo::copy_list(const PlaylistId& src_uuid)
{
    std::shared_ptr<Playlist> src = find_playlist_by_id(src_uuid);

    if (!src) {
        WUSIC_LOG(playlist_repo, warn, "[WARNING] Source playlist {} not found",
                  src_uuid.toString());
        return;
    }

    // deep-copy
    auto new_playlist = std::make_shared<Playlist>(*src);
    new_playlist->new_uuid();
    m_list.push_back(new_playlist);

    save_list_to_cache(new_playlist);

    emit sgn_playlist_changed();
}

/**
 * @note: 如果emit过多，可以考虑将add_one_track包装为两个函数，分别在两个函数的末尾进行emit
 * sgn_playlist_changed();
 */
void PlaylistRepo::add_track_to_playlist(const PlaylistId& pid, const QString& filepath)
{
    std::shared_ptr<Playlist> src = find_playlist_by_id(pid);
    if (!src) {
        WUSIC_LOG(playlist_repo, warn, "[WARNING] Playlist id {} not found", pid.toString());
        return;
    }
    WUSIC_LOG(playlist_repo, info, "[INFO] Add track {} to {}", filepath, pid.toString());

    Track newTrack = src->add_track(filepath);
    save_list_to_cache(src);
    emit sgn_playlist_changed();
}

void PlaylistRepo::add_tracks_to_playlist(const PlaylistId& pid, const QStringList& filepaths)
{
    std::shared_ptr<Playlist> src = find_playlist_by_id(pid);
    if (!src) {
        WUSIC_LOG(playlist_repo, warn, "[WARNING] Playlist id {} not found", pid.toString());
        return;
    }
    WUSIC_LOG(playlist_repo, info, "[INFO] Add {} tracks to {}", filepaths.size(), pid.toString());

    for (const auto& filepath : filepaths) {
        src->add_track(filepath);
    }
    save_list_to_cache(src);
    emit sgn_playlist_changed();
}

void PlaylistRepo::add_track_object(const PlaylistId& pid, const Track& track)
{
    std::shared_ptr<Playlist> src = find_playlist_by_id(pid);
    if (!src) {
        WUSIC_LOG(playlist_repo, warn, "[WARNING] Playlist id {} not found", pid.toString());
        return;
    }
    src->add_track_object(track);
    save_list_to_cache(src);
    emit sgn_playlist_changed();
}

void PlaylistRepo::add_track_objects(const PlaylistId& pid, const QVector<Track>& tracks)
{
    std::shared_ptr<Playlist> src = find_playlist_by_id(pid);
    if (!src) {
        WUSIC_LOG(playlist_repo, warn, "[WARNING] Playlist id {} not found", pid.toString());
        return;
    }
    for (const auto& track : tracks) {
        src->add_track_object(track);
    }
    save_list_to_cache(src);
    emit sgn_playlist_changed();
}

bool PlaylistRepo::isEmpty()
{
    m_list.shrink_to_fit();
    if (m_list.size() == 0) {
        return true;
    }
    return false;
}

std::shared_ptr<Playlist> PlaylistRepo::find_playlist_by_id(const PlaylistId& pid)
{
    if (pid.isNull())
        return nullptr;

    for (const auto& it : m_list) {
        if (it->id() == pid) {
            return it;
        }
    }
    WUSIC_LOG(playlist_repo, warn, "[WARNING] Playlist does not exist, UUID={}", pid.toString());
    return nullptr;
}

const QVector<std::shared_ptr<Playlist>>& PlaylistRepo::get_lists()
{
    return m_list;
}
