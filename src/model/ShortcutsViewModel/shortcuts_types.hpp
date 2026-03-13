#pragma once

#include <QString>
#include <QKeySequence>
#include <QVector>

enum class ShortcutScope
{
    Application,    // global
    MainWindow,
    PlaylistView,
    SearchPanel,
    DesktopLyrics
};

struct ShortcutDescriptor
{
    QString action_id;
    QString display_name;
    QString category;
    QKeySequence default_key;
    ShortcutScope scope;
    bool editable = true;
};

struct ShortcutBinding
{
    QString action_id;
    QKeySequence current_key;
    bool enabled = true;
};

struct ShortcutItem
{
    ShortcutDescriptor desc;
    ShortcutBinding binding;
    bool conflict = false;
    QString conflict_with_action_id;
};
