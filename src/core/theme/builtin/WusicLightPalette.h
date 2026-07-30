#pragma once
#include "core/theme/ThemePalette.h"

/// 内置亮色调色板
inline ThemePalette lightPalette()
{
    ThemePalette p;
    p.name             = "Wusic Light";
    p.author           = "WusicPlayer";
    p.isDark           = false;

    // QPalette 角色
    p.window           = QColor("#f5f5f5");
    p.windowText       = QColor("#2c2c2c");
    p.base             = QColor("#ffffff");
    p.alternateBase    = QColor("#f0f0f0");
    p.text             = QColor("#2c2c2c");
    p.button           = QColor("#e0e0e0");
    p.buttonText       = QColor("#2c2c2c");
    p.brightText       = QColor("#000000");
    p.highlight        = QColor("#2979ff");
    p.highlightedText  = QColor("#ffffff");
    p.toolTipBase      = QColor("#ffffff");
    p.toolTipText      = QColor("#2c2c2c");

    // 语义颜色
    p.sidebarBg        = QColor("#ffffff");
    p.controlBarBg     = QColor("#e8e8e8");
    p.controlBarBorder = QColor("#cccccc");
    p.menuBarBg        = QColor("#f5f5f5");
    p.splitterHandle   = QColor("#ffffff");
    p.scrollbarBg      = QColor("#f5f5f5");
    p.scrollbarHandle  = QColor("#bbbbbb");
    p.progressBarBg    = QColor("#e0e0e0");
    p.progressBarFill  = QColor("#2979ff");
    p.itemHover        = QColor("#2979ff");
    p.itemSelected     = QColor("#2979ff");
    p.frameBorder      = QColor("#cccccc");

    // 尺寸
    p.buttonRadius     = 4;
    p.panelRadius      = 6;
    p.menuRadius       = 4;
    p.scrollbarWidth   = 16;
    p.sliderGrooveH    = 4;
    p.sliderHandleW    = 12;
    p.separatorW       = 1;

    return p;
}
