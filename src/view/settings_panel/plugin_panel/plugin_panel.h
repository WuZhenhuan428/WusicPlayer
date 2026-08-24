#pragma once

#include "core/plugin_manager/plugin_types.h"

#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

class QListWidgetItem;

/// 只用于简单展示, 省略复杂操作和独立的视图模型
class PluginPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PluginPanel(const QVector<PluginDescriptor> descriptors, QWidget* parent = nullptr);

    void refresh();
    void refresh(const QVector<PluginDescriptor> descriptors);

    QListWidgetItem* get_title_item();

signals:
    void sgn_request_refresh_plugin();
    void sgn_request_import_plugin();
    void sgn_request_remove_plugin(QString plugin_id);

private:
    void init_ui();
    void init_connections();

    enum class PluginTableType : int
    {
        id = QTableWidgetItem::UserType + 1,
        name,
        version,
        description,
        author,
        categories,
    };

    QVBoxLayout* vbl_main_        = nullptr;
    QTableWidget* tw_plugin_list_ = nullptr;
    QPushButton* btn_refresh_     = nullptr;
    QPushButton* btn_import_      = nullptr;
    QPushButton* btn_remove_      = nullptr;
    QHBoxLayout* hbl_button_line_ = nullptr;
    QListWidgetItem* title_item_  = nullptr;

    /// 缓存, 用于定位
    QVector<PluginDescriptor> descriptors_;
};
