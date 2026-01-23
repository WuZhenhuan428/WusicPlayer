#include "playlist.h"

/**
 * @brief: 创建播放列表时生成UUID
 */
Playlist::Playlist(const QString& name) {
    m_name = name;
    m_uuid = QUuid::createUuid();
    qDebug() << "[INFO] Create QUUID is: " << m_uuid.toString();
}

Playlist::~Playlist() {
    qDebug() << "[INFO] Remove QUUID is: " << m_uuid.toString();
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
    m_tracks.push_back(t);
    qDebug() << "[INFO] Add uuid: " << t.uuid << "filepath: " << t.filepath;
    return t;
}

/**
 * @brief: 递归查找指定目录，并添加音频文件
 */
void Playlist::addFolder(const QString& directory) {
    auto files = Audio::findAll(directory.toStdString());
    for (const auto& file : files) {
        if (Audio::isAudioFile(file)) {
            addTrack(QString::fromStdString(file.string()));
        }
    }
}

/**
 * @brief: 查找并删除音轨
 * @note: 如果删除当前音轨, 则暂停播放
 */
void Playlist::removeTrack(const QUuid& uuid) {
    for (auto it = m_tracks.begin(); it != m_tracks.end(); ++it) {
        if (it->uuid == uuid) {
            QString path = it->filepath;
            QUuid removedId = it->uuid;
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

QString Playlist::getPlaylistName() {
    return m_name;
}

void Playlist::setPlaylistName(QString setname) {
    m_name = setname;
}

/**
 * @brief: 创建新的Uuid并覆盖原有Uuid，主要用于列表的复制
 */
void Playlist::newUuid() {
    m_uuid = QUuid::createUuid();
}

/**
 * @brief: 获取uuid，主要用于「当前列表」的切换，使用时应当配合对应的emit信号
 */
void Playlist::newUuid(const QUuid& uuid) {
    m_uuid = uuid;
}

/**
 * @brief: 通过uuid查找音轨位置并返回Track指针
 * @return：Track*
 */
Track* Playlist::findTrackByID(const QUuid& uuid) {
    for (auto it = m_tracks.begin(); it != m_tracks.end(); ++it) {
        if (it->uuid == uuid) {
            return &(*it);
            qDebug() << "[INFO] find track " << it->uuid << " at playlist " << m_name;
        }
    }
    qDebug() << "[WARNING] track " << uuid << " does not exist!";
    return nullptr;
}

/**
 * @brief：获取音轨全部id
 */
const QVector<Track>& Playlist::getTracks() const{
    return m_tracks;
}