#include "view/settings_panel/shortcuts_panel/shortcuts_panel.h"

#include "core/config_manager/config_manager.h"
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>

ShortcutsPanel::ShortcutsPanel(ConfigManager* cfg_mgr, QWidget* parent) :
    QWidget(parent), cfg_mgr_(cfg_mgr)
{
    lb_search_       = new QLabel(tr("Search functions:"), this);
    le_search_       = new QLineEdit(this);
    hbl_search_line_ = new QHBoxLayout();

    hbl_search_line_->addWidget(lb_search_);
    hbl_search_line_->addWidget(le_search_);

    table_view_shortcuts_ = new QTableView(this);
    table_view_shortcuts_->setEditTriggers(QAbstractItemView::DoubleClicked |
                                           QAbstractItemView::SelectedClicked |
                                           QAbstractItemView::EditKeyPressed);
    table_view_shortcuts_->verticalHeader()->setVisible(false);
    table_view_shortcuts_->horizontalHeader()->setStretchLastSection(true);

    btn_apply_   = new QPushButton(tr("Apply"), this);
    btn_restore_ = new QPushButton(tr("Restore"), this);
    btn_default_ = new QPushButton(tr("Default"), this);
    hbl_buttom_  = new QHBoxLayout();
    hbl_buttom_->addWidget(btn_apply_);
    hbl_buttom_->addWidget(btn_restore_);
    hbl_buttom_->addWidget(btn_default_);

    vbl_main_ = new QVBoxLayout();
    vbl_main_->addLayout(hbl_search_line_);
    vbl_main_->addWidget(table_view_shortcuts_);
    vbl_main_->addLayout(hbl_buttom_);

    this->setLayout(vbl_main_);

    QJsonObject config_obj = ConfigManager::read_sub_config(ShortcutsPanel::config_sub_key());
    this->load_from_json(config_obj);

    connect(btn_apply_, &QPushButton::clicked, this, [this]() { emit sgn_apply_config(); });
    connect(btn_default_, &QPushButton::clicked, this, [this]() { emit sgn_default_config(); });
    connect(btn_restore_, &QPushButton::clicked, this, [this]() { emit sgn_restore_config(); });
}

void ShortcutsPanel::closeEvent(QCloseEvent* event)
{
    cfg_mgr_->write_sub_config(ShortcutsPanel::config_sub_key(), this->save_to_json());
    QWidget::closeEvent(event);
}

QListWidgetItem* ShortcutsPanel::get_list_item()
{
    if (!list_item_) {
        list_item_ = new QListWidgetItem(tr("Shortctus"));
    }
    return list_item_;
}

void ShortcutsPanel::set_view_model(QAbstractTableModel* model)
{
    table_view_shortcuts_->setModel(model);
}

void ShortcutsPanel::load_from_json(const QJsonObject& json)
{
    const QByteArray geometry = QByteArray::fromBase64(json.value("geometry").toString().toUtf8());
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
}

QJsonObject ShortcutsPanel::save_to_json()
{
    QJsonObject obj;
    obj["geometry"] = QString::fromUtf8(saveGeometry().toBase64());
    return obj;
}

QString ShortcutsPanel::config_sub_key()
{
    return "shortcuts_panel";
}
