#include "playlist_manager.h"

PlaylistManager::PlaylistManager(QObject* parent)
    : QObject(parent)
{
    connect(m_context, &PlaylistContext::changedCurrentListId
            , m_view, &PlaylistViewModel::setPlaylist);
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

void PlaylistManager::loadPlaylist() {
    qDebug() << "[WARNING] stub: load playlist";
}

void PlaylistManager::renamePlaylist() {
    qDebug() << "[WARNING] stub: rename playlist";
}

void PlaylistManager::savePlaylist() {
    qDebug() << "[WARNING] stub: save playlist";
}


void PlaylistManager::addTrack(const QString& filepath) {
    auto curr_playlist_id = m_context->getPlaylistId();
    m_repo->addTrackToPlaylist(curr_playlist_id, filepath);
    qDebug() << "[INFO] Add track " << filepath << " to playlist " << curr_playlist_id;
}