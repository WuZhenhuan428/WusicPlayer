#include "DesktopLyricsBinder.hpp"
#include "view/DesktopLyricsWidget/DesktopLyricsWidget.h"
#include "DesktopLyricsSection.hpp"
#include <QDebug>
#include <QTimer>

void DesktopLyricsBinder::apply(MainWindowConfigContext& ctx) {
    if (!ctx.desktopLyrics || !ctx.desktopSec) {
        qDebug() << "[DEBUG] ctx.desktopLyrics (and)or ctx.desktopSec is not available!";
        return;
    }
        ctx.desktopLyrics->restoreGeometry(ctx.desktopSec->geometry);
        ctx.desktopSec->is_visible ? QTimer::singleShot(20, [ctx](){ctx.desktopLyrics->show();}) : ctx.desktopLyrics->hide();
}

void DesktopLyricsBinder::collect(MainWindowConfigContext& ctx) {
    ctx.desktopSec->geometry = ctx.desktopLyrics->saveGeometry();
}