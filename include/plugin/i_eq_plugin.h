#pragma once

#include "i_basic_plugin.h"
#include "plugin/eq_types.h"

#include <QtPlugin>

class QWidget;

/// EQ 插件接口。
/// 插件提供自己的 UI(create_eq_widget), 并通过 eq_config() 输出任意 band 的
/// 频率/Q/增益配置(无上下限, 由 FFmpeg 后端自身限制)。
/// 固定窗口负责"应用/取消/即时"编排; 插件只维护自己的编辑态与已应用快照。
class IEqPlugin : public IBasicPlugin
{
    Q_OBJECT

public:
    /// 插件自己的 UI(如十段滑杆), 父对象由固定窗口提供
    virtual QWidget* create_eq_widget(QWidget* parent = nullptr) = 0;
    /// 当前编辑中的配置
    virtual EqConfig eq_config() const                           = 0;
    /// 应用: 把当前编辑态标记为"已应用"(供取消回退)
    virtual void apply_current()                                 = 0;
    /// 取消: 回退到上次 apply 的状态
    virtual void revert()                                        = 0;

signals:
    /// 即时模式下 UI 改动即通知固定窗口
    void sgn_config_changed();
};

Q_DECLARE_INTERFACE(IEqPlugin, "com.wusicplayer.IEqPlugin/2.0")
