#include "playlist_repo.h"
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTimer>
#include <QPointer>
#include <QThread>
#include <QPointer>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonValueRef>
#include <QJsonObject>

static QJsonObject metaToJson(const TrackMetaData& meta) {
    QJsonObject obj;
    obj["album"] = meta.album;
    obj["album_artist"] = meta.album_artist;
    obj["artist"] = meta.artist;
    obj["bitrate"] = meta.bitrate;
    obj["comment"] = meta.comment;
    obj["composer"] = meta.composer;
    obj["date"] = meta.date;
    obj["disc_number"] = meta.disc_number;
    obj["disc_total"] = meta.disc_total;
    obj["duration_s"] = meta.duration_s;
    obj["encoder"] = meta.encoder;
    obj["filepath"] = meta.filepath;
    obj["filename"] = meta.filename;
    obj["genre"] = meta.genre;
    obj["lyrics"] = meta.lyrics;
    obj["start_at"] = meta.start_at;
    obj["title"] = meta.title;
    obj["track_number"] = meta.track_number;
    obj["year"] = meta.year;
    return obj;
}

static void applyJsonToMeta(const QJsonObject& obj, TrackMetaData& meta) {
    meta.album = obj.value("album").toString(meta.album);
    meta.album_artist = obj.value("album_artist").toString(meta.album_artist);
    meta.artist = obj.value("artist").toString(meta.artist);
    meta.bitrate = obj.value("bitrate").toInt(meta.bitrate);
    meta.comment = obj.value("comment").toString(meta.comment);
    meta.composer = obj.value("composer").toString(meta.composer);
    meta.date = obj.value("date").toString(meta.date);
    meta.disc_number = obj.value("disc_number").toInt(meta.disc_number);
    meta.disc_total = obj.value("disc_total").toInt(meta.disc_total);
    meta.duration_s = obj.value("duration_s").toInt(meta.duration_s);
    meta.encoder = obj.value("encoder").toString(meta.encoder);
    meta.filepath = obj.value("filepath").toString(meta.filepath);
    meta.filename = obj.value("filename").toString(meta.filename);
    meta.genre = obj.value("genre").toString(meta.genre);
    meta.lyrics = obj.value("lyrics").toString(meta.lyrics);
    meta.start_at = obj.value("start_at").toInt(meta.start_at);
    meta.title = obj.value("title").toString(meta.title);
    meta.track_number = obj.value("track_number").toInt(meta.track_number);
    meta.year = obj.value("year").toInt(meta.year);
}

static QString resolvePlaylistsCacheDir() {
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

static void migratePlaylistCache(const QString& fromDirPath, const QString& toDirPath) {
    if (fromDirPath.isEmpty() || fromDirPath == toDirPath) return;

    QDir fromDir(fromDirPath);
    if (!fromDir.exists()) return;

    QDir toDir(toDirPath);
    toDir.mkpath(".");

    if (QFileInfo(fromDir.absolutePath()) == QFileInfo(toDir.absolutePath())) return;

    const QStringList files = fromDir.entryList(QStringList() << "*.wcpl", QDir::Files);
    for (const QString& file : files) {
        const QString src = fromDir.filePath(file);
        const QString dst = toDir.filePath(file);
        if (QFile::exists(dst)) continue;
        if (!QFile::rename(src, dst)) {
            QFile::copy(src, dst);
        }
    }
}

PlaylistRepo::PlaylistRepo(QObject *parent)
    : QObject(parent)
{
    m_cacheDir = resolvePlaylistsCacheDir();
    
    
}

PlaylistRepo::~PlaylistRepo() {}

void PlaylistRepo::clearList() {
    m_list.clear();
}

void PlaylistRepo::saveListToCache(std::shared_ptr<Playlist> playlist) {
    if (!playlist) {
        return;
    }
    if (m_cacheDir.isEmpty()) {
        qDebug() << "[WARNING] Cache dir is empty, skip saving playlist.";
        return;
    }

    QFile file(cacheFilePath(playlist->id()));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qDebug() << "[WARNING] Failed to open" << file.fileName() << "for cache saving.";
        return;
    }
    if (!writeJsonPlaylist(file, playlist)) {
        qDebug() << "[WARNING] Failed to write cache file:" << file.fileName();
    }
}

void PlaylistRepo::loadCache() {
    emit cacheLoadStarted();
    loadCacheFromDisk();
    emit cacheLoadFinished(m_list.size());
}

