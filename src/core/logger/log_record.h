#pragma once

#include "core/logger/log_level.h"

#include <QDateTime>
#include <QString>

// 单条日志记录(内部全程 QString;sink 输出边界再转 UTF-8)
struct LogLocation
{
    QString file;
    QString function;
    size_t line;
    size_t column;
};

struct LogRecord
{
    LogLevel level = LogLevel::info;
    QString name;    // logger's name
    QString message; // 消息正文(已按 pattern 之外的内容)
    LogLocation location;
    QString thread_name;     // 线程名(可选;空 = 主线程/未命名)
    qint64 timestamp_ms = 0; // epoch ms(QDateTime 可回填)
};
