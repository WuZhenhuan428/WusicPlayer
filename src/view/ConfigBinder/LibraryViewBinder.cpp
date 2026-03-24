#include "LibraryViewBinder.hpp"
#include "LibraryViewSection.hpp"
#include "controller/PlaylistController.h"
#include <QAbstractItemModel>
#include "view/MainWindow.h"
#include <QDebug>

void LibraryViewBinder::apply(MainWindowConfigContext& ctx) {
    if (!ctx.librarySec || !ctx.playlistController || ! ctx.libraryPanel || !ctx.mainWindow) {
        qDebug() << "[DEBUG] LibraryViewBinder: ctx's member is not available!";
        return;
    }
    if (!ctx.librarySec->columns.isEmpty()) {
        ctx.playlistController->viewModel()->setColumns(ctx.librarySec->columns);
    }

    auto* panel = ctx.libraryPanel;
    auto* sec = ctx.librarySec;
    auto* vm = ctx.playlistController->viewModel();
    auto* main_window = ctx.mainWindow;

    auto restoreLibraryWidgetState = [panel, sec]() {
        if (!panel || !sec) return;
        panel->setSongTreeHeaderState(sec->song_tree_view_state);
        panel->setSplitterState(sec->splitter_state);
        panel->setSplitterOrientation(sec->splitter_orientation);
    };
    QObject::connect(vm, &QAbstractItemModel::modelReset, main_window,
            restoreLibraryWidgetState, Qt::SingleShotConnection);
}

void LibraryViewBinder::collect(MainWindowConfigContext& ctx) {
    ctx.librarySec->song_tree_view_state = ctx.libraryPanel->songTreeHeader()->saveState();
    ctx.librarySec->columns.clear();
    for (const auto& c : ctx.playlistController->viewModel()->getColumns()) {
        ctx.librarySec->columns.append(c);
    }
    ctx.librarySec->group_rules = ctx.playlistController->groupRules();
    ctx.librarySec->sort_rules = ctx.playlistController->sortRules();
    ctx.librarySec->splitter_state = ctx.libraryPanel->splitterState();
    ctx.librarySec->song_tree_view_state = ctx.libraryPanel->songTreeHeaderState();
    ctx.librarySec->splitter_orientation = ctx.libraryPanel->splitterOrientation();
}