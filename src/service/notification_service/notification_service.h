#pragma once

#include "core/notification_types.h"

#include <QObject>
#include <QTimer>
#include <QVector>

class AppContext;
class INotificationDisplay;

class NotificationService : public QObject
{
    Q_OBJECT
public:
    explicit NotificationService(AppContext& ctx, QObject* parent = nullptr);

    void add_display(INotificationDisplay* display);

private:
    void clear_current(); // 超时定时器由具体实现内部负责
    void notify(AppNotification notification);

    QVector<INotificationDisplay*> displays_;
    AppContext& ctx_;
};
