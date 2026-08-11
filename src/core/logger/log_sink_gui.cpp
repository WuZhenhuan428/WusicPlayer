#include "core/logger/log_sink_gui.h"

namespace wusic::log
{

LogSinkGui::LogSinkGui(QObject* parent) : QObject(parent) {}

void LogSinkGui::write(const Record& record)
{
    {
        QMutexLocker lock(&m_mutex);
        if (m_buffer.size() >= kMaxRecords) {
            m_buffer.remove(0, m_buffer.size() - kMaxRecords + 1);
        }
        m_buffer.push_back(record);
    }
    // 信号投递到主线程(窗口以 Qt::QueuedConnection 连接),不在此线程执行 UI
    emit sgn_record(static_cast<int>(record.level), record.module, record.message);
}

QVector<Record> LogSinkGui::snapshot() const
{
    QMutexLocker lock(&m_mutex);
    return m_buffer;
}

void LogSinkGui::clear_buffer()
{
    QMutexLocker lock(&m_mutex);
    m_buffer.clear();
}

} // namespace wusic::log
