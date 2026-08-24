#pragma once

#include "plugin/eq_types.h"

#include <QKeyEvent>
#include <QMetaObject>
#include <QVector>
#include <QWidget>

class AppContext;
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
    void sgnOpenSettingsRequested();

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
    QVector<IEqPlugin*> m_plugins;
    IEqPlugin* m_current_plugin = nullptr;
    QWidget* m_plugin_host      = nullptr; // 宿主(带 margin)
    QWidget* m_plugin_widget    = nullptr; // 当前插件 UI

    QComboBox* m_cb_plugins     = nullptr;
    QLabel* m_lb_current        = nullptr;
    QPushButton* m_btn_settings = nullptr;
    QCheckBox* m_cb_enable      = nullptr;
    QCheckBox* m_cb_instant     = nullptr;
    QPushButton* m_btn_apply    = nullptr;
    QPushButton* m_btn_reset    = nullptr;
    QPushButton* m_btn_cancel   = nullptr;

    QMetaObject::Connection m_instant_conn;
};
