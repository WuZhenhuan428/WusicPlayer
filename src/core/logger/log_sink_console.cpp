#include "core/logger/log_sink_console.h"

#include <QDateTime>
#include <QTextStream>
#include <QtGlobal>

namespace wusic::log
{

namespace
{
// ANSI 颜色(仅在 TTY 时启用);级别 → 颜色码
const char* level_color(Level level)
{
    switch (level) {
    case Level::trace:
        return "\x1b[90m"; // 亮灰
    case Level::debug:
        return "\x1b[36m"; // 青
    case Level::info:
        return "\x1b[32m"; // 绿
    case Level::warn:
        return "\x1b[33m"; // 黄
    case Level::error:
        return "\x1b[31m"; // 红
    case Level::fatal:
        return "\x1b[1;31m"; // 亮红
    }
    return "\x1b[0m";
}
} // namespace

ConsoleSink::ConsoleSink(bool use_color) : m_use_color(use_color) {}

void ConsoleSink::write(const Record& record)
{
    const QString ts = QDateTime::fromMSecsSinceEpoch(record.timestamp_ms)
                           .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));

    QString line;
    if (m_use_color) {
        line = QStringLiteral("\x1b[90m[%1]\x1b[0m %2[%3]\x1b[0m \x1b[1m[%4]\x1b[0m %5")
                   .arg(ts, level_color(record.level), level_name(record.level), record.module,
                        record.message);
    } else {
        line = QStringLiteral("[%1] [%2] [%3] %4")
                   .arg(ts, level_name(record.level), record.module, record.message);
    }
    // 控制台边界:QString → UTF-8 输出
    QTextStream stderr_stream(stderr);
    stderr_stream << line.toUtf8().constData() << '\n';
    stderr_stream.flush();
}

void ConsoleSink::flush()
{
    fflush(stderr);
}

} // namespace wusic::log
