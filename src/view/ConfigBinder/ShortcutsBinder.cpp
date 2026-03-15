#include "ShortcutsBinder.hpp"

#include "MainWindowConfigContext.hpp"
#include "ShortcutsSection.hpp"
#include "controller/shortcuts_controller.h"

void ShortcutsBinder::apply(MainWindowConfigContext& ctx)
{
    if (!ctx.shortcutsController || !ctx.shortcutsSec) {
        return;
    }

    ctx.shortcutsController->applyBindings(ctx.shortcutsSec->bindings);
}

void ShortcutsBinder::collect(MainWindowConfigContext& ctx)
{
    if (!ctx.shortcutsController || !ctx.shortcutsSec) {
        return;
    }

    ctx.shortcutsSec->bindings = ctx.shortcutsController->bindings();
}
