#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

/// 播放设备信息 —— 由 miniaudio 枚举, 替代 Qt Multimedia 的 QAudioDevice。
/// id 与 description 均取自 miniaudio 设备名(在枚举结果内唯一)。
struct AudioDeviceInfo
{
    QByteArray id;       // 设备标识(当前为 miniaudio 设备名, 供持久化/匹配)
    QString description; // 显示名称
    bool is_default = false;
};
