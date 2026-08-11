#pragma once

#include "core/logger/logger_manager.h"

#include <QString>

#include <format>
#include <string>
#include <string_view>

namespace wusic::log::detail
{

// 模块级 logger 的便捷访问:每模块一个静态缓存(避免每次 hash 查找)。
// 用法: inline LoggerRef logger() { return {"playback"}; }  (见 WUSIC_LOG_MODULE)
struct ModuleLoggerRef
{
    QStringView module;
};

// 真正的写入口:std::format 拼消息 → UTF-8 → QString → LoggerManager
template <typename... Args>
inline void write(const ModuleLoggerRef& ref, Level level, std::format_string<Args...> fmt,
                  Args&&... args)
{
    std::string raw;
    try {
        raw = std::format(fmt, std::forward<Args>(args)...);
    } catch (const std::format_error&) {
        raw = "<format error>";
    }
    LoggerManager::instance().log(level, ref.module.toString(),
                                  QString::fromUtf8(raw.c_str(), int(raw.size())));
}

// fatal:不返回(内部 abort)
template <typename... Args>
[[noreturn]] inline void write_fatal(const ModuleLoggerRef& ref, std::format_string<Args...> fmt,
                                     Args&&... args)
{
    std::string raw;
    try {
        raw = std::format(fmt, std::forward<Args>(args)...);
    } catch (const std::format_error&) {
        raw = "<format error>";
    }
    LoggerManager::instance().fatal(ref.module.toString(),
                                    QString::fromUtf8(raw.c_str(), int(raw.size())));
}

} // namespace wusic::log::detail

// 声明一个模块 logger(放在匿名命名空间或文件作用域):
//   WUSIC_LOG_MODULE(playback)
// 之后可用 WUSIC_LOG(playback, info, "..." , args...)
#define WUSIC_LOG_MODULE(name)                                                                     \
    static const ::wusic::log::detail::ModuleLoggerRef wusic_logger_##name{QStringLiteral(#name)};

// 记录日志:WUSIC_LOG(module, level, fmt, args...)
#define WUSIC_LOG(module, level, ...)                                                              \
    ::wusic::log::detail::write(wusic_logger_##module, ::wusic::log::Level::level, __VA_ARGS__)

// 致命错误(不返回):WUSIC_LOG_FATAL(module, fmt, args...)
#define WUSIC_LOG_FATAL(module, ...)                                                               \
    ::wusic::log::detail::write_fatal(wusic_logger_##module, __VA_ARGS__)
