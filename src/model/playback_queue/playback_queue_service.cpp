#include "playback_queue_service.h"

#include "core/utils/path.hpp"
#include "model/library/library_manager.h"
#include "model/playlist/playlist_manager.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <optional>

namespace
{

// ---- meta 序列化(模块内局部实现,切断点后抽取到 core/utils) ----
QJsonObject meta_to_json(const TrackMetaData& meta)
{
    QJsonObject o;
    o["album"]        = meta.album;
    o["album_artist"] = meta.album_artist;
    o["artist"]       = meta.artist;
    o["bitrate"]      = meta.bitrate;
    o["comment"]      = meta.comment;
    o["composer"]     = meta.composer;
    o["date"]         = meta.date;
    o["disc_number"]  = meta.disc_number;
    o["disc_total"]   = meta.disc_total;
    o["duration_s"]   = meta.duration_s;
    o["encoder"]      = meta.encoder;
    o["filepath"]     = meta.filepath;
    o["filename"]     = meta.filename;
    o["genre"]        = meta.genre;
    o["lyrics"]       = meta.lyrics;
    o["start_at"]     = meta.start_at;
    o["title"]        = meta.title;
    o["track_number"] = meta.track_number;
    o["year"]         = meta.year;
    o["isValid"]      = meta.isValid;
    return o;
}

void apply_meta_from_json(const QJsonObject& o, TrackMetaData* meta)
{
    meta->album        = o["album"].toString();
    meta->album_artist = o["album_artist"].toString();
    meta->artist       = o["artist"].toString();
    meta->bitrate      = o["bitrate"].toInt();
    meta->comment      = o["comment"].toString();
    meta->composer     = o["composer"].toString();
    meta->date         = o["date"].toString();
    meta->disc_number  = o["disc_number"].toInt();
    meta->disc_total   = o["disc_total"].toInt();
    meta->duration_s   = o["duration_s"].toInt();
    meta->encoder      = o["encoder"].toString();
    meta->filepath     = o["filepath"].toString();
    meta->filename     = o["filename"].toString();
    meta->genre        = o["genre"].toString();
    meta->lyrics       = o["lyrics"].toString();
    meta->start_at     = o["start_at"].toInt();
    meta->title        = o["title"].toString();
    meta->track_number = o["track_number"].toInt();
    meta->year         = o["year"].toInt();
    meta->isValid      = o["isValid"].toBool();
}

QJsonObject item_to_json(const QueueItem& item)
{
    QJsonObject o;
    o["library_track_id"]   = item.library_track_id.toString();
    o["playlist_entry_id"]  = item.playlist_entry_id.toString();
    o["source_playlist_id"] = item.source_playlist_id.toString();
    o["source_label"]       = item.source_label;
    o["filepath"]           = item.filepath;
    o["meta"]               = meta_to_json(item.meta);
    return o;
}

QueueItem item_from_json(const QJsonObject& o)
{
    QueueItem item;
    item.library_track_id   = TrackId(o["library_track_id"].toString());
    item.playlist_entry_id  = EntryId(o["playlist_entry_id"].toString());
    item.source_playlist_id = PlaylistId(o["source_playlist_id"].toString());
    item.source_label       = o["source_label"].toString();
    item.filepath           = o["filepath"].toString();
    apply_meta_from_json(o["meta"].toObject(), &item.meta);
    return item;
}

} // namespace

PlaybackQueueService::PlaybackQueueService(QObject* parent) : QObject(parent)
{
    connect(&m_queue, &PlaybackQueue::sgn_queue_changed, this,
            &PlaybackQueueService::sgn_queue_changed);
    connect(&m_queue, &PlaybackQueue::sgn_current_changed, this,
            &PlaybackQueueService::sgn_current_changed);
}

void PlaybackQueueService::set_playlist_manager(PlaylistManager* mgr)
{
    m_playlist_mgr = mgr;
}

void PlaybackQueueService::set_library_manager(LibraryManager* lib)
{
    m_library = lib;
}

PlaybackQueue* PlaybackQueueService::queue()
{
    return &m_queue;
}

bool PlaybackQueueService::enqueue_playlist_entry(const PlaylistId& pid, const EntryId& eid)
{
    if (m_playlist_mgr == nullptr) {
        return false;
    }
    for (const auto& pl : m_playlist_mgr->getPlaylists()) {
        if (pl->id() != pid) {
            continue;
        }
        const Track* track = pl->findTrackByID(eid);
        if (track == nullptr) {
            return false;
        }
        QueueItem item;
        item.playlist_entry_id  = eid;
        item.source_playlist_id = pid;
        item.filepath           = utils::path::normalize_path(track->filepath);
        item.meta               = track->meta;
        m_queue.enqueue(item);
        return true;
    }
    return false;
}

bool PlaybackQueueService::enqueue_library_track(const TrackId& track_id)
{
    if (m_library == nullptr) {
        return false;
    }
    const std::optional<LibraryTrack> lt = m_library->track_by_id(track_id);
    if (!lt.has_value()) {
        return false;
    }
    QueueItem item;
    item.library_track_id = track_id;
    item.filepath         = lt->filepath;
    item.meta             = lt->meta;
    m_queue.enqueue(item);
    return true;
}

int PlaybackQueueService::enqueue_external(const QString& filepath, const TrackMetaData& meta)
{
    QueueItem item;
    item.filepath = utils::path::normalize_path(filepath);
    item.meta     = meta;
    if (item.meta.filepath.isEmpty()) {
        item.meta.filepath = item.filepath;
    }
    if (item.meta.filename.isEmpty()) {
        item.meta.filename = QFileInfo(item.filepath).fileName();
    }
    return m_queue.enqueue(item);
}

bool PlaybackQueueService::play_library_track(const TrackId& track_id)
{
    if (!enqueue_library_track(track_id)) {
        return false;
    }
    const int idx = m_queue.size() - 1;
    m_queue.set_current(idx);
    if (auto item = m_queue.current()) {
        emit sgn_play_requested(*item);
    }
    return true;
}

int PlaybackQueueService::play_external(const QString& filepath, const TrackMetaData& meta)
{
    const int idx = enqueue_external(filepath, meta);
    m_queue.set_current(idx);
    if (auto item = m_queue.current()) {
        emit sgn_play_requested(*item);
    }
    return idx;
}

bool PlaybackQueueService::save_to(const QString& path) const
{
    QJsonObject root;
    root["version"]       = 1;
    root["current_index"] = m_queue.current_index();
    QJsonArray arr;
    for (const QueueItem& item : m_queue.items()) {
        arr.append(item_to_json(item));
    }
    root["items"] = arr;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        return false;
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    f.close();
    return true;
}

bool PlaybackQueueService::load_from(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return false;
    }
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }
    const QJsonObject root = doc.object();
    if (root["version"].toInt() != 1) {
        return false;
    }
    QVector<QueueItem> items;
    const QJsonArray arr = root["items"].toArray();
    items.reserve(arr.size());
    for (const QJsonValue& v : arr) {
        items.append(item_from_json(v.toObject()));
    }
    m_queue.clear();
    m_queue.enqueue_many(items);
    const int cur = root["current_index"].toInt(-1);
    if (cur >= 0 && cur < items.size()) {
        m_queue.set_current(cur);
    }
    return true;
}
