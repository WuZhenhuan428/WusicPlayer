#include "view/library_browser/library_browser_widget.h"

#include "model/library/library_manager.h"
#include "model/playback_queue/playback_queue_service.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonObject>
#include <QLineEdit>
#include <QMenu>
#include <QModelIndexList>
#include <QPushButton>
#include <QTreeView>
#include <QVBoxLayout>

LibraryBrowserWidget::LibraryBrowserWidget(QWidget* parent) : QWidget(parent)
{
    init_ui();
    init_connections();
}

void LibraryBrowserWidget::init_ui()
{
    m_cb_grouping = new QComboBox;
    m_cb_grouping->addItem(tr("Artist"), int(LibraryGrouping::artist));
    m_cb_grouping->addItem(tr("Album"), int(LibraryGrouping::album));
    m_cb_grouping->addItem(tr("Genre"), int(LibraryGrouping::genre));
    m_cb_grouping->addItem(tr("Folder"), int(LibraryGrouping::folder));
    m_cb_grouping->addItem(tr("Year"), int(LibraryGrouping::year));
    m_cb_grouping->addItem(tr("None"), int(LibraryGrouping::none));

    m_btn_settings = new QPushButton(tr("Settings"));
    m_btn_settings->setEnabled(false); // DSL 自定义规则后续统一改造
    m_btn_settings->setToolTip(tr("Sort Settings (DSL customize rules, subsequent version)"));

    m_btn_config = new QPushButton(tr("Configure"));
    m_btn_config->setToolTip(tr("Goto Settings->Library (the only entrance of setting library paths)"));

    auto* top = new QHBoxLayout;
    top->setContentsMargins(0, 0, 0, 0);
    top->setSpacing(2);
    top->addWidget(m_cb_grouping, 1);
    top->addWidget(m_btn_settings);
    top->addWidget(m_btn_config);

    m_le_keyword = new QLineEdit;
    m_le_keyword->setPlaceholderText(tr("Search in library (FTS5)..."));

    m_tree = new QTreeView;
    m_tree->setAlternatingRowColors(true);
    m_tree->setUniformRowHeights(true);
    m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    // 拖拽源(拖到播放列表树 / 歌曲表)
    m_tree->setDragEnabled(true);
    m_tree->setDragDropMode(QAbstractItemView::DragOnly);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tree->header()->setStretchLastSection(true);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    auto* vbl = new QVBoxLayout(this);
    vbl->setContentsMargins(2, 2, 2, 2);
    vbl->setSpacing(2);
    vbl->addLayout(top);
    vbl->addWidget(m_le_keyword);
    vbl->addWidget(m_tree, 1);

    m_model = new LibraryBrowseModel(nullptr, this);
    m_tree->setModel(m_model);

    m_tim_input = new QTimer(this);
    m_tim_input->setSingleShot(true);
    m_tim_input->setInterval(200);
}

void LibraryBrowserWidget::init_connections()
{
    connect(m_cb_grouping, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &LibraryBrowserWidget::on_grouping_changed);
    connect(m_le_keyword, &QLineEdit::textChanged, this, &LibraryBrowserWidget::on_keyword_changed);
    connect(m_tim_input, &QTimer::timeout, this, [this]() {
        if (m_model != nullptr) {
            m_model->set_keyword(m_le_keyword->text());
        }
    });
    connect(m_btn_config, &QPushButton::clicked, this,
            &LibraryBrowserWidget::sgnOpenLibrarySettingsRequested);
    connect(m_tree, &QTreeView::doubleClicked, this, &LibraryBrowserWidget::on_double_clicked);
    connect(m_tree, &QTreeView::customContextMenuRequested, this,
            &LibraryBrowserWidget::call_tree_context_menu);
}

void LibraryBrowserWidget::set_playlist_list_provider(PlaylistListProvider provider)
{
    m_playlist_provider = std::move(provider);
}

void LibraryBrowserWidget::set_library_manager(LibraryManager* lib)
{
    m_lib = lib;
    if (m_model != nullptr) {
        m_model->set_library(lib);
    }
}

void LibraryBrowserWidget::set_playback_queue_service(PlaybackQueueService* svc)
{
    m_queue_svc = svc;
}

void LibraryBrowserWidget::on_grouping_changed(int index)
{
    if (m_model == nullptr) {
        return;
    }
    const int g = m_cb_grouping->itemData(index).toInt();
    m_model->set_grouping(static_cast<LibraryGrouping>(g));
}

