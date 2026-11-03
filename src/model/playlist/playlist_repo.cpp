#include "playlist_repo.h"

#include <ranges>

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

#include "core/logger/logger_manager.h"
namespace
{
// NOLINTNEXTLINE
Logger* logger = LoggerManager::file_logger("playlist_repo", {"console", "gui"});
} // namespace

static QJsonObject meta_to_json(const TrackMetaData& meta)
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

static void apply_json_to_meta(const QJsonObject& obj, TrackMetaData& meta)
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

static QString resolve_playlists_cache_dir()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) {
        return {};
    }

    QDir base_dir(base);
    base_dir.mkpath(".");

    const QString playlists_dir = base_dir.filePath("playlists");
    QDir(playlists_dir).mkpath(".");

    return playlists_dir;
}

PlaylistRepo::PlaylistRepo(QObject* parent) : QObject(parent)
{
    cache_dir_ = resolve_playlists_cache_dir();
}

void PlaylistRepo::clear_list()
{
    list_.clear();
}

void PlaylistRepo::save_list_to_cache(const std::shared_ptr<Playlist>& playlist)
{
    if (!playlist) {
        return;
    }
    if (cache_dir_.isEmpty()) {
        logger->warn("Cache dir is empty, skip saving playlist.");
        return;
    }

    QFile file(cache_file_path(playlist->id()));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        logger->warn("Failed to open {} for cache saving.", file.fileName());
        return;
    }
    if (!write_json_playlist(file, playlist)) {
        logger->warn("Failed to write cache file: {}", file.fileName());
    }
}

void PlaylistRepo::load_cache()
{
    emit sgn_cache_load_started();
    load_cache_from_disk();
    emit sgn_cache_load_finished(static_cast<int>(list_.size()));
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
                    emit self->sgn_cache_load_finished(static_cast<int>(self->list_.size()));
                    return;
                }
                self->list_ += lists;
                emit self->sgn_playlist_changed();
                emit self->sgn_cache_load_finished(static_cast<int>(self->list_.size()));
            },
            Qt::QueuedConnection);
    });
    QObject::connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

QString PlaylistRepo::cache_file_path(const PlaylistId& pid) const
{
    QDir dir(cache_dir_);
    return dir.filePath(pid.to_string_without_brace() + ".wcpl");
}

