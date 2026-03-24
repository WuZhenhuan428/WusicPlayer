#include "playlist_search_panel.h"
#include "model/search_model/search_model.h"

#include "controller/search_backend/i_search_backend.h"

#include <QTimer>
#include <QHeaderView>

#include "core/search_types.h"

PlaylistSearchPanel::PlaylistSearchPanel(QWidget *parent)
    :QWidget(parent)
{
    setWindowTitle("Search");
    m_le_keyword = new QLineEdit;
    m_cb_mode = new QComboBox;
    m_le_keyword->setAttribute(Qt::WA_InputMethodEnabled, true);
    m_search_result_tree_view = new QTreeView;
    m_hbl_query = new QHBoxLayout;
    m_vbl_main = new QVBoxLayout;

    m_cb_mode->addItem(QStringLiteral("Plain"), QVariant::fromValue(static_cast<int>(SearchQueryMode::Plain)));
    m_cb_mode->addItem(QStringLiteral("Prefix"), QVariant::fromValue(static_cast<int>(SearchQueryMode::Prefix)));
    m_cb_mode->addItem(QStringLiteral("Fuzzy"), QVariant::fromValue(static_cast<int>(SearchQueryMode::Fuzzy)));
    
    m_search_result_tree_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_search_result_tree_view->setRootIsDecorated(false);
    m_search_result_tree_view->setAlternatingRowColors(true);
    
    m_hbl_query->addWidget(m_le_keyword, 1);
    m_hbl_query->addWidget(m_cb_mode);
    m_vbl_main->addLayout(m_hbl_query);
    m_vbl_main->addWidget(m_search_result_tree_view);
    setLayout(m_vbl_main);
    
    m_search_model = new SearchModel(nullptr, this);
    m_search_result_tree_view->setModel(m_search_model);
    
    m_tim_input = new QTimer(this);
    m_tim_input->setSingleShot(true);
    m_tim_input->setInterval(150);

    connect(m_le_keyword, &QLineEdit::textChanged, this, [this](const QString& keyword){
        Q_UNUSED(keyword);
        m_tim_input->start();
    });

    connect(m_tim_input, &QTimer::timeout, this, [this]() {
        if (!m_search_model) {
            return;
        }

        SearchQuery query;
        query.keyword = m_le_keyword->text();
        query.mode = static_cast<SearchQueryMode>(m_cb_mode->currentData().toInt());
        m_search_model->searchRequest(std::move(query));
    });

    connect(m_cb_mode, &QComboBox::currentIndexChanged, this, [this](int) {
        if (!m_tim_input) {
            return;
        }
        m_tim_input->start();
    });
    
    connect(m_search_result_tree_view, &QTreeView::doubleClicked, this, [this](const QModelIndex &index){
        if (!index.isValid() || !m_search_model) {
            return;
        }

        const trackId id = m_search_model->trackIdAt(index.row());
        if (id.isNull()) {
            return;
        }
        emit sgnRequestPlayTrack(id);
    });
}

PlaylistSearchPanel::~PlaylistSearchPanel() {}

void PlaylistSearchPanel::setSearchBackend(ISearchBackend* backend) {
    if (!m_search_model) {
        return;
    }
    m_search_model->setBackend(backend);
}

QTreeView* PlaylistSearchPanel::getView() const {
    return this->m_search_result_tree_view;
}

void PlaylistSearchPanel::applyHeaderStateDeferred(const QByteArray& state) {
    if (state.isEmpty()) return;

    QTimer::singleShot(0, this, [this, state](){
        if (m_search_result_tree_view && m_search_result_tree_view->header()) {
            m_search_result_tree_view->header()->restoreState(state);
        }
    });
}

void PlaylistSearchPanel::emitStateSnapshot() {
    QByteArray geo = saveGeometry();
    QByteArray header;
    if (m_search_result_tree_view && m_search_result_tree_view->header()) {
        header = m_search_result_tree_view->header()->saveState();
    }
    emit sgnStateSnapshot(geo, header);
}

void PlaylistSearchPanel::closeEvent(QCloseEvent* event) {
    emitStateSnapshot();
    QWidget::closeEvent(event);
}

void PlaylistSearchPanel::hideEvent(QHideEvent* event) {
    emitStateSnapshot();
    QWidget::hideEvent(event);
}

void PlaylistSearchPanel::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        this->close();
    } else {
        QWidget::keyPressEvent(event);
    }
}