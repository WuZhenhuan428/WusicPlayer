#include "view/settings_panel/theme_settings_page/theme_settings_page.h"

#include "core/theme/theme_manager.h"
#include "model/theme_settings_model.h"
#include "service/theme_service.h"

#include <QFileDialog>
#include <QHeaderView>

ThemeSettingsPage::ThemeSettingsPage(ThemeService* service, QWidget* parent) :
    QWidget(parent), m_service(service)
{
    init_ui();
    init_connections();

    // 初次扫描并填充表格
    m_service->scan_themes();
    m_table_view->setModel(m_service->model());
    refresh_current_label();
}

void ThemeSettingsPage::init_ui()
{
    // 来源过滤 RadioButton
    m_source_group = new QButtonGroup(this);
    m_source_group->setExclusive(true);

    m_rb_all      = new QRadioButton(QStringLiteral("All"), this);
    m_rb_system   = new QRadioButton(QStringLiteral("System"), this);
    m_rb_builtin  = new QRadioButton(QStringLiteral("Builtin"), this);
    m_rb_external = new QRadioButton(QStringLiteral("External"), this);

    m_source_group->addButton(m_rb_all, 0);
    m_source_group->addButton(m_rb_system, 1);
    m_source_group->addButton(m_rb_builtin, 2);
    m_source_group->addButton(m_rb_external, 3);
    m_rb_all->setChecked(true);

    m_hbl_filter = new QHBoxLayout();
    m_hbl_filter->addWidget(new QLabel(QStringLiteral("Source:"), this));
    m_hbl_filter->addWidget(m_rb_all);
    m_hbl_filter->addWidget(m_rb_system);
    m_hbl_filter->addWidget(m_rb_builtin);
    m_hbl_filter->addWidget(m_rb_external);
    m_hbl_filter->addStretch();

    // 图标模式选择
    m_icon_group = new QButtonGroup(this);
    m_icon_group->setExclusive(true);
    m_rb_icon_auto  = new QRadioButton(QStringLiteral("Auto"), this);
    m_rb_icon_light = new QRadioButton(QStringLiteral("Light"), this);
    m_rb_icon_dark  = new QRadioButton(QStringLiteral("Dark"), this);
    m_icon_group->addButton(m_rb_icon_auto, 0);
    m_icon_group->addButton(m_rb_icon_light, 1);
    m_icon_group->addButton(m_rb_icon_dark, 2);

    // 根据当前 ThemeManager 状态初始化选中
    switch (ThemeManager::instance().icon_mode()) {
    case ThemeManager::IconLight:
        m_rb_icon_light->setChecked(true);
        break;
    case ThemeManager::IconDark:
        m_rb_icon_dark->setChecked(true);
        break;
    default:
        m_rb_icon_auto->setChecked(true);
        break;
    }

    QHBoxLayout* hbl_icon = new QHBoxLayout();
    hbl_icon->addWidget(new QLabel(QStringLiteral("Icon:"), this));
    hbl_icon->addWidget(m_rb_icon_auto);
    hbl_icon->addWidget(m_rb_icon_light);
    hbl_icon->addWidget(m_rb_icon_dark);
    hbl_icon->addStretch();

    // 当前主题标签
    m_lb_current = new QLabel(this);
    m_lb_hint    = new QLabel(
        QStringLiteral("Double-click a theme to apply, or select and click Apply."), this);

    // 表格
    m_table_view = new QTableView(this);
    m_table_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table_view->verticalHeader()->setVisible(false);
    m_table_view->horizontalHeader()->setStretchLastSection(true);
    m_table_view->setAlternatingRowColors(true);

    // 按钮
    m_btn_apply   = new QPushButton(QStringLiteral("Apply"), this);
    m_btn_rescan  = new QPushButton(QStringLiteral("Rescan Plugins"), this);

    m_hbl_actions = new QHBoxLayout();
    m_hbl_actions->addStretch();
    m_hbl_actions->addWidget(m_btn_rescan);
    m_hbl_actions->addWidget(m_btn_apply);

    // 主布局
    m_vbl_main = new QVBoxLayout(this);
    m_vbl_main->addLayout(m_hbl_filter);
    m_vbl_main->addLayout(hbl_icon);
    m_vbl_main->addWidget(m_lb_current);
    m_vbl_main->addWidget(m_lb_hint);
    m_vbl_main->addWidget(m_table_view);
    m_vbl_main->addLayout(m_hbl_actions);
}

void ThemeSettingsPage::init_connections()
{
    connect(m_source_group, &QButtonGroup::idClicked, this,
            &ThemeSettingsPage::on_source_filter_changed);
    connect(m_btn_apply, &QPushButton::clicked, this, &ThemeSettingsPage::on_apply_clicked);
    connect(m_btn_rescan, &QPushButton::clicked, this, &ThemeSettingsPage::on_rescan_clicked);

    // 双击表格行 → 直接应用
    connect(m_table_view, &QTableView::doubleClicked, this,
            [this](const QModelIndex&) { on_apply_clicked(); });

    // 主题切换后刷新标签
    connect(m_service, &ThemeService::sgn_current_theme_changed, this,
            [this]() { refresh_current_label(); });

    // 图标模式切换
    connect(m_icon_group, &QButtonGroup::idClicked, this, [](int id) {
        switch (id) {
        case 1:
            ThemeManager::instance().set_icon_mode(ThemeManager::IconLight);
            break;
        case 2:
            ThemeManager::instance().set_icon_mode(ThemeManager::IconDark);
            break;
        default:
            ThemeManager::instance().set_icon_mode(ThemeManager::IconAuto);
            break;
        }
    });
}

void ThemeSettingsPage::on_source_filter_changed(int /*id*/)
{
    auto* model = m_service->model();
    if (!model)
        return;

    QString filter;
    if (m_rb_system->isChecked())
        filter = QStringLiteral("System");
    else if (m_rb_builtin->isChecked())
        filter = QStringLiteral("Builtin");
    else if (m_rb_external->isChecked())
        filter = QStringLiteral("External");
    // "All" 时 filter 为空，显示全部

    // 遍历模型行，隐藏不匹配的
    for (int r = 0; r < model->rowCount(); ++r) {
        QModelIndex idx = model->index(r, ThemeSettingsModel::ColSource);
        QString source  = model->data(idx).toString();
        bool visible    = filter.isEmpty() || source == filter;
        m_table_view->setRowHidden(r, !visible);
    }
}

void ThemeSettingsPage::on_apply_clicked()
{
    auto sel = m_table_view->selectionModel()->selectedRows();
    if (sel.isEmpty())
        return;

    int row = sel.first().row();
    // 行隐藏后 selection 可能指向被过滤掉的行，需要校验
    if (m_table_view->isRowHidden(row))
        return;

    m_service->apply_theme(row);
}

void ThemeSettingsPage::on_rescan_clicked()
{
    // 简易选择外部插件目录；用户可取消
    QString dir = QFileDialog::getExistingDirectory(
        this, QStringLiteral("Select external theme plugins directory"), QString());
    if (dir.isEmpty())
        return;

    m_service->rescan_external_plugins(dir);
    m_table_view->setModel(m_service->model());
    // 重新应用过滤
    on_source_filter_changed(0);
}

void ThemeSettingsPage::refresh_current_label()
{
    m_lb_current->setText(QStringLiteral("Current: %1").arg(m_service->current_theme_name()));
}

QListWidgetItem* ThemeSettingsPage::get_title_item()
{
    if (!m_title_item) {
        m_title_item = new QListWidgetItem(QStringLiteral("Appearance"));
    }
    return m_title_item;
}
