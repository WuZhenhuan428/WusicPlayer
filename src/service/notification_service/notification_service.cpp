#include "notification_service.h"

#include "app_context.h"
#include "core/event_bus.h"
#include "core/logger/logger_manager.h"
#include "core/notification_types.h"
#include "i_notification_display.h"

namespace
{
Logger* logger = LoggerManager::file_logger("notification", {"console", "gui"});
}

NotificationService::NotificationService(AppContext& ctx, QObject* parent) :
    QObject(parent), ctx_(ctx)
{
    assert(ctx.event_bus_ && "NotificationService: require EventBus");

    // 信号载荷为 int topic_id(避免枚举的 metatype 注册),这里再转回 Topic。
    connect(
        ctx.event_bus_, &EventBus::sgn_event, this,
        [this](int topic_id, const QVariant& data) {
            const auto t = static_cast<EventBus::Topic>(topic_id);
            if (t == EventBus::Topic::NotificationShown) {
                if (data.canConvert<AppNotification>()) {
                    this->notify(data.value<AppNotification>());
                }
            } else if (t == EventBus::Topic::NotificationDismissed) {
                this->clear_current();
            }
        },
        Qt::QueuedConnection);
}

void NotificationService::notify(AppNotification notification)
{
    logger->debug("[NOTIFY] level={} msg=\"{}\" dur={}ms", std::to_underlying(notification.level),
                  notification.message, notification.duration_ms);
    for (const auto& disp : this->displays_) {
        disp->show_notification(notification);
    }
}

void NotificationService::clear_current()
{
    for (const auto& disp : this->displays_) {
        disp->clear_notification();
    }
}

void NotificationService::add_display(INotificationDisplay* display)
{
    if (!display) {
        return;
    }
    this->displays_.push_back(display);
}
