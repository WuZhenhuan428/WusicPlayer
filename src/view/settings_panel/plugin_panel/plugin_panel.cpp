#include "plugin_panel.h"

#include "core/utils/string.hpp"

#include <QHeaderView>
#include <QListWidgetItem>
#include <QTableWidgetItem>

#include <utility>

PluginPanel::PluginPanel(const QVector<PluginDescriptor> descriptors, QWidget* parent) :
    QWidget(parent), descriptors_(descriptors)
{
    this->init_ui();
    this->init_connections();
    this->refresh();
}

void PluginPanel::init_ui()
{
    tw_plugin_list_ = new QTableWidget(this);
    tw_plugin_list_->setColumnCount(6); // PluginDescriptor 全部显示
    tw_plugin_list_->setHorizontalHeaderLabels(
        {tr("ID"), tr("Name"), tr("Version"), tr("Description"), tr("Author"), tr("Categories")});
    tw_plugin_list_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    tw_plugin_list_->horizontalHeader()->setSectionsMovable(true);
    tw_plugin_list_->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    tw_plugin_list_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tw_plugin_list_->setSelectionMode(QAbstractItemView::SingleSelection);
    tw_plugin_list_->verticalHeader()->hide();
    tw_plugin_list_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tw_plugin_list_->setDragEnabled(true);

    btn_refresh_     = new QPushButton(tr("Refresh"), this);
    btn_import_      = new QPushButton(tr("Import"), this);
    btn_remove_      = new QPushButton(tr("Remove"), this);

    hbl_button_line_ = new QHBoxLayout;
    hbl_button_line_->addWidget(btn_import_);
    hbl_button_line_->addWidget(btn_remove_);
    hbl_button_line_->addStretch();
    hbl_button_line_->addWidget(btn_refresh_);

    vbl_main_ = new QVBoxLayout;
    vbl_main_->addWidget(tw_plugin_list_);
    vbl_main_->addLayout(hbl_button_line_);

    this->setLayout(vbl_main_);
}

void PluginPanel::init_connections()
{
    connect(btn_refresh_, &QPushButton::clicked, this,
            [this]() { emit sgn_request_refresh_plugin(); });
    connect(btn_import_, &QPushButton::clicked, this,
            [this]() { emit sgn_request_import_plugin(); });
    connect(btn_remove_, &QPushButton::clicked, this, [this]() {
        const int row = tw_plugin_list_->currentRow();
        if (row < 0 || row >= tw_plugin_list_->rowCount()) {
            return;
        }
        emit sgn_request_remove_plugin(descriptors_[row].id);
    });
}

void PluginPanel::refresh()
{
    // clear() 不会重置 rowCount, 需先 clearContents + setRowCount,
    // 否则插件数量变少时残留空行
    tw_plugin_list_->clearContents();
    tw_plugin_list_->setRowCount(static_cast<int>(descriptors_.size()));
    for (int row = 0; row < static_cast<int>(descriptors_.size()); ++row) {
        auto* id =
            new QTableWidgetItem(descriptors_[row].id, std::to_underlying(PluginTableType::id));
        auto* name =
            new QTableWidgetItem(descriptors_[row].name, std::to_underlying(PluginTableType::name));
        auto* version = new QTableWidgetItem(descriptors_[row].version,
                                             std::to_underlying(PluginTableType::version));
        auto* desc    = new QTableWidgetItem(descriptors_[row].description,
                                             std::to_underlying(PluginTableType::description));
        auto* author  = new QTableWidgetItem(descriptors_[row].author,
                                             std::to_underlying(PluginTableType::author));
        auto* categories =
            new QTableWidgetItem(utils::string::unfold_string(descriptors_[row].categories, ';'),
                                 std::to_underlying(PluginTableType::categories));

        tw_plugin_list_->setItem(row, 0, id);
        tw_plugin_list_->setItem(row, 1, name);
        tw_plugin_list_->setItem(row, 2, version);
        tw_plugin_list_->setItem(row, 3, desc);
        tw_plugin_list_->setItem(row, 4, author);
        tw_plugin_list_->setItem(row, 5, categories);
    }
}

void PluginPanel::refresh(const QVector<PluginDescriptor> descriptors)
{
    descriptors_.clear();
    descriptors_ = descriptors;
    descriptors_.shrink_to_fit();

    this->refresh();
}

QListWidgetItem* PluginPanel::get_title_item()
{
    if (!title_item_) {
        title_item_ = new QListWidgetItem(tr("Plugins"));
    }
    return title_item_;
}
