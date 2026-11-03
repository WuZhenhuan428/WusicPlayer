#include "view/settings_panel/theme_settings_page/theme_settings_page.h"

#include "core/theme/theme_manager.h"
#include "model/theme_settings_model.h"
#include "service/theme_service.h"

#include <QFileDialog>
#include <QHeaderView>

ThemeSettingsPage::ThemeSettingsPage(ThemeService* service, QWidget* parent) :
    QWidget(parent), service_(service)
{
    init_ui();
    init_connections();

    // 初次扫描并填充表格
    service_->scan_themes();
    table_view_->setModel(service_->model());
    refresh_current_label();
}

void ThemeSettingsPage::init_ui()
{
    // 来源过滤 RadioButton
    source_group_ = new QButtonGroup(this);
    source_group_->setExclusive(true);

    rb_all_      = new QRadioButton(tr("All"), this);
    rb_system_   = new QRadioButton(tr("System"), this);
    rb_builtin_  = new QRadioButton(tr("Builtin"), this);
    rb_external_ = new QRadioButton(tr("External"), this);

    source_group_->addButton(rb_all_, 0);
    source_group_->addButton(rb_system_, 1);
    source_group_->addButton(rb_builtin_, 2);
    source_group_->addButton(rb_external_, 3);
    rb_all_->setChecked(true);

    hbl_filter_ = new QHBoxLayout();
    hbl_filter_->addWidget(new QLabel(tr("Source:"), this));
    hbl_filter_->addWidget(rb_all_);
    hbl_filter_->addWidget(rb_system_);
    hbl_filter_->addWidget(rb_builtin_);
    hbl_filter_->addWidget(rb_external_);
    hbl_filter_->addStretch();

    // 图标模式选择
    icon_group_ = new QButtonGroup(this);
    icon_group_->setExclusive(true);
    rb_icon_auto_  = new QRadioButton(tr("Auto"), this);
    rb_icon_light_ = new QRadioButton(tr("Light"), this);
    rb_icon_dark_  = new QRadioButton(tr("Dark"), this);
    icon_group_->addButton(rb_icon_auto_, 0);
    icon_group_->addButton(rb_icon_light_, 1);
    icon_group_->addButton(rb_icon_dark_, 2);

    // 根据当前 ThemeManager 状态初始化选中
    switch (ThemeManager::instance().icon_mode()) {
    case ThemeManager::IconMode::IconLight:
        rb_icon_light_->setChecked(true);
        break;
    case ThemeManager::IconMode::IconDark:
        rb_icon_dark_->setChecked(true);
        break;
    default:
        rb_icon_auto_->setChecked(true);
        break;
    }

    QHBoxLayout* hbl_icon = new QHBoxLayout();
    hbl_icon->addWidget(new QLabel(tr("Icon:"), this));
    hbl_icon->addWidget(rb_icon_auto_);
    hbl_icon->addWidget(rb_icon_light_);
    hbl_icon->addWidget(rb_icon_dark_);
    hbl_icon->addStretch();

    // 当前主题标签
    lb_current_ = new QLabel(this);
    lb_hint_    = new QLabel(tr("Double-click a theme to apply, or select and click Apply."), this);

    // 表格
    table_view_ = new QTableView(this);
    table_view_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_view_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_view_->verticalHeader()->setVisible(false);
    table_view_->horizontalHeader()->setStretchLastSection(true);
    table_view_->setAlternatingRowColors(true);

    // 按钮
    btn_apply_   = new QPushButton(tr("Apply"), this);
    btn_rescan_  = new QPushButton(tr("Rescan Plugins"), this);

    hbl_actions_ = new QHBoxLayout();
    hbl_actions_->addStretch();
    hbl_actions_->addWidget(btn_rescan_);
    hbl_actions_->addWidget(btn_apply_);

    // 主布局
    vbl_main_ = new QVBoxLayout(this);
    vbl_main_->addLayout(hbl_filter_);
    vbl_main_->addLayout(hbl_icon);
    vbl_main_->addWidget(lb_current_);
    vbl_main_->addWidget(lb_hint_);
    vbl_main_->addWidget(table_view_);
    vbl_main_->addLayout(hbl_actions_);
}

void ThemeSettingsPage::init_connections()
{
    connect(source_group_, &QButtonGroup::idClicked, this,
            &ThemeSettingsPage::on_source_filter_changed);
    connect(btn_apply_, &QPushButton::clicked, this, &ThemeSettingsPage::on_apply_clicked);
    connect(btn_rescan_, &QPushButton::clicked, this, &ThemeSettingsPage::on_rescan_clicked);

    // 双击表格行 → 直接应用
    connect(table_view_, &QTableView::doubleClicked, this,
            [this](const QModelIndex&) { on_apply_clicked(); });

    // 主题切换后刷新标签
    connect(service_, &ThemeService::sgn_current_theme_changed, this,
            [this]() { refresh_current_label(); });

    // 图标模式切换
    connect(icon_group_, &QButtonGroup::idClicked, this, [](int id) {
        switch (id) {
        case 1:
            ThemeManager::instance().set_icon_mode(ThemeManager::IconMode::IconLight);
            break;
        case 2:
            ThemeManager::instance().set_icon_mode(ThemeManager::IconMode::IconDark);
            break;
        default:
            ThemeManager::instance().set_icon_mode(ThemeManager::IconMode::IconAuto);
            break;
        }
    });
}

void ThemeSettingsPage::on_source_filter_changed(int /*id*/)
{
    auto* model = service_->model();
    if (!model) {
        return;
    }

    QString filter;
    if (rb_system_->isChecked()) {
        filter = tr("System");
    } else if (rb_builtin_->isChecked()) {
        filter = tr("Builtin");
    } else if (rb_external_->isChecked()) {
        filter = tr("External");
    }
    // "All" 时 filter 为空，显示全部

    // 遍历模型行，隐藏不匹配的
    for (int r = 0; r < model->rowCount(); ++r) {
        QModelIndex idx = model->index(r, ThemeSettingsModel::ColSource);
        QString source  = model->data(idx).toString();
        bool visible    = filter.isEmpty() || source == filter;
        table_view_->setRowHidden(r, !visible);
    }
}

void ThemeSettingsPage::on_apply_clicked()
{
    auto sel = table_view_->selectionModel()->selectedRows();
    if (sel.isEmpty()) {
        return;
    }

    int row = sel.first().row();
    // 行隐藏后 selection 可能指向被过滤掉的行，需要校验
    if (table_view_->isRowHidden(row)) {
        return;
    }

    service_->apply_theme(row);
}

void ThemeSettingsPage::on_rescan_clicked()
{
    // 简易选择外部插件目录；用户可取消
    QString dir = QFileDialog::getExistingDirectory(
        this, tr("Select external theme plugins directory"), QString());
    if (dir.isEmpty()) {
        return;
    }

    service_->rescan_external_plugins(dir);
    table_view_->setModel(service_->model());
    // 重新应用过滤
    on_source_filter_changed(0);
}

void ThemeSettingsPage::refresh_current_label()
{
    lb_current_->setText(tr("Current: %1").arg(ThemeService::current_theme_name()));
}

QListWidgetItem* ThemeSettingsPage::get_title_item()
{
    if (!title_item_) {
        title_item_ = new QListWidgetItem(tr("Appearance"));
    }
    return title_item_;
}