bool PlaylistRepo::write_json_playlist(QIODevice& device, const std::shared_ptr<Playlist>& playlist)
{
    if (!playlist) {
        return false;
    }

    QJsonObject root;
    root["schemaVersion"] = kSchemaVersion;
    root["id"]            = playlist->id().to_string_without_brace();
    root["name"]          = playlist->name();

    QJsonArray tracks;
    const auto& list = playlist->get_tracks();
    for (const auto& track : list) {
        QJsonObject t;
        t["entry_id"]         = track.entry_id.to_string_without_brace();
        t["library_track_id"] = track.library_track_id.to_string_without_brace();
        t["source"]           = track.source == TrackSource::Library ? "library" : "external";
        t["filepath"]         = track.filepath;
        t["missing"]          = track.missing;
        if (track.meta.is_valid) {
            t["meta"] = meta_to_json(track.meta);
        }
        tracks.append(t);
    }
    root["tracks"] = tracks;

    QJsonDocument doc(root);
    device.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

bool PlaylistRepo::load_json_playlist(const QByteArray& data, const QString& fallback_name,
                                      std::shared_ptr<Playlist>& out_playlist,
                                      bool* out_legacy_format)
{
    if (out_legacy_format) {
        *out_legacy_format = false;
    }

    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parse_error);
    if (parse_error.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    QJsonObject root        = doc.object();
    QJsonValue tracks_value = root.value("tracks");
    if (!tracks_value.isArray()) {
        return false;
    }

    QJsonValue id_value = root.value("id");
    PlaylistId pid      = PlaylistId(id_value.toString());
    if (pid.is_null()) {
        pid = PlaylistId::create_uuid();
    }

    QString name = root.value("name").toString(fallback_name);
    out_playlist->new_uuid(pid);
    out_playlist->set_playlist_name(name);

    QJsonArray tracks = tracks_value.toArray();
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
        if (tid.is_null()) {
            // 旧格式回退:阶段 1 前字段名为 "id",语义即条目身份,复用保持身份稳定
            tid = EntryId(obj.value("id").toString());
        }
        if (obj.contains("id") && !obj.contains("entry_id") && out_legacy_format) {
            *out_legacy_format = true;
        }
        Track t = tid.is_null() ? Track::from_filepath(filepath) : Track::from_entry(tid, filepath);
        t.source           = obj.value("source").toString() == "library" ? TrackSource::Library
                                                                         : TrackSource::External;
        t.library_track_id = TrackId(obj.value("library_track_id").toString());
        t.missing          = obj.value("missing").toBool(false);
        out_playlist->add_track_object(t);

        QJsonValue meta_value = obj.value("meta");
        if (meta_value.isObject()) {
            TrackMetaData meta;
            meta.filepath = filepath;
            meta.filename = QFileInfo(filepath).fileName();
            apply_json_to_meta(meta_value.toObject(), meta);
            meta.is_valid = true;
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
        list_ += lists;
        emit sgn_playlist_changed();
    }
}

QVector<std::pair<std::shared_ptr<Playlist>, bool>>
PlaylistRepo::load_cache_from_disk_to_vector() const
{
    QVector<std::pair<std::shared_ptr<Playlist>, bool>> loaded;
    if (cache_dir_.isEmpty()) {
        return loaded;
    }

    QDir dir(cache_dir_);
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
        QFileInfo file_info(filepath);
        QString fallback_name = file_info.baseName();
        bool legacy_format    = false;
        if (!load_json_playlist(data, fallback_name, playlist, &legacy_format)) {
            continue;
        }
        loaded.push_back({playlist, legacy_format});
    }

    return loaded;
}

PlaylistId PlaylistRepo::create_list()
{
    PlaylistId new_id = PlaylistId::create_uuid();
    auto new_playlist = std::make_shared<Playlist>();
    new_playlist->new_uuid(new_id);
    QString default_name = QString("New playlist %1").arg(list_.size() + 1);
    new_playlist->set_playlist_name(default_name);
    list_.push_back(new_playlist);

    save_list_to_cache(new_playlist);

    emit sgn_playlist_changed();
    return new_id;
}

/* ---- load list from file ---- */
PlaylistId PlaylistRepo::load_list(const QString& filepath)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        logger->warn("Failed to open file for loading: {}", filepath);
        return {};
    }

    auto new_playlist = std::make_shared<Playlist>();
    QFileInfo file_info(filepath);
    QString fallback_name = file_info.baseName();

    QByteArray data       = file.readAll();
    if (!load_json_playlist(data, fallback_name, new_playlist)) {
        // old style: use filename as playlist
        file.seek(0);
        new_playlist->set_playlist_name(fallback_name);

        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                // todo: check wheteher the file path is valid
                new_playlist->add_track(line);
            }
        }
    }

    list_.push_back(new_playlist);
    emit sgn_playlist_changed();
    logger->info("Loading playlist from: {}", filepath);
    return new_playlist->id();
}

