#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QTreeView>
#include <QVBoxLayout>
#include <QAbstractItemModel>

#include <QCloseEvent>
#include <QHideEvent>
#include <QKeyEvent>

#include "model/playlist/playlist_search_proxy_model.h"

class PlaylistSearchPanel : public QWidget
{
    Q_OBJECT
public:
    explicit PlaylistSearchPanel(QWidget *parent = nullptr);
    ~PlaylistSearchPanel();

    void setSourceModel(QAbstractItemModel* source_model);
    QTreeView* getView() const;
    void applyHeaderStateDeferred(const QByteArray& state);
    void emitStateSnapshot();

signals:
    void sgnRequestPlayTrack(const QModelIndex &source_index);
    void sgnStateSnapshot(const QByteArray& geometry, const QByteArray& header);

protected:
    void closeEvent(QCloseEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QLineEdit* leKeyword;
    QTreeView* treeSearchResult;
    QVBoxLayout* vbMainLayout;

    PlaylistSearchProxyModel* m_searchModel;
};