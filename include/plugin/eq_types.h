#pragma once

#include <QVector>

/// EQ 滤波器类型 → FFmpeg 滤镜映射:
///   Parametric / Peak  → equalizer
///   LowShelf           → bass
///   HighShelf          → treble
///   LowPass            → lowpass
///   HighPass           → highpass
enum class EqFilterType : int
{
    Parametric,
    Peak,
    LowShelf,
    HighShelf,
    LowPass,
    HighPass,
};

/// 单个 EQ band。
/// 频率/Q/增益均不设上下限——由 FFmpeg 后端自身能力决定,
/// 允许远超传统十段 ±12dB 的高级用法。
/// q 统一为 Q factor(equalizer/bass/treble/lowpass/highpass 均按 width_type=q)。
struct EqBand
{
    EqFilterType type = EqFilterType::Parametric;
    double freq       = 1000.0; // Hz
    double q          = 1.0;    // Q factor
    double gain_db    = 0.0;    // 增益(低通/高通不使用)
};

/// 完整 EQ 配置(插件 → 解码器)。
struct EqConfig
{
    bool enabled = false;
    QVector<EqBand> bands; // 任意数量/顺序
};
