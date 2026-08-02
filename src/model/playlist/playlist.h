#pragma once

#include "core/types.h"

#include <QDebug>
#include <QString>
#include <QUuid>
#include <QVector>

class Playlist
{
public:
    explicit Playlist(const QString& name = "default");
    ~Playlist();

    // Playlist metadata
    PlaylistId id() const;
    QString name();
    void setPlaylistName(QString setname);
    void newUuid();
    void newUuid(const PlaylistId& pid);
    size_t track_count();

    // Modify & Manage
    void clearList();
    Track addTrack(const QString& filepath);
    Track addTrackWithId(const EntryId& tid, const QString& filepath);
    void addTrackObject(const Track& track); // 直接加入已构造好的 Track(反序列化用)
    bool updateTrackMeta(const EntryId& tid, const TrackMetaData& meta);
    void removeTrack(const EntryId& tid);

    // 非拥有:返回指针生命周期由所属 Playlist 管理,Playlist 未被修改/析构前有效;调用方不得 delete
    const Track* findTrackByID(const EntryId& eid) const;

    const QVector<Track>& getTracks() const;

    // status
    bool isEmpty();

private:
    QVector<Track> m_tracks;
    QString m_name;
    PlaylistId m_pid;
};
