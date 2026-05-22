#pragma once

#include <QWidget>
#include <QRadioButton>
#include <QButtonGroup>
#include <QTableView>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidgetItem>

class ThemeService;

/// 主题设置页面——展示所有可用主题，支持按来源过滤和一键应用。
class ThemeSettingsPage : public QWidget {
    Q_OBJECT
public:
    explicit ThemeSettingsPage(ThemeService* service, QWidget* parent = nullptr);

    QListWidgetItem* getTitleItem();

private slots:
    void onSourceFilterChanged(int id);
    void onApplyClicked();
    void onRescanClicked();
    void refreshCurrentLabel();

private:
    void initUI();
    void initConnections();

    ThemeService* m_service;

    // 来源过滤 RadioButton
    QRadioButton* m_rb_all;
    QRadioButton* m_rb_system;
    QRadioButton* m_rb_builtin;
    QRadioButton* m_rb_external;
    QButtonGroup* m_source_group;

    // 图标模式 RadioButton
    QRadioButton* m_rb_icon_auto;
    QRadioButton* m_rb_icon_light;
    QRadioButton* m_rb_icon_dark;
    QButtonGroup* m_icon_group;

    // 表格 + 标签
    QTableView* m_table_view;
    QLabel*      m_lb_current;
    QLabel*      m_lb_hint;

    // 操作按钮
    QPushButton* m_btn_apply;
    QPushButton* m_btn_rescan;

    // 布局
    QHBoxLayout* m_hbl_filter;
    QHBoxLayout* m_hbl_actions;
    QVBoxLayout* m_vbl_main;

    QListWidgetItem* m_title_item = nullptr;
};