void PlaylistRepo::loadCacheAsync() {
    QPointer<PlaylistRepo> self(this);
    emit cacheLoadStarted();
    QThread* worker = QThread::create([self]() {
        if (!self) {
            return;
        }
        QVector<std::shared_ptr<Playlist>> loaded = self->loadCacheFromDiskToVector();
        if (!self) {
            return;
        }
        QMetaObject::invokeMethod(self, [self, loaded = std::move(loaded)]() mutable {
            if (!self) {
                return;
            }
            if (loaded.isEmpty()) {
                emit self->cacheLoadFinished(self->m_list.size());
                return;
            }
            self->m_list += loaded;
            emit self->playlistChanged();
            emit self->cacheLoadFinished(self->m_list.size());
        }, Qt::QueuedConnection);
    });
    QObject::connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

QString PlaylistRepo::cacheFilePath(const QUuid& id) const {
    QDir dir(m_cacheDir);
    return dir.filePath(id.toString(QUuid::WithoutBraces) + ".wcpl");
}

bool PlaylistRepo::writeJsonPlaylist(QIODevice& device, const std::shared_ptr<Playlist>& playlist) const {
    if (!playlist) {
        return false;
    }

    QJsonObject root;
    root["schemaVersion"] = kSchemaVersion;
    root["id"] = playlist->id().toString(QUuid::WithoutBraces);
    root["name"] = playlist->name();

    QJsonArray tracks;
    const auto& list = playlist->getTracks();
    for (const auto& track : list) {
        QJsonObject t;
        t["id"] = track.uuid.toString(QUuid::WithoutBraces);
        t["filepath"] = track.filepath;
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

bool PlaylistRepo::loadJsonPlaylist(const QByteArray& data, const QString& fallbackName, std::shared_ptr<Playlist>& outPlaylist) const {
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    QJsonObject root = doc.object();
    QJsonValue tracksValue = root.value("tracks");
    if (!tracksValue.isArray()) {
        return false;
    }

    QJsonValue idValue = root.value("id");
    QUuid playlistId = QUuid(idValue.toString());
    if (playlistId.isNull()) {
        playlistId = QUuid::createUuid();
    }

    QString name = root.value("name").toString(fallbackName);
    outPlaylist->newUuid(playlistId);
    outPlaylist->setPlaylistName(name);

    QJsonArray tracks = tracksValue.toArray();
    for (const auto& item : tracks) {
        if (!item.isObject()) {
            continue;
        }
        QJsonObject obj = item.toObject();
        QString filepath = obj.value("filepath").toString();
        if (filepath.isEmpty()) {
            continue;
        }
        QUuid trackId = QUuid(obj.value("id").toString());
        Track t = trackId.isNull() ? outPlaylist->addTrack(filepath)
                                   : outPlaylist->addTrackWithId(trackId, filepath);
        QJsonValue metaValue = obj.value("meta");
        if (metaValue.isObject()) {
            TrackMetaData meta;
            meta.filepath = filepath;
            meta.filename = QFileInfo(filepath).fileName();
            applyJsonToMeta(metaValue.toObject(), meta);
            meta.isValid = true;
            outPlaylist->updateTrackMeta(t.uuid, meta);
        }
    }

    return true;
}

void PlaylistRepo::loadCacheFromDisk() {
    QVector<std::shared_ptr<Playlist>> loaded = loadCacheFromDiskToVector();
    if (!loaded.isEmpty()) {
        m_list += loaded;
        emit playlistChanged();
    }
}

QVector<std::shared_ptr<Playlist>> PlaylistRepo::loadCacheFromDiskToVector() const {
    QVector<std::shared_ptr<Playlist>> loaded;
    if (m_cacheDir.isEmpty()) {
        return loaded;
    }

    QDir dir(m_cacheDir);
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
        auto playlist = std::make_shared<Playlist>();
        QFileInfo fileInfo(filepath);
        QString fallbackName = fileInfo.baseName();
        if (!loadJsonPlaylist(data, fallbackName, playlist)) {
            continue;
        }
        loaded.push_back(playlist);
    }

    return loaded;
}

QUuid PlaylistRepo::createList() {
    QUuid new_id = QUuid::createUuid();
    auto new_playlist = std::make_shared<Playlist>();
    new_playlist->newUuid(new_id);
    QString default_name = QString("New playlist %1").arg(m_list.size()+1);
    new_playlist->setPlaylistName(default_name);
    m_list.push_back(new_playlist);

    saveListToCache(new_playlist);

    emit playlistChanged();
    return new_id;
}

/* ---- load list from file ---- */
QUuid PlaylistRepo::loadList(const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "[WARNING] Failed to open file for loading:" << filepath;
        return QUuid();
    }

    auto new_playlist = std::make_shared<Playlist>();
    QFileInfo fileInfo(filepath);
    QString fallbackName = fileInfo.baseName();

    QByteArray data = file.readAll();
    if (!loadJsonPlaylist(data, fallbackName, new_playlist)) {
        // old style: use filename as playlist
        file.seek(0);
        new_playlist->setPlaylistName(fallbackName);

        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                // todo: check wheteher the file path is valid
                new_playlist->addTrack(line);
            }
        }
    }
    
    m_list.push_back(new_playlist);
    emit playlistChanged();
    qDebug() << "[INFO] Loading playlist from:" << filepath;
    return new_playlist->id();
}

