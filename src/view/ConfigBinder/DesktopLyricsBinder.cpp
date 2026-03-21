#include "DesktopLyricsBinder.hpp"
#include "view/DesktopLyricsWidget/DesktopLyricsWidget.h"
#include "DesktopLyricsSection.hpp"
#include <QDebug>
#include <QTimer>
#include <QFont>
#include "core/hsv_types.h"

void DesktopLyricsBinder::apply(MainWindowConfigContext& ctx) {
    if (!ctx.desktopLyrics || !ctx.desktopSec) {
        qDebug() << "[DEBUG] ctx.desktopLyrics (and)or ctx.desktopSec is not available!";
        return;
    }

    ctx.desktopLyrics->restoreGeometry(ctx.desktopSec->geometry);
    ctx.desktopLyrics->setActiveLineColor(rgb_t{
        (unsigned char)ctx.desktopSec->rgb_active_r,
        (unsigned char)ctx.desktopSec->rgb_active_g,
        (unsigned char)ctx.desktopSec->rgb_active_b
    });
    
    ctx.desktopLyrics->setInactiveLineColor(rgb_t{
        (unsigned char)ctx.desktopSec->rgb_inactive_r,
        (unsigned char)ctx.desktopSec->rgb_inactive_g,
        (unsigned char)ctx.desktopSec->rgb_inactive_b
    });

    QFont font;
    font.fromString(ctx.desktopSec->font_string);
    ctx.desktopLyrics->setLrcFont(font);

    ctx.desktopSec->is_visible ? QTimer::singleShot(20, [ctx](){ctx.desktopLyrics->show();}) : ctx.desktopLyrics->hide();
}

void DesktopLyricsBinder::collect(MainWindowConfigContext& ctx) {
    ctx.desktopSec->geometry = ctx.desktopLyrics->saveGeometry();

    rgb_t ac = ctx.desktopLyrics->getActiveLineColor();
    rgb_t inac = ctx.desktopLyrics->getInactiveLineColor();

    ctx.desktopSec->rgb_active_r = ac.r;
    ctx.desktopSec->rgb_active_g = ac.g;
    ctx.desktopSec->rgb_active_b = ac.b;
    ctx.desktopSec->rgb_inactive_r = inac.r;
    ctx.desktopSec->rgb_inactive_g = inac.g;
    ctx.desktopSec->rgb_inactive_b = inac.b;

    ctx.desktopSec->font_string = ctx.desktopLyrics->getFont().toString();
}