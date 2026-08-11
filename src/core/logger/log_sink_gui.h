#pragma once

#include "core/logger/log_sink.h"

#include <QObject>
#include <QString>
#include <QVector>

#include <QMutex>

namespace wusic::log
{

// GUI sink:把日志转发到主线程(QueuedConnection),并保留环形缓冲供窗口打开时回显。
// 线程安全:write() 可能来自任意线程(如扫描 worker),内部持锁 + emit 信号。
class LogSinkGui : public QObject, public LogSink
{
    Q_OBJECT
public:
    explicit LogSinkGui(QObject* parent = nullptr);

    void write(const Record& record) override;
    void flush() override {}

    // 当前缓冲快照(供窗口打开时回显)
    QVector<Record> snapshot() const;
    // 清空缓冲(窗口"清空"按钮)
    void clear_buffer();

    static constexpr int kMaxRecords = 5000; // 环形缓冲上限

signals:
    // 跨线程投递(Qt::QueuedConnection);level 为 int(避免 metatype 注册)
    void sgn_record(int level, QString module, QString message);

private:
    mutable QMutex m_mutex;
    QVector<Record> m_buffer;
};

} // namespace wusic::log
