#pragma once

#include "log_level.h"
#include "log_record.h"
#include "log_sink.h"

#include <QDateTime>
#include <QString>
#include <QVector>

#include <format>
#include <memory>
#include <mutex>
#include <source_location>
#include <string>

template <>
struct std::formatter<QString> : formatter<std::string_view>
{
    auto format(const QString& qstr, std::format_context& ctx) const
    {
        return std::formatter<string_view>::format(qstr.toStdString(), ctx);
    }
};

class Logger
{
public:
    explicit Logger(const QString& name, QVector<std::shared_ptr<LogSink>> sinks, LogLevel level) :
        name_(name), sinks_(std::move(sinks)), level_(level)
    {}

    template <typename... Args>
    void log(LogLevel level, const std::source_location& loc, std::format_string<Args...> fmt,
             Args&&... args)
    {
        if (std::to_underlying(level) < std::to_underlying(level_)) {
            return;
        }
        std::string raw;
        try {
            raw = std::format(fmt, std::forward<Args>(args)...);
        } catch (const std::format_error&) {
            raw = "<format error>";
        }
        QString msg   = QString::fromStdString(raw);
        LogRecord rec = this->make_record(level, std::move(msg), loc);

        {
            std::lock_guard<std::mutex> lock(this->mutex_);
            for (const auto& sink : this->sinks_) {
                sink->write(rec);
            }
        }
    }

    template <typename... Args>
    void log(LogLevel level, std::format_string<Args...> fmt, Args&&... args)
    {
        this->log(level, std::source_location::current(), fmt, std::forward<Args>(args)...);
    }

    void log_qt(const LogRecord& rec)
    {
        if (std::to_underlying(rec.level) < std::to_underlying(level_)) {
            return;
        }
        {
            std::lock_guard<std::mutex> lock(this->mutex_);
            for (const auto& sink : this->sinks_) {
                sink->write(rec);
            }
        }
    }

    // Wrap of log(..., sourc_location, ...);
    template <typename... Args>
    void trace(std::format_string<Args...> fmt, Args&&... args)
    {
        this->log(LogLevel::trace, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void debug(std::format_string<Args...> fmt, Args&&... args)
    {
        this->log(LogLevel::debug, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void info(std::format_string<Args...> fmt, Args&&... args)
    {
        this->log(LogLevel::info, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn(std::format_string<Args...> fmt, Args&&... args)
    {
        this->log(LogLevel::warn, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void error(std::format_string<Args...> fmt, Args&&... args)
    {
        this->log(LogLevel::error, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void warn_force(std::format_string<Args...> fmt, Args&&... args,
                    std::source_location loc = std::source_location::current())
    {
        std::string raw = std::format(fmt, std::forward<Args>(args)...);
        QString msg     = QString::fromStdString(raw);
        LogRecord rec   = this->make_record(LogLevel::warn, std::move(msg), loc);

        {
            std::lock_guard<std::mutex> lock(this->mutex_);
            for (const auto& sink : this->sinks_) {
                sink->write(rec);
            }
        }
    }

    template <typename... Args>
    [[noreturn]] void fatal(std::format_string<Args...> fmt, Args&&... args)
    {
        this->log(LogLevel::fatal, fmt, std::forward<Args>(args)...);
        for (const auto& sink : this->sinks_) {
            sink->flush();
        }
        std::abort();
    }

    const QString& name()
    {
        return this->name_;
    }

    void set_level(LogLevel level)
    {
        this->level_ = level;
    }

    void add_sink(std::shared_ptr<LogSink> sink)
    {
        std::lock_guard<std::mutex> lock(this->mutex_);
        this->sinks_.push_back(std::move(sink));
    }

    void remove_sink(const QString& sink_name)
    {
        std::lock_guard<std::mutex> lock(this->mutex_);
        for (auto it = sinks_.begin(); it != sinks_.end();) {
            if (it->get()->name() == sink_name) {
                it = sinks_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void replace_sinks(QVector<std::shared_ptr<LogSink>>&& sinks)
    {
        std::lock_guard<std::mutex> lock(this->mutex_);
        this->sinks_ = std::move(sinks);
    }

private:
    LogLocation from_std_source_ocation(std::source_location loc)
    {
        LogLocation qt_loc;
        qt_loc.file     = loc.file_name();
        qt_loc.function = loc.function_name();
        qt_loc.line     = loc.line();
        qt_loc.column   = loc.column();
        return qt_loc;
    }

    LogRecord make_record(LogLevel level, QString&& msg, std::source_location loc)
    {
        LogRecord r;
        r.level        = level;
        r.name         = name_;
        r.message      = msg;
        r.location     = this->from_std_source_ocation(loc);
        r.timestamp_ms = QDateTime::currentMSecsSinceEpoch();
        return r;
    }

    QString name_;
    QVector<std::shared_ptr<LogSink>> sinks_;
    LogLevel level_;

    std::mutex mutex_;
};
