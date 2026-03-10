#include "SearchPanelBinder.hpp"
#include "SearchPanelSection.hpp"
#include "view/MainWindow.h"

void SearchPanelBinder::apply(MainWindowConfigContext& ctx) {
    if (!ctx.searchSec->geometry.isEmpty()) {
        ctx.mainWindow->m_searchPanelGeoCache = ctx.searchSec->geometry;
    }
    if (!ctx.searchSec->header_state.isEmpty()) {
        ctx.mainWindow->m_searchPanelHeaderStateCache = ctx.searchSec->header_state;
    }
    if (ctx.searchPanel) {
        ctx.searchSec->is_visible ? ctx.searchPanel->show() : ctx.searchPanel->hide();
    }
}

void SearchPanelBinder::collect(MainWindowConfigContext& ctx) {
    ctx.searchSec->geometry = ctx.mainWindow->m_searchPanelGeoCache;
    ctx.searchSec->header_state = ctx.mainWindow->m_searchPanelHeaderStateCache;
    ctx.searchSec->is_visible = false;
}