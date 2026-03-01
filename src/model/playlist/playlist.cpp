#include "playlist.h"

#include <QFileInfo>

/**
 * @brief: 创建播放列表时生成UUID
 */
Playlist::Playlist(const QString& name) {
    m_name = name;
    m_pid = playlistId::createUuid();
    qDebug() << "[INFO] Create playlist uuid: " << m_pid.toString();
}

Playlist::~Playlist() {
    qDebug() << "[INFO] Remove playlist uuid: " << m_pid.toString();
}

/**
 * @brief: 清除列表内容并回收空间
 */
void Playlist::clearList() {
    m_tracks.clear();
    m_tracks.shrink_to_fit();
}
/**
 * @brief: 添加音轨, 原来为空时自动指向第一首
 * @return: 所添加音轨的Uuid
 */
Track Playlist::addTrack(const QString& filepath) {
    Track t;
    t.filepath = filepath;
    t.meta.filepath = filepath;
    t.meta.filename = QFileInfo(filepath).fileName();
    t.meta.isValid = false;
    m_tracks.emplace_back(t);
    // qDebug() << "[INFO] Add uuid:" << t.tid << "filepath:" << t.filepath;
    return t;
}

Track Playlist::addTrackWithId(const trackId& tid, const QString& filepath) {
    Track t;
    t.tid = tid;
    t.filepath = filepath;
    t.meta.filepath = filepath;
    t.meta.filename = QFileInfo(filepath).fileName();
    t.meta.isValid = false;
    m_tracks.emplace_back(t);
    // qDebug() << "[INFO] Add uuid:" << t.tid << "filepath:" << t.filepath;
    return t;
}

bool Playlist::updateTrackMeta(const trackId& tid, const TrackMetaData& meta) {
    for (auto it = m_tracks.begin(); it != m_tracks.end(); ++it) {
        if (it->tid == tid) {
            it->meta = meta;
            if (it->meta.filepath.isEmpty()) {
                it->meta.filepath = it->filepath;
            }
            if (it->meta.filename.isEmpty()) {
                it->meta.filename = QFileInfo(it->filepath).fileName();
            }
            return true;
        }
    }
    return false;
}

/**
 * @brief: 查找并删除音轨
 * @note: 如果删除当前音轨, 则暂停播放
 */
void Playlist::removeTrack(const trackId& tid) {
    for (auto it = m_tracks.begin(); it != m_tracks.end(); ++it) {
        if (it->tid == tid) {
            QString path = it->filepath;
            trackId removedId = it->tid;
            m_tracks.erase(it);

            qDebug() << "[INFO] Remove UUID=" << removedId << ", filepath=" << path;
            return;
        }
    }

    qDebug() << "[WARNING] file does not in playlist!";
};

/**
 * @return: 检查播放列表是否为空并返回bool
 */
bool Playlist::isEmpty() {
    return m_tracks.empty();
};

playlistId Playlist::id() const {
    return m_pid;
}

QString Playlist::name() {
    return m_name;
}

void Playlist::setPlaylistName(QString setname) {
    m_name = setname;
}

void Playlist::newUuid() {
    m_pid = playlistId::createUuid();
}


void Playlist::newUuid(const playlistId& pid) {
    m_pid = pid;
}

Track* Playlist::findTrackByID(const trackId& tid) {
    for (auto it = m_tracks.begin(); it != m_tracks.end(); ++it) {
        if (it->tid == tid) {
            return &(*it);
            qDebug() << "[INFO] find track " << it->tid << " at playlist " << m_name;
        }
    }
    qDebug() << "[WARNING] track " << tid << " does not exist!";
    return nullptr;
}

const QVector<Track>& Playlist::getTracks() const{
    return m_tracks;
}