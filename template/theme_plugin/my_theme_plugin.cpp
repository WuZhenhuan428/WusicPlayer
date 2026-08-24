#include "my_theme_plugin.h"

QString MyThemePlugin::id() const
{
    return QStringLiteral("com.wusicplayer.theme.mytheme");
}

QString MyThemePlugin::name() const
{
    return QStringLiteral("My Theme");
}

QString MyThemePlugin::version() const
{
    return QStringLiteral("1.0");
}

QString MyThemePlugin::description() const
{
    return QStringLiteral("A sample theme plugin");
}

QString MyThemePlugin::author() const
{
    return QStringLiteral("Your Name");
}

QVector<QString> MyThemePlugin::categories() const
{
    return {QStringLiteral("theme")};
}

ThemePalette MyThemePlugin::createPalette() const
{
    ThemePalette p;
    p.name             = this->name();
    p.author           = this->author();
    p.isDark           = true;

    // ---- QPalette 角色 ----
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

    // ---- 语义颜色(供 WusicProxyStyle 绘制特定区域) ----
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

    // ---- 尺寸参数 ----
    p.buttonRadius     = 6;
    p.panelRadius      = 8;
    p.menuRadius       = 4;
    p.scrollbarWidth   = 16;
    p.sliderGrooveH    = 4;
    p.sliderHandleW    = 12;
    p.separatorW       = 1;
    p.tabBarHeight     = 36;

    return p;
}
