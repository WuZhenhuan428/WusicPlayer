#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QTreeView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QAbstractItemModel>

#include <QCloseEvent>
#include <QHideEvent>
#include <QKeyEvent>
#include <QTimer>
#include <QPoint>

#include "core/types.h"

class PlaylistSearchPanel : public QWidget
{
    Q_OBJECT
public:
    explicit PlaylistSearchPanel(QWidget *parent = nullptr);
    ~PlaylistSearchPanel();

    void setSearchBackend(class ISearchBackend* backend);
    QTreeView* getView() const;
    void applyHeaderStateDeferred(const QByteArray& state);
    void emitStateSnapshot();

signals:
    void sgnRequestPlayTrack(const trackId& track_id);
    void sgnStateSnapshot(const QByteArray& geometry, const QByteArray& header);

protected:
    void closeEvent(QCloseEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void showHeaderContextMenu(const QPoint& pos);
    bool hasOtherVisibleColumns(int column_to_hide) const;

private:
    QLineEdit* m_le_keyword;
    QComboBox* m_cb_mode;
    QTreeView* m_search_result_tree_view;
    QHBoxLayout* m_hbl_query;
    QVBoxLayout* m_vbl_main;

    class SearchModel* m_search_model;

    QTimer* m_tim_input = nullptr;
};