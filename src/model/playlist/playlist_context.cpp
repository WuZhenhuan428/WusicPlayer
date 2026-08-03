#include "playlist_context.h"
#include <random>

PlaylistContext::PlaylistContext(QObject* parent) : QObject(parent)
{
    this->m_mode = PlayMode::in_order;
}

PlaylistContext::~PlaylistContext() {}

void PlaylistContext::set_play_mode(PlayMode mode)
{
    if (this->m_mode == mode) {
        return;
    }
    this->m_mode = mode;

    emit sgn_current_play_mode_changed(mode);
}

void PlaylistContext::set_playlist(const PlaylistId& pid)
{
    if (this->m_current_playlist_id == pid) {
        return;
    }
    // PlaylistManager从UI获取id并通过PlaylistRepo进行检查，
    // 此处不需要进行额外的检查（大概）
    m_current_playlist_id = pid;
    emit sgn_current_list_changed(pid);
}

void PlaylistContext::set_play_track(const EntryId& current)
{
    // 输入端应当提前进行合法性检查
    // 重复播放音轨应当从头开始播放，无需保证是同一个轨道
    m_current_track_id = current;
    emit sgn_current_track_changed(current);
}

const PlaylistId& PlaylistContext::get_playlist_id()
{
    return this->m_current_playlist_id;
}

const EntryId& PlaylistContext::get_play_track_id()
{
    return this->m_current_track_id;
}

PlayMode PlaylistContext::get_play_mode()
{
    return this->m_mode;
}

namespace PlaylistNavigator
{
EntryId next_of_in_order(const QVector<EntryId>& queue, EntryId current)
{
    int index = queue.indexOf(current);
    if (index != -1 && index < queue.size() - 1) {
        return queue.at(index + 1);
    }
    return EntryId();
}

EntryId next_of_loop(const QVector<EntryId>& queue, EntryId current)
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

EntryId next_of_out_of_order_track(const QVector<EntryId>& queue, EntryId current)
{
    int index = queue.indexOf(current);
    if (index != -1 && index <= queue.size() - 1) {
        return queue.at(index + 1);
    }
    return EntryId();
}

EntryId next_of_shuffle(const QVector<EntryId>& queue)
{
    int index = generate_random_index(queue.size() - 1);
    return queue.at(index);
}

EntryId next_of_out_of_order_group(const QVector<EntryId>& queue, EntryId current)
{
    int index;
    index = queue.indexOf(current);
    if (index != -1 && index <= queue.size() - 1) {
        return queue.at(index + 1);
    }
    return EntryId();
}

EntryId previous_of_in_order(const QVector<EntryId>& queue, EntryId current)
{
    int index = queue.indexOf(current);
    if (index > 0) { // -1 and 0
        return queue.at(index - 1);
    }
    return EntryId();
}

EntryId previous_of_loop(const QVector<EntryId>& queue, EntryId current)
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

EntryId previous_of_shuffle(const QVector<EntryId>& queue)
{
    int index = generate_random_index(queue.size() - 1);
    return queue.at(index);
}

EntryId previous_of_out_of_order_track(const QVector<EntryId>& queue, EntryId current)
{
    int index = queue.indexOf(current);
    if (index > 0) {
        return queue.at(index - 1);
    }
    return EntryId();
}

EntryId previous_of_out_of_order_group(const QVector<EntryId>& queue, EntryId current)
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
