#pragma once

#include <QAbstractTableModel>
#include <QVector>
#include <QUuid>
#include <QHash>
#include <QVariant>
#include <QModelIndex>
#include <QStringList>

#include "../../include/header.h"
#include "playlist_repo.h"

enum class SortType
{
    title = 0,
    artist,
    album,
    disc_number,
    track_number,
    duration
    // ...
};

using trackId = QUuid;
using playlistId = QUuid;

class PlaylistViewModel : public QAbstractTableModel
{
    PlaylistRepo* m_repo = nullptr;
public:
    explicit PlaylistViewModel(PlaylistRepo* repo);
    ~PlaylistViewModel();

    void rebuild();

/* ==== Context & Repo 绑定 ==== */
public:
    void setPlaylist(const playlistId& playlist_id);
    void setSortMode(SortType sort_type);
    // void setFilter();
    void clear();

/* ==== View视图数据访问 ====*/
public:
    int rowCount() const;
    /* ---- QAbstractTableModel Interface ----*/
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    trackId trackAt(int index) const;
    const QVector<trackId>& playbackQueue() const;
    bool hasMetaData(const trackId& track_id) const;
    TrackMetaData metaData(const trackId& track_id) const;

/* ==== 播放顺序辅助（用于Player） ==== */
public:
    trackId nextOf(const trackId& track_id) const;
    trackId previousOf(const trackId& track_id) const;

/* ==== 元数据请求 ==== */
public:
    void requestMetaData(const trackId& track_id);

signals:
    void changedPlaybackQueue();
    void updatedTrackMetadata(const trackId& track_id);
    void changedData(int row);  // UI行刷新
    
private:
    playlistId m_playlistId;
    SortType m_sort_type;
    QHash<trackId, TrackMetaData> m_metaCache;
    QVector<trackId> m_playbackQueue;
    QStringList m_horizontalHead;
};
