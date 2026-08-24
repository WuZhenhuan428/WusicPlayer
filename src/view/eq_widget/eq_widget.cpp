#include "view/eq_widget/eq_widget.h"

#include "app_context.h"
#include "controller/playback_controller.h"
#include "controller/plugin_controller/plugin_controller.h"
#include "plugin/i_eq_plugin.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

EQWidget::EQWidget(AppContext& ctx, QWidget* parent) : QWidget(parent), ctx_(ctx)
{
    this->init_ui();
    this->reload_plugins();
}

void EQWidget::init_ui()
{
    // ---- 第 1 行: 插件选择 + 跳转设置 ----------------
    auto* hbl_select = new QHBoxLayout;
    hbl_select->addWidget(new QLabel(tr("Select EQ:"), this));
    m_cb_plugins = new QComboBox(this);
    m_cb_plugins->setMinimumWidth(220);
    m_btn_settings = new QPushButton(tr("EQ Settings..."), this);
    m_btn_settings->setToolTip(tr("Jump to the plugin management page"));
    hbl_select->addWidget(m_cb_plugins, 1);
    hbl_select->addWidget(m_btn_settings);

    // ---- 第 2 行: 当前插件名称 ------------------------
    m_lb_current      = new QLabel(this);

    // ---- 第 3 行: 插件控件宿主(留边以突出插件边界) ----
    m_plugin_host     = new QWidget(this);
    auto* host_layout = new QVBoxLayout(m_plugin_host);
    host_layout->setContentsMargins(8, 8, 8, 8);

    // ---- 第 4 行: 通用控制 ---------------------------
    m_cb_enable       = new QCheckBox(tr("Enable EQ"), this);
    m_cb_instant      = new QCheckBox(tr("Instant apply"), this);
    m_btn_apply       = new QPushButton(tr("Apply"), this);
    m_btn_reset       = new QPushButton(tr("Reset"), this);
    m_btn_cancel      = new QPushButton(tr("Cancel"), this);

    auto* hbl_control = new QHBoxLayout;
    hbl_control->addWidget(m_cb_enable);
    hbl_control->addWidget(m_cb_instant);
    hbl_control->addStretch();
    hbl_control->addWidget(m_btn_apply);
    hbl_control->addWidget(m_btn_reset);
    hbl_control->addWidget(m_btn_cancel);

    auto* main_layout = new QVBoxLayout(this);
    main_layout->addLayout(hbl_select);
    main_layout->addWidget(m_lb_current);
    main_layout->addWidget(m_plugin_host, 1);
    main_layout->addLayout(hbl_control);

    // 打开窗口时同步后端 EQ 启用状态
    m_cb_enable->setChecked(ctx_.playback_controller_->eq_config().enabled);

    connect(m_cb_plugins, &QComboBox::currentIndexChanged, this, &EQWidget::switch_plugin);
    connect(m_btn_settings, &QPushButton::clicked, this, &EQWidget::sgnOpenSettingsRequested);
    connect(m_btn_apply, &QPushButton::clicked, this, [this]() {
        if (!m_current_plugin) {
            return;
        }
        m_current_plugin->apply_current();
        this->apply_to_backend();
    });
    connect(m_btn_reset, &QPushButton::clicked, this, [this]() {
        if (!m_current_plugin) {
            return;
        }
        m_current_plugin->reset();
    });
    connect(m_btn_cancel, &QPushButton::clicked, this, [this]() {
        if (!m_current_plugin) {
            return;
        }
        m_current_plugin->revert();
    });
    connect(m_cb_enable, &QCheckBox::toggled, this, [this](bool /*checked*/) {
        if (m_cb_instant->isChecked()) {
            this->apply_to_backend();
        }
    });
}

void EQWidget::reload_plugins()
{
    m_plugins = ctx_.plugin_controller_->plugin_manager()->plugins<IEqPlugin>();

    m_cb_plugins->blockSignals(true);
    m_cb_plugins->clear();
    for (IEqPlugin* plugin : m_plugins) {
        m_cb_plugins->addItem(plugin->name());
    }
    m_cb_plugins->blockSignals(false);

    if (m_plugins.empty()) {
        m_lb_current->setText(tr("Current plugin: (none)"));
        return;
    }

    // 恢复上次选中的插件(id 匹配; 找不到则默认第一个)
    const QString saved_id = ctx_.playback_controller_->eq_plugin_id();
    int index              = 0;
    for (int i = 0; i < m_plugins.size(); ++i) {
        if (m_plugins[i]->id() == saved_id) {
            index = i;
            break;
        }
    }
    m_cb_plugins->setCurrentIndex(index);
    // 注意: addItem 添加首个条目时 Qt 已自动把 currentIndex 置为 0,
    // 此后再 setCurrentIndex(0) 不会触发 currentIndexChanged 信号,
    // 必须显式调用 switch_plugin 完成首次插件加载。
    // switch_plugin 内部按 m_loaded_index_ 去重, 重复调用安全。
    this->switch_plugin(index);
}

void EQWidget::switch_plugin(int index)
{
    // 防重: setCurrentIndex 触发信号与显式调用可能重复进入
    if (m_loaded_index_ == index && m_current_plugin) {
        return;
    }
    m_loaded_index_ = index;

    if (m_instant_conn) {
        disconnect(m_instant_conn);
        m_instant_conn = {};
    }
    if (m_plugin_widget) {
        m_plugin_widget->deleteLater();
        m_plugin_widget = nullptr;
    }

    m_current_plugin = (index >= 0 && index < m_plugins.size()) ? m_plugins[index] : nullptr;
    if (!m_current_plugin) {
        m_lb_current->setText(tr("Current plugin: (none)"));
        return;
    }

    m_lb_current->setText(tr("Current plugin: %1").arg(m_current_plugin->name()));
    m_plugin_widget = m_current_plugin->create_eq_widget(m_plugin_host);
    if (m_plugin_widget) {
        m_plugin_host->layout()->addWidget(m_plugin_widget);
    }

    // 用后端当前配置同步插件 UI(启动恢复/持久化的配置)。
    // 必须在连接 sgn_config_changed 之前执行, 避免 restore 引发的
    // valueChanged 触发即时推送。
    m_current_plugin->restore_from_config(ctx_.playback_controller_->eq_config());

    // 记录本次选中的插件(退出时持久化)
    ctx_.playback_controller_->set_eq_plugin_id(m_current_plugin->id());

    // 即时模式: 插件 UI 改动即同步后端
    m_instant_conn = connect(m_current_plugin, &IEqPlugin::sgn_config_changed, this,
                             [this]() { this->on_plugin_config_changed(); });
}

void EQWidget::on_plugin_config_changed()
{
    if (m_cb_instant->isChecked()) {
        this->apply_to_backend();
    }
}

EqConfig EQWidget::compose_config() const
{
    EqConfig cfg;
    if (m_current_plugin) {
        cfg = m_current_plugin->eq_config();
    }
    cfg.enabled = m_cb_enable->isChecked();
    return cfg;
}

void EQWidget::apply_to_backend()
{
    ctx_.playback_controller_->set_eq_config(this->compose_config());
}

void EQWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        this->close();
        return;
    }
    QWidget::keyPressEvent(event);
}
