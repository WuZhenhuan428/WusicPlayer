#include "core/logger/logger_manager.h"

#include "core/logger/log_sink_console.h"
#include "core/logger/log_sink_gui.h"

#include <QDebug>
#include <QVector>
#include <QtGlobal>

LoggerManager& LoggerManager::instance()
{
    // 有意泄漏:Logger 与所有 sink 的生命周期 = 进程生命周期。
    // 原因:后台 worker 线程(播放队列重建、标签回写等)可能在 main 返回后的
    // 静态析构期仍调用 logger;若单例在此被析构并释放 loggers_/sink_pool_ 的
    // 引用,正在运行的线程将访问已销毁对象(纯虚调用/use-after-free 崩溃)。
    // 日志系统本就不需要析构,进程退出时由 OS 回收。
    static LoggerManager* mgr = new LoggerManager();
    return *mgr;
}

Logger* LoggerManager::file_logger(const QString& logger_name,
                                   std::initializer_list<QString> sink_names)
{
    // 裸指针:单例永活,manager 的 loggers_ 持有常驻 shared_ptr,
    // 返回的指针在进程生命周期内始终有效。
    return LoggerManager::instance().get(logger_name, sink_names).get();
}

std::shared_ptr<Logger> LoggerManager::get(const QString& logger_name,
                                           std::initializer_list<QString> sink_names,
                                           bool force_update)
{
    std::lock_guard<std::mutex> lock(this->mutex_);
    // 寻找已创建的 Logger 实例
    auto logger_it = loggers_.find(logger_name);
    if (logger_it != loggers_.end()) {
        if (force_update) {
            QVector<std::shared_ptr<LogSink>> new_sinks;
            for (const auto& sink_name : sink_names) {
                // QApplication 之前请求 "gui" 可能返回 nullptr,跳过
                if (auto sink = this->find_or_create_sink(sink_name); sink) {
                    new_sinks.push_back(std::move(sink));
                }
            }
            logger_it.value()->replace_sinks(std::move(new_sinks));
        }
        return logger_it.value();
    }

    // 如果未找到: 先获取已存在的 sink 实例 & 创建未存在的 sink 新实例
    QVector<std::shared_ptr<LogSink>> sinks;
    bool wants_gui = false;
    for (auto name : sink_names) {
        auto sink = this->find_or_create_sink(name);
        if (sink) {
            sinks.push_back(sink);
        } else if (name == QStringLiteral("gui")) {
            // QApplication 尚未创建,不能在此构造 QObject 子类 sink;
            // 记入待挂载列表,由 mark_app_ready() 在 QApplication 之后补挂。
            wants_gui = true;
        }
    }

    // 然后创建 Logger, 添加 sink 并返回
    auto logger = std::make_shared<Logger>(logger_name, std::move(sinks), this->m_global_level);
    this->loggers_[logger_name] = logger;
    if (wants_gui) {
        this->pending_gui_loggers_.push_back(logger);
    }
    return logger;
}

void LoggerManager::mark_app_ready()
{
    std::lock_guard<std::mutex> lock(this->mutex_);
    app_ready_    = true;
    // QApplication 已存在,此时构造 LogSinkGui 才是安全的。
    auto gui_sink = this->find_or_create_sink(QStringLiteral("gui"));
    if (!gui_sink) {
        return;
    }
    // 关键:入池,保证后续 get_sink_by_name("gui") 取到同一实例(否则 LogViewer
    // 会连到一个收不到日志的"新" sink)。
    this->sink_pool_[QStringLiteral("gui")] = gui_sink;
    // 为静态初始化期创建的 logger 补挂 GUI sink。
    for (const auto& logger : this->pending_gui_loggers_) {
        logger->add_sink(gui_sink);
    }
    this->pending_gui_loggers_.clear();
}

void LoggerManager::add_sink(std::shared_ptr<LogSink> sink)
{
    if (!sink) {
        return;
    }
    std::lock_guard lock(mutex_);
    sink_pool_[sink->name()] = sink;
}

void LoggerManager::clear_sinks()
{
    std::lock_guard lock(mutex_);
    sink_pool_.clear();
}

void LoggerManager::set_global_level(LogLevel level)
{
    std::lock_guard lock(mutex_);
    m_global_level = level;
}

LogLevel LoggerManager::global_level() const
{
    std::lock_guard lock(mutex_);
    return m_global_level;
}

void LoggerManager::qt_bridge(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    // make record manually
    LogRecord rec;

    LogLevel level;
    switch (type) {
    case QtDebugMsg:
        level = LogLevel::debug;
        break;
    case QtInfoMsg:
        level = LogLevel::info;
        break;
    case QtWarningMsg:
        level = LogLevel::warn;
        break;
    case QtCriticalMsg:
        level = LogLevel::error;
        break;
    case QtFatalMsg:
        level = LogLevel::fatal;
        break;
    default:
        level = LogLevel::debug;
        break;
    }
    rec.level    = level;

    // 模块名固定为 "qt"(与下方 logger 名一致),便于在控制台/GUI 中识别 Qt 自身日志
    rec.name     = QStringLiteral("qt");

    rec.message  = msg;

    rec.location = LogLocation{
        .file     = context.file,
        .function = context.function,
        .line     = static_cast<size_t>(context.line),
        .column   = 0, // no column member in context
    };
    // no thread name

    rec.timestamp_ms         = QDateTime::currentMSecsSinceEpoch();

    // 同时输出到控制台与 GUI 窗口;若在 QApplication 之前被调用,"gui" 会被安全跳过。
    // 裸指针(见 file_logger 注释):Qt 可能在静态析构期仍有日志输出。
    static Logger* qt_logger = LoggerManager::file_logger("qt", {"console", "gui"});
    qt_logger->log_qt(std::move(rec));
}

std::shared_ptr<LogSink> LoggerManager::get_sink_by_name(const QString& name)
{
    std::lock_guard<std::mutex> lock(this->mutex_);
    return this->find_or_create_sink(name);
}

std::shared_ptr<LogSink> LoggerManager::find_or_create_sink(const QString& sink_name)
{
    auto it = this->sink_pool_.find(sink_name);
    if (it != sink_pool_.end()) {
        return it.value();
    }

    // or create new one
    std::shared_ptr<LogSink> sink;
    if (sink_name == "console") {
        sink = std::make_shared<ConsoleSink>();
    } else if (sink_name == "gui") {
        // LogSinkGui 是 QObject 子类,必须在 QApplication 创建之后才能构造。
        // 静态初始化期(QApplication 之前)请求 "gui" 时返回空,由 get() 记入
        // pending_gui_loggers_,待 mark_app_ready() 补挂。
        if (!this->app_ready_) {
            return nullptr;
        }
        sink = std::make_shared<LogSinkGui>();
    } // else if ...

    return sink;
}
