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
    cb_plugins_ = new QComboBox(this);
    cb_plugins_->setMinimumWidth(220);
    btn_settings_ = new QPushButton(tr("EQ Settings..."), this);
    btn_settings_->setToolTip(tr("Jump to the plugin management page"));
    hbl_select->addWidget(cb_plugins_, 1);
    hbl_select->addWidget(btn_settings_);

    // ---- 第 2 行: 当前插件名称 ------------------------
    lb_current_       = new QLabel(this);

    // ---- 第 3 行: 插件控件宿主(留边以突出插件边界) ----
    plugin_host_      = new QWidget(this);
    auto* host_layout = new QVBoxLayout(plugin_host_);
    host_layout->setContentsMargins(8, 8, 8, 8);

    // ---- 第 4 行: 通用控制 ---------------------------
    cb_enable_        = new QCheckBox(tr("Enable EQ"), this);
    cb_instant_       = new QCheckBox(tr("Instant apply"), this);
    btn_apply_        = new QPushButton(tr("Apply"), this);
    btn_reset_        = new QPushButton(tr("Reset"), this);
    btn_cancel_       = new QPushButton(tr("Cancel"), this);

    auto* hbl_control = new QHBoxLayout;
    hbl_control->addWidget(cb_enable_);
    hbl_control->addWidget(cb_instant_);
    hbl_control->addStretch();
    hbl_control->addWidget(btn_apply_);
    hbl_control->addWidget(btn_reset_);
    hbl_control->addWidget(btn_cancel_);

    auto* main_layout = new QVBoxLayout(this);
    main_layout->addLayout(hbl_select);
    main_layout->addWidget(lb_current_);
    main_layout->addWidget(plugin_host_, 1);
    main_layout->addLayout(hbl_control);

    // 打开窗口时同步后端 EQ 启用状态
    cb_enable_->setChecked(ctx_.playback_controller->eq_config().enabled);

    connect(cb_plugins_, &QComboBox::currentIndexChanged, this, &EQWidget::switch_plugin);
    connect(btn_settings_, &QPushButton::clicked, this, &EQWidget::sgn_open_settings_requested);
    connect(btn_apply_, &QPushButton::clicked, this, [this]() {
        if (!current_plugin_) {
            return;
        }
        current_plugin_->apply_current();
        this->apply_to_backend();
    });
    connect(btn_reset_, &QPushButton::clicked, this, [this]() {
        if (!current_plugin_) {
            return;
        }
        current_plugin_->reset();
    });
    connect(btn_cancel_, &QPushButton::clicked, this, [this]() {
        if (!current_plugin_) {
            return;
        }
        current_plugin_->revert();
    });
    connect(cb_enable_, &QCheckBox::toggled, this, [this](bool /*checked*/) {
        if (cb_instant_->isChecked()) {
            this->apply_to_backend();
        }
    });
}

void EQWidget::reload_plugins()
{
    plugins_ = ctx_.plugin_controller->plugin_manager()->plugins<IEqPlugin>();

    cb_plugins_->blockSignals(true);
    cb_plugins_->clear();
    for (IEqPlugin* plugin : plugins_) {
        cb_plugins_->addItem(plugin->name());
    }
    cb_plugins_->blockSignals(false);

    if (plugins_.empty()) {
        lb_current_->setText(tr("Current plugin: (none)"));
        return;
    }

    // 恢复上次选中的插件(id 匹配; 找不到则默认第一个)
    const QString saved_id = ctx_.playback_controller->eq_plugin_id();
    int index              = 0;
    for (int i = 0; i < plugins_.size(); ++i) {
        if (plugins_[i]->id() == saved_id) {
            index = i;
            break;
        }
    }
    cb_plugins_->setCurrentIndex(index);
    // 注意: addItem 添加首个条目时 Qt 已自动把 currentIndex 置为 0,
    // 此后再 setCurrentIndex(0) 不会触发 currentIndexChanged 信号,
    // 必须显式调用 switch_plugin 完成首次插件加载。
    // switch_plugin 内部按 m_loaded_index_ 去重, 重复调用安全。
    this->switch_plugin(index);
}

void EQWidget::switch_plugin(int index)
{
    // 防重: setCurrentIndex 触发信号与显式调用可能重复进入
    if (loaded_index_ == index && current_plugin_) {
        return;
    }
    loaded_index_ = index;

    if (instant_conn_) {
        disconnect(instant_conn_);
        instant_conn_ = {};
    }
    if (plugin_widget_) {
        plugin_widget_->deleteLater();
        plugin_widget_ = nullptr;
    }

    current_plugin_ = (index >= 0 && index < plugins_.size()) ? plugins_[index] : nullptr;
    if (!current_plugin_) {
        lb_current_->setText(tr("Current plugin: (none)"));
        return;
    }

    lb_current_->setText(tr("Current plugin: %1").arg(current_plugin_->name()));
    plugin_widget_ = current_plugin_->create_eq_widget(plugin_host_);
    if (plugin_widget_) {
        plugin_host_->layout()->addWidget(plugin_widget_);
    }

    // 用后端当前配置同步插件 UI(启动恢复/持久化的配置)。
    // 必须在连接 sgn_config_changed 之前执行, 避免 restore 引发的
    // valueChanged 触发即时推送。
    current_plugin_->restore_from_config(ctx_.playback_controller->eq_config());

    // 记录本次选中的插件(退出时持久化)
    ctx_.playback_controller->set_eq_plugin_id(current_plugin_->id());

    // 即时模式: 插件 UI 改动即同步后端
    instant_conn_ = connect(current_plugin_, &IEqPlugin::sgn_config_changed, this,
                            [this]() { this->on_plugin_config_changed(); });
}

void EQWidget::on_plugin_config_changed()
{
    if (cb_instant_->isChecked()) {
        this->apply_to_backend();
    }
}

EqConfig EQWidget::compose_config() const
{
    EqConfig cfg;
    if (current_plugin_) {
        cfg = current_plugin_->eq_config();
    }
    cfg.enabled = cb_enable_->isChecked();
    return cfg;
}

void EQWidget::apply_to_backend()
{
    ctx_.playback_controller->set_eq_config(this->compose_config());
}

void EQWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        this->close();
        return;
    }
    QWidget::keyPressEvent(event);
}
