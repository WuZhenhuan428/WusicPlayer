#pragma once

#include <QString>

// 日志级别(与 QtMsgType 兼容映射;数值越大越严重)
enum class LogLevel : int
{
    trace = 0,
    debug = 1,
    info  = 2,
    warn  = 3,
    error = 4,
    fatal = 5,
};

// 级别 → 显示字符串(控制台/pattern 用)
inline QString level_name(LogLevel level)
{
    switch (level) {
    case LogLevel::trace:
        return QStringLiteral("trace");
    case LogLevel::debug:
        return QStringLiteral("debug");
    case LogLevel::info:
        return QStringLiteral("info");
    case LogLevel::warn:
        return QStringLiteral("warn");
    case LogLevel::error:
        return QStringLiteral("error");
    case LogLevel::fatal:
        return QStringLiteral("fatal");
    }
    return QStringLiteral("?");
}
