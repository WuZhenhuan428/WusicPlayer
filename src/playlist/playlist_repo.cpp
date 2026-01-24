#include "playlist_repo.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>

PlaylistRepo::PlaylistRepo(QObject *parent)
    : QObject(parent)
{}

PlaylistRepo::~PlaylistRepo() {}

void PlaylistRepo::clearList() {
    m_list.clear();
}

QUuid PlaylistRepo::createList() {
    QString default_name = QString("New playlist %1").arg(m_list.size()+1);
    auto new_playlist = std::make_shared<Playlist>();
    new_playlist->setPlaylistName(default_name);
    m_list.push_back(new_playlist);
    emit playlistChanged();
    return new_playlist->id();
}

/* ---- load list from file ---- */
QUuid PlaylistRepo::loadList(const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "[WARNING] Failed to open file for loading:" << filepath;
    }

    auto new_playlist = std::make_shared<Playlist>();
    QFileInfo fileInfo(filepath);
    // 使用文件名作为列表名
    new_playlist->setPlaylistName(fileInfo.baseName());

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty()) {
            // 这里简单地假设每一行都是有效路径
            // 实际生产中可能需要检查 QFile::exists(line)
            new_playlist->addTrack(line);
        }
    }
    
    m_list.push_back(new_playlist);
    emit playlistChanged();
    qDebug() << "[INFO] Loaded playlist from:" << filepath;
    return new_playlist->id();
}

/* ---- save list to file ---- */
void PlaylistRepo::saveList(const QUuid& uuid, const QString& toPath) {
    std::shared_ptr<Playlist> src = findPlaylistById(uuid);
    if (!src) {
        qDebug() << "[WARNING] Playlist (" << uuid.toString() << ") not found, save failed.";
        return;
    }

    QFile file(toPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "[WARNING] Failed to open file for saving:" << toPath;
        return;
    }

    QTextStream out(&file);
    // 遍历所有音轨并写入路径
    const QVector<Track>& tracks = src->getTracks();
    for (const auto& track : tracks) {
        out << track.filepath << "\n";
    }

    qDebug() << "[INFO] Saved playlist to:" << toPath;
}

void PlaylistRepo::removeList(QUuid& uuid) {
    std::shared_ptr<Playlist> src = findPlaylistById(uuid);
    if (!src) {
        qDebug() << "[WARNING] Playlist " << uuid <<"not found";
        return;
    }
    m_list.removeOne(src);
    emit playlistChanged();
}

/**
 * @note: this function means "copy-and-paste", but not copy only
 */
void PlaylistRepo::copyList(const QUuid& src_uuid) {
    std::shared_ptr<Playlist> src = findPlaylistById(src_uuid);
    
    if (!src) {
        qDebug() << "[WARNING] Source playlist " << src_uuid <<"not found";
        return;
    }
    
    // deep-copy
    auto new_playlist = std::make_shared<Playlist>(*src);
    new_playlist->newUuid();
    m_list.push_back(new_playlist);

    emit playlistChanged();
}

/**
 * @note: 如果emit过多，可以考虑将add_one_track包装为两个函数，分别在两个函数的末尾进行emit playlistChanged();
 */
void PlaylistRepo::addTrackToPlaylist(const QUuid& playlistId, const QString& filepath) {
    std::shared_ptr<Playlist> src = findPlaylistById(playlistId);
    if (!src) {
        qDebug() << "[WARNING] Playlist id " << playlistId.toString() << "not found";
        return;
    }
    qDebug() << "[INFO] Add track " << filepath << "to " << playlistId.toString();

    Track newTrack = src->addTrack(filepath);
    emit playlistChanged();
}

bool PlaylistRepo::isEmpty() {
    m_list.shrink_to_fit();
    if (m_list.size() == 0) {
        return true;
    }
    return false;
}

/**
 * @todo: 已经切换shared_ptr，确保shared_ptr的使用正确
 */
//note: 关于使用QVector<Playlist*>类型：需要手动控制指针生命周期以及手动释放内存，不建议使用
std::shared_ptr<Playlist> PlaylistRepo::findPlaylistById(const QUuid& uuid) {
    qDebug() << "[DEBUG] current uuid is: " << uuid.toString();
    for (const auto& it : m_list) {
        if (it->id() == uuid) {
            qDebug() << "[DEBUG] iterator's uuid is: " << it->id().toString();
            return it;
        }
    }
    qDebug() << "[WARNING] Playlist does not exist, UUID=" << uuid.toString();
    return nullptr;
}
