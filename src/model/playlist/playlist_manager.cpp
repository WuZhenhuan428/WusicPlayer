#include "playlist_manager.h"

#include <QTimer>

PlaylistManager::PlaylistManager(QObject* parent)
    : QObject(parent)
{
    m_context = new PlaylistContext(this);
    m_repo = new PlaylistRepo(this);
    m_view = new PlaylistViewModel(m_repo, this);

    connect(m_context, &PlaylistContext::changedCurrentListId
            , m_view, &PlaylistViewModel::setPlaylist);
    connect(m_context, &PlaylistContext::changedCurrentTrackId,
            m_view, &PlaylistViewModel::setActiveTrack);

    connect(m_repo, &PlaylistRepo::playlistChanged, this, &PlaylistManager::retransmissionPlaylistChanged);
    connect(m_repo, &PlaylistRepo::cacheLoadStarted, this, &PlaylistManager::cacheLoadStarted);
    connect(m_repo, &PlaylistRepo::playlistLoadStarted, this, &PlaylistManager::playlistLoadStarted);
    connect(m_repo, &PlaylistRepo::playlistLoadFinished, this, &PlaylistManager::playlistLoadFinished);
    connect(m_repo, &PlaylistRepo::cacheLoadFinished, this, [this](int count) {
        emit cacheLoadFinished(count);
        if (m_context->getPlaylistId().isNull()) {
            auto lists = m_repo->getLists();
            if (!lists.isEmpty()) {
                m_context->setPlaylist(lists.first()->id());
            }
        }
    });

}

PlaylistManager::~PlaylistManager() {}

void PlaylistManager::createPlaylist() {
    m_repo->createList();
}

void PlaylistManager::removePlaylist(const playlistId& to_remove_uuid) {
    const auto& pl = m_repo->findPlaylistById(to_remove_uuid);
    if (!pl) return;

    m_repo->removeList(to_remove_uuid);
    if (m_context->getPlaylistId() == to_remove_uuid) {
        auto remaining = m_repo->getLists();
        if (!remaining.isEmpty()) {
            m_context->setPlaylist(remaining.first()->id());
        } else {
            m_context->setPlaylist(playlistId());
        }
    }
}

void PlaylistManager::copyPlaylist(const playlistId& pid) {
    m_repo->copyList(pid);
}

void PlaylistManager::loadPlaylist(const QString& playlist_path) {
    playlistId new_id = m_repo->loadListBatched(playlist_path, 500);
    if (!new_id.isNull()) {
        m_context->setPlaylist(new_id);
    }
}

void PlaylistManager::renamePlaylist(const playlistId& src_pid, const QString dst_name) {
    m_repo->renameList(src_pid, dst_name);
}

void PlaylistManager::savePlaylist(const playlistId& pid, const QString& save_path) {
    auto pl = m_repo->findPlaylistById(pid);
    if (!pl->isEmpty()) {
        m_repo->saveList(pid, save_path);
        qDebug() << "[INFO] save playlist " << pid.toString() << " at " << save_path;
    }
}

void PlaylistManager::loadCacheAfterShown() {
    m_repo->loadCacheAsync();
}


void PlaylistManager::addTrack(const playlistId& pid, const QString& filepath) {
    m_repo->addTrackToPlaylist(pid, filepath);
}

// a wrap of this->addTrack
void PlaylistManager::addFolder(const playlistId& pid, const QString& directory) {
    playlistId curr_pid = pid;
    if (pid.isNull()) {
        curr_pid = m_repo->createList();
        m_context->setPlaylist(curr_pid);
    }

    const auto& files = AudioUtils::findAll(directory.toStdString());
    QStringList tracksToAdd;
    tracksToAdd.reserve(static_cast<int>(files.size()));

    for(const auto& file : files) {
        if (AudioUtils::isAudioFile(file)) {
            tracksToAdd.append(QString::fromStdString(file));
        }
    }

    if (!tracksToAdd.isEmpty()) {
        m_repo->addTracksToPlaylist(curr_pid, tracksToAdd);
    }
}

