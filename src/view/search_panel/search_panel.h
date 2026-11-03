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

    void set_search_backend(class ISearchBackend* backend);
    [[nodiscard]] QTreeView* get_view() const;
    void apply_header_state_deferred(const QByteArray& state);

    // config S/L, temporary
    void load_from_json(const QJsonObject& json);
    QJsonObject save_to_json();
    [[nodiscard]] static QString config_sub_key();

signals:
    // 双击结果:优先携带 filepath(播放列表条目/外部条目直接播放)
    void sgn_request_play_file(const QString& filepath);
    // 库级曲目身份(库引用条目,经库解析播放)
    void sgn_request_play_track(const TrackId& track_id);
    void sgn_state_snapshot(const QByteArray& geometry, const QByteArray& header);

protected:
    void closeEvent(QCloseEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void show_header_context_menu(const QPoint& pos);
    [[nodiscard]] bool has_other_visible_columns(int column_to_hide) const;

private:
    QLineEdit* le_keyword_              = nullptr;
    QComboBox* cb_mode_                 = nullptr;
    QTreeView* search_result_tree_view_ = nullptr;
    QHBoxLayout* hbl_query_             = nullptr;
    QVBoxLayout* vbl_main_              = nullptr;

    class SearchModel* search_model_    = nullptr;

    QTimer* tim_input_                  = nullptr;

    ConfigManager* cfg_mgr_;
};
