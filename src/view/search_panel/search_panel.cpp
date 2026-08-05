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
    QWidget(parent), m_cfg_mgr(cfg_mgr)
{
    setWindowTitle("Search");
    m_le_keyword = new QLineEdit;
    m_cb_mode    = new QComboBox;
    m_le_keyword->setAttribute(Qt::WA_InputMethodEnabled, true);
    m_search_result_tree_view = new QTreeView;
    m_hbl_query               = new QHBoxLayout;
    m_vbl_main                = new QVBoxLayout;

    m_cb_mode->addItem(QStringLiteral("Plain"),
                       QVariant::fromValue(static_cast<int>(SearchQueryMode::Plain)));
    m_cb_mode->addItem(QStringLiteral("Prefix"),
                       QVariant::fromValue(static_cast<int>(SearchQueryMode::Prefix)));
    m_cb_mode->addItem(QStringLiteral("Fuzzy"),
                       QVariant::fromValue(static_cast<int>(SearchQueryMode::Fuzzy)));

    m_search_result_tree_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_search_result_tree_view->setRootIsDecorated(false);
    m_search_result_tree_view->setAlternatingRowColors(true);
    m_search_result_tree_view->setMinimumWidth(120);

    m_hbl_query->addWidget(m_le_keyword, 1);
    m_hbl_query->addWidget(m_cb_mode);
    m_vbl_main->addLayout(m_hbl_query);
    m_vbl_main->addWidget(m_search_result_tree_view);
    setLayout(m_vbl_main);

    m_search_model = new SearchModel(nullptr, this);
    m_search_result_tree_view->setModel(m_search_model);
    if (m_search_result_tree_view->header()) {
        m_search_result_tree_view->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    }

    m_tim_input = new QTimer(this);
    m_tim_input->setSingleShot(true);
    m_tim_input->setInterval(150);

    // load configurations
    QJsonObject config_obj = m_cfg_mgr->read_sub_config(this->config_sub_key());
    this->load_from_json(config_obj);

    connect(m_le_keyword, &QLineEdit::textChanged, this,
            [this]([[maybe_unused]] const QString& keyword) { m_tim_input->start(); });

    connect(m_tim_input, &QTimer::timeout, this, [this]() {
        if (!m_search_model) {
            return;
        }

        SearchQuery query;
        query.keyword = m_le_keyword->text();
        query.mode    = static_cast<SearchQueryMode>(m_cb_mode->currentData().toInt());
        m_search_model->search_request(std::move(query));
    });

    connect(m_cb_mode, &QComboBox::currentIndexChanged, this, [this](int) {
        if (!m_tim_input) {
            return;
        }
        m_tim_input->start();
    });

    connect(m_search_result_tree_view, &QTreeView::doubleClicked, this,
            [this](const QModelIndex& index) {
                if (!index.isValid() || !m_search_model) {
                    return;
                }

                const SearchHint hint = m_search_model->hint_at(index.row());
                if (!hint.filepath.isEmpty()) {
                    // 播放列表条目 / 外部条目:直接按路径播放(定位回当前列表)
                    emit sgnRequestPlayFile(hint.filepath);
                    return;
                }
                const TrackId id = hint.track_id;
                if (!id.isNull()) {
                    emit sgnRequestPlayTrack(id);
                }
            });

    if (m_search_result_tree_view->header()) {
        connect(m_search_result_tree_view->header(), &QHeaderView::customContextMenuRequested, this,
                &SearchPanel::show_header_context_menu);
    }
}

SearchPanel::~SearchPanel() {}

void SearchPanel::set_search_backend(ISearchBackend* backend)
{
    if (!m_search_model) {
        return;
    }
    m_search_model->set_backend(backend);
}

QTreeView* SearchPanel::get_view() const
{
    return this->m_search_result_tree_view;
}

void SearchPanel::apply_header_state_deferred(const QByteArray& state)
{
    if (state.isEmpty())
        return;

    QTimer::singleShot(0, this, [this, state]() {
        if (m_search_result_tree_view && m_search_result_tree_view->header()) {
            m_search_result_tree_view->header()->restoreState(state);
        }
    });
}

void SearchPanel::closeEvent(QCloseEvent* event)
{
    m_cfg_mgr->write_sub_config(this->config_sub_key(), this->save_to_json());
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
    if (!m_search_result_tree_view || !m_search_result_tree_view->header() || !m_search_model) {
        return;
    }

    QHeaderView* header = m_search_result_tree_view->header();
    QMenu menu(this);
    const int count = m_search_model->columnCount();

    for (int logical_index = 0; logical_index < count; ++logical_index) {

        QString label =
            m_search_model->headerData(logical_index, Qt::Horizontal, Qt::DisplayRole).toString();
        if (label.isEmpty()) {
            label = tr("Column %1").arg(logical_index + 1);
        }

        QAction* action = menu.addAction(label);
        action->setCheckable(true);
        const bool visible = !header->isSectionHidden(logical_index);
        action->setChecked(visible);

        connect(action, &QAction::toggled, this, [this, logical_index](bool checked) {
            if (!m_search_result_tree_view || !m_search_result_tree_view->header()) {
                return;
            }

            QHeaderView* local_header = m_search_result_tree_view->header();
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
    if (!m_search_result_tree_view || !m_search_result_tree_view->header() || !m_search_model) {
        return false;
    }

    QHeaderView* header = m_search_result_tree_view->header();
    const int count     = m_search_model->columnCount();
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
    if (m_search_result_tree_view && m_search_result_tree_view->header()) {
        const QByteArray header_state =
            QByteArray::fromBase64(json.value("header_state").toString().toUtf8());
        if (!header_state.isEmpty()) {
            m_search_result_tree_view->header()->restoreState(header_state);
        }
    }
}

QJsonObject SearchPanel::save_to_json()
{
    QJsonObject obj;
    obj["geometry"] = QString::fromUtf8(this->saveGeometry().toBase64());
    obj["header_state"] =
        QString::fromUtf8(this->m_search_result_tree_view->header()->saveState().toBase64());
    return obj;
}

QString SearchPanel::config_sub_key() const
{
    return "search_panel";
}
