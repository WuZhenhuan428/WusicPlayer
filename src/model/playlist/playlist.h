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
    bool updateTrackMeta(const EntryId& tid, const TrackMetaData& meta);
    void removeTrack(const EntryId& tid);

    Track* findTrackByID(const EntryId& tid);

    const QVector<Track>& getTracks() const;

    // status
    bool isEmpty();

private:
    QVector<Track> m_tracks;
    QString m_name;
    PlaylistId m_pid;
};
