#include "service/theme_service.h"

#include "core/theme/theme_manager.h"
#include "model/theme_settings_model.h"

ThemeService::ThemeService(QObject* parent) :
    QObject(parent), m_model(std::make_unique<ThemeSettingsModel>(this))
{
    // 监听外部主题变更（如其他地方调用了 applyXxxTheme）
    connect(&ThemeManager::instance(), &ThemeManager::sgn_theme_changed, this, [this]() {
        emit sgn_current_theme_changed(ThemeManager::instance().current_theme_name());
    });
}

void ThemeService::scan_themes()
{
    QVector<ThemeEntry> entries;
    auto& tm = ThemeManager::instance();

    // 系统主题（无法可靠检测明暗，统一标为 false）
    for (const auto& key : tm.system_themes()) {
        ThemeEntry e;
        e.name   = key;
        e.source = QStringLiteral("System");
        e.author = QStringLiteral("Qt");
        e.isDark = false;
        entries.append(e);
    }

    // 内置主题——从注册的调色板读取 isDark
    for (const auto& name : tm.builtin_themes()) {
        const auto* pal = tm.palette_by_name(name);
        ThemeEntry e;
        e.name   = name;
        e.source = QStringLiteral("Builtin");
        e.author = QStringLiteral("WusicPlayer");
        e.isDark = pal ? pal->isDark : false;
        entries.append(e);
    }

    // 外部主题——仅名称已知，元信息在插件内部
    for (const auto& name : tm.external_themes()) {
        ThemeEntry e;
        e.name   = name;
        e.source = QStringLiteral("External");
        e.author = QStringLiteral("-");
        e.isDark = false;
        entries.append(e);
    }

    m_model->set_entries(entries);
}

void ThemeService::apply_theme(int row)
{
    const auto& entries = m_model->entries();
    if (row < 0 || row >= entries.size())
        return;

    const auto& e = entries.at(row);
    auto& tm      = ThemeManager::instance();

    if (e.source == QStringLiteral("System")) {
        tm.apply_system_theme(e.name);
    } else if (e.source == QStringLiteral("Builtin")) {
        tm.apply_builtin_theme(e.name);
    } else if (e.source == QStringLiteral("External")) {
        tm.apply_external_theme(e.name);
    }

    emit sgn_current_theme_changed(e.name);
}

ThemeSettingsModel* ThemeService::model() const
{
    return m_model.get();
}

QString ThemeService::current_theme_name() const
{
    return ThemeManager::instance().current_theme_name();
}

void ThemeService::rescan_external_plugins(const QString& dir)
{
    ThemeManager::instance().scan_external_plugins(dir);
    scan_themes(); // 刷新模型
}
