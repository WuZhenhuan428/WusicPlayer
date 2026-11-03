#pragma once

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidgetItem>
#include <QPushButton>
#include <QRadioButton>
#include <QTableView>
#include <QVBoxLayout>
#include <QWidget>

class ThemeService;

/// 主题设置页面——展示所有可用主题，支持按来源过滤和一键应用。
class ThemeSettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit ThemeSettingsPage(ThemeService* service, QWidget* parent = nullptr);

    QListWidgetItem* get_title_item();

private slots:
    void on_source_filter_changed(int id);
    void on_apply_clicked();
    void on_rescan_clicked();
    void refresh_current_label();

private:
    void init_ui();
    void init_connections();

    ThemeService* service_       = nullptr;

    // 来源过滤
    QRadioButton* rb_all_        = nullptr;
    QRadioButton* rb_system_     = nullptr;
    QRadioButton* rb_builtin_    = nullptr;
    QRadioButton* rb_external_   = nullptr;
    QButtonGroup* source_group_  = nullptr;

    // 图标模式
    QRadioButton* rb_icon_auto_  = nullptr;
    QRadioButton* rb_icon_light_ = nullptr;
    QRadioButton* rb_icon_dark_  = nullptr;
    QButtonGroup* icon_group_    = nullptr;

    // 表格 + 标签
    QTableView* table_view_      = nullptr;
    QLabel* lb_current_          = nullptr;
    QLabel* lb_hint_             = nullptr;

    // 操作按钮
    QPushButton* btn_apply_      = nullptr;
    QPushButton* btn_rescan_     = nullptr;

    // 布局
    QHBoxLayout* hbl_filter_     = nullptr;
    QHBoxLayout* hbl_actions_    = nullptr;
    QVBoxLayout* vbl_main_       = nullptr;

    QListWidgetItem* title_item_ = nullptr;
};
