#include "playlist_repo.h"

PlaylistRepo::PlaylistRepo(QObject *parent)
    : QObject(parent)
{}

PlaylistRepo::~PlaylistRepo() {}

void PlaylistRepo::clearList() {
    m_list.clear();
}

void PlaylistRepo::createList() {
    QString default_name = QString("New playlist %1").arg(m_list.size()+1);
    auto new_playlist = std::make_shared<Playlist>();
    new_playlist->setPlaylistName(default_name);
    m_list.push_back(new_playlist);
    emit playlistChanged();
}

/* ---- stub ---- */
void PlaylistRepo::loadList(const QString& filepath) {
    qDebug() << "[INFO] Stub of load file: " << filepath;
    // Playlist dst = Audio::parse_list_file(fp);
    // dst.newUuid();
    // m_list.push_back(dst);
    // emit playlistChanged();
}

/* ---- stub ---- */
void PlaylistRepo::saveList(const QUuid& uuid, const QString& toPath) {
    std::shared_ptr<Playlist> src = findPlaylistById(uuid);
    if (!src) {
        qDebug() << "[WARNING] Stub: save playlist (" << uuid.toString() << ") to " << toPath.toStdString();
        // ...
    }
    qDebug() << "[WARNING] stub: save list as external file.";
    return;
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
    for (const auto& it : m_list) {
        if (it->id() == uuid) {
            return it;
        }
    }
    qDebug() << "[WARNING] Playlist doen not exist, UUID=" << uuid.toString();
    return nullptr;
}
