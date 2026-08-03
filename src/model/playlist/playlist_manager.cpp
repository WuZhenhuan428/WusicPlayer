#include "playlist_manager.h"

#include "core/utils/audio.hpp"
#include "core/utils/path.hpp"
#include "model/library/library_manager.h"
#include <QFileInfo>
#include <QTimer>

#include <optional>

PlaylistManager::PlaylistManager(QObject* parent) : QObject(parent)
{
    m_context = new PlaylistContext(this);
    m_repo    = new PlaylistRepo(this);
    m_view    = new PlaylistViewModel(m_repo, this);

    connect(m_context, &PlaylistContext::changedCurrentListId, m_view,
            &PlaylistViewModel::setPlaylist);
    connect(m_context, &PlaylistContext::changedCurrentTrackId, m_view,
            &PlaylistViewModel::setActiveTrack);

    connect(m_repo, &PlaylistRepo::playlistChanged, this,
            &PlaylistManager::retransmissionPlaylistChanged);
    connect(m_repo, &PlaylistRepo::cacheLoadStarted, this, &PlaylistManager::cacheLoadStarted);
    connect(m_repo, &PlaylistRepo::playlistLoadStarted, this,
            &PlaylistManager::playlistLoadStarted);
    connect(m_repo, &PlaylistRepo::playlistLoadFinished, this,
            &PlaylistManager::playlistLoadFinished);
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

void PlaylistManager::createPlaylist()
{
    m_repo->createList();
}

void PlaylistManager::removePlaylist(const PlaylistId& to_remove_uuid)
{
    const auto& pl = m_repo->findPlaylistById(to_remove_uuid);
    if (!pl)
        return;

    m_repo->removeList(to_remove_uuid);
    if (m_context->getPlaylistId() == to_remove_uuid) {
        auto remaining = m_repo->getLists();
        if (!remaining.isEmpty()) {
            m_context->setPlaylist(remaining.first()->id());
        } else {
            m_context->setPlaylist(PlaylistId());
        }
    }
}

void PlaylistManager::copyPlaylist(const PlaylistId& pid)
{
    m_repo->copyList(pid);
}

void PlaylistManager::loadPlaylist(const QString& playlist_path)
{
    PlaylistId new_id = m_repo->loadListBatched(playlist_path, 500);
    if (!new_id.isNull()) {
        m_context->setPlaylist(new_id);
    }
}

void PlaylistManager::renamePlaylist(const PlaylistId& src_pid, const QString dst_name)
{
    m_repo->renameList(src_pid, dst_name);
}

void PlaylistManager::savePlaylist(const PlaylistId& pid, const QString& save_path)
{
    auto pl = m_repo->findPlaylistById(pid);
    if (!pl->isEmpty()) {
        m_repo->saveList(pid, save_path);
        qDebug() << "[INFO] save playlist " << pid.toString() << " at " << save_path;
    }
}

void PlaylistManager::loadCacheAfterShown()
{
    m_repo->loadCacheAsync();
}

void PlaylistManager::addTrack(const PlaylistId& pid, const QString& filepath, AddFilePolicy policy)
{
    // 单文件默认:仅外部文件;import_to_library 时把父目录注册到库(扫描后 upgrade 为库引用)
    const AddFilePolicy eff = resolve_effective_policy(policy, AddFilePolicy::keep_external);
    if (m_library && eff == AddFilePolicy::import_to_library) {
        const QString norm = utils::path::normalize_path(filepath);
        if (!m_library->track_by_path(norm).has_value()) {
            m_library->add_watched_folder(QFileInfo(norm).absolutePath());
        }
    }
    m_repo->addTrackObject(pid, resolve_track(filepath));
}

// 通过音乐库解析曲目:库中有则引用库条目(元数据走库缓存);否则作为外部条目
Track PlaylistManager::resolve_track(const QString& filepath) const
{
    const QString norm = utils::path::normalize_path(filepath);
    if (m_library) {
        const auto lib = m_library->track_by_path(norm);
        if (lib) {
            Track t;
            t.source           = TrackSource::library;
            t.library_track_id = lib->track_id;
            t.filepath         = lib->filepath;
            t.meta             = lib->meta;
            t.missing          = lib->missing;
            return t;
        }
    }
    return Track::from_filepath(filepath); // 外部条目(不强制入库)
}

void PlaylistManager::removeTrack(const EntryId& tid)
{
    if (tid.isNull() || !m_repo || !m_context) {
        return;
    }

    auto playlist = m_repo->findPlaylistById(m_context->getPlaylistId());
    if (!playlist) {
        return;
    }

    const Track* track = playlist->findTrackByID(tid);
    if (!track) {
        return;
    }

    playlist->removeTrack(tid);
    m_repo->saveListToCache(playlist);

    if (m_context->getPlayTrackId() == tid) {
        m_context->setPlayTrack(EntryId());
    }

    if (m_view) {
        m_view->rebuildAsync();
    }

    emit playlistChanged();
}

void PlaylistManager::removeMissingTracks()
{
    auto playlist = m_repo->findPlaylistById(m_context->getPlaylistId());
    if (!playlist) {
        return;
    }
    if (playlist->removeMissingTracks() == 0) {
        return;
    }
    m_repo->saveListToCache(playlist);
    if (m_view) {
        m_view->rebuildAsync();
    }
    emit playlistChanged();
}

void PlaylistManager::set_library_manager(LibraryManager* lib)
{
    if (m_library == lib) {
        return;
    }
    if (m_library) {
        disconnect(m_library, nullptr, this, nullptr);
    }
    m_library = lib;
    if (m_library) {
        connect(m_library, &LibraryManager::sgn_library_changed, this,
                &PlaylistManager::on_library_changed);
    }
}

void PlaylistManager::set_add_file_policy(AddFilePolicy policy)
{
    m_add_file_policy = policy;
}

AddFilePolicy PlaylistManager::resolve_effective_policy(AddFilePolicy requested,
                                                        AddFilePolicy by_operation_default) const
{
    if (requested != AddFilePolicy::by_operation) {
        return requested; // 显式策略(import/external;ask 由上层展开,不应到达 model)
    }
    switch (m_add_file_policy) {
    case AddFilePolicy::import_to_library:
    case AddFilePolicy::keep_external:
        return m_add_file_policy;
    default: // by_operation / always_ask:按操作类型默认
        return by_operation_default;
    }
}

void PlaylistManager::on_library_changed()
{
    if (!m_library) {
        return;
    }
    bool changed = false;
    for (const auto& pl : m_repo->getLists()) {
        const int refreshed = pl->refreshLibraryTracks(
            [this](const TrackId& id) { return m_library->track_by_id(id); });
        const int upgraded = pl->upgradeExternalTracks(
            [this](const QString& path) { return m_library->track_by_path(path); });
        if (refreshed > 0 || upgraded > 0) {
            changed = true;
        }
    }
    if (changed) {
        if (m_view) {
            m_view->rebuildAsync();
        }
        emit playlistChanged();
    }
}

// a wrap of this->addTrack
void PlaylistManager::addFolder(const PlaylistId& pid, const QString& directory,
                                AddFilePolicy policy)
{
    // 文件夹添加默认同步入库:目录注册到库(异步扫描),未命中条目随后升级为库引用
    const AddFilePolicy eff = resolve_effective_policy(policy, AddFilePolicy::import_to_library);
    if (m_library && eff == AddFilePolicy::import_to_library) {
        m_library->add_watched_folder(directory);
    }

    PlaylistId curr_pid = pid;
    if (pid.isNull()) {
        curr_pid = m_repo->createList();
        m_context->setPlaylist(curr_pid);
    }

    const auto& files = utils::audio::find_all(directory);
    QVector<Track> tracks_to_add;
    tracks_to_add.reserve(static_cast<int>(files.size()));

    for (const auto& file : files) {
        if (utils::audio::is_audio_file(file)) {
            tracks_to_add.append(resolve_track(utils::audio::from_fs_path(file)));
        }
    }

    if (!tracks_to_add.isEmpty()) {
        m_repo->addTrackObjects(curr_pid, tracks_to_add);
    }
}

QString PlaylistManager::nextTrack(PlayMode mode)
{
    auto pl = m_repo->findPlaylistById(m_context->getPlaylistId());
    if (!pl) {
        return QString();
    }
    EntryId next_id = EntryId();
    EntryId curr_id = m_context->getPlayTrackId();
    PlaybackQueueSnapshot queue;
    switch (mode) {
    case PlayMode::in_order:
        next_id = PlaylistNavigator::nextOfInOrder(m_view->playbackQueueSnapshot().queue, curr_id);
        break;
    case PlayMode::loop:
        next_id = PlaylistNavigator::nextOfLoop(m_view->playbackQueueSnapshot().queue, curr_id);
        break;
    case PlayMode::shuffle:
        next_id = PlaylistNavigator::nextOfShuffle(m_view->playbackQueueSnapshot().queue);
        break;
    case PlayMode::out_of_order_track:
        next_id = PlaylistNavigator::nextOfOutOfOrderTrack(
            m_view->singleShuffleQueueSnapshot().queue, curr_id);
        break;
    case PlayMode::out_of_order_group:
        next_id = PlaylistNavigator::nextOfOutOfOrderGroup(
            m_view->groupShuffleQueueSnapshot().queue, curr_id);
        break;
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

QString PlaylistManager::prevTrack(PlayMode mode)
{
    auto pl = m_repo->findPlaylistById(m_context->getPlaylistId());
    if (!pl) {
        return QString();
    }

    EntryId prev_id = EntryId();
    EntryId curr_id = m_context->getPlayTrackId();
    PlaybackQueueSnapshot queue;
    switch (mode) {
    case PlayMode::in_order:
        prev_id =
            PlaylistNavigator::previousOfInOrder(m_view->playbackQueueSnapshot().queue, curr_id);
        break;
    case PlayMode::loop:
        prev_id = PlaylistNavigator::previousOfLoop(m_view->playbackQueueSnapshot().queue, curr_id);
        break;
    case PlayMode::shuffle:
        prev_id = PlaylistNavigator::previousOfShuffle(m_view->playbackQueueSnapshot().queue);
        break;
    case PlayMode::out_of_order_track:
        prev_id = PlaylistNavigator::previousOfOutOfOrderTrack(
            m_view->singleShuffleQueueSnapshot().queue, curr_id);
        break;
    case PlayMode::out_of_order_group:
        prev_id = PlaylistNavigator::previousOfOutOfOrderGroup(
            m_view->groupShuffleQueueSnapshot().queue, curr_id);
        break;
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

PlaylistViewModel* PlaylistManager::getViewModel()
{
    return this->m_view;
}

void PlaylistManager::play(int index)
{
    EntryId tid = m_view->trackAt(index);
    m_context->setPlayTrack(tid);

    auto pid      = m_context->getPlaylistId();
    auto playlist = m_repo->findPlaylistById(pid);
    if (!playlist) {
        return;
    }
    const Track* t = playlist->findTrackByID(tid);
    if (t) {
        emit requestPlay(t->filepath);
    }
}

QString PlaylistManager::getCurrentTrack() const
{
    EntryId tid = m_context->getPlayTrackId();
    auto pl     = m_repo->findPlaylistById(m_context->getPlaylistId());
    if (!pl) {
        return QString();
    }
    const Track* track = pl->findTrackByID(tid);
    if (!track) {
        return QString();
    }
    return track->filepath;
}

QString PlaylistManager::getCurrentPlaylistName() const
{
    PlaylistId pid = m_context->getPlaylistId();
    auto pl        = m_repo->findPlaylistById(pid);
    return pl ? pl->name() : QString();
}

const EntryId& PlaylistManager::getCurrentTrackId() const
{
    return this->m_context->getPlayTrackId();
}

const PlaylistId& PlaylistManager::getCurrentPlaylistId() const
{
    return this->m_context->getPlaylistId();
}

QVector<PlaylistInfo> PlaylistManager::getAllPlaylists()
{
    QVector<PlaylistInfo> infos;

    auto playlists = m_repo->getLists();
    for (const auto& pl : playlists) {
        infos.append({pl->id(), pl->name()});
    }
    return infos;
}

void PlaylistManager::retransmissionPlaylistChanged()
{
    emit playlistChanged();
}

void PlaylistManager::switchToPlaylist(const PlaylistId& pid)
{
    m_context->setPlaylist(pid);
}

QVector<std::shared_ptr<Playlist>> PlaylistManager::getPlaylists()
{
    return m_repo->getLists();
}

TrackMetaData PlaylistManager::getCurrentMetadata()
{
    EntryId tid   = m_context->getPlayTrackId();
    auto playlist = m_repo->findPlaylistById(m_context->getPlaylistId());

    if (playlist) {
        const Track* track = playlist->findTrackByID(tid);
        if (track) {
            return track->meta;
        }
    }
    TrackMetaData empty_meta;
    empty_meta.isValid = false;
    return empty_meta;
}

QString PlaylistManager::getPlaylistById(const PlaylistId& pid) const
{
    auto pl = m_repo->findPlaylistById(pid);
    if (!pl || pl->isEmpty()) {
        return QString();
    }
    return pl->name();
}
