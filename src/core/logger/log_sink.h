#pragma once

#include "core/logger/log_record.h"

#include <QString>
#include <QStringView>

// Sink 抽象:所有输出目标(控制台/GUI/未来文件)实现此接口。
// 线程安全约定:LoggerManager 在调用 write() 前已持锁,实现无需再同步。
class LogSink
{
public:
    LogSink()                                   = default;
    virtual ~LogSink()                          = default;

    // 多态接口:禁止拷贝/移动(避免切片)
    LogSink(const LogSink&)                     = delete;
    LogSink& operator=(const LogSink&)          = delete;
    LogSink(LogSink&&)                          = delete;
    LogSink& operator=(LogSink&&)               = delete;

    // 写一条记录(已通过级别过滤)
    virtual void write(const LogRecord& record) = 0;

    // 致命错误路径:同步刷出所有已写内容(fatal 后立即 abort,不能依赖异步)
    virtual void flush()                        = 0;

    // sink id, 用于通过 QString 字符串查找 sink 实例
    [[nodiscard]] virtual QString name() const  = 0;
};
