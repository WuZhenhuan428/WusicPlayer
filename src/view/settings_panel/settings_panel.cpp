#include "view/settings_panel/settings_panel.h"

#include "core/config_manager/config_manager.h"
#include <QJsonObject>

SettingsPanel::SettingsPanel(ConfigManager* cfg_mgr, QWidget* parent) :
    QWidget(parent), m_cfg_mgr(cfg_mgr)
{
    m_list_widget    = new QListWidget(this);
    m_stacked_widget = new QStackedWidget(this);

    m_hbl_settings   = new QHBoxLayout();

    m_hbl_settings->addWidget(m_list_widget);
    m_hbl_settings->addWidget(m_stacked_widget);

    m_btn_close  = new QPushButton("Close", this);
    m_hbl_bottom = new QHBoxLayout();
    m_hbl_bottom->addStretch();
    m_hbl_bottom->addWidget(m_btn_close);

    m_vbl_main = new QVBoxLayout();
    m_vbl_main->addLayout(m_hbl_settings);
    m_vbl_main->addLayout(m_hbl_bottom);

    this->setWindowTitle(tr("Settings"));

    this->setLayout(m_vbl_main);

    m_list_widget->setMinimumWidth(120);
    m_list_widget->setMaximumWidth(120);

    QJsonObject config_obj = m_cfg_mgr->read_sub_config(this->config_sub_key());
    this->load_from_json(config_obj);

    connect(m_list_widget, &QListWidget::doubleClicked, this,
            [this](const QModelIndex& index) { m_stacked_widget->setCurrentIndex(index.row()); });
    connect(m_btn_close, &QPushButton::clicked, this, &QWidget::close);
}

SettingsPanel::~SettingsPanel() {}

void SettingsPanel::register_widget(QListWidgetItem* title, QWidget* widget)
{
    m_list_widget->addItem(title);
    m_stacked_widget->addWidget(widget);
}

void SettingsPanel::switch_to_page_by_title(const QString& title)
{
    if (title.isEmpty()) {
        return;
    }

    for (int i = 0; i < m_list_widget->count(); ++i) {
        QListWidgetItem* item = m_list_widget->item(i);
        if (item && item->text() == title) {
            m_list_widget->setCurrentRow(i);
            m_stacked_widget->setCurrentIndex(i);
            return;
        }
    }
}

void SettingsPanel::closeEvent(QCloseEvent* event)
{
    m_cfg_mgr->write_sub_config(this->config_sub_key(), this->save_to_json());
    QWidget::closeEvent(event);
}

void SettingsPanel::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
}

void SettingsPanel::load_from_json(const QJsonObject& json)
{
    const QByteArray geometry = QByteArray::fromBase64(json.value("geometry").toString().toUtf8());
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
}

QJsonObject SettingsPanel::save_to_json()
{
    QJsonObject obj;
    obj["geometry"] = QString::fromUtf8(this->saveGeometry().toBase64());
    return obj;
}

QString SettingsPanel::config_sub_key() const
{
    return "settings_panel";
}
