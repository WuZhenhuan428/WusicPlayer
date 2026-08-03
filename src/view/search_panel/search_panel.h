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

    void set_search_backend(class ISearchBackend* backend);
    QTreeView* get_view() const;
    void apply_header_state_deferred(const QByteArray& state);

    // config S/L, temporary
    void load_from_json(const QJsonObject& json);
    QJsonObject save_to_json();
    QString config_sub_key() const;

signals:
    // 双击结果:优先携带 filepath(播放列表条目/外部条目直接播放)
    void sgnRequestPlayFile(const QString& filepath);
    // 库级曲目身份(库引用条目,经库解析播放)
    void sgnRequestPlayTrack(const TrackId& track_id);
    void sgnStateSnapshot(const QByteArray& geometry, const QByteArray& header);

protected:
    void closeEvent(QCloseEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void show_header_context_menu(const QPoint& pos);
    bool has_other_visible_columns(int column_to_hide) const;

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
