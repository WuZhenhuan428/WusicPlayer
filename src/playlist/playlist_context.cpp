#include "playlist_context.h"

PlaylistContext::PlaylistContext(QObject* parent)
    : QObject(parent)
{
    this->m_mode = PlayMode::in_order;
}

PlaylistContext::~PlaylistContext() {}

void PlaylistContext::setPlayMode(PlayMode mode) {
    if (this->m_mode == mode) { return; }
    this->m_mode = mode;

    emit changedCurrentPlayMode(mode);
}

void PlaylistContext::setPlaylist(const playlistId& pid) {
    if (this->m_currentPlaylistId == pid) { return; }
    // PlaylistManager从UI获取id并通过PlaylistRepo进行检查，
    // 此处不需要进行额外的检查（大概）
    m_currentPlaylistId = pid;
    emit changedCurrentListId(pid);
}

void PlaylistContext::setPlayTrack(const trackId& tid) {
    // 输入端应当提前进行合法性检查
    // 重复播放音轨应当从头开始播放，无需保证是同一个轨道
    m_currentTrackId = tid;
    emit changedCurrentTrackId(tid);
}

const playlistId& PlaylistContext::getPlaylistId() {
    return this->m_currentPlaylistId;
}

const trackId& PlaylistContext::getPlayTrackId() {
    return this->m_currentTrackId;
}

PlayMode PlaylistContext::getPlayMode() {
    return this->m_mode;
}
