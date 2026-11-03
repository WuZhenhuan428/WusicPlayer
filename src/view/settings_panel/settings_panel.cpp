#include "view/settings_panel/settings_panel.h"

#include "core/config_manager/config_manager.h"
#include <QJsonObject>

SettingsPanel::SettingsPanel(ConfigManager* cfg_mgr, QWidget* parent) :
    QWidget(parent), cfg_mgr_(cfg_mgr)
{
    list_widget_    = new QListWidget(this);
    stacked_widget_ = new QStackedWidget(this);

    hbl_settings_   = new QHBoxLayout();

    hbl_settings_->addWidget(list_widget_);
    hbl_settings_->addWidget(stacked_widget_);

    btn_close_  = new QPushButton(tr("Close"), this);
    hbl_bottom_ = new QHBoxLayout();
    hbl_bottom_->addStretch();
    hbl_bottom_->addWidget(btn_close_);

    vbl_main_ = new QVBoxLayout();
    vbl_main_->addLayout(hbl_settings_);
    vbl_main_->addLayout(hbl_bottom_);

    this->setWindowTitle(tr("Settings"));

    this->setLayout(vbl_main_);

    list_widget_->setMinimumWidth(120);
    list_widget_->setMaximumWidth(120);

    QJsonObject config_obj = ConfigManager::read_sub_config(SettingsPanel::config_sub_key());
    this->load_from_json(config_obj);

    connect(list_widget_, &QListWidget::doubleClicked, this,
            [this](const QModelIndex& index) { stacked_widget_->setCurrentIndex(index.row()); });
    connect(btn_close_, &QPushButton::clicked, this, &QWidget::close);
}

void SettingsPanel::register_widget(QListWidgetItem* title, QWidget* widget)
{
    list_widget_->addItem(title);
    stacked_widget_->addWidget(widget);
}

void SettingsPanel::switch_to_page_by_title(const QString& title)
{
    if (title.isEmpty()) {
        return;
    }

    for (int i = 0; i < list_widget_->count(); ++i) {
        QListWidgetItem* item = list_widget_->item(i);
        if (item && item->text() == title) {
            list_widget_->setCurrentRow(i);
            stacked_widget_->setCurrentIndex(i);
            return;
        }
    }
}

void SettingsPanel::closeEvent(QCloseEvent* event)
{
    cfg_mgr_->write_sub_config(config_sub_key(), this->save_to_json());
    QWidget::closeEvent(event);
}

void SettingsPanel::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
}

void SettingsPanel::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
        this->close();
        break;
    default:
        break;
    }
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

QString SettingsPanel::config_sub_key()
{
    return "settings_panel";
}