QString PlaylistManager::nextTrack(PlayMode mode) {
    auto pl = m_repo->findPlaylistById(m_context->getPlaylistId());
    if (!pl) {
        return QString();
    }
    trackId next_id = trackId();
    trackId curr_id = m_context->getPlayTrackId();
    PlaybackQueueSnapshot queue;
    if (mode == PlayMode::in_order){
        next_id = PlaylistNavigator::nextOfInOrder(m_view->playbackQueueSnapshot().queue, curr_id);
    } else if (mode == PlayMode::loop) {
        next_id = PlaylistNavigator::nextOfLoop(m_view->playbackQueueSnapshot().queue, curr_id);
    } else if (mode == PlayMode::shuffle) {
        next_id = PlaylistNavigator::nextOfShuffle(m_view->playbackQueueSnapshot().queue, curr_id);
    } else if (mode == PlayMode::out_of_order_track) {
        next_id = PlaylistNavigator::nextOfOutOfOrderTrack(m_view->singleShuffleQueueSnapshot().queue, curr_id);
    } else if (mode == PlayMode::out_of_order_group) {
        next_id = PlaylistNavigator::nextOfOutOfOrderGroup(m_view->groupShuffleQueueSnapshot().queue, curr_id);
    }

    if (!next_id.isNull()) {
        m_context->setPlayTrack(next_id);
        auto track = pl->findTrackByID(next_id);
        if (track) {
            return track->filepath;
        }
    }
    return QString();
}

QString PlaylistManager::prevTrack(PlayMode mode) {
    auto pl = m_repo->findPlaylistById(m_context->getPlaylistId());
    if (!pl) {
        return QString();
    }

    trackId prev_id = trackId();
    trackId curr_id = m_context->getPlayTrackId();
    PlaybackQueueSnapshot queue;
    if (mode == PlayMode::in_order){
        prev_id = PlaylistNavigator::previousOfInOrder(m_view->playbackQueueSnapshot().queue, curr_id);
    } else if (mode == PlayMode::loop) {
        prev_id = PlaylistNavigator::previousOfLoop(m_view->playbackQueueSnapshot().queue, curr_id);
    } else if (mode == PlayMode::shuffle) {
        prev_id = PlaylistNavigator::previousOfShuffle(m_view->playbackQueueSnapshot().queue, curr_id);
    } else if (mode == PlayMode::out_of_order_track) {
        prev_id = PlaylistNavigator::previousOfOutOfOrderTrack(m_view->singleShuffleQueueSnapshot().queue, curr_id);
    } else if (mode == PlayMode::out_of_order_group) {
        prev_id = PlaylistNavigator::previousOfOutOfOrderGroup(m_view->groupShuffleQueueSnapshot().queue, curr_id);
    }

    if (!prev_id.isNull()) {
        m_context->setPlayTrack(prev_id);
        auto track = pl->findTrackByID(prev_id);
        if (track) {
            return track->filepath;
        }
    }
    return QString();
}


PlaylistViewModel* PlaylistManager::getViewModel() {
    return this->m_view;
}

void PlaylistManager::play(int index) {
    trackId id = m_view->trackAt(index);
    m_context->setPlayTrack(id);

    auto listId = m_context->getPlaylistId();
    auto playlist = m_repo->findPlaylistById(listId);
    if (!playlist) {
        return;
    }
    Track* t = playlist->findTrackByID(id);
    if (t) {
        emit requestPlay(t->filepath);
    }
}

QString PlaylistManager::getCurrentTrack() const {
    trackId tid = m_context->getPlayTrackId();
    auto pl = m_repo->findPlaylistById(m_context->getPlaylistId());
    if (!pl) {
        return QString();
    }
    Track* track = pl->findTrackByID(tid);
    if (!track) {
        return QString();
    }
    return track->filepath;
}

QString PlaylistManager::getCurrentPlaylistName() const {
    playlistId pid = m_context->getPlaylistId();
    auto pl = m_repo->findPlaylistById(pid);
    return pl->name();
}

const trackId& PlaylistManager::getCurrentTrackId() const {
    return this->m_context->getPlayTrackId();
}

const playlistId& PlaylistManager::getCurrentPlaylist() const{
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

void PlaylistManager::switchToPlaylist(const playlistId& pid) {
    m_context->setPlaylist(pid);
}

QVector<std::shared_ptr<Playlist>> PlaylistManager::getPlaylists() {
        return m_repo->getLists();
}

TrackMetaData PlaylistManager::getCurrentMetadata() {
    trackId tid = m_context->getPlayTrackId();
    auto playlist = m_repo->findPlaylistById(m_context->getPlaylistId());

    if (playlist) {
        Track* track = playlist->findTrackByID(tid);
        if (track) {
            return track->meta;
        }
    }
    TrackMetaData empty_meta;
    empty_meta.isValid = false;
    return empty_meta;
}


QString PlaylistManager::getPlaylistById(const playlistId& pid) const {
    auto pl = m_repo->findPlaylistById(pid);
    if (!pl->isEmpty()) {
        return pl->name();
    }
    return QString();
}