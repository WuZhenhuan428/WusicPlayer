#pragma once

#include "plugin/i_eq_plugin.h"

#include <QLabel>
#include <QSlider>
#include <array>

class QWidget;

/// 内建 EQ 插件: 保留原十段 ±12dB 滑杆 UI, 内部以十段增益为模型,
/// 输出为 EqConfig(10 条 Parametric, q≈1.414 对应 1 octave)。
/// 通过 PluginManager::register_builtin() 注册。
/// 注意: 只继承接口链(IEqPlugin→IBasicPlugin→QObject), 不要直接继承 QObject。
class BuiltinEqPlugin : public IEqPlugin
{
    Q_OBJECT
    Q_INTERFACES(IBasicPlugin IEqPlugin)

public:
    BuiltinEqPlugin();

    // ---- IBasicPlugin ----
    QString id() const override;
    QString name() const override;
    QString version() const override;
    QString description() const override;
    QString author() const override;
    QVector<QString> categories() const override;

    // ---- IEqPlugin ----
    QWidget* create_eq_widget(QWidget* parent = nullptr) override;
    EqConfig eq_config() const override;
    void apply_current() override;
    void revert() override;
    void reset() override;

private:
    void sync_sliders();

    std::array<float, 10> m_gains{};         // 当前编辑态
    std::array<float, 10> m_gains_applied{}; // 上次 apply 的快照(供 revert)
    std::array<QSlider*, 10> m_sliders{};
    std::array<QLabel*, 10> m_lb_values{};
};
