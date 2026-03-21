#pragma once

#include <QString>
#include <QKeySequence>
#include <QVector>
#include <magic_enum/magic_enum.hpp>

enum class ShortcutScope
{
    Application,    // global
    MainWindow,
    PlaylistView,
    SearchPanel,
    DesktopLyrics
};

enum class ShortcutActionId
{
    play_pause,
    stop,
    open_search,
    show_hide_desktop_lyrics,
    save_playlist,
    open_file,
    open_playlist,
    open_settings,
    open_hsv_test
    // ...
};

inline QString shortcutActionIdToString(ShortcutActionId actionId)
{
    return QString::fromStdString(std::string(magic_enum::enum_name(actionId)));
}

inline bool shortcutActionIdFromString(const QString& value, ShortcutActionId& out)
{
    const auto parsed = magic_enum::enum_cast<ShortcutActionId>(value.toStdString());
    if (!parsed.has_value()) {
        return false;
    }
    out = parsed.value();
    return true;
}

inline size_t qHash(ShortcutActionId key, size_t seed = 0)
{
    return qHash(static_cast<int>(key), seed);
}

struct ShortcutDescriptor
{
    ShortcutActionId action_id;
    QString display_name;
    // QString category;
    ShortcutScope scope;
    QKeySequence default_key;
    bool editable = true;
};

struct ShortcutBinding
{
    ShortcutActionId action_id;
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
