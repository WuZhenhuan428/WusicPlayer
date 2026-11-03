#pragma once

#include <QString>

// 日志级别(与 QtMsgType 兼容映射;数值越大越严重)
enum class LogLevel : int
{
    Trace = 0,
    Debug = 1,
    Info  = 2,
    Warn  = 3,
    Error = 4,
    Fatal = 5,
};

// 级别 → 显示字符串(控制台/pattern 用)
inline QString level_name(LogLevel level)
{
    switch (level) {
    case LogLevel::Trace:
        return QStringLiteral("trace");
    case LogLevel::Debug:
        return QStringLiteral("debug");
    case LogLevel::Info:
        return QStringLiteral("info");
    case LogLevel::Warn:
        return QStringLiteral("warn");
    case LogLevel::Error:
        return QStringLiteral("error");
    case LogLevel::Fatal:
        return QStringLiteral("fatal");
    }
    return QStringLiteral("?");
}
