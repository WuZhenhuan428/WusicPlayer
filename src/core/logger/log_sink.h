#pragma once

#include "core/logger/log_record.h"

#include <QString>

namespace wusic::log
{

// Sink 抽象:所有输出目标(控制台/GUI/未来文件)实现此接口。
// 线程安全约定:LoggerManager 在调用 write() 前已持锁,实现无需再同步。
class LogSink
{
public:
    virtual ~LogSink()                       = default;

    // 写一条记录(已通过级别过滤)
    virtual void write(const Record& record) = 0;

    // 致命错误路径:同步刷出所有已写内容(fatal 后立即 abort,不能依赖异步)
    virtual void flush()                     = 0;
};

} // namespace wusic::log
