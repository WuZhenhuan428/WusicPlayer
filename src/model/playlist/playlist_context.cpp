#include "playlist_context.h"
#include <random>

PlaylistContext::PlaylistContext(QObject* parent) : QObject(parent)
{
    this->mode_ = PlayMode::InOrder;
}

void PlaylistContext::set_play_mode(PlayMode mode)
{
    if (this->mode_ == mode) {
        return;
    }
    this->mode_ = mode;

    emit sgn_current_play_mode_changed(mode);
}

void PlaylistContext::set_playlist(const PlaylistId& pid)
{
    if (this->current_playlist_id_ == pid) {
        return;
    }
    // PlaylistManager从UI获取id并通过PlaylistRepo进行检查，
    // 此处不需要进行额外的检查（大概）
    current_playlist_id_ = pid;
    emit sgn_current_list_changed(pid);
}

void PlaylistContext::set_play_track(const EntryId& eid)
{
    // 输入端应当提前进行合法性检查
    // 重复播放音轨应当从头开始播放，无需保证是同一个轨道
    current_track_id_ = eid;
    emit sgn_current_track_changed(eid);
}

const PlaylistId& PlaylistContext::get_playlist_id()
{
    return this->current_playlist_id_;
}

const EntryId& PlaylistContext::get_play_track_id()
{
    return this->current_track_id_;
}

PlayMode PlaylistContext::get_play_mode()
{
    return this->mode_;
}

namespace playlist_navigator
{
EntryId next_of_in_order(const QVector<EntryId>& queue, EntryId current)
{
    qsizetype index = queue.indexOf(current);
    if (index != -1 && index < queue.size() - 1) {
        return queue.at(index + 1);
    }
    return {};
}

EntryId next_of_loop(const QVector<EntryId>& queue, EntryId current)
{
    qsizetype index = queue.indexOf(current);
    if (index != -1) {
        if (index < queue.size() - 1) {
            return queue.at(index + 1);
        }
        if (index == queue.size() - 1) {
            return queue.at(0);
        }
    }
    return {};
}

EntryId next_of_out_of_order_track(const QVector<EntryId>& queue, EntryId current)
{
    qsizetype index = queue.indexOf(current);
    if (index != -1 && index <= queue.size() - 1) {
        return queue.at(index + 1);
    }
    return {};
}

EntryId next_of_shuffle(const QVector<EntryId>& queue)
{
    qsizetype index = generate_random_index(queue.size() - 1);
    return queue.at(index);
}

EntryId next_of_out_of_order_group(const QVector<EntryId>& queue, EntryId current)
{
    qsizetype index = queue.indexOf(current);
    if (index != -1 && index <= queue.size() - 1) {
        return queue.at(index + 1);
    }
    return {};
}

EntryId previous_of_in_order(const QVector<EntryId>& queue, EntryId current)
{
    qsizetype index = queue.indexOf(current);
    if (index > 0) { // -1 and 0
        return queue.at(index - 1);
    }
    return {};
}

EntryId previous_of_loop(const QVector<EntryId>& queue, EntryId current)
{
    qsizetype index = queue.indexOf(current);
    if (index != -1) {
        if (index > 0) {
            return queue.at(index - 1);
        }
        if (index == 0) {
            return queue.at(queue.size() - 1);
        }
    }
    return {};
}

EntryId previous_of_shuffle(const QVector<EntryId>& queue)
{
    qsizetype index = generate_random_index(queue.size() - 1);
    return queue.at(index);
}

EntryId previous_of_out_of_order_track(const QVector<EntryId>& queue, EntryId current)
{
    qsizetype index = queue.indexOf(current);
    if (index > 0) {
        return queue.at(index - 1);
    }
    return {};
}

EntryId previous_of_out_of_order_group(const QVector<EntryId>& queue, EntryId current)
{
    qsizetype index = queue.indexOf(current);
    if (index > 0) {
        return queue.at(index - 1);
    }
    return {};
}

qsizetype generate_random_index(qsizetype max_index)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<qsizetype> dist(0, max_index);
    return dist(gen);
};
} // namespace playlist_navigator
