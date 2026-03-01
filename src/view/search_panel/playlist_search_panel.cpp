#include "playlist_search_panel.h"
#include <QTimer>
#include <QHeaderView>

PlaylistSearchPanel::PlaylistSearchPanel(QWidget *parent)
    :QWidget(parent)
{
    setWindowTitle("Search");
    leKeyword = new QLineEdit;
    leKeyword->setAttribute(Qt::WA_InputMethodEnabled, true);
    treeSearchResult = new QTreeView;
    vbMainLayout = new QVBoxLayout;
    
    treeSearchResult->setSelectionMode(QAbstractItemView::SingleSelection);
    treeSearchResult->setRootIsDecorated(true);
    treeSearchResult->setAlternatingRowColors(true);
    
    vbMainLayout->addWidget(leKeyword);
    vbMainLayout->addWidget(treeSearchResult);
    setLayout(vbMainLayout);
    
    m_searchModel = new PlaylistSearchProxyModel(this);
    treeSearchResult->setModel(m_searchModel);
    
    connect(leKeyword, &QLineEdit::textChanged, this, [this](const QString& keyword){
        QTimer::singleShot(150, [this, keyword](){
            m_searchModel->setKeyword(keyword);
            QAbstractItemModel* model = treeSearchResult->model();
            if (model) {
                for (int i = 0; i < model->rowCount(); ++i) {
                    QModelIndex idx = model->index(i, 0);
                    if (model->hasChildren()) {
                        treeSearchResult->setFirstColumnSpanned(i, QModelIndex(), true);
                        treeSearchResult->expandAll();
                    }
                }
            }
        });
    });
    connect(treeSearchResult, &QTreeView::doubleClicked, this, [this](const QModelIndex &proxy_index){
        if (!proxy_index.isValid() || !m_searchModel) {
            return;
        }
        const QModelIndex source_index = m_searchModel->mapToSource(proxy_index);
        if (!source_index.isValid()) {
            return;
        }
        emit sgnRequestPlayTrack(source_index);
    });
}

PlaylistSearchPanel::~PlaylistSearchPanel() {}

void PlaylistSearchPanel::setSourceModel(QAbstractItemModel* source_model) {
    this->m_searchModel->setSourceModel(source_model);
    this->treeSearchResult->expandAll();
}

QTreeView* PlaylistSearchPanel::getView() const {
    return this->treeSearchResult;
}

void PlaylistSearchPanel::applyHeaderStateDeferred(const QByteArray& state) {
    if (state.isEmpty()) {
        return;
    }

    QTimer::singleShot(0, this, [this, state](){
        if (!treeSearchResult || !treeSearchResult->header()) {
            treeSearchResult->header()->restoreState(state);
        }
    });
}

void PlaylistSearchPanel::closeEvent(QCloseEvent* event) {
    QByteArray geo = saveGeometry();
    QByteArray header;
    if (treeSearchResult && treeSearchResult->header()) {
        header = treeSearchResult->header()->saveState();
    }
    emit sgnAboutToClose(geo, header);
    QWidget::closeEvent(event);
}

void PlaylistSearchPanel::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        this->close();
    } else {
        QWidget::keyPressEvent(event);
    }
}