#pragma once

#include "core/logger/log_level.h"

#include <QDateTime>
#include <QString>

namespace wusic::log
{

// 单条日志记录(内部全程 QString;sink 输出边界再转 UTF-8)
struct Record
{
    Level level = Level::info;
    QString module;          // 模块名(playback/playlist/library/...)
    QString message;         // 消息正文(已按 pattern 之外的内容)
    QString thread_name;     // 线程名(可选;空 = 主线程/未命名)
    qint64 timestamp_ms = 0; // epoch ms(QDateTime 可回填)
};

inline Record make_record(Level level, QString module, QString message)
{
    Record r;
    r.level        = level;
    r.module       = std::move(module);
    r.message      = std::move(message);
    r.timestamp_ms = QDateTime::currentMSecsSinceEpoch();
    return r;
}

} // namespace wusic::log
