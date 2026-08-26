#include "view/settings_panel/shortcuts_panel/shortcuts_panel.h"

#include "core/config_manager/config_manager.h"
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonObject>

ShortcutsPanel::ShortcutsPanel(ConfigManager* cfg_mgr, QWidget* parent) :
    QWidget(parent), m_cfg_mgr(cfg_mgr)
{
    m_lb_search       = new QLabel(tr("Search functions:"), this);
    m_le_search       = new QLineEdit(this);
    m_hbl_search_line = new QHBoxLayout();

    m_hbl_search_line->addWidget(m_lb_search);
    m_hbl_search_line->addWidget(m_le_search);

    m_table_view_shortcuts = new QTableView(this);
    m_table_view_shortcuts->setEditTriggers(QAbstractItemView::DoubleClicked |
                                            QAbstractItemView::SelectedClicked |
                                            QAbstractItemView::EditKeyPressed);
    m_table_view_shortcuts->verticalHeader()->setVisible(false);
    m_table_view_shortcuts->horizontalHeader()->setStretchLastSection(true);

    m_btn_apply   = new QPushButton(tr("Apply"), this);
    m_btn_restore = new QPushButton(tr("Restore"), this);
    m_btn_default = new QPushButton(tr("Default"), this);
    m_hbl_buttom  = new QHBoxLayout();
    m_hbl_buttom->addWidget(m_btn_apply);
    m_hbl_buttom->addWidget(m_btn_restore);
    m_hbl_buttom->addWidget(m_btn_default);

    m_vbl_main = new QVBoxLayout();
    m_vbl_main->addLayout(m_hbl_search_line);
    m_vbl_main->addWidget(m_table_view_shortcuts);
    m_vbl_main->addLayout(m_hbl_buttom);

    this->setLayout(m_vbl_main);

    QJsonObject config_obj = m_cfg_mgr->read_sub_config(this->config_sub_key());
    this->load_from_json(config_obj);

    connect(m_btn_apply, &QPushButton::clicked, this, [this]() { emit sgnApplyConfig(); });
    connect(m_btn_default, &QPushButton::clicked, this, [this]() { emit sgnDefaultConfig(); });
    connect(m_btn_restore, &QPushButton::clicked, this, [this]() { emit sgnRestoreConfig(); });
}

ShortcutsPanel::~ShortcutsPanel() {}

void ShortcutsPanel::closeEvent(QCloseEvent* event)
{
    m_cfg_mgr->write_sub_config(this->config_sub_key(), this->save_to_json());
    QWidget::closeEvent(event);
}

QListWidgetItem* ShortcutsPanel::get_list_item()
{
    if (!m_list_item) {
        m_list_item = new QListWidgetItem(tr("Shortctus"));
    }
    return m_list_item;
}

void ShortcutsPanel::set_view_model(QAbstractTableModel* model)
{
    m_table_view_shortcuts->setModel(model);
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

QString ShortcutsPanel::config_sub_key() const
{
    return "shortcuts_panel";
}
