#pragma once

#include "core/logger/log_sink.h"

#include <QString>

namespace wusic::log
{

// 控制台 sink:按 pattern 拼装后输出到 stderr(UTF-8)。
class ConsoleSink : public LogSink
{
public:
    explicit ConsoleSink(bool use_color = true);
    ~ConsoleSink() override = default;

    void write(const Record& record) override;
    void flush() override;

private:
    bool m_use_color = true;
};

} // namespace wusic::log
