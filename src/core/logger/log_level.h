#pragma once

#include <QString>

namespace wusic::log
{

// 日志级别(与 QtMsgType 兼容映射;数值越大越严重)
enum class Level : int
{
    trace = 0,
    debug = 1,
    info  = 2,
    warn  = 3,
    error = 4,
    fatal = 5,
};

// 级别 → 显示字符串(控制台/pattern 用)
inline QString level_name(Level level)
{
    switch (level) {
    case Level::trace:
        return QStringLiteral("trace");
    case Level::debug:
        return QStringLiteral("debug");
    case Level::info:
        return QStringLiteral("info");
    case Level::warn:
        return QStringLiteral("warn");
    case Level::error:
        return QStringLiteral("error");
    case Level::fatal:
        return QStringLiteral("fatal");
    }
    return QStringLiteral("?");
}

} // namespace wusic::log
