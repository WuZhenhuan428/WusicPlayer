#include "library_settings_page.h"

#include "model/library/library_manager.h"

#include <QFileDialog>
#include <QHBoxLayout>

LibrarySettingsPage::LibrarySettingsPage(LibraryManager* lib, QWidget* parent) :
    QWidget(parent), m_lib(lib)
{
    initUI();
    refreshFolders();
}

QListWidgetItem* LibrarySettingsPage::getTitleItem()
{
    if (m_title_item == nullptr) {
        m_title_item = new QListWidgetItem("Media Library");
    }
    return m_title_item;
}

void LibrarySettingsPage::initUI()
{
    m_list        = new QListWidget;
    m_btn_add     = new QPushButton(tr("Add folder"));
    m_btn_remove  = new QPushButton(tr("Remove"));

    auto* btn_row = new QHBoxLayout;
    btn_row->addWidget(m_btn_add);
    btn_row->addWidget(m_btn_remove);
    btn_row->addStretch(1);

    auto* vbl = new QVBoxLayout(this);
    vbl->addWidget(m_list, 1);
    vbl->addLayout(btn_row);

    connect(m_btn_add, &QPushButton::clicked, this, &LibrarySettingsPage::addFolder);
    connect(m_btn_remove, &QPushButton::clicked, this, &LibrarySettingsPage::removeSelected);
}

void LibrarySettingsPage::refreshFolders()
{
    m_list->clear();
    if (m_lib == nullptr) {
        return;
    }
    const QStringList folders = m_lib->watched_folders();
    for (const QString& folder : folders) {
        m_list->addItem(folder);
    }
}

void LibrarySettingsPage::addFolder()
{
    if (m_lib == nullptr) {
        return;
    }
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Add media folder"));
    if (dir.isEmpty()) {
        return;
    }
    m_lib->add_watched_folder(dir); // 去重 + 触发异步扫描
    refreshFolders();
}

void LibrarySettingsPage::removeSelected()
{
    if (m_lib == nullptr) {
        return;
    }
    const int row = m_list->currentRow();
    if (row < 0) {
        return;
    }
    const QString path = m_list->item(row)->text();
    m_lib->remove_watched_folder(path);
    refreshFolders();
}
