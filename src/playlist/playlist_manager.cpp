#include "playlist_manager.h"

PlaylistManager::PlaylistManager(QObject* parent)
    : QObject(parent)
{
    connect(m_context, &PlaylistContext::changedCurrentListId
            , m_view, &PlaylistViewModel::setPlaylist);

    // Initialize with a default playlist
    QUuid defaultId = m_repo->createList();
    m_context->setPlaylist(defaultId);
}

PlaylistManager::~PlaylistManager() {}

void PlaylistManager::createPlaylist() {
    m_repo->createList();
}

void PlaylistManager::removePlaylist() {
    qDebug() << "[WARNING] stub: remove playlist";
}

void PlaylistManager::copyPlaylist() {
    // m_repo->copyList(const QUuid &src);
    qDebug() << "[WARNING] stub: copy-and-past playlist";
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
    qDebug() << "[INFO] Add track " << filepath << " to playlist " << curr_playlist_id;
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