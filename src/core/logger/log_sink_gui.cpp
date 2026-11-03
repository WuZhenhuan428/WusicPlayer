#include "core/logger/log_sink_gui.h"

LogSinkGui::LogSinkGui(QObject* parent) : QObject(parent) {}

void LogSinkGui::write(const LogRecord& record)
{
    {
        QMutexLocker lock(&mutex_);
        if (buffer_.size() >= kMaxRecords) {
            buffer_.remove(0, buffer_.size() - kMaxRecords + 1);
        }
        buffer_.push_back(record);
    }
    // 信号投递到主线程(窗口以 Qt::QueuedConnection 连接),不在此线程执行 UI
    emit sgn_record(static_cast<int>(record.level), record.name, record.message);
}

QVector<LogRecord> LogSinkGui::snapshot() const
{
    QMutexLocker lock(&mutex_);
    return buffer_;
}

void LogSinkGui::clear_buffer()
{
    QMutexLocker lock(&mutex_);
    buffer_.clear();
}
