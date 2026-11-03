#include "playback_queue.h"

#include <QRandomGenerator>

PlaybackQueue::PlaybackQueue(QObject* parent) : QObject(parent) {}

int PlaybackQueue::enqueue(const QueueItem& item)
{
    items_.append(item);
    emit sgn_queue_changed();
    return static_cast<int>(items_.size()) - 1;
}

int PlaybackQueue::enqueue_next(const QueueItem& item)
{
    const int insert_at = (current_ < 0) ? 0 : current_ + 1;
    items_.insert(insert_at, item);
    emit sgn_queue_changed();
    return insert_at;
}

void PlaybackQueue::enqueue_many(const QVector<QueueItem>& items)
{
    if (items.isEmpty()) {
        return;
    }
    items_ += items;
    emit sgn_queue_changed();
}

void PlaybackQueue::remove_at(int index)
{
    if (index < 0 || index >= items_.size()) {
        return;
    }
    items_.removeAt(index);
    if (index < current_) {
        --current_;
    } else if (index == current_) {
        current_ = -1;
    }
    emit sgn_queue_changed();
}

void PlaybackQueue::clear()
{
    if (items_.isEmpty()) {
        return;
    }
    items_.clear();
    const bool had_current = current_ >= 0;
    current_               = -1;
    emit sgn_queue_changed();
    if (had_current) {
        emit sgn_current_changed(-1);
    }
}

void PlaybackQueue::move(int from, int to)
{
    if (from == to || from < 0 || from >= items_.size() || to < 0 || to >= items_.size()) {
        return;
    }
    items_.move(from, to);
    // 修正当前下标:当前项本身移动;其余情况下标随元素整体移动而偏移
    if (current_ == from) {
        current_ = to;
    } else if (from < current_ && to >= current_) {
        --current_;
    } else if (from > current_ && to <= current_) {
        ++current_;
    }
    emit sgn_queue_changed();
}

bool PlaybackQueue::set_current(int index)
{
    if (index < 0 || index >= items_.size() || index == current_) {
        return false;
    }
    current_ = index;
    emit sgn_current_changed(current_);
    return true;
}

void PlaybackQueue::clear_current()
{
    if (current_ < 0) {
        return;
    }
    current_ = -1;
    emit sgn_current_changed(-1);
}

std::optional<QueueItem> PlaybackQueue::current() const
{
    if (current_ < 0 || current_ >= items_.size()) {
        return std::nullopt;
    }
    return items_.at(current_);
}

std::optional<QueueItem> PlaybackQueue::item_at(int index) const
{
    if (index < 0 || index >= items_.size()) {
        return std::nullopt;
    }
    return items_.at(index);
}

namespace
{
// shuffle / out_of_order_* 走随机;in_order / loop 走线性
bool is_random_mode(PlayMode mode)
{
    return mode == PlayMode::Shuffle || mode == PlayMode::OutOfOrderByTrack ||
           mode == PlayMode::OutOfOrderByGroup;
}
} // namespace

std::optional<QueueItem> PlaybackQueue::next(PlayMode mode)
{
    if (items_.isEmpty()) {
        return std::nullopt;
    }
    int target = -1;
    if (is_random_mode(mode)) {
        target = static_cast<int>(QRandomGenerator::global()->bounded(items_.size()));
    } else if (mode == PlayMode::Loop) {
        const int base = (current_ < 0) ? 0 : current_;
        target         = static_cast<int>((base + 1) % items_.size());
    } else { // in_order:不自动回绕
        target = (current_ < 0) ? 0 : current_ + 1;
    }
    if (target < 0 || target >= items_.size()) {
        return std::nullopt;
    }
    current_ = target;
    emit sgn_current_changed(current_);
    return items_.at(current_);
}

std::optional<QueueItem> PlaybackQueue::prev(PlayMode mode)
{
    if (items_.isEmpty()) {
        return std::nullopt;
    }
    int target = -1;
    if (is_random_mode(mode)) {
        target = static_cast<int>(QRandomGenerator::global()->bounded(items_.size()));
    } else if (mode == PlayMode::Loop) {
        const int base = (current_ < 0) ? 0 : current_;
        target         = static_cast<int>((base - 1 + items_.size()) % items_.size());
    } else { // in_order:不自动回绕
        target = (current_ < 0) ? static_cast<int>(items_.size() - 1) : current_ - 1;
    }
    if (target < 0 || target >= items_.size()) {
        return std::nullopt;
    }
    current_ = target;
    emit sgn_current_changed(current_);
    return items_.at(current_);
}
