#include "view/library_browser/library_browser_widget.h"

#include "model/library/library_manager.h"
#include "model/playback_queue/playback_queue_service.h"
#include "view/playlist/playlist_widgets.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonObject>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QModelIndexList>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTreeView>
#include <QVBoxLayout>

#include <utility>

namespace
{
// 分类下拉框 "Custom (DSL)" 项的 data 标记(与 LibraryGrouping 枚举不冲突)
constexpr int kCustomGroupingData = 10000;
} // namespace

LibraryBrowserWidget::LibraryBrowserWidget(QWidget* parent) : QWidget(parent)
{
    init_ui();
    init_connections();
}

void LibraryBrowserWidget::init_ui()
{
    cb_grouping_ = new QComboBox;
    cb_grouping_->addItem(tr("Artist"), std::to_underlying(LibraryGrouping::Artist));
    cb_grouping_->addItem(tr("Album"), std::to_underlying(LibraryGrouping::Album));
    cb_grouping_->addItem(tr("Genre"), std::to_underlying(LibraryGrouping::Genre));
    cb_grouping_->addItem(tr("Folder"), std::to_underlying(LibraryGrouping::Folder));
    cb_grouping_->addItem(tr("Year"), std::to_underlying(LibraryGrouping::Year));
    cb_grouping_->addItem(tr("None"), std::to_underlying(LibraryGrouping::None));
    cb_grouping_->addItem(tr("Custom (DSL)..."), kCustomGroupingData);

    btn_settings_ = new QPushButton(tr("Settings"));
    btn_settings_->setToolTip(tr("Custom grouping rules (DSL)"));

    btn_config_ = new QPushButton(tr("Configure"));
    btn_config_->setToolTip(
        tr("Goto Settings->Library (the only entrance of setting library paths)"));

    auto* top = new QHBoxLayout;
    top->setContentsMargins(0, 0, 0, 0);
    top->setSpacing(2);
    top->addWidget(cb_grouping_, 1);
    top->addWidget(btn_settings_);
    top->addWidget(btn_config_);

    le_keyword_ = new QLineEdit;
    le_keyword_->setPlaceholderText(tr("Search in library (FTS5)..."));

    tree_ = new QTreeView;
    tree_->setAlternatingRowColors(true);
    tree_->setUniformRowHeights(true);
    tree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // 拖拽源(拖到播放列表树 / 歌曲表)
    tree_->setDragEnabled(true);
    tree_->setDragDropMode(QAbstractItemView::DragOnly);
    tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_->header()->setStretchLastSection(true);
    tree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    tree_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tree_->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tree_->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    auto* vbl = new QVBoxLayout(this);
    vbl->setContentsMargins(2, 2, 2, 2);
    vbl->setSpacing(2);
    vbl->addLayout(top);
    vbl->addWidget(le_keyword_);
    vbl->addWidget(tree_, 1);

    model_ = new LibraryBrowseModel(nullptr, this);
    tree_->setModel(model_);

    tim_input_ = new QTimer(this);
    tim_input_->setSingleShot(true);
    tim_input_->setInterval(200);
}

void LibraryBrowserWidget::init_connections()
{
    connect(cb_grouping_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &LibraryBrowserWidget::on_grouping_changed);
    connect(le_keyword_, &QLineEdit::textChanged, this, &LibraryBrowserWidget::on_keyword_changed);
    connect(tim_input_, &QTimer::timeout, this, [this]() {
        if (model_ != nullptr) {
            model_->set_keyword(le_keyword_->text());
        }
    });
    connect(btn_config_, &QPushButton::clicked, this,
            &LibraryBrowserWidget::sgn_open_library_settings_requested);
    connect(btn_settings_, &QPushButton::clicked, this, &LibraryBrowserWidget::open_dsl_dialog);
    connect(tree_, &QTreeView::doubleClicked, this, &LibraryBrowserWidget::on_double_clicked);
    connect(tree_, &QTreeView::customContextMenuRequested, this,
            &LibraryBrowserWidget::call_tree_context_menu);
}

void LibraryBrowserWidget::set_playlist_list_provider(PlaylistListProvider provider)
{
    playlist_provider_ = std::move(provider);
}

void LibraryBrowserWidget::set_library_manager(LibraryManager* lib)
{
    lib_ = lib;
    if (model_ != nullptr) {
        model_->set_library(lib);
    }
}

void LibraryBrowserWidget::set_playback_queue_service(PlaybackQueueService* svc)
{
    queue_svc_ = svc;
}

void LibraryBrowserWidget::on_grouping_changed(int index)
{
    if (model_ == nullptr) {
        return;
    }
    const int g = cb_grouping_->itemData(index).toInt();
    if (g == kCustomGroupingData) {
        open_dsl_dialog(); // 选中 "Custom (DSL)" → 弹出编辑对话框
        return;
    }
    // 预设: set_grouping 内部会清除 DSL
    model_->set_grouping(static_cast<LibraryGrouping>(g));
}

void LibraryBrowserWidget::open_dsl_dialog()
{
    if (model_ == nullptr) {
        return;
    }
    WSortTypeSetDialog dlg(this);
    dlg.set_text(model_->dsl_source()); // 预填当前 DSL(若有)
    if (dlg.exec() == QDialog::Accepted) {
        model_->set_dsl_grouping(dlg.get_text()); // 空文本 → 清除 DSL
        if (!model_->dsl_error().isEmpty()) {
            QMessageBox::warning(this, tr("DSL Error"), model_->dsl_error());
        }
    }
    sync_grouping_combo();
}