void LibraryBrowserWidget::load_from_json(const QJsonObject& json)
{
    const QJsonObject obj = json.value(this->config_sub_key()).toObject();

    // 分类(先恢复模型分组,再同步下拉框,避免触发多余重建)
    if (m_model != nullptr) {
        const int g = obj.value("grouping").toInt(int(LibraryGrouping::artist));
        m_model->set_grouping(static_cast<LibraryGrouping>(g));
        const int idx = m_cb_grouping->findData(g);
        if (idx >= 0) {
            m_cb_grouping->setCurrentIndex(idx);
        }
    }

    // 关键字(直接设置模型;QLineEdit 仅展示)
    const QString keyword = obj.value("keyword").toString();
    m_le_keyword->setText(keyword);
    if (m_model != nullptr) {
        m_model->set_keyword(keyword);
    }

    // 树表头状态(列宽/顺序/可见性)
    const QByteArray state =
        QByteArray::fromBase64(obj.value("tree_header_state").toString().toUtf8());
    if (!state.isEmpty()) {
        m_tree->header()->restoreState(state);
    }
}

QJsonObject LibraryBrowserWidget::save_to_json()
{
    QJsonObject obj;
    obj["grouping"] = int(m_model != nullptr ? m_model->grouping() : LibraryGrouping::artist);
    obj["keyword"]  = m_le_keyword->text();
    obj["tree_header_state"] = QString::fromUtf8(m_tree->header()->saveState().toBase64());
    return obj;
}

QString LibraryBrowserWidget::config_sub_key() const
{
    return "library_browser";
}

void LibraryBrowserWidget::on_keyword_changed([[maybe_unused]] const QString& keyword)
{
    m_tim_input->start(); // 防抖
}

void LibraryBrowserWidget::on_double_clicked(const QModelIndex& index)
{
    if (m_model == nullptr || !index.isValid()) {
        return;
    }
    if (index.parent().isValid()) {
        // 曲目行 → 入队即播
        const std::optional<TrackId> tid = m_model->track_id_at(index);
        if (!tid.has_value()) {
            return;
        }
        if (m_queue_svc != nullptr) {
            m_queue_svc->play_library_track(*tid);
        } else {
            emit sgnPlayRequested(*tid);
        }
        return;
    }
    // 分组节点 → 展开/折叠
    const bool expanded = m_tree->isExpanded(index);
    m_tree->setExpanded(index, !expanded);
}

void LibraryBrowserWidget::call_tree_context_menu(const QPoint& pos)
{
    if (m_model == nullptr) {
        return;
    }
    const QModelIndex index = m_tree->indexAt(pos);
    if (!index.isValid()) {
        return; // 不提供背景右键
    }
    // 收集右键行(或多选集合)对应的库曲目;组节点展开为该组全部
    QModelIndexList rows;
    if (m_tree->selectionModel()) {
        rows = m_tree->selectionModel()->selectedRows(0);
    }
    if (!rows.contains(index)) {
        rows = {index};
    }
    const QVector<TrackId> tids = m_model->collect_track_ids(rows);
    if (tids.isEmpty()) {
        return;
    }

    QMenu menu(this);
    QAction* actPlay   = menu.addAction(tr("&Play"));
    QAction* actAddSub = menu.addAction(tr("Add to Playlist...")); // 占位,下转为子菜单
    menu.addSeparator();
    QAction* actRefresh = menu.addAction(tr("&Refresh Library"));

    connect(actPlay, &QAction::triggered, this, [this, tids]() {
        if (m_queue_svc != nullptr) {
            for (const TrackId& tid : tids) {
                m_queue_svc->play_library_track(tid);
            }
        } else if (tids.size() == 1) {
            emit sgnPlayRequested(tids.first());
        }
    });
    connect(actRefresh, &QAction::triggered, this,
            &LibraryBrowserWidget::sgnRefreshLibraryRequested);

    // Add to Playlist 子菜单(替换占位项)
    if (m_playlist_provider) {
        const auto lists = m_playlist_provider();
        if (!lists.isEmpty()) {
            menu.removeAction(actAddSub);
            auto* sub = menu.addMenu(tr("Add to Playlist..."));
            for (const auto& [pid, name] : lists) {
                QAction* act = sub->addAction(name);
                connect(act, &QAction::triggered, this,
                        [this, pid, tids]() { emit sgnAddTracksToPlaylist(pid, tids); });
            }
        } else {
            actAddSub->setEnabled(false);
        }
    } else {
        actAddSub->setEnabled(false);
    }

    menu.exec(m_tree->viewport()->mapToGlobal(pos));
}
