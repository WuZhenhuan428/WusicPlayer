#include "playlist_view_model.h"
#include <QFileInfo>
#include <QTime>
#include <algorithm>
#include "../../include/audio.h"

PlaylistViewModel::PlaylistViewModel(PlaylistRepo* repo)
    : m_repo(repo)
{
    m_root = new Node();
    if (m_repo) {
        connect(m_repo, &PlaylistRepo::playlistChanged, this, &PlaylistViewModel::rebuild);
    }
}

PlaylistViewModel::~PlaylistViewModel() {
    delete m_root;
}

void PlaylistViewModel::rebuild() {
    beginResetModel();
    
    // Clear old data
    delete m_root;
    m_root = new Node();
    m_playbackQueue.clear();
    m_metaCache.clear();

    if (!m_repo) {
        endResetModel();
        return;
    }

    auto pl = m_repo->findPlaylistById(m_playlistId);
    if (!pl) {
        endResetModel();
        return;
    }

    QVector<Track> ts = pl->getTracks();
    QHash<QString, Node*> groupMap;

    for (const auto& t : ts) {
        TrackMetaData meta = Audio::parse(t.filepath.toStdString());
        if (!meta.isValid || meta.title.isEmpty()) {
            QFileInfo fi(t.filepath);
            meta.title = fi.fileName();
        }
        m_metaCache.insert(t.uuid, meta);

        // Determine Parent Node (Group or Root)
        Node* parentNode = m_root;

        if (m_enableGrouping) {
            QString key = getGroupKey(meta, m_groupType);
            if (!groupMap.contains(key)) {
                Node* groupNode = new Node(m_root);
                groupNode->groupName = key;
                m_root->children.append(groupNode);
                groupMap.insert(key, groupNode);
            }
            parentNode = groupMap.value(key);
        }

        // Create Track Node
        Node* trackNode = new Node(parentNode);
        trackNode->id = t.uuid; // Only leaf nodes have IDs
        parentNode->children.append(trackNode);
    }

    /* ==== @TODO sort ==== */
    

    // Re-populate playback queue (Linear representation)
    if (m_enableGrouping) {
        for (Node* group : m_root->children) {
            for (Node* track : group->children) {
                m_playbackQueue.append(track->id);
            }
        }
    } else {
         for (Node* track : m_root->children) {
            m_playbackQueue.append(track->id);
         }
    }
    
    endResetModel();
    qDebug() << "[INFO] rebuild finished. Queue size:" << m_playbackQueue.size();
    emit changedPlaybackQueue();
}

QString PlaylistViewModel::getGroupKey(const TrackMetaData& data, SortType type) {
    switch (type) {
        case SortType::Artist: return data.artist.isEmpty() ? "Unknown Artist" : data.artist;
        case SortType::Album: return data.album.isEmpty() ? "Unknown Album" : data.album;
        case SortType::Duration: return "Duration"; 
        default: return "Other";
    }
}

void PlaylistViewModel::setPlaylist(const playlistId& playlist_id) {
    if (m_playlistId == playlist_id) { return; }
    m_playlistId = playlist_id;
    this->rebuild();
}

void PlaylistViewModel::setGrouping(SortType type, bool enable) {
    if (m_groupType == type && m_enableGrouping == enable) return;
    m_groupType = type;
    m_enableGrouping = enable;
    rebuild();
}

void PlaylistViewModel::clear() {
    beginResetModel();
    delete m_root;
    m_root = new Node();
    m_playbackQueue.clear();
    m_metaCache.clear();
    endResetModel();
}

/* ==== QAbstractItemModel Interface ==== */

QModelIndex PlaylistViewModel::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent)) return QModelIndex();

    Node* parentNode;
    if (!parent.isValid())
        parentNode = m_root;
    else
        parentNode = static_cast<Node*>(parent.internalPointer());

    if (row < 0 || row >= parentNode->children.size()) return QModelIndex();
    
    Node* childNode = parentNode->children.at(row);
    return createIndex(row, column, childNode);
}