void LibraryBrowserWidget::sync_grouping_combo()
{
    if (model_ == nullptr || cb_grouping_ == nullptr) {
        return;
    }
    QSignalBlocker block(cb_grouping_);
    if (model_->has_dsl()) {
        const int idx = cb_grouping_->findData(kCustomGroupingData);
        if (idx >= 0) {
            cb_grouping_->setCurrentIndex(idx);
        }
    } else {
        const int idx = cb_grouping_->findData(static_cast<int>(model_->grouping()));
        if (idx >= 0) {
            cb_grouping_->setCurrentIndex(idx);
        }
    }
}

void LibraryBrowserWidget::load_from_json(const QJsonObject& json)
{
    const QJsonObject obj = json.value(this->config_sub_key()).toObject();

    // 分类(先恢复模型分组,再同步下拉框,避免触发多余重建)
    if (model_ != nullptr) {
        const int g = obj.value("grouping").toInt(std::to_underlying(LibraryGrouping::Artist));
        model_->set_grouping(static_cast<LibraryGrouping>(g));
    }

    // DSL 自定义分类(存在则优先于预设)
    if (model_ != nullptr) {
        const QString dsl = obj.value("dsl_grouping").toString();
        if (!dsl.trimmed().isEmpty()) {
            model_->set_dsl_grouping(dsl);
            if (!model_->dsl_error().isEmpty()) {
                model_->clear_dsl_grouping(); // 历史规则失效则回退预设
            }
        }
    }
    sync_grouping_combo();

    // 关键字(直接设置模型;QLineEdit 仅展示)
    const QString keyword = obj.value("keyword").toString();
    le_keyword_->setText(keyword);
    if (model_ != nullptr) {
        model_->set_keyword(keyword);
    }

    // 树表头状态(列宽/顺序/可见性)
    const QByteArray state =
        QByteArray::fromBase64(obj.value("tree_header_state").toString().toUtf8());
    if (!state.isEmpty()) {
        tree_->header()->restoreState(state);
    }
}

QJsonObject LibraryBrowserWidget::save_to_json()
{
    QJsonObject obj;
    obj["grouping"] =
        static_cast<int>(model_ != nullptr ? model_->grouping() : LibraryGrouping::Artist);
    obj["keyword"]           = le_keyword_->text();
    obj["dsl_grouping"]      = (model_ != nullptr) ? model_->dsl_source() : QString();
    obj["tree_header_state"] = QString::fromUtf8(tree_->header()->saveState().toBase64());
    return obj;
}

QString LibraryBrowserWidget::config_sub_key() const
{
    return "library_browser";
}

void LibraryBrowserWidget::on_keyword_changed([[maybe_unused]] const QString& keyword)
{
    tim_input_->start(); // 防抖
}

void LibraryBrowserWidget::on_double_clicked(const QModelIndex& index)
{
    if (model_ == nullptr || !index.isValid()) {
        return;
    }
    if (index.parent().isValid()) {
        // 曲目行 → 入队即播
        const std::optional<TrackId> tid = model_->track_id_at(index);
        if (!tid.has_value()) {
            return;
        }
        if (queue_svc_ != nullptr) {
            queue_svc_->play_library_track(*tid);
        } else {
            emit sgn_play_requested(*tid);
        }
        return;
    }
    // 分组节点 → 展开/折叠
    const bool expanded = tree_->isExpanded(index);
    tree_->setExpanded(index, !expanded);
}

void LibraryBrowserWidget::call_tree_context_menu(const QPoint& pos)
{
    if (model_ == nullptr) {
        return;
    }
    const QModelIndex index = tree_->indexAt(pos);
    if (!index.isValid()) {
        return; // 不提供背景右键
    }
    // 收集右键行(或多选集合)对应的库曲目;组节点展开为该组全部
    QModelIndexList rows;
    if (tree_->selectionModel()) {
        rows = tree_->selectionModel()->selectedRows(0);
    }
    if (!rows.contains(index)) {
        rows = {index};
    }
    const QVector<TrackId> tids = model_->collect_track_ids(rows);
    if (tids.isEmpty()) {
        return;
    }

    QMenu menu(this);
    QAction* act_play    = menu.addAction(tr("&Play"));
    QAction* act_add_sub = menu.addAction(tr("Add to Playlist...")); // 占位,下转为子菜单
    menu.addSeparator();
    QAction* act_refresh = menu.addAction(tr("&Refresh Library"));

    connect(act_play, &QAction::triggered, this, [this, tids]() {
        if (queue_svc_ != nullptr) {
            for (const TrackId& tid : tids) {
                queue_svc_->play_library_track(tid);
            }
        } else if (tids.size() == 1) {
            emit sgn_play_requested(tids.first());
        }
    });
    connect(act_refresh, &QAction::triggered, this,
            &LibraryBrowserWidget::sdn_refresh_library_requested);

    // Add to Playlist 子菜单(替换占位项)
    if (playlist_provider_) {
        const auto lists = playlist_provider_();
        if (!lists.isEmpty()) {
            menu.removeAction(act_add_sub);
            auto* sub = menu.addMenu(tr("Add to Playlist..."));
            for (const auto& [pid, name] : lists) {
                QAction* act = sub->addAction(name);
                connect(act, &QAction::triggered, this,
                        [this, pid, tids]() { emit sgn_add_tracks_to_playlist(pid, tids); });
            }
        } else {
            act_add_sub->setEnabled(false);
        }
    } else {
        act_add_sub->setEnabled(false);
    }

    menu.exec(tree_->viewport()->mapToGlobal(pos));
}
