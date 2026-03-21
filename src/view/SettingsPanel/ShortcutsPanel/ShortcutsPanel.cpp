#include "ShortcutsPanel.hpp"

ShortcutsPanel::ShortcutsPanel(QWidget* parent)
    : QWidget(parent)
{
    m_labelSearch = new QLabel("Search functions:", this);
    m_lineEditSearch = new QLineEdit(this);
    m_searchLineLayout = new QHBoxLayout();

    m_searchLineLayout->addWidget(m_labelSearch);
    m_searchLineLayout->addWidget(m_lineEditSearch);

    m_viewShortcuts = new QTableView(this);
    m_viewShortcuts->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);

    m_btnApply = new QPushButton("Apply", this);
    m_btnRestore = new QPushButton("Restore", this);
    m_btnDefault = new QPushButton("Default", this);
    m_buttomLayout = new QHBoxLayout();
    m_buttomLayout->addWidget(m_btnApply);
    m_buttomLayout->addWidget(m_btnRestore);
    m_buttomLayout->addWidget(m_btnDefault);

    m_mainLayout = new QVBoxLayout();
    m_mainLayout->addLayout(m_searchLineLayout);
    m_mainLayout->addWidget(m_viewShortcuts);
    m_mainLayout->addLayout(m_buttomLayout);

    this->setLayout(m_mainLayout);

    connect(m_btnApply, &QPushButton::clicked, this, [this](){ emit sgnApplyConfig(); });
    connect(m_btnDefault, &QPushButton::clicked, this, [this](){ emit sgnDefaultConfig(); });
    connect(m_btnRestore, &QPushButton::clicked, this, [this](){ emit sgnRestoreConfig(); });
}

ShortcutsPanel::~ShortcutsPanel() {}

QListWidgetItem* ShortcutsPanel::getListItem() {
    if (!m_list_item) {
        m_list_item = new QListWidgetItem("Shortctus");
    }
    return m_list_item;
}

void ShortcutsPanel::setViewModel(QAbstractTableModel* model) {
    m_viewShortcuts->setModel(model);
}