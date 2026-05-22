#include "ThemeManager.h"
#include "WusicProxyStyle.h"

#include <QApplication>
#include <QStyleFactory>
#include <QPluginLoader>
#include <QDir>
#include <QDirIterator>
#include <QJsonObject>
#include <QDebug>

#include "plugin/IThemePlugin.h"

// ============================================================================
// 单例
// ============================================================================

ThemeManager& ThemeManager::instance() {
    static ThemeManager inst;
    return inst;
}

ThemeManager::ThemeManager()
    : QObject(nullptr)
{
}

// ============================================================================
// 注册内置调色板
// ============================================================================

void ThemeManager::registerBuiltinPalette(const ThemePalette& palette) {
    m_builtinPalettes.insert(palette.name, palette);
}

// ============================================================================
// 扫描外部主题插件
// ============================================================================

void ThemeManager::scanExternalPlugins(const QString& dir) {
    QDir pluginDir(dir);
    if (!pluginDir.exists()) return;

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

QStringList ThemeManager::systemThemes() const {
    return QStyleFactory::keys();
}

QStringList ThemeManager::builtinThemes() const {
    return m_builtinPalettes.keys();
}

QStringList ThemeManager::externalThemes() const {
    return m_externalPlugins.keys();
}

// ============================================================================
// 应用主题
// ============================================================================

void ThemeManager::applySystemTheme(const QString& key) {
    QStyle* s = QStyleFactory::create(key);
    if (!s) return;
    qApp->setStyle(s);
    qApp->setPalette(s->standardPalette());
    setCurrent(System, key, qApp->style());
}

void ThemeManager::applyBuiltinTheme(const QString& name) {
    auto it = m_builtinPalettes.find(name);
    if (it == m_builtinPalettes.end()) return;

    // WusicProxyStyle 以 Fusion 为基础样式
    auto* style = new WusicProxyStyle(it.value());
    qApp->setStyle(style);  // qApp 接管所有权
    qApp->setPalette(style->standardPalette());

    setCurrent(Builtin, name, style);
}

void ThemeManager::applyExternalTheme(const QString& name) {
    auto it = m_externalPlugins.find(name);
    if (it == m_externalPlugins.end()) return;

    QPluginLoader loader(it.value());
    auto* plugin = qobject_cast<IThemePlugin*>(loader.instance());
    if (!plugin) return;

    ThemePalette p = plugin->createPalette();
    auto* style = new WusicProxyStyle(p);
    qApp->setStyle(style);
    qApp->setPalette(style->standardPalette());

    // 同时将外部调色板注册到内置表，方便 currentPalette() 查询
    m_builtinPalettes.insert(name, p);

    setCurrent(External, name, style);
}

// ============================================================================
// 内部
// ============================================================================

void ThemeManager::setCurrent(Source source, const QString& name, QStyle* style) {
    m_currentSource = source;
    m_currentName   = name;
    m_currentStyle  = style;
    emit themeChanged();
}

const ThemePalette* ThemeManager::currentPalette() const {
    if (m_currentSource == System) return nullptr;
    auto it = m_builtinPalettes.find(m_currentName);
    return it != m_builtinPalettes.end() ? &it.value() : nullptr;
}

const ThemePalette* ThemeManager::paletteByName(const QString& name) const {
    auto it = m_builtinPalettes.find(name);
    return it != m_builtinPalettes.end() ? &it.value() : nullptr;
}

void ThemeManager::setIconMode(IconMode mode) {
    m_iconMode = mode;
    // 图标切换后通知 UI 刷新（WControlBar 会通过 iconPath 读取）
    emit themeChanged();
}

bool ThemeManager::effectiveIconIsDark() const {
    switch (m_iconMode) {
    case IconDark:  return true;
    case IconLight: return false;
    case IconAuto:
    default: {
        const auto* pal = currentPalette();
        return pal ? pal->isDark : false;
    }
    }
}

// ============================================================================
// IConfigurable —— 持久化
// ============================================================================

void ThemeManager::loadFromJson(const QJsonObject& json) {
    const QJsonObject sub = json.value(configSubKey()).toObject();
    const QString source = sub.value("source").toString(QStringLiteral("system"));
    const QString name   = sub.value("name").toString(QStringLiteral("Fusion"));

    // 图标模式
    const QString iconStr = sub.value("iconMode").toString(QStringLiteral("auto"));
    if (iconStr == QStringLiteral("light"))       m_iconMode = IconLight;
    else if (iconStr == QStringLiteral("dark"))   m_iconMode = IconDark;
    else                                          m_iconMode = IconAuto;

    if (source == QStringLiteral("builtin")) {
        applyBuiltinTheme(name);
    } else if (source == QStringLiteral("external")) {
        applyExternalTheme(name);
    } else {
        applySystemTheme(name);
    }
}

QJsonObject ThemeManager::saveToJson() {
    QJsonObject obj;
    switch (m_currentSource) {
    case System:   obj["source"] = "system";   break;
    case Builtin:  obj["source"] = "builtin";  break;
    case External: obj["source"] = "external"; break;
    }
    obj["name"] = m_currentName;
    switch (m_iconMode) {
    case IconLight: obj["iconMode"] = "light";  break;
    case IconDark:  obj["iconMode"] = "dark";   break;
    default:        obj["iconMode"] = "auto";   break;
    }
    return obj;
}
