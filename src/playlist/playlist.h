#pragma once

#include <QString>
#include <QVector>
#include <QDebug>
#include <QUuid>

#include <vector>

#include "../../src/core/utils/AudioUtils.h"

struct Track
{
    trackId tid;
    QString filepath;
    TrackMetaData meta;
    Track() : tid(trackId::createUuid()) {}
};

class Playlist
{
public:
    explicit Playlist(const QString& name = "default");
    ~Playlist();

    // Playlist metadata
    playlistId id() const;
    QString name();
    void setPlaylistName(QString setname);
    void newUuid();
    void newUuid(const playlistId& pid);
    
    // Modify & Manage
    void clearList();
    Track addTrack(const QString& filepath);
    Track addTrackWithId(const trackId& tid, const QString& filepath);
    bool updateTrackMeta(const trackId& tid, const TrackMetaData& meta);
    void removeTrack(const trackId& tid);

    Track* findTrackByID(const trackId& tid);
    
    const QVector<Track>& getTracks() const;

    // status
    bool isEmpty();
    
private:
    QVector<Track> m_tracks;
    QString m_name;
    playlistId m_pid;
};