QUuid PlaylistRepo::loadListBatched(const QString& filepath, int batchSize) {
    if (batchSize <= 0) {
        batchSize = 500;
    }

    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "[WARNING] Failed to open file for loading:" << filepath;
        return QUuid();
    }

    struct LoadEntry {
        QUuid id;
        QString filepath;
        bool hasMeta = false;
        TrackMetaData meta;
    };

    QVector<LoadEntry> entries;
    entries.reserve(1024);

    auto new_playlist = std::make_shared<Playlist>();
    QFileInfo fileInfo(filepath);
    QString fallbackName = fileInfo.baseName();

    QByteArray data = file.readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
        QJsonObject root = doc.object();
        QJsonValue tracksValue = root.value("tracks");

        QUuid playlistId = QUuid(root.value("id").toString());
        if (playlistId.isNull()) {
            playlistId = QUuid::createUuid();
        }

        QString name = root.value("name").toString(fallbackName);
        new_playlist->newUuid(playlistId);
        new_playlist->setPlaylistName(name);

        if (tracksValue.isArray()) {
            QJsonArray tracks = tracksValue.toArray();
            entries.reserve(tracks.size());
            for (const auto& item : tracks) {
                if (!item.isObject()) {
                    continue;
                }
                QJsonObject obj = item.toObject();
                QString trackPath = obj.value("filepath").toString();
                if (trackPath.isEmpty()) {
                    continue;
                }
                QUuid trackId = QUuid(obj.value("id").toString());
                LoadEntry entry;
                entry.id = trackId;
                entry.filepath = trackPath;
                QJsonValue metaValue = obj.value("meta");
                if (metaValue.isObject()) {
                    TrackMetaData meta;
                    meta.filepath = trackPath;
                    meta.filename = QFileInfo(trackPath).fileName();
                    applyJsonToMeta(metaValue.toObject(), meta);
                    meta.isValid = true;
                    entry.hasMeta = true;
                    entry.meta = meta;
                }
                entries.push_back(entry);
            }
        }
    } else {
        file.seek(0);
        new_playlist->setPlaylistName(fallbackName);

        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                entries.push_back({QUuid(), line});
            }
        }
    }

    if (new_playlist->id().isNull()) {
        new_playlist->newUuid(QUuid::createUuid());
    }

    m_list.push_back(new_playlist);
    emit playlistChanged();

    const QUuid playlistId = new_playlist->id();
    const int totalCount = entries.size();
    emit playlistLoadStarted(playlistId, totalCount);

    auto entriesPtr = std::make_shared<QVector<LoadEntry>>(std::move(entries));
    auto indexPtr = std::make_shared<int>(0);

    QPointer<PlaylistRepo> self(this);
    std::shared_ptr<Playlist> playlistPtr = new_playlist;

    std::function<void()> processBatch;
    processBatch = [self, playlistPtr, entriesPtr, indexPtr, batchSize, totalCount, playlistId, &processBatch]() mutable {
        if (!self) {
            return;
        }

        int start = *indexPtr;
        int end = std::min(start + batchSize, totalCount);
        for (int i = start; i < end; ++i) {
            const LoadEntry& entry = entriesPtr->at(i);
            if (entry.filepath.isEmpty()) {
                continue;
            }
            Track t = entry.id.isNull() ? playlistPtr->addTrack(entry.filepath)
                                        : playlistPtr->addTrackWithId(entry.id, entry.filepath);
            if (entry.hasMeta) {
                playlistPtr->updateTrackMeta(t.uuid, entry.meta);
            }
        }

        *indexPtr = end;
        emit self->playlistBatchLoaded(playlistId, end, totalCount);

        if (end < totalCount) {
            QTimer::singleShot(0, self, processBatch);
            return;
        }

        self->saveListToCache(playlistPtr);
        emit self->playlistLoadFinished(playlistId);
    };

    QTimer::singleShot(0, this, processBatch);

    qDebug() << "[INFO] Loading playlist (batched) from:" << filepath << "total:" << totalCount;
    return playlistId;
}

