#pragma once

#include "core/types.h"
#include "queue_item.h"

#include <QObject>
#include <QVector>

#include <optional>

/**
 * @brief 现在播放队列容器:入队 / 移除 / 移动 / 当前项,支持按 PlayMode 导航。
 *
 * 数据信号自下而上(sgn_queue_changed / sgn_current_changed),视图层订阅。
 * 容器不持有播放后端、不触发播放;播放由上层响应当前项变化驱动。
 */
class PlaybackQueue : public QObject
{
    Q_OBJECT
public:
    explicit PlaybackQueue(QObject* parent = nullptr);

    // ---- 修改 ----
    // 追加到末尾;返回新项下标
    int enqueue(const QueueItem& item);
    // 插入到当前项之后(无当前项则插入头部);返回新项下标
    int enqueue_next(const QueueItem& item);
    // 批量追加;只发一次 sgn_queue_changed
    void enqueue_many(const QVector<QueueItem>& items);
    // 移除;移除当前项会清空当前(不自动切下一首,策略由上层决定)
    void remove_at(int index);
    void clear();
    // 移动并修正当前下标
    void move(int from, int to);

    // ---- 当前项 ----
    // 越界或未变化时返回 false 且不改动
    bool set_current(int index);
    void clear_current();
    int current_index() const
    {
        return m_current;
    }
    std::optional<QueueItem> current() const;
    std::optional<QueueItem> item_at(int index) const;

    // ---- 只读 ----
    int size() const
    {
        return m_items.size();
    }
    bool is_empty() const
    {
        return m_items.isEmpty();
    }
    // 非拥有:引用内部容器,仅本次调用内有效
    const QVector<QueueItem>& items() const
    {
        return m_items;
    }

    // ---- 导航 ----
    // 按 PlayMode 前进 / 后退;无项可走时返回 nullopt 且不改动当前;
    // 成功时更新当前下标并 emit sgn_current_changed
    std::optional<QueueItem> next(PlayMode mode);
    std::optional<QueueItem> prev(PlayMode mode);

signals:
    void sgn_queue_changed();
    void sgn_current_changed(int index);

private:
    QVector<QueueItem> m_items;
    int m_current = -1; // -1 = 无当前项
};
