#pragma once
#include "core/theme/theme_palette.h"

/// 内置亮色调色板
inline ThemePalette light_palette()
{
    ThemePalette p;
    p.name               = "Wusic Light";
    p.author             = "WusicPlayer";
    p.is_dark            = false;

    // QPalette 角色
    p.window             = QColor("#f5f5f5");
    p.window_text        = QColor("#2c2c2c");
    p.base               = QColor("#ffffff");
    p.alternate_base     = QColor("#f0f0f0");
    p.text               = QColor("#2c2c2c");
    p.button             = QColor("#e0e0e0");
    p.button_text        = QColor("#2c2c2c");
    p.bright_text        = QColor("#000000");
    p.highlight          = QColor("#2979ff");
    p.highlighted_text   = QColor("#ffffff");
    p.tool_tip_base      = QColor("#ffffff");
    p.tool_tip_text      = QColor("#2c2c2c");

    // 语义颜色
    p.sidebar_bg         = QColor("#ffffff");
    p.control_bar_bg     = QColor("#e8e8e8");
    p.control_bar_border = QColor("#cccccc");
    p.menu_bar_bg        = QColor("#f5f5f5");
    p.splitter_handle    = QColor("#ffffff");
    p.scroll_bar_bg      = QColor("#f5f5f5");
    p.scroll_bar_handle  = QColor("#bbbbbb");
    p.progress_bar_bg    = QColor("#e0e0e0");
    p.progress_bar_fill  = QColor("#2979ff");
    p.item_hover         = QColor("#2979ff");
    p.item_selected      = QColor("#2979ff");
    p.frame_border       = QColor("#cccccc");

    // 尺寸
    p.button_radius      = 4;
    p.panel_radius       = 6;
    p.menu_radius        = 4;
    p.scroll_bar_width   = 16;
    p.slider_groove_h    = 4;
    p.slider_handle_w    = 12;
    p.separator_w        = 1;
    p.tab_bar_height     = 36;

    return p;
}
