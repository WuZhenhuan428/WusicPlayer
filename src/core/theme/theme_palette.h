#pragma once

#include <QColor>
#include <QPalette>
#include <QString>

/// 主题调色板——整个主题系统的唯一数据源。
/// QPalette 角色直接映射到 Qt 标准色角色；语义颜色和尺寸供 WusicProxyStyle 绘制时使用。
struct ThemePalette
{
    // ---- 元信息 ----
    QString name;   // 主题名称，如 "Wusic Dark"
    QString author; // 作者
    bool is_dark = false;

    // ---- QPalette 角色（将直接设入 QPalette） ----
    QColor window; // 主窗口背景
    QColor window_text;
    QColor base;           // 输入框 / 列表背景
    QColor alternate_base; // 列表交替行
    QColor text;
    QColor button;
    QColor button_text;
    QColor bright_text;
    QColor highlight;        // 选中背景
    QColor highlighted_text; // 选中文字
    QColor tool_tip_base;
    QColor tool_tip_text;

    // ---- 语义颜色（供 WusicProxyStyle 绘制特定区域） ----
    QColor sidebar_bg;
    QColor control_bar_bg;
    QColor control_bar_border;
    QColor menu_bar_bg;
    QColor splitter_handle;
    QColor scroll_bar_bg;
    QColor scroll_bar_handle;
    QColor progress_bar_bg;
    QColor progress_bar_fill;
    QColor item_hover;
    QColor item_selected;
    QColor frame_border;

    // ---- 尺寸参数（QStyle::pixelMetric 使用） ----
    int button_radius    = 4;
    int panel_radius     = 6;
    int menu_radius      = 4;
    int scroll_bar_width = 8;
    int slider_groove_h  = 4;
    int slider_handle_w  = 12;
    int separator_w      = 1;
    int tab_bar_height   = 30; // QTabBar 标签总高度（px），越大标签越高

    /// 生成 QPalette
    [[nodiscard]] QPalette to_qpalette() const
    {
        QPalette p;
        p.setColor(QPalette::Window, window);
        p.setColor(QPalette::WindowText, window_text);
        p.setColor(QPalette::Base, base);
        p.setColor(QPalette::AlternateBase, alternate_base);
        p.setColor(QPalette::Text, text);
        p.setColor(QPalette::Button, button);
        p.setColor(QPalette::ButtonText, button_text);
        p.setColor(QPalette::BrightText, bright_text);
        p.setColor(QPalette::Highlight, highlight);
        p.setColor(QPalette::HighlightedText, highlighted_text);
        p.setColor(QPalette::ToolTipBase, tool_tip_base);
        p.setColor(QPalette::ToolTipText, tool_tip_text);
        return p;
    }
};
