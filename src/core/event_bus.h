#pragma once

#include "core/notification_types.h"

#include <QObject>
#include <QVariant>

#include <utility>

class EventBus : public QObject
{
    Q_OBJECT
public:
    enum class Topic : int
    {
        NotificationShown,     // 添加临时通知
        NotificationDismissed, // 手动关闭临时通知 (假设设置为持续存在)
    };

    void publish(Topic t, AppNotification d)
    {
        emit sgn_event(std::to_underlying(t), QVariant::fromValue(d));
    }
signals:
    void sgn_event(int topic_id, QVariant data);
};

// subscriber usage: connect(EventBus, &EventBus::sgn_event, ..., Qt::QueuedConnection)
