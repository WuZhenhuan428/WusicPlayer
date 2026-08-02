#include "playlist.h"

#include <QFileInfo>

/**
 * @brief: 创建播放列表时生成UUID
 */
Playlist::Playlist(const QString& name)
{
    m_name = name;
    m_pid  = PlaylistId::createUuid();
    qDebug() << "[INFO] Create playlist uuid: " << m_pid.toString();
}

Playlist::~Playlist()
{
    qDebug() << "[INFO] Remove playlist uuid: " << m_pid.toString();
}

/**
 * @brief: 清除列表内容并回收空间
 */
void Playlist::clearList()
{
    m_tracks.clear();
    m_tracks.shrink_to_fit();
}
/**
 * @brief: 添加音轨, 原来为空时自动指向第一首
 * @return: 所添加音轨的Uuid
 */
Track Playlist::addTrack(const QString& filepath)
{
    Track t = Track::from_filepath(filepath);
    addTrackObject(t);
    return t;
}

Track Playlist::addTrackWithId(const EntryId& tid, const QString& filepath)
{
    Track t = Track::from_entry(tid, filepath);
    addTrackObject(t);
    return t;
}

void Playlist::addTrackObject(const Track& track)
{
    m_tracks.emplace_back(track);
}

bool Playlist::updateTrackMeta(const EntryId& tid, const TrackMetaData& meta)
{
    for (auto it = m_tracks.begin(); it != m_tracks.end(); ++it) {
        if (it->entry_id == tid) {
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
void Playlist::removeTrack(const EntryId& tid)
{
    for (auto it = m_tracks.begin(); it != m_tracks.end(); ++it) {
        if (it->entry_id == tid) {
            QString path      = it->filepath;
            EntryId removedId = it->entry_id;
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
bool Playlist::isEmpty()
{
    return m_tracks.empty();
};

PlaylistId Playlist::id() const
{
    return m_pid;
}

QString Playlist::name()
{
    return m_name;
}

void Playlist::setPlaylistName(QString setname)
{
    m_name = setname;
}

void Playlist::newUuid()
{
    m_pid = PlaylistId::createUuid();
}

void Playlist::newUuid(const PlaylistId& pid)
{
    m_pid = pid;
}

const Track* Playlist::findTrackByID(const EntryId& eid) const
{
    for (auto it = m_tracks.begin(); it != m_tracks.end(); ++it) {
        if (it->entry_id == eid) {
            return &(*it);
            qDebug() << "[INFO] find track " << it->entry_id << " at playlist " << m_name;
        }
    }
    qDebug() << "[WARNING] track " << eid << " does not exist!";
    return nullptr;
}

const QVector<Track>& Playlist::getTracks() const
{
    return m_tracks;
}

size_t Playlist::track_count()
{
    return m_tracks.size();
}
