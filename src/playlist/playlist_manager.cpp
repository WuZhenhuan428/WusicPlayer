#include "playlist_manager.h"

PlaylistManager::PlaylistManager(QObject* parent)
    : QObject(parent)
{
    connect(m_context, &PlaylistContext::changedCurrentListId
            , m_view, &PlaylistViewModel::setPlaylist);
    connect(m_context, &PlaylistContext::changedCurrentTrackId,
            m_view, &PlaylistViewModel::setActiveTrack);

    connect(m_repo, &PlaylistRepo::playlistChanged, this, &PlaylistManager::retransmissionPlaylistChanged);
}

PlaylistManager::~PlaylistManager() {}

void PlaylistManager::createPlaylist() {
    m_repo->createList();
}

void PlaylistManager::removePlaylist(const QUuid& to_remove_uuid) {
    const auto& pl = m_repo->findPlaylistById(to_remove_uuid);
    if (!pl) return;

    m_repo->removeList(to_remove_uuid);
    if (m_context->getPlaylistId() == to_remove_uuid) {
        auto remaining = m_repo->getLists();
        if (!remaining.isEmpty()) {
            m_context->setPlaylist(remaining.first()->id());
        } else {
            m_context->setPlaylist(QUuid());
        }
    }
}

void PlaylistManager::copyPlaylist(const QUuid& playlist_id) {
    m_repo->copyList(playlist_id);
}

void PlaylistManager::loadPlaylist(const QString& playlist_path) {
    QUuid new_id = m_repo->loadList(playlist_path);
    if (!new_id.isNull()) {
        m_context->setPlaylist(new_id);
    }
}

void PlaylistManager::renamePlaylist(const QUuid& src_uuid, const QString dst_name) {
    const auto& temp = m_repo->findPlaylistById(src_uuid);
    if (temp) {
        auto src_name = temp->name();
        temp->setPlaylistName(dst_name);
        emit m_repo->playlistChanged();
        qDebug() << "[INFO] set playlist" << src_name << "to" <<temp->name();
    } else {
        qDebug() << "[WARNING] Playlist" << src_uuid << "does not exist";
    }
}

void PlaylistManager::saveCurrentPlaylist(const QString& save_path) {
    QUuid uuid = m_context->getPlaylistId();
    m_repo->saveList(uuid, save_path);
    qDebug() << "[INFO] save playlist " << uuid.toString() << " at " << save_path;
}


void PlaylistManager::addTrack(const QString& filepath) {
    auto curr_playlist_id = m_context->getPlaylistId();
    if (curr_playlist_id.isNull()) {
        m_repo->createList();
        auto lists = m_repo->getLists();
        if (!lists.isEmpty()) {
            curr_playlist_id = lists.last()->id();
            m_context->setPlaylist(curr_playlist_id);
        }
    }
    m_repo->addTrackToPlaylist(curr_playlist_id, filepath);
}


/**
 * @brief: PlaylistManager::addTrack的包装
 */
void PlaylistManager::addFolder(const QString& directory) {
    // +++ wrap to a method
    auto curr_playlist_id = m_context->getPlaylistId();
    if (curr_playlist_id.isNull()) {
        m_repo->createList();
        auto lists = m_repo->getLists();
        if (!lists.isEmpty()) {
            curr_playlist_id = lists.last()->id();
            m_context->setPlaylist(curr_playlist_id);
        }
    }
    // ---

    const auto& files = Audio::findAll(directory.toStdString());
    auto list = m_repo->findPlaylistById(curr_playlist_id);
    list->m_tracks.reserve(list->m_tracks.size() + static_cast<int>(files.size()));
    
    QStringList tracksToAdd;
    tracksToAdd.reserve(static_cast<int>(files.size()));

    for(const auto& file : files) {
        if (Audio::isAudioFile(file)) {
            tracksToAdd.append(QString::fromStdString(file));
        }
    }

    if (!tracksToAdd.isEmpty()) {
        m_repo->addTracksToPlaylist(curr_playlist_id, tracksToAdd);
    }
    list->m_tracks.shrink_to_fit();
}

QString PlaylistManager::nextTrack() {
    auto pl = m_repo->findPlaylistById(m_context->getPlaylistId());
    auto next_id = m_view->nextOf(m_context->getPlayTrackId());
    if (!next_id.isNull()) {
        m_context->setPlayTrack(next_id);
        auto track = pl->findTrackByID(next_id);
        return track->filepath;
    } else {
        return QString();
    }
}

QString PlaylistManager::prevTrack() {
    auto pl = m_repo->findPlaylistById(m_context->getPlaylistId());
    auto prev_id = m_view->previousOf(m_context->getPlayTrackId());
    if (!prev_id.isNull()) {
        m_context->setPlayTrack(prev_id);
        auto track = pl->findTrackByID(prev_id);
        return track->filepath;
    } else {
        return QString();
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

const QString& PlaylistManager::getCurrentTrack() {
    QUuid track_id = m_context->getPlayTrackId();
    auto pl = m_repo->findPlaylistById(m_context->getPlaylistId());
    return pl->findTrackByID(track_id)->filepath;
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