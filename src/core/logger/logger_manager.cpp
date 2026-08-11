#include "core/logger/logger_manager.h"

#include <QDebug>
#include <QtGlobal>

#include <cstdio>
#include <cstdlib>

namespace wusic::log
{

LoggerManager& LoggerManager::instance()
{
    static LoggerManager mgr;
    return mgr;
}

void LoggerManager::add_sink(std::shared_ptr<LogSink> sink)
{
    if (!sink) {
        return;
    }
    std::lock_guard lock(m_mutex);
    m_sinks.push_back(std::move(sink));
}

void LoggerManager::clear_sinks()
{
    std::lock_guard lock(m_mutex);
    m_sinks.clear();
}

void LoggerManager::set_global_level(Level level)
{
    std::lock_guard lock(m_mutex);
    m_global_level = level;
}

Level LoggerManager::global_level() const
{
    std::lock_guard lock(m_mutex);
    return m_global_level;
}

void LoggerManager::set_module_level(QString module, Level level)
{
    std::lock_guard lock(m_mutex);
    m_module_levels.insert(std::move(module), level);
}

void LoggerManager::reset_module_level(const QString& module)
{
    std::lock_guard lock(m_mutex);
    m_module_levels.remove(module);
}

bool LoggerManager::should_log(Level level, const QString& module) const
{
    const auto it         = m_module_levels.constFind(module);
    const Level threshold = (it != m_module_levels.constEnd()) ? it.value() : m_global_level;
    return static_cast<int>(level) >= static_cast<int>(threshold);
}

void LoggerManager::write_record(const Record& record)
{
    for (const auto& sink : m_sinks) {
        sink->write(record);
    }
}

void LoggerManager::log(Level level, QString module, QString message)
{
    std::lock_guard lock(m_mutex);
    if (!should_log(level, module)) {
        return;
    }
    write_record(make_record(level, std::move(module), std::move(message)));
}

void LoggerManager::fatal(QString module, QString message)
{
    {
        std::lock_guard lock(m_mutex);
        // fatal 不过滤:无条件写出
        write_record(make_record(Level::fatal, std::move(module), std::move(message)));
        for (const auto& sink : m_sinks) {
            sink->flush();
        }
    }
    std::abort();
}

void LoggerManager::qt_bridge(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    // 从文件路径提取模块名(如 src/core/player/player.cpp → "player")
    QString file    = context.file ? QString::fromUtf8(context.file) : QString();
    const int slash = file.lastIndexOf(QLatin1Char('/'));
    QString module  = (slash >= 0) ? file.mid(slash + 1) : file;
    if (module.endsWith(QLatin1String(".cpp")) || module.endsWith(QLatin1String(".c"))) {
        module.chop(module.endsWith(QLatin1String(".cpp")) ? 4 : 2);
    }
    if (module.isEmpty()) {
        module = QStringLiteral("qt");
    }

    Level level;
    switch (type) {
    case QtDebugMsg:
        level = Level::debug;
        break;
    case QtInfoMsg:
        level = Level::info;
        break;
    case QtWarningMsg:
        level = Level::warn;
        break;
    case QtCriticalMsg:
        level = Level::error;
        break;
    case QtFatalMsg:
        level = Level::fatal;
        break;
    default:
        level = Level::debug;
        break;
    }

    auto& mgr = instance();
    if (level == Level::fatal) {
        mgr.fatal(std::move(module), msg);
        return; // unreachable
    }
    mgr.log(level, std::move(module), msg);
}

} // namespace wusic::log