PlaylistId PlaylistRepo::load_list_batched(const QString& filepath, int batch_size)
{
    if (batch_size <= 0) {
        batch_size = 500;
    }

    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        logger->warn("Failed to open file for loading: {}", filepath);
        return {};
    }

    struct LoadEntry
    {
        PlaylistId id;
        QString filepath;
        bool has_meta = false;
        TrackMetaData meta;
        TrackSource source = TrackSource::External;
        TrackId library_track_id;
        bool missing = false;
    };

    QVector<LoadEntry> entries;
    entries.reserve(1024);

    auto new_playlist = std::make_shared<Playlist>();
    QFileInfo file_info(filepath);
    QString fallback_name = file_info.baseName();

    QByteArray data       = file.readAll();
    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parse_error);
    if (parse_error.error == QJsonParseError::NoError && doc.isObject()) {
        QJsonObject root        = doc.object();
        QJsonValue tracks_value = root.value("tracks");

        PlaylistId pid          = PlaylistId(root.value("id").toString());
        if (pid.is_null()) {
            pid = PlaylistId::create_uuid();
        }

        QString name = root.value("name").toString(fallback_name);
        new_playlist->new_uuid(pid);
        new_playlist->set_playlist_name(name);

        if (tracks_value.isArray()) {
            QJsonArray tracks = tracks_value.toArray();
            entries.reserve(tracks.size());
            for (const auto& item : tracks) {
                if (!item.isObject()) {
                    continue;
                }
                QJsonObject obj    = item.toObject();
                QString track_path = obj.value("filepath").toString();
                if (track_path.isEmpty()) {
                    continue;
                }
                EntryId eid = EntryId(obj.value("entry_id").toString());
                if (eid.is_null()) {
                    // 旧格式回退:阶段 1 前字段名为 "id"
                    eid = EntryId(obj.value("id").toString());
                }
                LoadEntry entry;
                entry.id       = eid.to_tag_of<PlaylistIdTag>();
                entry.filepath = track_path;
                entry.source = obj.value("source").toString() == "library" ? TrackSource::Library
                                                                           : TrackSource::External;
                entry.library_track_id = TrackId(obj.value("library_track_id").toString());
                entry.missing          = obj.value("missing").toBool(false);
                QJsonValue meta_value  = obj.value("meta");
                if (meta_value.isObject()) {
                    TrackMetaData meta;
                    meta.filepath = track_path;
                    meta.filename = QFileInfo(track_path).fileName();
                    apply_json_to_meta(meta_value.toObject(), meta);
                    meta.is_valid  = true;
                    entry.has_meta = true;
                    entry.meta     = meta;
                }
                entries.push_back(entry);
            }
        }
    } else {
        file.seek(0);
        new_playlist->set_playlist_name(fallback_name);

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

    if (new_playlist->id().is_null()) {
        new_playlist->new_uuid(PlaylistId::create_uuid());
    }

    list_.push_back(new_playlist);
    emit sgn_playlist_changed();

    const PlaylistId pid  = new_playlist->id();
    const int total_count = static_cast<int>(entries.size());
    emit sgn_playlist_load_started(pid, total_count);

    auto entries_ptr = std::make_shared<QVector<LoadEntry>>(std::move(entries));
    auto index_ptr   = std::make_shared<int>(0);

    QPointer<PlaylistRepo> self(this);
    std::shared_ptr<Playlist>& playlist_ptr = new_playlist;

    std::function<void()> process_batch;
    process_batch = [self, playlist_ptr, entries_ptr, index_ptr, batch_size, total_count, pid,
                     &process_batch]() mutable {
        if (!self) {
            return;
        }

        int start = *index_ptr;
        int end   = std::min(start + batch_size, total_count);
        for (int i = start; i < end; ++i) {
            const LoadEntry& entry = entries_ptr->at(i);
            if (entry.filepath.isEmpty()) {
                continue;
            }
            Track t;
            if (entry.id.is_null()) {
                t = Track::from_filepath(entry.filepath);
            } else {
                t = Track::from_entry(entry.id.to_tag_of<EntryIdTag>(), entry.filepath);
            }
            t.source           = entry.source;
            t.library_track_id = entry.library_track_id;
            t.missing          = entry.missing;
            playlist_ptr->add_track_object(t);
            if (entry.has_meta) {
                playlist_ptr->update_track_meta(t.entry_id, entry.meta);
            }
        }

        *index_ptr = end;
        emit self->playlist_batch_loaded(pid, end, total_count);

        if (end < total_count) {
            QTimer::singleShot(0, self, process_batch);
            return;
        }

        self->save_list_to_cache(playlist_ptr);
        emit self->sgn_playlist_load_finished(pid);
    };

    QTimer::singleShot(0, this, process_batch);

    logger->info("Loading playlist (batched) from: {} total: {}", filepath, total_count);
    return pid;
}

