#pragma once

#include "core/logger/log_level.h"
#include "core/logger/log_record.h"
#include "core/logger/log_sink.h"

#include <QHash>
#include <QString>
#include <QStringView>
#include <QVector>

#include <functional>
#include <memory>
#include <mutex>

namespace wusic::log
{

// 全局日志管理器(单例):
//  - 持有所有 sink,按级别过滤后路由
//  - 按模块管理 logger 实例(懒创建缓存)
//  - 提供 Qt 日志桥接(qInstallMessageHandler 转发)
// 线程安全:所有公开方法内部持锁;sink.write() 在锁内调用。
class LoggerManager
{
public:
    static LoggerManager& instance();

    LoggerManager(const LoggerManager&)            = delete;
    LoggerManager& operator=(const LoggerManager&) = delete;

    // ---- sink 管理 ----
    void add_sink(std::shared_ptr<LogSink> sink);
    void clear_sinks();

    // ---- 级别控制 ----
    // 全局兜底级别(未单独设置的模块使用)
    void set_global_level(Level level);
    Level global_level() const;
    // 某模块独立级别(如排查时把 playlist 调到 trace)
    void set_module_level(QString module, Level level);
    void reset_module_level(const QString& module);

    // ---- 核心写入口 ----
    void log(Level level, QString module, QString message);
    // 致命错误:同步写所有 sink 后 abort()(不返回)
    [[noreturn]] void fatal(QString module, QString message);

    // ---- Qt 桥接(qInstallMessageHandler 用) ----
    static void qt_bridge(QtMsgType type, const QMessageLogContext& context, const QString& msg);

private:
    LoggerManager()  = default;
    ~LoggerManager() = default;

    bool should_log(Level level, const QString& module) const;
    void write_record(const Record& record);

    mutable std::mutex m_mutex;
    QVector<std::shared_ptr<LogSink>> m_sinks;
    Level m_global_level = Level::debug;
    QHash<QString, Level> m_module_levels;
};

} // namespace wusic::log
