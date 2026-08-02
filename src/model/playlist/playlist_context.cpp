#include "playlist_context.h"
#include <random>

PlaylistContext::PlaylistContext(QObject* parent) : QObject(parent)
{
    this->m_mode = PlayMode::in_order;
}

PlaylistContext::~PlaylistContext() {}

void PlaylistContext::setPlayMode(PlayMode mode)
{
    if (this->m_mode == mode) {
        return;
    }
    this->m_mode = mode;

    emit changedCurrentPlayMode(mode);
}

void PlaylistContext::setPlaylist(const PlaylistId& pid)
{
    if (this->m_current_playlist_id == pid) {
        return;
    }
    // PlaylistManager从UI获取id并通过PlaylistRepo进行检查，
    // 此处不需要进行额外的检查（大概）
    m_current_playlist_id = pid;
    emit changedCurrentListId(pid);
}

void PlaylistContext::setPlayTrack(const EntryId& current)
{
    // 输入端应当提前进行合法性检查
    // 重复播放音轨应当从头开始播放，无需保证是同一个轨道
    m_current_track_id = current;
    emit changedCurrentTrackId(current);
}

const PlaylistId& PlaylistContext::getPlaylistId()
{
    return this->m_current_playlist_id;
}

const EntryId& PlaylistContext::getPlayTrackId()
{
    return this->m_current_track_id;
}

PlayMode PlaylistContext::getPlayMode()
{
    return this->m_mode;
}

namespace PlaylistNavigator
{
EntryId nextOfInOrder(const QVector<EntryId>& queue, EntryId current)
{
    int index = queue.indexOf(current);
    if (index != -1 && index < queue.size() - 1) {
        return queue.at(index + 1);
    }
    return EntryId();
}

EntryId nextOfLoop(const QVector<EntryId>& queue, EntryId current)
{
    int index = queue.indexOf(current);
    if (index != -1) {
        if (index < queue.size() - 1) {
            return queue.at(index + 1);
        } else if (index == queue.size() - 1) {
            return queue.at(0);
        }
    }
    return EntryId();
}

EntryId nextOfOutOfOrderTrack(const QVector<EntryId>& queue, EntryId current)
{
    int index = queue.indexOf(current);
    if (index != -1 && index <= queue.size() - 1) {
        return queue.at(index + 1);
    }
    return EntryId();
}

EntryId nextOfShuffle(const QVector<EntryId>& queue)
{
    int index = generate_random_index(queue.size() - 1);
    return queue.at(index);
}

EntryId nextOfOutOfOrderGroup(const QVector<EntryId>& queue, EntryId current)
{
    int index;
    index = queue.indexOf(current);
    if (index != -1 && index <= queue.size() - 1) {
        return queue.at(index + 1);
    }
    return EntryId();
}

EntryId previousOfInOrder(const QVector<EntryId>& queue, EntryId current)
{
    int index = queue.indexOf(current);
    if (index > 0) { // -1 and 0
        return queue.at(index - 1);
    }
    return EntryId();
}

EntryId previousOfLoop(const QVector<EntryId>& queue, EntryId current)
{
    int index = queue.indexOf(current);
    if (index != -1) {
        if (index > 0) {
            return queue.at(index - 1);
        } else if (index == 0) {
            return queue.at(queue.size() - 1);
        }
    }
    return EntryId();
}

EntryId previousOfShuffle(const QVector<EntryId>& queue)
{
    int index = generate_random_index(queue.size() - 1);
    return queue.at(index);
}

EntryId previousOfOutOfOrderTrack(const QVector<EntryId>& queue, EntryId current)
{
    int index = queue.indexOf(current);
    if (index > 0) {
        return queue.at(index - 1);
    }
    return EntryId();
}

EntryId previousOfOutOfOrderGroup(const QVector<EntryId>& queue, EntryId current)
{
    int index = queue.indexOf(current);
    if (index > 0) {
        return queue.at(index - 1);
    }
    return EntryId();
}

size_t generate_random_index(size_t max_index)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, max_index);
    return dist(gen);
};
} // namespace PlaylistNavigator
