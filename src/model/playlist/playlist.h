#pragma once

#include "core/types.h"
#include "model/library/library_track.h"

#include <QHash>
#include <QString>
#include <QUuid>
#include <QVector>

#include <functional>
#include <memory>

class Playlist
{
public:
    explicit Playlist(const QString& name = "default");
    ~Playlist();

    // Playlist metadata
    PlaylistId id() const;
    QString name();
    void set_playlist_name(QString setname);
    void new_uuid();
    void new_uuid(const PlaylistId& pid);
    size_t track_count();

    // Modify & Manage
    void clear_list();
    Track add_track(const QString& filepath);
    Track add_track_with_id(const EntryId& eid, const QString& filepath);
    void add_track_object(const Track& track); // 直接加入已构造好的 Track(反序列化用)
    bool update_track_meta(const EntryId& eid, const TrackMetaData& meta);
    bool set_track_missing(const EntryId& eid, bool missing);
    // 用库解析器刷新所有库引用条目(source==library)的元数据/缺失标记;返回更新的条目数
    int refresh_library_tracks(
        const std::function<std::shared_ptr<const LibraryTrack>(const TrackId&)>& resolver);
    // 将路径已在库中的外部条目升级为库引用条目(库变更后调用);返回升级数
    int upgrade_external_tracks(
        const std::function<std::shared_ptr<const LibraryTrack>(const QString& path)>& resolver);
    int remove_missing_tracks();
    void remove_track(const EntryId& eid);

    // 非拥有:返回指针生命周期由所属 Playlist 管理,Playlist 未被修改/析构前有效;调用方不得 delete
    const Track* find_track_by_id(const EntryId& eid) const;
    // 按规范化路径查找条目;找不到返回 nullptr
    const Track* find_track_by_filepath(const QString& filepath) const;

    const QVector<Track>& get_tracks() const;

    // status
    bool isEmpty();

private:
    void rebuild_index();

    QVector<Track> m_tracks;
    QHash<EntryId, qsizetype> m_track_index;
    QString m_name;
    PlaylistId m_pid;
};
