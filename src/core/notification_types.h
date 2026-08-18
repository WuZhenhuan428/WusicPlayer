#pragma once

#include <QObject>
#include <QString>

/**
 * @brief 结构化数据通知
 */
struct AppNotification
{
    enum class Level
    {
        Info,
        Warn,
        Error,
    };
    Level level;
    QString message;
    int duration_ms = 5000; // 0 = 常驻
};

Q_DECLARE_METATYPE(AppNotification)
