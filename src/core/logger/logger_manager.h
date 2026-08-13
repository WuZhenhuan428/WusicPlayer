#pragma once

#include "core/logger/log_level.h"
#include "core/logger/log_sink.h"
#include "core/logger/logger.h"

#include <QHash>
#include <QString>
#include <QStringView>
#include <QVector>

#include <initializer_list>
#include <memory>
#include <mutex>

// 全局日志管理器(单例):
//  - 持有所有 sink,按级别过滤后路由
//  - 按模块管理 logger 实例(懒创建缓存)
//  - 提供 Qt 日志桥接(qInstallMessageHandler 转发)
// 线程安全:所有公开方法内部持锁;sink.write() 在锁内调用。
class LoggerManager
{
public:
    // 单例实现
    static LoggerManager& instance();

    LoggerManager(const LoggerManager&)            = delete;
    LoggerManager& operator=(const LoggerManager&) = delete;

    // 获取 Logger 实例
    std::shared_ptr<Logger> get(const QString& logger_name,
                                std::initializer_list<QString> sink_names = {},
                                bool force_update                         = false);

    // sink 管理
    void add_sink(std::shared_ptr<LogSink> sink);
    void clear_sinks();
    std::shared_ptr<LogSink> get_sink_by_name(const QString& name);

    // 在 QApplication 创建后调用:此时才允许构造 QObject 子类 sink(LogSinkGui)。
    // 静态初始化期请求过 "gui" 的 logger 会在此处补挂上 GUI sink。
    void mark_app_ready();

    // 全局级别控制
    void set_global_level(LogLevel level);
    LogLevel global_level() const;

    // Qt 桥接 (qInstallMessageHandler)
    static void qt_bridge(QtMsgType type, const QMessageLogContext& context, const QString& msg);

    // 供文件级 static 使用:返回进程生命周期内永活的 Logger 裸指针。
    // 不能返回 shared_ptr——后台线程(重建队列/标签回写等)可能在 main 返回后的
    // 静态析构期仍调用 logger,shared_ptr 的 static 对象届时已被析构 → use-after-free。
    static Logger* file_logger(const QString& logger_name,
                               std::initializer_list<QString> sink_names = {});

private:
    // 单例实现
    LoggerManager()  = default;
    ~LoggerManager() = default;

    // helpers
    std::shared_ptr<LogSink> find_or_create_sink(const QString& sink_name);

    // members
    mutable std::mutex mutex_;
    QHash<QString, std::shared_ptr<LogSink>> sink_pool_;
    QHash<QString, std::shared_ptr<Logger>> loggers_;

    // 静态初始化期(QApplication 之前)请求过 "gui" sink、但尚未挂载的 logger;
    // 由 mark_app_ready() 统一补挂,避免在无 QApplication 时构造 QObject。
    QVector<std::shared_ptr<Logger>> pending_gui_loggers_;

    bool app_ready_         = false;

    LogLevel m_global_level = LogLevel::debug;
};
