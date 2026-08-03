#include "playback_queue.h"

#include <QRandomGenerator>

PlaybackQueue::PlaybackQueue(QObject* parent) : QObject(parent) {}

int PlaybackQueue::enqueue(const QueueItem& item)
{
    m_items.append(item);
    emit sgn_queue_changed();
    return m_items.size() - 1;
}

int PlaybackQueue::enqueue_next(const QueueItem& item)
{
    const int insert_at = (m_current < 0) ? 0 : m_current + 1;
    m_items.insert(insert_at, item);
    emit sgn_queue_changed();
    return insert_at;
}

void PlaybackQueue::enqueue_many(const QVector<QueueItem>& items)
{
    if (items.isEmpty()) {
        return;
    }
    m_items += items;
    emit sgn_queue_changed();
}

void PlaybackQueue::remove_at(int index)
{
    if (index < 0 || index >= m_items.size()) {
        return;
    }
    m_items.removeAt(index);
    if (index < m_current) {
        --m_current;
    } else if (index == m_current) {
        m_current = -1;
    }
    emit sgn_queue_changed();
}

void PlaybackQueue::clear()
{
    if (m_items.isEmpty()) {
        return;
    }
    m_items.clear();
    const bool had_current = m_current >= 0;
    m_current              = -1;
    emit sgn_queue_changed();
    if (had_current) {
        emit sgn_current_changed(-1);
    }
}

void PlaybackQueue::move(int from, int to)
{
    if (from == to || from < 0 || from >= m_items.size() || to < 0 || to >= m_items.size()) {
        return;
    }
    m_items.move(from, to);
    // 修正当前下标:当前项本身移动;其余情况下标随元素整体移动而偏移
    if (m_current == from) {
        m_current = to;
    } else if (from < m_current && to >= m_current) {
        --m_current;
    } else if (from > m_current && to <= m_current) {
        ++m_current;
    }
    emit sgn_queue_changed();
}

bool PlaybackQueue::set_current(int index)
{
    if (index < 0 || index >= m_items.size() || index == m_current) {
        return false;
    }
    m_current = index;
    emit sgn_current_changed(m_current);
    return true;
}

void PlaybackQueue::clear_current()
{
    if (m_current < 0) {
        return;
    }
    m_current = -1;
    emit sgn_current_changed(-1);
}

std::optional<QueueItem> PlaybackQueue::current() const
{
    if (m_current < 0 || m_current >= m_items.size()) {
        return std::nullopt;
    }
    return m_items.at(m_current);
}

std::optional<QueueItem> PlaybackQueue::item_at(int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return std::nullopt;
    }
    return m_items.at(index);
}

namespace
{
// shuffle / out_of_order_* 走随机;in_order / loop 走线性
bool is_random_mode(PlayMode mode)
{
    return mode == PlayMode::shuffle || mode == PlayMode::out_of_order_track ||
           mode == PlayMode::out_of_order_group;
}
} // namespace

std::optional<QueueItem> PlaybackQueue::next(PlayMode mode)
{
    if (m_items.isEmpty()) {
        return std::nullopt;
    }
    int target = -1;
    if (is_random_mode(mode)) {
        target = QRandomGenerator::global()->bounded(m_items.size());
    } else if (mode == PlayMode::loop) {
        const int base = (m_current < 0) ? 0 : m_current;
        target         = (base + 1) % m_items.size();
    } else { // in_order:不自动回绕
        target = (m_current < 0) ? 0 : m_current + 1;
    }
    if (target < 0 || target >= m_items.size()) {
        return std::nullopt;
    }
    m_current = target;
    emit sgn_current_changed(m_current);
    return m_items.at(m_current);
}

std::optional<QueueItem> PlaybackQueue::prev(PlayMode mode)
{
    if (m_items.isEmpty()) {
        return std::nullopt;
    }
    int target = -1;
    if (is_random_mode(mode)) {
        target = QRandomGenerator::global()->bounded(m_items.size());
    } else if (mode == PlayMode::loop) {
        const int base = (m_current < 0) ? 0 : m_current;
        target         = (base - 1 + m_items.size()) % m_items.size();
    } else { // in_order:不自动回绕
        target = (m_current < 0) ? m_items.size() - 1 : m_current - 1;
    }
    if (target < 0 || target >= m_items.size()) {
        return std::nullopt;
    }
    m_current = target;
    emit sgn_current_changed(m_current);
    return m_items.at(m_current);
}
