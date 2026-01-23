#include "playlist_view_model.h"

PlaylistViewModel::PlaylistViewModel(PlaylistRepo* repo)
    : m_repo(repo)
{
    if (m_repo) {
        connect(m_repo, &PlaylistRepo::playlistChanged, this, &PlaylistViewModel::rebuild);
    }
}

PlaylistViewModel::~PlaylistViewModel() {}

void PlaylistViewModel::rebuild() {
    clear();
    if (!m_repo) return;

    auto pl = m_repo->findPlaylistById(m_playlistId);
    if (!pl) {
        return;
    }

    QVector<Track> ts = pl->getTracks();

    for (const auto& t : ts) {
        m_playbackQueue.append(t.uuid);
    }

    // sort(m_playtbackQueue, m_sort_type, m_sub_sort_type);

    qDebug() << "[INFO] stub: rebuild playback list";

    emit changedPlaybackQueue();
}

/* ==== Context & Repo 绑定 ==== */
void PlaylistViewModel::setPlaylist(const playlistId& playlist_id) {
    if (m_playlistId == playlist_id) { return; }
    m_playlistId = playlist_id;
    this->rebuild();
}

void PlaylistViewModel::setSortMode(SortType sort_type) {
    if (m_sort_type == sort_type) { return; }
    m_sort_type = sort_type;
}

void PlaylistViewModel::clear() {
    m_playbackQueue.clear();
    m_metaCache.clear();
}

/* ==== View视图数据访问 ====*/
int PlaylistViewModel::rowCount() const {
    return m_playbackQueue.size();
}

// QAbstractTableModel Interface
int PlaylistViewModel::rowCount(const QModelIndex &parent = QModelIndex()) const {
    return m_playbackQueue.size();
}

int PlaylistViewModel::columnCount(const QModelIndex &parent = QModelIndex()) const {
    qDebug() << "[DEBUG] Let columnCount = 7";
    return 6;
}

QVariant PlaylistViewModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_playbackQueue.size()) {
        return QVariant();
    }

    if (role = Qt::DisplayRole) {
        const TrackMetaData& d = m_metaCache.find(m_playbackQueue.at(index.column())).value();
        switch (index.column()) {
        case 0: return d.disc_number;
        case 1: return d.track_number;
        case 2: return d.title;
        case 3: return d.artist;
        case 4: return d.duration_ms;
        case 5: return d.album;
        default: break;
        }
    }
    return QVariant();
}

QVariant PlaylistViewModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole) {
        return QAbstractTableModel::headerData(section, orientation, role);
    }

    if (role == Qt::DisplayRole && orientation == Qt::Orientation::Horizontal) {
        switch (section) {
        case 0: return "disc_number";
        case 1: return "track_number";
        case 2: return "title";
        case 3: return "artist";
        case 4: return "duration_ms";
        case 5: return "album";
        }
        qDebug() << "[WARNING] uncompleted: m_horizontalHead";
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}


trackId PlaylistViewModel::trackAt(int index) const {
    return m_playbackQueue.at(index);
}

const QVector<trackId>& PlaylistViewModel::playbackQueue() const {
    return m_playbackQueue;
}

bool PlaylistViewModel::hasMetaData(const trackId& track_id) const {
    // find in m_metaCache
    auto t = m_metaCache.find(track_id);    //QUuid作为key无需考虑重复项
    if (m_metaCache.contains(track_id)) {
        return true;
    }
    return false;
}

TrackMetaData PlaylistViewModel::metaData(const trackId& track_id) const {
    // need bound check?
    return m_metaCache.find(track_id).value();
}


/* ==== 播放顺序辅助（用于Player） ==== */
trackId PlaylistViewModel::nextOf(const trackId& track_id) const {
    for (auto it = m_playbackQueue.begin(); it != m_playbackQueue.end(); ++it) {
        if (*it == track_id) {
            if (it + 1 != m_playbackQueue.end()) {
                return *(it + 1);
            }
            else {
                // return what? circulation / null?
                return QUuid();
            }
        }
    }
    return QUuid();
}

trackId PlaylistViewModel::previousOf(const trackId& track_id) const {
    for (auto it = m_playbackQueue.begin(); it != m_playbackQueue.end(); ++it) {
        if (*it == track_id) {
            if (it != m_playbackQueue.begin()) {
                return *(it - 1);
            }
            else {
                return QUuid();
            }
        }
    }
    return QUuid();
}


/* ==== 元数据请求 ==== */
void PlaylistViewModel::requestMetaData(const trackId& track_id) {
    if (m_metaCache.contains(track_id)) {
        return;
    }
    // emit requestedMetaData();
}