#include "theme_service.h"

#include "model/theme_settings_model.h"
#include "core/theme/ThemeManager.h"

ThemeService::ThemeService(QObject* parent)
    : QObject(parent)
    , m_model(std::make_unique<ThemeSettingsModel>(this))
{
    // 监听外部主题变更（如其他地方调用了 applyXxxTheme）
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this]() {
                emit currentThemeChanged(ThemeManager::instance().currentThemeName());
            });
}

void ThemeService::scanThemes() {
    QVector<ThemeEntry> entries;
    auto& tm = ThemeManager::instance();

    // 系统主题（无法可靠检测明暗，统一标为 false）
    for (const auto& key : tm.systemThemes()) {
        ThemeEntry e;
        e.name   = key;
        e.source = QStringLiteral("System");
        e.author = QStringLiteral("Qt");
        e.isDark = false;
        entries.append(e);
    }

    // 内置主题——从注册的调色板读取 isDark
    for (const auto& name : tm.builtinThemes()) {
        const auto* pal = tm.paletteByName(name);
        ThemeEntry e;
        e.name   = name;
        e.source = QStringLiteral("Builtin");
        e.author = QStringLiteral("WusicPlayer");
        e.isDark = pal ? pal->isDark : false;
        entries.append(e);
    }

    // 外部主题——仅名称已知，元信息在插件内部
    for (const auto& name : tm.externalThemes()) {
        ThemeEntry e;
        e.name   = name;
        e.source = QStringLiteral("External");
        e.author = QStringLiteral("-");
        e.isDark = false;
        entries.append(e);
    }

    m_model->setEntries(entries);
}

void ThemeService::applyTheme(int row) {
    const auto& entries = m_model->entries();
    if (row < 0 || row >= entries.size()) return;

    const auto& e = entries.at(row);
    auto& tm = ThemeManager::instance();

    if (e.source == QStringLiteral("System")) {
        tm.applySystemTheme(e.name);
    } else if (e.source == QStringLiteral("Builtin")) {
        tm.applyBuiltinTheme(e.name);
    } else if (e.source == QStringLiteral("External")) {
        tm.applyExternalTheme(e.name);
    }

    emit currentThemeChanged(e.name);
}

ThemeSettingsModel* ThemeService::model() const {
    return m_model.get();
}

QString ThemeService::currentThemeName() const {
    return ThemeManager::instance().currentThemeName();
}

void ThemeService::rescanExternalPlugins(const QString& dir) {
    ThemeManager::instance().scanExternalPlugins(dir);
    scanThemes();  // 刷新模型
}
