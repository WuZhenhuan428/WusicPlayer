#pragma once

#include <QAbstractTableModel>
#include <QVector>
#include <QUuid>
#include <QHash>
#include <QVariant>
#include <QModelIndex>
#include <QStringList>


#include "playlist_repo.h"
#include "playlist_definitions.h"


using trackId = QUuid;
using playlistId = QUuid;

struct Node {
    QUuid id; // Track UUID. If null, it's a group node.
    QString groupName;
    Node* parent = nullptr;
    QVector<Node*> children;

    int row() const {
        if (parent) {
            return parent->children.indexOf(const_cast<Node*>(this));
        }
        return 0;
    }

    explicit Node(Node* p = nullptr) : parent(p) {}
    ~Node() { qDeleteAll(children); }
};

class PlaylistViewModel : public QAbstractItemModel
{
    Q_OBJECT

public:

public:
    explicit PlaylistViewModel(PlaylistRepo* repo);
    ~PlaylistViewModel();

    void rebuild();

/* ==== Context & Repo 绑定 ==== */
public:
    void setPlaylist(const playlistId& playlist_id);
    void setGrouping(SortType type, bool enable);
    void clear();

/* ==== View视图数据访问 ====*/
public:
    // QAbstractItemModel Interface
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    // Helper to get logic data
    trackId trackAt(int index) const; // Still useful for linear queue access
    trackId trackAt(const QModelIndex& index) const;
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
    void changedData(int row); 
    
private:
    PlaylistRepo* m_repo = nullptr;
    playlistId m_playlistId;
    Node* m_root = nullptr;

    QHash<trackId, TrackMetaData> m_metaCache;
    QVector<trackId> m_playbackQueue; // Linear queue for playback logic (separate from Tree structure)
    QStringList m_horizontalHead;

    SortType m_groupType = SortType::Artist;
    bool m_enableGrouping = false;

    QString getGroupKey(const TrackMetaData& data, SortType type);
};
