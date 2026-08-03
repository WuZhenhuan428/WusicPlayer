#include "core/theme/theme_manager.h"
#include "wusic_proxy_style.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QJsonObject>
#include <QPluginLoader>
#include <QStyleFactory>

#include "plugin/i_theme_plugin.h"

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
// 注册内置调色板
// ============================================================================

void ThemeManager::register_builtin_palette(const ThemePalette& palette)
{
    m_builtinPalettes.insert(palette.name, palette);
}

// ============================================================================
// 扫描外部主题插件
// ============================================================================

void ThemeManager::scan_external_plugins(const QString& dir)
{
    QDir pluginDir(dir);
    if (!pluginDir.exists())
        return;

    const auto entries = pluginDir.entryInfoList(QDir::Files);
    for (const auto& info : entries) {
        QPluginLoader loader(info.absoluteFilePath());
        auto* plugin = qobject_cast<IThemePlugin*>(loader.instance());
        if (plugin) {
            m_externalPlugins.insert(plugin->name(), info.absoluteFilePath());
            qDebug() << "[ThemeManager] found external theme:" << plugin->name();
        }
    }
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
    return m_builtinPalettes.keys();
}

QStringList ThemeManager::external_themes() const
{
    return m_externalPlugins.keys();
}

// ============================================================================
// 应用主题
// ============================================================================

void ThemeManager::apply_system_theme(const QString& key)
{
    QStyle* s = QStyleFactory::create(key);
    if (!s)
        return;
    qApp->setStyle(s);
    qApp->setPalette(s->standardPalette());
    set_current(System, key, qApp->style());
}

void ThemeManager::apply_builtin_theme(const QString& name)
{
    auto it = m_builtinPalettes.find(name);
    if (it == m_builtinPalettes.end())
        return;

    // WusicProxyStyle 以 Fusion 为基础样式
    auto* style = new WusicProxyStyle(it.value());
    qApp->setStyle(style); // qApp 接管所有权
    qApp->setPalette(style->standardPalette());

    set_current(Builtin, name, style);
}

void ThemeManager::apply_external_theme(const QString& name)
{
    auto it = m_externalPlugins.find(name);
    if (it == m_externalPlugins.end())
        return;

    QPluginLoader loader(it.value());
    auto* plugin = qobject_cast<IThemePlugin*>(loader.instance());
    if (!plugin)
        return;

    ThemePalette p = plugin->createPalette();
    auto* style    = new WusicProxyStyle(p);
    qApp->setStyle(style);
    qApp->setPalette(style->standardPalette());

    // 同时将外部调色板注册到内置表，方便 current_palette() 查询
    m_builtinPalettes.insert(name, p);

    set_current(External, name, style);
}

// ============================================================================
// 内部
// ============================================================================

void ThemeManager::set_current(Source source, const QString& name, QStyle* style)
{
    m_current_source = source;
    m_currentName    = name;
    m_current_style  = style;
    emit sgn_theme_changed();
}

const ThemePalette* ThemeManager::current_palette() const
{
    if (m_current_source == System)
        return nullptr;
    auto it = m_builtinPalettes.find(m_currentName);
    return it != m_builtinPalettes.end() ? &it.value() : nullptr;
}

const ThemePalette* ThemeManager::palette_by_name(const QString& name) const
{
    auto it = m_builtinPalettes.find(name);
    return it != m_builtinPalettes.end() ? &it.value() : nullptr;
}

void ThemeManager::set_icon_mode(IconMode mode)
{
    m_icon_mode = mode;
    // 图标切换后通知 UI 刷新（WControlBar 会通过 iconPath 读取）
    emit sgn_theme_changed();
}

bool ThemeManager::effective_icon_is_dark() const
{
    switch (m_icon_mode) {
    case IconDark:
        return true;
    case IconLight:
        return false;
    case IconAuto:
    default: {
        const auto* pal = current_palette();
        return pal ? pal->isDark : false;
    }
    }
}

// ============================================================================
// IConfigurable —— 持久化
// ============================================================================

void ThemeManager::load_from_json(const QJsonObject& json)
{
    const QJsonObject sub = json.value(config_sub_key()).toObject();
    const QString source  = sub.value("source").toString(QStringLiteral("system"));
    const QString name    = sub.value("name").toString(QStringLiteral("Fusion"));

    // 图标模式
    const QString iconStr = sub.value("icon_mode").toString(QStringLiteral("auto"));
    if (iconStr == QStringLiteral("light"))
        m_icon_mode = IconLight;
    else if (iconStr == QStringLiteral("dark"))
        m_icon_mode = IconDark;
    else
        m_icon_mode = IconAuto;

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
    switch (m_current_source) {
    case System:
        obj["source"] = "system";
        break;
    case Builtin:
        obj["source"] = "builtin";
        break;
    case External:
        obj["source"] = "external";
        break;
    }
    obj["name"] = m_currentName;
    switch (m_icon_mode) {
    case IconLight:
        obj["icon_mode"] = "light";
        break;
    case IconDark:
        obj["icon_mode"] = "dark";
        break;
    default:
        obj["icon_mode"] = "auto";
        break;
    }
    return obj;
}
