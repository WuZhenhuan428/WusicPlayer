#include "playlist_search_panel.h"
#include <QTimer>
#include <QHeaderView>
#include <qabstractitemmodel.h>
#include <qnamespace.h>

PlaylistSearchPanel::PlaylistSearchPanel(QWidget *parent)
    :QWidget(parent)
{
    setWindowTitle("Search");
    m_le_keyword = new QLineEdit;
    m_le_keyword->setAttribute(Qt::WA_InputMethodEnabled, true);
    m_search_result_tree_view = new QTreeView;
    m_vbl_main = new QVBoxLayout;
    
    m_search_result_tree_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_search_result_tree_view->setRootIsDecorated(true);
    m_search_result_tree_view->setAlternatingRowColors(true);
    
    m_vbl_main->addWidget(m_le_keyword);
    m_vbl_main->addWidget(m_search_result_tree_view);
    setLayout(m_vbl_main);
    
    m_search_model = new PlaylistSearchProxyModel(this);
    m_search_result_tree_view->setModel(m_search_model);
    
    connect(m_le_keyword, &QLineEdit::textChanged, this, [this](const QString& keyword){
        if (!m_tim_input) {
            m_tim_input = new QTimer;
            m_tim_input->setSingleShot(true);
        }
        m_tim_input->setInterval(150);
        m_keywords = keyword;

        connect(m_tim_input, &QTimer::timeout, this, [this](){
            /// move this lambda to function: void handlerTimTimerout();
            m_search_model->setKeyword(m_keywords);
            QAbstractItemModel* search_result_model = m_search_result_tree_view->model();
            if (search_result_model) {
                for (int i = 0; i < search_result_model->rowCount(); ++i) {
                    QModelIndex idx = search_result_model->index(i, 0);
                    if (search_result_model->hasChildren(idx)) {
                        m_search_result_tree_view->setFirstColumnSpanned(i, QModelIndex(), true);
                        m_search_result_tree_view->setExpanded(idx, true);
                    }
                }
            }
        }, Qt::SingleShotConnection);

        m_tim_input->start();
    });
    
    connect(m_search_result_tree_view, &QTreeView::doubleClicked, this, [this](const QModelIndex &proxy_index){
        if (!proxy_index.isValid() || !m_search_model) {
            return;
        }
        const QModelIndex source_index = m_search_model->mapToSource(proxy_index);
        if (!source_index.isValid()) {
            return;
        }
        emit sgnRequestPlayTrack(source_index);
    });
}

PlaylistSearchPanel::~PlaylistSearchPanel() {}

void PlaylistSearchPanel::setSourceModel(QAbstractItemModel* source_model) {
    this->m_search_model->setSourceModel(source_model);
    this->m_search_result_tree_view->expandAll();
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