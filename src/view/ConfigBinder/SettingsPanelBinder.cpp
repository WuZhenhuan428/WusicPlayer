#include "SettingsPanelBinder.hpp"
#include "SettingsPanelSection.hpp"
#include "app_controller.h"

void SettingsPanelBinder::apply(MainWindowConfigContext& ctx) {
    if (!ctx.settingsSec || !ctx.appController) {
        return;
    }

    if (!ctx.settingsSec->geometry.isEmpty()) {
        ctx.appController->m_settings_panel_geo_cache = ctx.settingsSec->geometry;
    }
}

void SettingsPanelBinder::collect(MainWindowConfigContext& ctx) {
    if (!ctx.settingsSec || !ctx.appController) {
        return;
    }

    ctx.settingsSec->geometry = ctx.appController->m_settings_panel_geo_cache;
}