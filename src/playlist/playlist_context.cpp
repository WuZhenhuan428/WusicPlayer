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

void PlaylistContext::setPlaylist(const QUuid& playlistId) {
    if (this->m_currentPlaylistId == playlistId) { return; }
    // PlaylistManager从UI获取id并通过PlaylistRepo进行检查，
    // 此处不需要进行额外的检查（大概）
    m_currentPlaylistId = playlistId;
    emit changedCurrentListId(playlistId);
}

void PlaylistContext::setPlayTrack(const QUuid& trackUuid) {
    // 输入端应当提前进行合法性检查
    // 重复播放音轨应当从头开始播放，无需保证是同一个轨道
    m_currentTrackId = trackUuid;
    emit changedCurrentTrackId(trackUuid);
}

const QUuid& PlaylistContext::getPlaylistId() {
    return this->m_currentPlaylistId;
}

const QUuid& PlaylistContext::getPlayTrackId() {
    return this->m_currentTrackId;
}

PlayMode PlaylistContext::getPlayMode() {
    return this->m_mode;
}
