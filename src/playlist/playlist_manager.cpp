#include "playlist_manager.h"

PlaylistManager::PlaylistManager(QObject* parent)
    : QObject(parent)
{
    connect(m_context, &PlaylistContext::changedCurrentListId
            , m_view, &PlaylistViewModel::setPlaylist);

    // Initialize with a default playlist
    // QUuid defaultId = m_repo->createList();
    // m_context->setPlaylist(defaultId);
    connect(m_repo, &PlaylistRepo::playlistChanged, this, &PlaylistManager::retransmissionPlaylistChanged);
}

PlaylistManager::~PlaylistManager() {}

void PlaylistManager::createPlaylist() {
    m_repo->createList();
}

void PlaylistManager::removePlaylist() {
    qDebug() << "[WARNING] stub: remove playlist";
}

void PlaylistManager::copyPlaylist(const QUuid& playlist_id) {
    m_repo->copyList(playlist_id);
    qDebug() << "[INFO] copy-and-paste playlist";
}

void PlaylistManager::loadPlaylist(const QString& playlist_path) {
    QUuid new_id = m_repo->loadList(playlist_path);
    if (!new_id.isNull()) {
        m_context->setPlaylist(new_id);
    }
}

void PlaylistManager::renamePlaylist() {
    qDebug() << "[WARNING] stub: rename playlist";
}

void PlaylistManager::saveCurrentPlaylist(const QString& save_path) {
    QUuid uuid = m_context->getPlaylistId();
    m_repo->saveList(uuid, save_path);
    qDebug() << "[INFO] save playlist " << uuid.toString() << " at " << save_path;
}


void PlaylistManager::addTrack(const QString& filepath) {
    auto curr_playlist_id = m_context->getPlaylistId();
    m_repo->addTrackToPlaylist(curr_playlist_id, filepath);
}


/**
 * @brief: PlaylistManager::addTrack的包装
 */
void PlaylistManager::addFolder(const QString& directory) {
    const auto& files = Audio::findAll(directory.toStdString());
    for(const auto& file : files) {
        if (Audio::isAudioFile(file)) {
            addTrack(file.c_str());
        }
    }
}

PlaylistViewModel* PlaylistManager::getViewModel() {
    return this->m_view;
}

void PlaylistManager::play(int index) {
    trackId id = m_view->trackAt(index);
    m_context->setPlayTrack(id);

    auto listId = m_context->getPlaylistId();
    auto playlist = m_repo->findPlaylistById(listId);
    if (playlist) {
        Track* t = playlist->findTrackByID(id);
        if (t) {
            emit requestPlay(t->filepath);
        }
    }
}

const QUuid& PlaylistManager::getCurrentPlaylist() const{
    return this->m_context->getPlaylistId();
}

QVector<PlaylistInfo> PlaylistManager::getAllPlaylists() {
    QVector<PlaylistInfo> infos;

    auto playlists = m_repo->getLists();
    for(const auto& pl : playlists) {
        infos.append({pl->id(), pl->name()});
    }
    return infos;
}

void PlaylistManager::retransmissionPlaylistChanged() {
    emit playlistChanged();
}

void PlaylistManager::switchToPlaylist(const QUuid& id) {
    m_context->setPlaylist(id);
}

QVector<std::shared_ptr<Playlist>> PlaylistManager::getPlaylists() {
        return m_repo->getLists();
}