/* ---- save list to file ---- */
void PlaylistRepo::saveList(const QUuid& uuid, const QString& toPath) {
    std::shared_ptr<Playlist> src = findPlaylistById(uuid);
    if (!src) {
        qDebug() << "[WARNING] Playlist (" << uuid.toString() << ") not found, save failed.";
        return;
    }

    QFile file(toPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qDebug() << "[WARNING] Failed to open file for saving:" << toPath;
        return;
    }

    if (!writeJsonPlaylist(file, src)) {
        qDebug() << "[WARNING] Failed to write playlist file:" << toPath;
        return;
    }

    qDebug() << "[INFO] Saved playlist to:" << toPath;
}

void PlaylistRepo::renameList(const QUuid& uuid, const QString& name) {
    std::shared_ptr<Playlist> src = findPlaylistById(uuid);
    if (!src) {
        qDebug() << "[WARNING] Playlist" << uuid << "does not exist";
        return;
    }
    src->setPlaylistName(name);
    saveListToCache(src);
    emit playlistChanged();
}

void PlaylistRepo::removeList(const QUuid& uuid) {
    std::shared_ptr<Playlist> src = findPlaylistById(uuid);
    if (!src) {
        qDebug() << "[WARNING] Playlist " << uuid <<"not found";
        return;
    }
    m_list.removeOne(src);
    if (!m_cacheDir.isEmpty()) {
        QFile::remove(cacheFilePath(uuid));
    }
    emit playlistChanged();
}

/**
 * @note: this function means "copy-and-paste", but not copy only
 */
void PlaylistRepo::copyList(const QUuid& src_uuid) {
    std::shared_ptr<Playlist> src = findPlaylistById(src_uuid);
    
    if (!src) {
        qDebug() << "[WARNING] Source playlist " << src_uuid <<"not found";
        return;
    }
    
    // deep-copy
    auto new_playlist = std::make_shared<Playlist>(*src);
    new_playlist->newUuid();
    m_list.push_back(new_playlist);

    saveListToCache(new_playlist);

    emit playlistChanged();
}

/**
 * @note: 如果emit过多，可以考虑将add_one_track包装为两个函数，分别在两个函数的末尾进行emit playlistChanged();
 */
void PlaylistRepo::addTrackToPlaylist(const QUuid& playlistId, const QString& filepath) {
    std::shared_ptr<Playlist> src = findPlaylistById(playlistId);
    if (!src) {
        qDebug() << "[WARNING] Playlist id " << playlistId.toString() << "not found";
        return;
    }
    qDebug() << "[INFO] Add track " << filepath << "to " << playlistId.toString();

    Track newTrack = src->addTrack(filepath);
    saveListToCache(src);
    emit playlistChanged();
}

void PlaylistRepo::addTracksToPlaylist(const QUuid& playlistId, const QStringList& filepaths) {
    std::shared_ptr<Playlist> src = findPlaylistById(playlistId);
    if (!src) {
        qDebug() << "[WARNING] Playlist id " << playlistId.toString() << "not found";
        return;
    }
    qDebug() << "[INFO] Add " << filepaths.size() << " tracks to " << playlistId.toString();

    for (const auto& filepath : filepaths) {
        src->addTrack(filepath);
    }
    saveListToCache(src);
    emit playlistChanged();
}

bool PlaylistRepo::isEmpty() {
    m_list.shrink_to_fit();
    if (m_list.size() == 0) {
        return true;
    }
    return false;
}

std::shared_ptr<Playlist> PlaylistRepo::findPlaylistById(const QUuid& uuid) {
    if (uuid.isNull()) return nullptr;

    for (const auto& it : m_list) {
        if (it->id() == uuid) {
            return it;
        }
    }
    qDebug() << "[WARNING] Playlist does not exist, UUID=" << uuid.toString();
    return nullptr;
}

const QVector<std::shared_ptr<Playlist>>& PlaylistRepo::getLists() {
        return m_list;
}