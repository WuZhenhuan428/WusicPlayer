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

ThemePalette MyThemePlugin::create_palette() const
{
    ThemePalette p;
    p.name               = this->name();
    p.author             = this->author();
    p.is_dark            = true;

    // ---- QPalette 角色 ----
    p.window             = QColor("#1a1a2e");
    p.window_text        = QColor("#e0e0e0");
    p.base               = QColor("#16213e");
    p.alternate_base     = QColor("#1e2a4a");
    p.text               = QColor("#e0e0e0");
    p.button             = QColor("#0f3460");
    p.button_text        = QColor("#e0e0e0");
    p.bright_text        = QColor("#ffffff");
    p.highlight          = QColor("#e94560");
    p.highlighted_text   = QColor("#ffffff");
    p.tool_tip_base      = QColor("#2a2a3e");
    p.tool_tip_text      = QColor("#e0e0e0");

    // ---- 语义颜色(供 WusicProxyStyle 绘制特定区域) ----
    p.sidebar_bg         = QColor("#16213e");
    p.control_bar_bg     = QColor("#0f3460");
    p.control_bar_border = QColor("#533483");
    p.menu_bar_bg        = QColor("#1a1a2e");
    p.splitter_handle    = QColor("#0f3460");
    p.scroll_bar_bg      = QColor("#1a1a2e");
    p.scroll_bar_handle  = QColor("#533483");
    p.progress_bar_bg    = QColor("#16213e");
    p.progress_bar_fill  = QColor("#e94560");
    p.item_hover         = QColor("#533483");
    p.item_selected      = QColor("#e94560");
    p.frame_border       = QColor("#3a3a5a");

    // ---- 尺寸参数 ----
    p.button_radius      = 6;
    p.panel_radius       = 8;
    p.menu_radius        = 4;
    p.scroll_bar_width   = 16;
    p.slider_groove_h    = 4;
    p.slider_handle_w    = 12;
    p.separator_w        = 1;
    p.tab_bar_height     = 36;

    return p;
}
