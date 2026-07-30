#pragma once
#include "core/theme/ThemePalette.h"

/// 内置暗色调色板
inline ThemePalette darkPalette()
{
    ThemePalette p;
    p.name             = "Wusic Dark";
    p.author           = "WusicPlayer";
    p.isDark           = true;

    // QPalette 角色
    p.window           = QColor("#1a1a2e");
    p.windowText       = QColor("#e0e0e0");
    p.base             = QColor("#16213e");
    p.alternateBase    = QColor("#1e2a4a");
    p.text             = QColor("#e0e0e0");
    p.button           = QColor("#0f3460");
    p.buttonText       = QColor("#e0e0e0");
    p.brightText       = QColor("#ffffff");
    p.highlight        = QColor("#e94560");
    p.highlightedText  = QColor("#ffffff");
    p.toolTipBase      = QColor("#2a2a3e");
    p.toolTipText      = QColor("#e0e0e0");

    // 语义颜色
    p.sidebarBg        = QColor("#16213e");
    p.controlBarBg     = QColor("#0f3460");
    p.controlBarBorder = QColor("#533483");
    p.menuBarBg        = QColor("#1a1a2e");
    p.splitterHandle   = QColor("#0f3460");
    p.scrollbarBg      = QColor("#1a1a2e");
    p.scrollbarHandle  = QColor("#533483");
    p.progressBarBg    = QColor("#16213e");
    p.progressBarFill  = QColor("#e94560");
    p.itemHover        = QColor("#533483");
    p.itemSelected     = QColor("#e94560");
    p.frameBorder      = QColor("#3a3a5a");

    // 尺寸
    p.buttonRadius     = 6;
    p.panelRadius      = 8;
    p.menuRadius       = 4;
    p.scrollbarWidth   = 16;
    p.sliderGrooveH    = 4;
    p.sliderHandleW    = 12;
    p.separatorW       = 1;

    return p;
}