QModelIndex PlaylistViewModel::parent(const QModelIndex &child) const {
    if (!child.isValid()) return QModelIndex();

    Node* childNode = static_cast<Node*>(child.internalPointer());
    Node* parentNode = childNode->parent;

    if (parentNode == m_root || !parentNode) return QModelIndex();

    return createIndex(parentNode->row(), 0, parentNode);
}

int PlaylistViewModel::rowCount(const QModelIndex &parent) const {
    Node* parentNode;
    if (parent.column() > 0) return 0;

    if (!parent.isValid())
        parentNode = m_root;
    else
        parentNode = static_cast<Node*>(parent.internalPointer());

    return parentNode->children.size();
}

int PlaylistViewModel::columnCount(const QModelIndex &parent) const {
    return 7;
}

QVariant PlaylistViewModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();

    Node* node = static_cast<Node*>(index.internalPointer());
    bool isGroup = node->id.isNull(); 

    if (role == Qt::TextAlignmentRole && isGroup) {
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    }

    if (role == Qt::DisplayRole) {
        // Group Logic
        if (isGroup) {
            if (index.column() == 0) {
                return node->groupName + QString(" (%1)").arg(node->children.size());
            }
            return QVariant();
        }

        // Track Logic
        if (!m_metaCache.contains(node->id)) return QVariant();
        const TrackMetaData& d = m_metaCache.value(node->id);
        
        switch (index.column()) {
            case 0: return "[ ]";
            case 1: return d.disc_number;
            case 2: return d.track_number;
            case 3: return d.title;
            case 4: return d.artist;
            case 5: {
                // Time formatting
                int time_s = d.duration_s;
                int hours = time_s / 3600;
                int mins = (time_s % 3600) / 60;
                int secs = time_s % 60;
                if (hours > 0) return QString("%1:%2:%3").arg(hours, 2, 10, QChar('0')).arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
                return QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
            }
            case 6: return d.album;
        }
    }
    return QVariant();
}

QVariant PlaylistViewModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole) return QVariant();
    if (orientation == Qt::Horizontal) {
        switch (section) {
            case 0: return "state";
            case 1: return "Disc";
            case 2: return "#";
            case 3: return "Title";
            case 4: return "Artist";
            case 5: return "Duration";
            case 6: return "Album";
        }
    }
    return QVariant();
}

/* ==== Helpers ==== */

trackId PlaylistViewModel::trackAt(int index) const {
    if (index >= 0 && index < m_playbackQueue.size())
        return m_playbackQueue.at(index);
    return QUuid();
}

trackId PlaylistViewModel::trackAt(const QModelIndex& index) const {
    if (!index.isValid()) return QUuid();
    Node* node = static_cast<Node*>(index.internalPointer());
    return node->id; 
}

const QVector<trackId>& PlaylistViewModel::playbackQueue() const {
    return m_playbackQueue;
}

bool PlaylistViewModel::hasMetaData(const trackId& track_id) const {
    return m_metaCache.contains(track_id);
}

TrackMetaData PlaylistViewModel::metaData(const trackId& track_id) const {
    return m_metaCache.value(track_id);
}

trackId PlaylistViewModel::nextOf(const trackId& track_id) const {
    int idx = m_playbackQueue.indexOf(track_id);
    if (idx != -1 && idx < m_playbackQueue.size() - 1) {
        return m_playbackQueue.at(idx + 1);
    }
    return QUuid();
}

trackId PlaylistViewModel::previousOf(const trackId& track_id) const {
    int idx = m_playbackQueue.indexOf(track_id);
    if (idx > 0) {
        return m_playbackQueue.at(idx - 1);
    }
    return QUuid();
}

void PlaylistViewModel::requestMetaData(const trackId& track_id) {
    // Sync parsing used currently
}