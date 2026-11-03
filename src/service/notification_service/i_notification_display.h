#pragma once

#include "core/notification_types.h"

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class INotificationDisplay
{
public:
    virtual ~INotificationDisplay()                          = default;

    virtual void show_notification(const AppNotification& n) = 0;
    virtual void clear_notification()                        = 0;
};
