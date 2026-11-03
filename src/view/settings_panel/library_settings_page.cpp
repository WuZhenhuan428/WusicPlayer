#include "view/settings_panel/library_settings_page.h"

#include "model/library/library_manager.h"

#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>

#include <utility>

LibrarySettingsPage::LibrarySettingsPage(LibraryManager* lib, QWidget* parent) :
    QWidget(parent), lib_(lib)
{
    init_ui();
    refresh_folders();
}

QListWidgetItem* LibrarySettingsPage::get_title_item()
{
    if (title_item_ == nullptr) {
        title_item_ = new QListWidgetItem(tr("Media Library"));
    }
    return title_item_;
}

void LibrarySettingsPage::init_ui()
{
    // 添加未入库文件时的解析策略
    auto* policy_row = new QHBoxLayout;
    auto* lbl_policy = new QLabel(tr("Add un-library files:"));
    cb_add_policy_   = new QComboBox;
    cb_add_policy_->addItem(tr("By operation"), std::to_underlying(AddFilePolicy::ByOperation));
    cb_add_policy_->addItem(tr("Import to library"),
                            std::to_underlying(AddFilePolicy::ImportToLibrary));
    cb_add_policy_->addItem(tr("Keep external"), std::to_underlying(AddFilePolicy::KeepExternal));
    cb_add_policy_->addItem(tr("Ask every time"), std::to_underlying(AddFilePolicy::AlwaysAsk));
    cb_add_policy_->setToolTip(tr("Folders add to library by default, Single file keeps external; "
                                  "Default behavior can be overriden here"));
    policy_row->addWidget(lbl_policy);
    policy_row->addWidget(cb_add_policy_, 1);

    list_         = new QListWidget;
    btn_add_      = new QPushButton(tr("Add folder"));
    btn_remove_   = new QPushButton(tr("Remove"));

    auto* btn_row = new QHBoxLayout;
    btn_row->addWidget(btn_add_);
    btn_row->addWidget(btn_remove_);
    btn_row->addStretch(1);

    auto* vbl = new QVBoxLayout(this);
    vbl->addLayout(policy_row);
    vbl->addWidget(list_, 1);
    vbl->addLayout(btn_row);

    connect(btn_add_, &QPushButton::clicked, this, &LibrarySettingsPage::add_folder);
    connect(btn_remove_, &QPushButton::clicked, this, &LibrarySettingsPage::remove_selected);
    connect(cb_add_policy_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                const int policy = cb_add_policy_->itemData(index).toInt();
                emit sgn_add_file_policy_changed(policy);
            });
}

void LibrarySettingsPage::set_add_file_policy(AddFilePolicy policy)
{
    const int idx = cb_add_policy_->findData(std::to_underlying(policy));
    if (idx < 0) {
        return;
    }
    cb_add_policy_->blockSignals(true);
    cb_add_policy_->setCurrentIndex(idx);
    cb_add_policy_->blockSignals(false);
}

void LibrarySettingsPage::refresh_folders()
{
    list_->clear();
    if (lib_ == nullptr) {
        return;
    }
    const QStringList folders = lib_->watched_folders();
    for (const QString& folder : folders) {
        list_->addItem(folder);
    }
}

void LibrarySettingsPage::add_folder()
{
    if (lib_ == nullptr) {
        return;
    }
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Add media folder"));
    if (dir.isEmpty()) {
        return;
    }
    lib_->add_watched_folder(dir); // 去重 + 触发异步扫描
    refresh_folders();
}

void LibrarySettingsPage::remove_selected()
{
    if (lib_ == nullptr) {
        return;
    }
    const int row = list_->currentRow();
    if (row < 0) {
        return;
    }
    const QString path = list_->item(row)->text();
    lib_->remove_watched_folder(path);
    refresh_folders();
}