/* ---- save list to file ---- */
void PlaylistRepo::save_list(const PlaylistId& pid, const QString& dst_path)
{
    std::shared_ptr<Playlist> src = find_playlist_by_id(pid);
    if (!src) {
        logger->warn("Playlist ({}) not found, save failed.", pid.to_string());
        return;
    }

    QFile file(dst_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        logger->warn("Failed to open file for saving: {}", dst_path);
        return;
    }

    if (!write_json_playlist(file, src)) {
        logger->warn("Failed to write playlist file: {}", dst_path);
        return;
    }

    logger->info("Saved playlist to: {}", dst_path);
}

void PlaylistRepo::rename_list(const PlaylistId& pid, const QString& name)
{
    std::shared_ptr<Playlist> src = find_playlist_by_id(pid);
    if (!src) {
        logger->warn("Playlist {} does not exist", pid.to_string());
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
        logger->warn("Playlist {} not found", pid.to_string());
        return;
    }
    list_.removeOne(src);
    if (!cache_dir_.isEmpty()) {
        QFile::remove(cache_file_path(pid));
    }
    emit sgn_playlist_changed();
}

void PlaylistRepo::reorder_lists(const QVector<PlaylistId>& ordered_ids)
{
    if (ordered_ids.size() != list_.size()) {
        logger->warn("[PlaylistRepo] reorder_lists: size mismatch {} vs {}", ordered_ids.size(),
                     list_.size());
        return;
    }
    QVector<std::shared_ptr<Playlist>> new_order;
    new_order.reserve(ordered_ids.size());
    for (const PlaylistId& pid : ordered_ids) {
        auto it = std::ranges::find_if(
            list_, [&pid](const std::shared_ptr<Playlist>& pl) { return pl->id() == pid; });
        if (it != list_.end()) {
            new_order.push_back(*it);
        }
    }
    if (new_order.size() != list_.size()) {
        logger->warn("[PlaylistRepo] reorder_lists: dropped entries, abort");
        return;
    }
    list_ = std::move(new_order);
    emit sgn_playlist_changed();
}

/**
 * @note: this function means "copy-and-paste", but not copy only
 */
void PlaylistRepo::copy_list(const PlaylistId& src_uuid)
{
    std::shared_ptr<Playlist> src = find_playlist_by_id(src_uuid);

    if (!src) {
        logger->warn("Source playlist {} not found", src_uuid.to_string());
        return;
    }

    // deep-copy
    auto new_playlist = std::make_shared<Playlist>(*src);
    new_playlist->new_uuid();
    list_.push_back(new_playlist);

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
        logger->warn("Playlist id {} not found", pid.to_string());
        return;
    }
    logger->info("Add track {} to {}", filepath, pid.to_string());

    Track new_track = src->add_track(filepath);
    save_list_to_cache(src);
    emit sgn_playlist_changed();
}

void PlaylistRepo::add_tracks_to_playlist(const PlaylistId& pid, const QStringList& filepaths)
{
    std::shared_ptr<Playlist> src = find_playlist_by_id(pid);
    if (!src) {
        logger->warn("Playlist id {} not found", pid.to_string());
        return;
    }
    logger->info("Add {} tracks to {}", filepaths.size(), pid.to_string());

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
        logger->warn("Playlist id {} not found", pid.to_string());
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
        logger->warn("Playlist id {} not found", pid.to_string());
        return;
    }
    for (const auto& track : tracks) {
        src->add_track_object(track);
    }
    save_list_to_cache(src);
    emit sgn_playlist_changed();
}

bool PlaylistRepo::empty()
{
    list_.shrink_to_fit();
    if (list_.empty()) {
        return true;
    }
    return false;
}

std::shared_ptr<Playlist> PlaylistRepo::find_playlist_by_id(const PlaylistId& pid)
{
    if (pid.is_null()) {
        return nullptr;
    }

    for (const auto& it : list_) {
        if (it->id() == pid) {
            return it;
        }
    }
    logger->warn("Playlist does not exist, UUID={}", pid.to_string());
    return nullptr;
}

const QVector<std::shared_ptr<Playlist>>& PlaylistRepo::get_lists()
{
    return list_;
}
