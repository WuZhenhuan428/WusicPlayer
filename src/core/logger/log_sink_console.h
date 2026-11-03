#pragma once

#include "core/logger/log_sink.h"

#include <QString>

// 控制台 sink:按 pattern 拼装后输出到 stderr(UTF-8)。
class ConsoleSink : public LogSink
{
public:
    explicit ConsoleSink(bool use_color = true);
    ~ConsoleSink() override                    = default;

    ConsoleSink(const ConsoleSink&)            = delete;
    ConsoleSink& operator=(const ConsoleSink&) = delete;
    ConsoleSink(ConsoleSink&&)                 = delete;
    ConsoleSink& operator=(ConsoleSink&&)      = delete;

    void write(const LogRecord& record) override;
    void flush() override;
    [[nodiscard]] QString name() const override;

private:
    bool use_color_ = true;
};
