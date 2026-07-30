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
    bool isDark = false;

    // ---- QPalette 角色（将直接设入 QPalette） ----
    QColor window; // 主窗口背景
    QColor windowText;
    QColor base;          // 输入框 / 列表背景
    QColor alternateBase; // 列表交替行
    QColor text;
    QColor button;
    QColor buttonText;
    QColor brightText;
    QColor highlight;       // 选中背景
    QColor highlightedText; // 选中文字
    QColor toolTipBase;
    QColor toolTipText;

    // ---- 语义颜色（供 WusicProxyStyle 绘制特定区域） ----
    QColor sidebarBg;
    QColor controlBarBg;
    QColor controlBarBorder;
    QColor menuBarBg;
    QColor splitterHandle;
    QColor scrollbarBg;
    QColor scrollbarHandle;
    QColor progressBarBg;
    QColor progressBarFill;
    QColor itemHover;
    QColor itemSelected;
    QColor frameBorder;

    // ---- 尺寸参数（QStyle::pixelMetric 使用） ----
    int buttonRadius   = 4;
    int panelRadius    = 6;
    int menuRadius     = 4;
    int scrollbarWidth = 8;
    int sliderGrooveH  = 4;
    int sliderHandleW  = 12;
    int separatorW     = 1;

    /// 生成 QPalette
    QPalette toQPalette() const
    {
        QPalette p;
        p.setColor(QPalette::Window, window);
        p.setColor(QPalette::WindowText, windowText);
        p.setColor(QPalette::Base, base);
        p.setColor(QPalette::AlternateBase, alternateBase);
        p.setColor(QPalette::Text, text);
        p.setColor(QPalette::Button, button);
        p.setColor(QPalette::ButtonText, buttonText);
        p.setColor(QPalette::BrightText, brightText);
        p.setColor(QPalette::Highlight, highlight);
        p.setColor(QPalette::HighlightedText, highlightedText);
        p.setColor(QPalette::ToolTipBase, toolTipBase);
        p.setColor(QPalette::ToolTipText, toolTipText);
        return p;
    }
};
