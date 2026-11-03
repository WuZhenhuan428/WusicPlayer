#pragma once

#include "plugin/eq_types.h"

#include <QKeyEvent>
#include <QMetaObject>
#include <QVector>
#include <QWidget>

struct AppContext;
class IEqPlugin;
class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;

/// EQ 固定窗口容器。
/// 布局:
///   [选择 EQ: combo] [跳转至设置]
///   [当前插件: <名称>]         stretch
///   [插件控件宿主(带 margin)]
///   [启用EQ] [立即生效] stretch [Apply] [Reset] [Cancel]
/// 插件来自 PluginManager::plugins<IEqPlugin>(), 容器只做编排, 不持有插件状态。
class EQWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EQWidget(AppContext& ctx, QWidget* parent = nullptr);

signals:
    /// 点击“跳转至设置页面”
    void sgn_open_settings_requested();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    void init_ui();
    void reload_plugins();
    void switch_plugin(int index);
    void on_plugin_config_changed();
    EqConfig compose_config() const;
    void apply_to_backend();

    AppContext& ctx_;
    QVector<IEqPlugin*> plugins_;
    IEqPlugin* current_plugin_ = nullptr;
    QWidget* plugin_host_      = nullptr; // 宿主(带 margin)
    QWidget* plugin_widget_    = nullptr; // 当前插件 UI
    int loaded_index_          = -1;      // 已加载的插件索引(防重复切换)

    QComboBox* cb_plugins_     = nullptr;
    QLabel* lb_current_        = nullptr;
    QPushButton* btn_settings_ = nullptr;
    QCheckBox* cb_enable_      = nullptr;
    QCheckBox* cb_instant_     = nullptr;
    QPushButton* btn_apply_    = nullptr;
    QPushButton* btn_reset_    = nullptr;
    QPushButton* btn_cancel_   = nullptr;

    QMetaObject::Connection instant_conn_;
};
