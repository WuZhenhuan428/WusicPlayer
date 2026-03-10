#include "WindowConfigBinder.hpp"
#include "WindowConfigSection.hpp"
#include "view/MainWindow.h"

void WindowConfigBinder::apply(MainWindowConfigContext& ctx) {
    if (!ctx.windowsSec->geometry.isEmpty()) {
        ctx.mainWindow->restoreGeometry(ctx.windowsSec->geometry);
    }
    if (!ctx.windowsSec->state.isEmpty()) {
        ctx.mainWindow->restoreState(ctx.windowsSec->state);
    }
}

void WindowConfigBinder::collect(MainWindowConfigContext& ctx) {
    ctx.windowsSec->geometry = (ctx.mainWindow->saveGeometry());
    ctx.windowsSec->state = (ctx.mainWindow->saveState());
}