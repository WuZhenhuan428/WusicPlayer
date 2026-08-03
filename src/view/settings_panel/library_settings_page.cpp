#include "view/settings_panel/library_settings_page.h"

#include "model/library/library_manager.h"

#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>

LibrarySettingsPage::LibrarySettingsPage(LibraryManager* lib, QWidget* parent) :
    QWidget(parent), m_lib(lib)
{
    init_ui();
    refresh_folders();
}

QListWidgetItem* LibrarySettingsPage::get_title_item()
{
    if (m_title_item == nullptr) {
        m_title_item = new QListWidgetItem("Media Library");
    }
    return m_title_item;
}

void LibrarySettingsPage::init_ui()
{
    // 添加未入库文件时的解析策略
    auto* policy_row = new QHBoxLayout;
    auto* lbl_policy = new QLabel(tr("Add un-library files:"));
    m_cb_add_policy  = new QComboBox;
    m_cb_add_policy->addItem(tr("By operation"), int(AddFilePolicy::by_operation));
    m_cb_add_policy->addItem(tr("Import to library"), int(AddFilePolicy::import_to_library));
    m_cb_add_policy->addItem(tr("Keep external"), int(AddFilePolicy::keep_external));
    m_cb_add_policy->addItem(tr("Ask every time"), int(AddFilePolicy::always_ask));
    m_cb_add_policy->setToolTip(tr("文件夹默认同步入库,单文件默认仅外部;此处可覆盖默认行为"));
    policy_row->addWidget(lbl_policy);
    policy_row->addWidget(m_cb_add_policy, 1);

    m_list        = new QListWidget;
    m_btn_add     = new QPushButton(tr("Add folder"));
    m_btn_remove  = new QPushButton(tr("Remove"));

    auto* btn_row = new QHBoxLayout;
    btn_row->addWidget(m_btn_add);
    btn_row->addWidget(m_btn_remove);
    btn_row->addStretch(1);

    auto* vbl = new QVBoxLayout(this);
    vbl->addLayout(policy_row);
    vbl->addWidget(m_list, 1);
    vbl->addLayout(btn_row);

    connect(m_btn_add, &QPushButton::clicked, this, &LibrarySettingsPage::add_folder);
    connect(m_btn_remove, &QPushButton::clicked, this, &LibrarySettingsPage::remove_selected);
    connect(m_cb_add_policy, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                const int policy = m_cb_add_policy->itemData(index).toInt();
                emit sgnAddFilePolicyChanged(policy);
            });
}

void LibrarySettingsPage::set_add_file_policy(AddFilePolicy policy)
{
    const int idx = m_cb_add_policy->findData(int(policy));
    if (idx < 0) {
        return;
    }
    m_cb_add_policy->blockSignals(true);
    m_cb_add_policy->setCurrentIndex(idx);
    m_cb_add_policy->blockSignals(false);
}

void LibrarySettingsPage::refresh_folders()
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

void LibrarySettingsPage::add_folder()
{
    if (m_lib == nullptr) {
        return;
    }
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Add media folder"));
    if (dir.isEmpty()) {
        return;
    }
    m_lib->add_watched_folder(dir); // 去重 + 触发异步扫描
    refresh_folders();
}

void LibrarySettingsPage::remove_selected()
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
    refresh_folders();
}
