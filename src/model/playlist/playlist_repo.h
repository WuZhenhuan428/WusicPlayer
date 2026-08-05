#pragma once

#include "model/playlist/playlist.h"

#include <QDebug>
#include <QObject>
#include <QString>
#include <QUuid>
#include <QVector>

#include <memory>
#include <utility>

class PlaylistRepo : public QObject
{
    Q_OBJECT
public:
    explicit PlaylistRepo(QObject* parent = nullptr);
    ~PlaylistRepo();

    // Playlist management
public:
    void clear_list();
    PlaylistId create_list();
    PlaylistId load_list(const QString& filepath);
    PlaylistId load_list_batched(const QString& filepath, int batch_size = 500);
    void save_list(const PlaylistId& pid, const QString& dst_path);
    void rename_list(const PlaylistId& pid, const QString& name);
    void remove_list(const PlaylistId& pid);
    void copy_list(const PlaylistId& src);
    // 按给定顺序重排播放列表(拖动排序);缺失 id 保持原位
    void reorder_lists(const QVector<PlaylistId>& ordered_ids);
    std::shared_ptr<Playlist> find_playlist_by_id(const PlaylistId& pid);
    void add_track_to_playlist(const PlaylistId& pid, const QString& filepath);
    void add_tracks_to_playlist(const PlaylistId& pid, const QStringList& filepaths);
    void add_track_object(const PlaylistId& pid, const Track& track);
    void add_track_objects(const PlaylistId& pid, const QVector<Track>& tracks);
    bool isEmpty();
    const QVector<std::shared_ptr<Playlist>>& get_lists();

    void save_list_to_cache(std::shared_ptr<Playlist> playlist);
    void load_cache();
    void load_cache_async();

signals:
    void sgn_playlist_changed();
    void sgn_cache_load_started();
    void sgn_cache_load_finished(int playlist_count);
    void sgn_playlist_load_started(const PlaylistId& pid, int total_count);
    void playlistBatchLoaded(const PlaylistId& pid, int loaded_count, int total_count);
    void sgn_playlist_load_finished(const PlaylistId& pid);

private:
    QString cache_file_path(const PlaylistId& pid) const;
    void load_cache_from_disk();
    QVector<std::pair<std::shared_ptr<Playlist>, bool>> load_cache_from_disk_to_vector() const;
    bool load_json_playlist(const QByteArray& data, const QString& fallbackName,
                            std::shared_ptr<Playlist>& out_playlist,
                            bool* out_legacy_format = nullptr) const;
    bool write_json_playlist(QIODevice& device, const std::shared_ptr<Playlist>& playlist) const;

    QVector<std::shared_ptr<Playlist>> m_list;
    QString m_cache_dir; // <standard app data dir>/playlists
    static constexpr int kSchemaVersion = 1;
};
