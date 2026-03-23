#include "SettingsPanel.hpp"

SettingsPanel::SettingsPanel(QWidget *parent)
    : QWidget(parent)
{
    m_list_widget = new QListWidget(this);
    m_stacked_widget = new QStackedWidget(this);

    m_hbl_settings = new QHBoxLayout();
    
    m_hbl_settings->addWidget(m_list_widget);
    m_hbl_settings->addWidget(m_stacked_widget);

    m_btn_close = new QPushButton("Close", this);
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


    connect(m_list_widget, &QListWidget::doubleClicked, this, [this](const QModelIndex &index){
        m_stacked_widget->setCurrentIndex(index.row());
    });
    connect(m_btn_close, &QPushButton::clicked, this, &QWidget::close);
}

SettingsPanel::~SettingsPanel() {}


void SettingsPanel::registerWidget(QListWidgetItem* title, QWidget* widget) {
    m_list_widget->addItem(title);
    m_stacked_widget->addWidget(widget);
}

void SettingsPanel::emitStateSnapshot() {
    QByteArray geometry = saveGeometry();
    emit sgnStateSnapshot(geometry);
}

void SettingsPanel::closeEvent(QCloseEvent* event) {
    emitStateSnapshot();
    QWidget::closeEvent(event);
}

void SettingsPanel::hideEvent(QHideEvent* event) {
    emitStateSnapshot();
    QWidget::hideEvent(event);
}
