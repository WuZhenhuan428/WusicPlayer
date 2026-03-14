#include "SettingsPanel.hpp"

SettingsPanel::SettingsPanel(QWidget *parent)
    : QWidget(parent)
{
    m_listWidget = new QListWidget(this);
    m_stackedWidget = new QStackedWidget(this);

    m_settingsLayout = new QHBoxLayout();
    
    m_settingsLayout->addWidget(m_listWidget);
    m_settingsLayout->addWidget(m_stackedWidget);

    m_btnClose = new QPushButton("Close", this);
    m_bottomLayout = new QHBoxLayout();
    m_bottomLayout->addStretch();
    m_bottomLayout->addWidget(m_btnClose);

    m_mainLayout = new QVBoxLayout();
    m_mainLayout->addLayout(m_settingsLayout);
    m_mainLayout->addLayout(m_bottomLayout);

    this->setWindowTitle(tr("Settings"));

    this->setLayout(m_mainLayout);


    m_listWidget->setMinimumWidth(120);
    m_listWidget->setMaximumWidth(120);


    connect(m_listWidget, &QListWidget::doubleClicked, this, [this](const QModelIndex &index){
        m_stackedWidget->setCurrentIndex(index.row());
    });
    connect(m_btnClose, &QPushButton::clicked, this, &QWidget::close);
}

SettingsPanel::~SettingsPanel() {}


void SettingsPanel::registerWidget(QListWidgetItem* title, QWidget* widget) {
    m_listWidget->addItem(title);
    m_stackedWidget->addWidget(widget);
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
