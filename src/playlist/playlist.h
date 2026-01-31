#pragma once

#include <QString>
#include <QVector>
#include <QDebug>
#include <QUuid>
#include <QUuid>

#include <vector>

#include "../../include/audio.h"

struct Track
{
    QUuid uuid;
    QString filepath;
    Track() : uuid(QUuid::createUuid()) {}
};

class Playlist
{
public:
    explicit Playlist(const QString& name = "default");
    ~Playlist();

    // Playlist metadata
    QUuid id() const;
    QString name();
    void setPlaylistName(QString setname);
    void newUuid();
    void newUuid(const QUuid& uuid);
    
    // Modify & Manage
    void clearList();
    Track addTrack(const QString& filepath);
    void removeTrack(const QUuid& uuid);

    Track* findTrackByID(const QUuid& uuid);
    
    const QVector<Track>& getTracks() const;

    // status
    bool isEmpty();
    
    QVector<Track> m_tracks;    // @TODO: switch to QMap<QUuid, filepath>
private:
    QString m_name;
    QUuid m_uuid;
};

