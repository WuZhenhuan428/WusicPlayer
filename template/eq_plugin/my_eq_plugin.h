#pragma once

#include "plugin/i_eq_plugin.h"

#include <QLabel>
#include <QSlider>
#include <QString>
#include <QVector>
#include <QtPlugin>
#include <array>

class QWidget;

/// EQ 插件模板(示例: 3 段 ±12dB 滑杆)。
/// 插件维护自己的"编辑态"与"已应用快照", 固定窗口(EQWidget)负责
/// Apply/Reset/Cancel/即时 编排, 插件只通过接口暴露配置与状态。
///
/// 注意: 只继承接口链(IEqPlugin -> IBasicPlugin -> QObject), 不要直接继承
/// QObject。
class MyEqPlugin : public IEqPlugin
{
    Q_OBJECT
    Q_INTERFACES(IBasicPlugin IEqPlugin)
    Q_PLUGIN_METADATA(IID "com.wusicplayer.IEqPlugin/1.0" FILE "my_eq_plugin.json")

public:
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
    void restore_from_config(const EqConfig& cfg) override;

private:
    void sync_sliders();

    static constexpr int kBandCount            = 3;
    static constexpr double kFreqs[kBandCount] = {250, 1000, 4000};

    std::array<float, kBandCount> m_gains{};         // 当前编辑态
    std::array<float, kBandCount> m_gains_applied{}; // 上次 apply 的快照(供 revert)
    std::array<QSlider*, kBandCount> m_sliders{};
    std::array<QLabel*, kBandCount> m_lb_values{};
};
