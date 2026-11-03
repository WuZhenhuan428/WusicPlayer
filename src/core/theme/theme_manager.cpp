#include "core/theme/theme_manager.h"

#include "wusic_proxy_style.h"

#include <QApplication>
#include <QJsonObject>
#include <QStyleFactory>

#include "core/plugin_manager/plugin_manager.h"
#include "plugin/i_theme_plugin.h"

#include "core/logger/logger_manager.h"
namespace
{
// NOLINTNEXTLINE
Logger* logger = LoggerManager::file_logger("theme", {"console", "gui"});
} // namespace
// ============================================================================
// 单例
// ============================================================================
ThemeManager& ThemeManager::instance()
{
    static ThemeManager inst;
    return inst;
}

ThemeManager::ThemeManager() : QObject(nullptr) {}

// ============================================================================
// 扫描外部主题插件
// ============================================================================

void ThemeManager::set_plugin_manager(PluginManager* plugin_manager)
{
    plugin_manager_ = plugin_manager;
}

void ThemeManager::scan_external_plugins(const QString& dir)
{
    // 外部主题插件统一由 PluginManager 扫描/加载, 本管理器仅查询
    if (!plugin_manager_) {
        logger->warn("[ThemeManager] plugin manager not injected, skip scan: {}", dir);
        return;
    }
    plugin_manager_->scan_directory(dir);
}

// ============================================================================
// 查询可用主题列表
// ============================================================================

QStringList ThemeManager::system_themes() const
{
    return QStyleFactory::keys();
}

QStringList ThemeManager::builtin_themes() const
{
    QStringList names;
    if (plugin_manager_) {
        for (auto* plugin : plugin_manager_->builtin_plugins<IThemePlugin>()) {
            names.push_back(plugin->name());
        }
    }
    return names;
}

QStringList ThemeManager::external_themes() const
{
    QStringList names;
    if (plugin_manager_) {
        for (auto* plugin : plugin_manager_->external_plugins<IThemePlugin>()) {
            names.push_back(plugin->name());
        }
    }
    return names;
}

QString ThemeManager::theme_author(const QString& name) const
{
    if (plugin_manager_) {
        for (auto* plugin : plugin_manager_->plugins<IThemePlugin>()) {
            if (plugin->name() == name) {
                return plugin->author();
            }
        }
    }
    return QStringLiteral("WusicPlayer");
}

bool ThemeManager::theme_is_dark(const QString& name) const
{
    if (plugin_manager_) {
        for (auto* plugin : plugin_manager_->plugins<IThemePlugin>()) {
            if (plugin->name() == name) {
                return plugin->create_palette().is_dark;
            }
        }
    }
    auto it = builtin_palettes_.find(name);
    return it != builtin_palettes_.end() && it->is_dark;
}

// ============================================================================
// 应用主题
// ============================================================================

void ThemeManager::apply_system_theme(const QString& key)
{
    QStyle* s = QStyleFactory::create(key);
    if (!s) {
        return;
    }
    qApp->setStyle(s);
    qApp->setPalette(s->standardPalette());
    set_current(Source::System, key, qApp->style());
}

void ThemeManager::apply_builtin_theme(const QString& name)
{
    if (!plugin_manager_) {
        return;
    }
    IThemePlugin* plugin = nullptr;
    for (auto* p : plugin_manager_->builtin_plugins<IThemePlugin>()) {
        if (p->name() == name) {
            plugin = p;
            break;
        }
    }
    if (!plugin) {
        logger->warn("[ThemeManager] builtin theme '{}' not found", name);
        return;
    }

    // WusicProxyStyle 以 Fusion 为基础样式
    ThemePalette p = plugin->create_palette();
    auto* style    = new WusicProxyStyle(p);
    qApp->setStyle(style); // qApp 接管所有权
    qApp->setPalette(style->standardPalette());
    builtin_palettes_.insert(name, p); // 缓存供 current_palette() 查询

    set_current(Source::Builtin, name, style);
}

void ThemeManager::apply_external_theme(const QString& name)
{
    if (!plugin_manager_) {
        return;
    }
    IThemePlugin* plugin = nullptr;
    for (auto* p : plugin_manager_->external_plugins<IThemePlugin>()) {
        if (p->name() == name) {
            plugin = p;
            break;
        }
    }
    if (!plugin) {
        logger->warn("[ThemeManager] external theme '{}' not found", name);
        return;
    }

    ThemePalette p = plugin->create_palette();
    auto* style    = new WusicProxyStyle(p);
    qApp->setStyle(style);
    qApp->setPalette(style->standardPalette());

    // 同时将调色板注册到内置表，方便 current_palette() 查询
    builtin_palettes_.insert(name, p);

    set_current(Source::External, name, style);
}

// ============================================================================
// 内部
// ============================================================================

void ThemeManager::set_current(Source source, const QString& name, QStyle* style)
{
    current_source_ = source;
    currentName_    = name;
    current_style_  = style;
    emit sgn_theme_changed();
}

const ThemePalette* ThemeManager::current_palette() const
{
    if (current_source_ == Source::System) {
        return nullptr;
    }
    auto it = builtin_palettes_.find(currentName_);
    return it != builtin_palettes_.end() ? &it.value() : nullptr;
}

const ThemePalette* ThemeManager::palette_by_name(const QString& name) const
{
    auto it = builtin_palettes_.find(name);
    return it != builtin_palettes_.end() ? &it.value() : nullptr;
}

void ThemeManager::set_icon_mode(IconMode mode)
{
    icon_mode_ = mode;
    // 图标切换后通知 UI 刷新（WControlBar 会通过 iconPath 读取）
    emit sgn_theme_changed();
}

bool ThemeManager::effective_icon_is_dark() const
{
    switch (icon_mode_) {
    case IconMode::IconDark:
        return true;
    case IconMode::IconLight:
        return false;
    case IconMode::IconAuto:
    default: {
        const auto* pal = current_palette();
        return pal ? pal->is_dark : false;
    }
    }
}

// ============================================================================
// IConfigurable —— 持久化
// ============================================================================

void ThemeManager::load_from_json(const QJsonObject& json)
{
    const QJsonObject sub  = json.value(config_sub_key()).toObject();
    const QString source   = sub.value("source").toString(QStringLiteral("system"));
    const QString name     = sub.value("name").toString(QStringLiteral("Fusion"));

    // 图标模式
    const QString icon_str = sub.value("icon_mode").toString(QStringLiteral("auto"));
    if (icon_str == QStringLiteral("light")) {
        icon_mode_ = IconMode::IconLight;
    } else if (icon_str == QStringLiteral("dark")) {
        icon_mode_ = IconMode::IconDark;
    } else {
        icon_mode_ = IconMode::IconAuto;
    }

    if (source == QStringLiteral("builtin")) {
        apply_builtin_theme(name);
    } else if (source == QStringLiteral("external")) {
        apply_external_theme(name);
    } else {
        apply_system_theme(name);
    }
}

QJsonObject ThemeManager::save_to_json()
{
    QJsonObject obj;
    switch (current_source_) {
    case Source::System:
        obj["source"] = "system";
        break;
    case Source::Builtin:
        obj["source"] = "builtin";
        break;
    case Source::External:
        obj["source"] = "external";
        break;
    }
    obj["name"] = currentName_;
    switch (icon_mode_) {
    case IconMode::IconLight:
        obj["icon_mode"] = "light";
        break;
    case IconMode::IconDark:
        obj["icon_mode"] = "dark";
        break;
    default:
        obj["icon_mode"] = "auto";
        break;
    }
    return obj;
}
