#include "view/search_panel/search_panel.h"

#include "controller/search_backend/i_search_backend.h"
#include "core/config_manager/config_manager.h"
#include "core/search_types.h"
#include "model/search_model/search_model.h"

#include <QAction>
#include <QHeaderView>
#include <QJsonObject>
#include <QMenu>
#include <QTimer>

SearchPanel::SearchPanel(ConfigManager* cfg_mgr, QWidget* parent) :
    QWidget(parent), cfg_mgr_(cfg_mgr)
{
    setWindowTitle(tr("Search"));
    le_keyword_ = new QLineEdit;
    cb_mode_    = new QComboBox;
    le_keyword_->setAttribute(Qt::WA_InputMethodEnabled, true);
    search_result_tree_view_ = new QTreeView;
    hbl_query_               = new QHBoxLayout;
    vbl_main_                = new QVBoxLayout;

    cb_mode_->addItem(QStringLiteral("Plain"),
                      QVariant::fromValue(static_cast<int>(SearchQueryMode::Plain)));
    cb_mode_->addItem(QStringLiteral("Prefix"),
                      QVariant::fromValue(static_cast<int>(SearchQueryMode::Prefix)));
    cb_mode_->addItem(QStringLiteral("Fuzzy"),
                      QVariant::fromValue(static_cast<int>(SearchQueryMode::Fuzzy)));

    search_result_tree_view_->setSelectionMode(QAbstractItemView::SingleSelection);
    search_result_tree_view_->setRootIsDecorated(false);
    search_result_tree_view_->setAlternatingRowColors(true);
    search_result_tree_view_->setMinimumWidth(120);

    hbl_query_->addWidget(le_keyword_, 1);
    hbl_query_->addWidget(cb_mode_);
    vbl_main_->addLayout(hbl_query_);
    vbl_main_->addWidget(search_result_tree_view_);
    setLayout(vbl_main_);

    search_model_ = new SearchModel(nullptr, this);
    search_result_tree_view_->setModel(search_model_);
    if (search_result_tree_view_->header()) {
        search_result_tree_view_->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    }

    tim_input_ = new QTimer(this);
    tim_input_->setSingleShot(true);
    tim_input_->setInterval(150);

    // load configurations
    QJsonObject config_obj = ConfigManager::read_sub_config(SearchPanel::config_sub_key());
    this->load_from_json(config_obj);

    connect(le_keyword_, &QLineEdit::textChanged, this,
            [this]([[maybe_unused]] const QString& keyword) { tim_input_->start(); });

    connect(tim_input_, &QTimer::timeout, this, [this]() {
        if (!search_model_) {
            return;
        }

        SearchQuery query;
        query.keyword = le_keyword_->text();
        query.mode    = static_cast<SearchQueryMode>(cb_mode_->currentData().toInt());
        search_model_->search_request(std::move(query));
    });

    connect(cb_mode_, &QComboBox::currentIndexChanged, this, [this](int) {
        if (!tim_input_) {
            return;
        }
        tim_input_->start();
    });

    connect(search_result_tree_view_, &QTreeView::doubleClicked, this,
            [this](const QModelIndex& index) {
                if (!index.isValid() || !search_model_) {
                    return;
                }

                const SearchHint hint = search_model_->hint_at(index.row());
                if (!hint.filepath.isEmpty()) {
                    // 播放列表条目 / 外部条目:直接按路径播放(定位回当前列表)
                    emit sgn_request_play_file(hint.filepath);
                    return;
                }
                const TrackId id = hint.track_id;
                if (!id.is_null()) {
                    emit sgn_request_play_track(id);
                }
            });

    if (search_result_tree_view_->header()) {
        connect(search_result_tree_view_->header(), &QHeaderView::customContextMenuRequested, this,
                &SearchPanel::show_header_context_menu);
    }
}
void SearchPanel::set_search_backend(ISearchBackend* backend)
{
    if (!search_model_) {
        return;
    }
    search_model_->set_backend(backend);
}

QTreeView* SearchPanel::get_view() const
{
    return this->search_result_tree_view_;
}

void SearchPanel::apply_header_state_deferred(const QByteArray& state)
{
    if (state.isEmpty()) {
        return;
    }

    QTimer::singleShot(0, this, [this, state]() {
        if (search_result_tree_view_ && search_result_tree_view_->header()) {
            search_result_tree_view_->header()->restoreState(state);
        }
    });
}

void SearchPanel::closeEvent(QCloseEvent* event)
{
    cfg_mgr_->write_sub_config(SearchPanel::config_sub_key(), this->save_to_json());
    QWidget::closeEvent(event);
}

void SearchPanel::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
}

void SearchPanel::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        this->close();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void SearchPanel::show_header_context_menu(const QPoint& pos)
{
    if (!search_result_tree_view_ || !search_result_tree_view_->header() || !search_model_) {
        return;
    }

    QHeaderView* header = search_result_tree_view_->header();
    QMenu menu(this);
    const int count = search_model_->columnCount();

    for (int logical_index = 0; logical_index < count; ++logical_index) {

        QString label =
            search_model_->headerData(logical_index, Qt::Horizontal, Qt::DisplayRole).toString();
        if (label.isEmpty()) {
            label = tr("Column %1").arg(logical_index + 1);
        }

        QAction* action = menu.addAction(label);
        action->setCheckable(true);
        const bool visible = !header->isSectionHidden(logical_index);
        action->setChecked(visible);

        connect(action, &QAction::toggled, this, [this, logical_index](bool checked) {
            if (!search_result_tree_view_ || !search_result_tree_view_->header()) {
                return;
            }

            QHeaderView* local_header = search_result_tree_view_->header();
            if (!checked && !has_other_visible_columns(logical_index)) {
                return;
            }

            local_header->setSectionHidden(logical_index, !checked);
        });
    }

    menu.exec(header->mapToGlobal(pos));
}

bool SearchPanel::has_other_visible_columns(int column_to_hide) const
{
    if (!search_result_tree_view_ || !search_result_tree_view_->header() || !search_model_) {
        return false;
    }

    QHeaderView* header = search_result_tree_view_->header();
    const int count     = search_model_->columnCount();
    for (int i = 0; i < count; ++i) {
        if (i == column_to_hide) {
            continue;
        }
        if (!header->isSectionHidden(i)) {
            return true;
        }
    }
    return false;
}

void SearchPanel::load_from_json(const QJsonObject& json)
{
    const QByteArray geometry = QByteArray::fromBase64(json.value("geometry").toString().toUtf8());
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    if (search_result_tree_view_ && search_result_tree_view_->header()) {
        const QByteArray header_state =
            QByteArray::fromBase64(json.value("header_state").toString().toUtf8());
        if (!header_state.isEmpty()) {
            search_result_tree_view_->header()->restoreState(header_state);
        }
    }
}

QJsonObject SearchPanel::save_to_json()
{
    QJsonObject obj;
    obj["geometry"] = QString::fromUtf8(this->saveGeometry().toBase64());
    obj["header_state"] =
        QString::fromUtf8(this->search_result_tree_view_->header()->saveState().toBase64());
    return obj;
}

QString SearchPanel::config_sub_key()
{
    return "search_panel";
}
