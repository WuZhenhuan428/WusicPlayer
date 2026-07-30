#pragma once

#include "core/types.h"

#include <QAbstractItemModel>
#include <QCloseEvent>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPoint>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>
class ConfigManager;

class QJsonObject;

class SearchPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SearchPanel(ConfigManager* cfg_mgr, QWidget* parent = nullptr);
    ~SearchPanel();

    void setSearchBackend(class ISearchBackend* backend);
    QTreeView* getView() const;
    void applyHeaderStateDeferred(const QByteArray& state);

    // config S/L, temporary
    void loadFromJson(const QJsonObject& json);
    QJsonObject saveToJson();
    QString configSubKey() const;

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

    ConfigManager* m_cfg_mgr;
};
