#include "panel_coordinator.h"

#include "controller/playback_controller.h"
#include "controller/playlist_controller.h"
#include "controller/search_backend/in_memory_search_backend.h"
#include "controller/shortcuts_controller.h"
#include "core/config_manager/config_manager.h"
#include "core/types.h"
#include "model/library/library_manager.h"
#include "model/shortcuts_view_model/shortcuts_types.hpp"
#include "service/theme_service.h"
#include "view/eq_widget/eq_widget.h"
#include "view/main_window.h"
#include "view/search_panel/search_panel.h"
#include "view/settings_panel/library_settings_page.h"
#include "view/settings_panel/lyrics_setting_panel/lyrics_setting_panel.h"
#include "view/settings_panel/settings_panel.h"
#include "view/settings_panel/shortcuts_panel/shortcuts_panel.h"
#include "view/settings_panel/theme_settings_page/theme_settings_page.h"

#include <QAbstractItemModel>
#include <QEvent>
#include <QJsonObject>
#include <QKeySequence>
#include <QListWidgetItem>
#include <QShortcut>

PanelCoordinator::PanelCoordinator(MainWindow* main_window, PlaybackController* playback_ctl,
                                   PlaylistController* playlist_ctl, LibraryManager* library_mgr,
                                   ThemeService* theme_service,
                                   InMemorySearchBackend* search_backend, QObject* parent) :
    QObject(parent), main_window_(main_window), playback_ctl_(playback_ctl),
    playlist_ctl_(playlist_ctl), library_mgr_(library_mgr), theme_service_(theme_service),
    search_backend_(search_backend)
{
    ensure_shortcuts_controller();
}

PanelCoordinator::~PanelCoordinator() = default;

ShortcutsController* PanelCoordinator::shortcuts_controller()
{
    ensure_shortcuts_controller();
    return shortcuts_controller_;
}

void PanelCoordinator::save_panel_configs()
{
    ConfigManager& cm = ConfigManager::get_instance();
    if (settings_panel_) {
        cm.write_sub_config(settings_panel_->config_sub_key(), settings_panel_->save_to_json());
    }
    if (shortcuts_panel_) {
        cm.write_sub_config(shortcuts_panel_->config_sub_key(), shortcuts_panel_->save_to_json());
    }
    if (search_panel_) {
        cm.write_sub_config(search_panel_->config_sub_key(), search_panel_->save_to_json());
    }
}

void PanelCoordinator::open_settings_panel()
{
    ensure_settings_panel();
    ensure_shortcuts_page();

    // 歌词设置页(依赖桌面歌词控件颜色/字体)
    if (!lyrics_settings_panel_) {
        lyrics_settings_panel_ = new LyricsSettingPanel(
            main_window_->desktop_lyrics_widget()->get_active_line_color(),
            main_window_->desktop_lyrics_widget()->get_inactive_line_color());
        lyrics_settings_panel_->set_line_edit_text(
            main_window_->desktop_lyrics_widget()->get_font());

        connect(lyrics_settings_panel_, &LyricsSettingPanel::sgnActiveColorChanged, this,
                [this](rgb_t rgb) {
                    main_window_->desktop_lyrics_widget()->set_active_line_color(rgb);
                });
        connect(lyrics_settings_panel_, &LyricsSettingPanel::sgnInactiveColorChanged, this,
                [this](rgb_t rgb) {
                    main_window_->desktop_lyrics_widget()->set_inactive_line_color(rgb);
                });
        connect(lyrics_settings_panel_, &LyricsSettingPanel::sgnDisplayModeChanged, this,
                [this](bool is_two_line) {
                    main_window_->desktop_lyrics_widget()->set_display_mode(
                        is_two_line ? DisplayMode::TwoLine : DisplayMode::OneLine);
                });
        connect(lyrics_settings_panel_, &LyricsSettingPanel::sgnFontChanged,
                main_window_->desktop_lyrics_widget(), &DesktopLyricsWidget::set_lrc_font);

        settings_panel_->register_widget(lyrics_settings_panel_->get_title_item(),
                                         lyrics_settings_panel_);
    }

    // 主题设置页
    if (!theme_settings_page_) {
        theme_settings_page_ = new ThemeSettingsPage(theme_service_, settings_panel_);
        settings_panel_->register_widget(theme_settings_page_->get_title_item(),
                                         theme_settings_page_);
    }

    // 媒体库设置页(watched folders 唯一管理入口 + 添加解析策略配置)
    if (!library_settings_page_) {
        library_settings_page_ = new LibrarySettingsPage(library_mgr_, settings_panel_);
        library_settings_page_->set_add_file_policy(playlist_ctl_->add_file_policy());
        connect(library_settings_page_, &LibrarySettingsPage::sgnAddFilePolicyChanged, this,
                [this](int policy) {
                    playlist_ctl_->set_add_file_policy(static_cast<AddFilePolicy>(policy));
                });
        settings_panel_->register_widget(library_settings_page_->get_title_item(),
                                         library_settings_page_);
    }

    settings_panel_->show();
    settings_panel_->raise();
    settings_panel_->activateWindow();
}

void PanelCoordinator::open_settings_panel_page(const QString& title)
{
    open_settings_panel();
    if (settings_panel_) {
        settings_panel_->switch_to_page_by_title(title);
    }
}

void PanelCoordinator::open_search_panel()
{
    ensure_search_panel();

    search_panel_->show();
    search_panel_->raise();
    search_panel_->activateWindow();
}

void PanelCoordinator::open_eq_widget()
{
    if (!eq_widget_) {
        eq_widget_ = new EQWidget(playback_ctl_->gains(), playback_ctl_->is_eq_enabled(), false,
                                  main_window_);
        eq_widget_->setWindowFlag(Qt::Window, true);
        eq_widget_->setAttribute(Qt::WA_DeleteOnClose);

        connect(eq_widget_, &EQWidget::sgnGainChanged, playback_ctl_,
                &PlaybackController::set_gains);
        connect(eq_widget_, &EQWidget::sgnEqEnabledChanged, playback_ctl_,
                &PlaybackController::set_eq_enabled);

        // QPointer auto-nulls on destroy; config saved via save_config() on exit
        connect(eq_widget_, &QObject::destroyed, this, []() {});
    }

    eq_widget_->show();
    eq_widget_->raise();
    eq_widget_->activateWindow();
}

bool PanelCoordinator::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == settings_panel_ && event->type() == QEvent::Hide) {
        // 设置面板关闭后激活主窗口,防止菜单栏焦点丢失
        if (main_window_) {
            main_window_->activateWindow();
            main_window_->raise();
        }
    }
    return QObject::eventFilter(obj, event);
}

void PanelCoordinator::ensure_settings_panel()
{
    if (settings_panel_) {
        return;
    }
    settings_panel_ = new SettingsPanel(&ConfigManager::get_instance(), main_window_);
    settings_panel_->setWindowFlag(Qt::Window, true);
    // 不使用 WA_DeleteOnClose：改为 hide/show 复用，避免销毁/重建循环中的状态不一致
    settings_panel_->installEventFilter(this);
}

void PanelCoordinator::ensure_shortcuts_controller()
{
    if (shortcuts_controller_) {
        return;
    }
    shortcuts_controller_ = new ShortcutsController(this);
    register_default_shortcuts();
}

void PanelCoordinator::ensure_shortcuts_page()
{
    ensure_shortcuts_controller();

    if (!shortcuts_panel_) {
        shortcuts_panel_ = new ShortcutsPanel(&ConfigManager::get_instance(), settings_panel_);
        shortcuts_panel_->set_view_model(shortcuts_controller_->view_model());

        connect(shortcuts_panel_, &ShortcutsPanel::sgnDefaultConfig, this, [this]() {
            if (shortcuts_controller_) {
                shortcuts_controller_->reset_all_to_default();
            }
        });

        connect(shortcuts_panel_, &ShortcutsPanel::sgnRestoreConfig, this, [this]() {
            if (!shortcuts_controller_) {
                return;
            }
            QJsonObject sub_obj = ConfigManager::get_instance().read_sub_config(
                shortcuts_controller_->config_sub_key());
            QJsonObject root;
            root.insert(shortcuts_controller_->config_sub_key(), sub_obj);
            shortcuts_controller_->load_from_json(root);
        });

        connect(shortcuts_panel_, &ShortcutsPanel::sgnApplyConfig, this,
                []() { ConfigManager::get_instance().save_all(); });

        settings_panel_->register_widget(shortcuts_panel_->get_list_item(), shortcuts_panel_);
    }
}

void PanelCoordinator::register_default_shortcuts()
{
    if (!shortcuts_controller_ || has_shortcuts_registered_) {
        return;
    }

    shortcuts_controller_->set_scope_host(ShortcutScope::Application, main_window_);
    shortcuts_controller_->set_scope_host(ShortcutScope::MainWindow, main_window_);
    shortcuts_controller_->set_scope_host(ShortcutScope::DesktopLyrics, main_window_);

    shortcuts_controller_->register_operation(
        ShortcutActionId::save_playlist, "Save Playlist", ShortcutScope::PlaylistView,
        QKeySequence(Qt::CTRL | Qt::Key_S), [this]() { playlist_ctl_->save_playlist(); },
        main_window_, true);

    shortcuts_controller_->register_operation(
        ShortcutActionId::open_file, "Open File", ShortcutScope::PlaylistView,
        QKeySequence(Qt::CTRL | Qt::Key_O), [this]() { playlist_ctl_->import_files(); },
        main_window_, true);

    shortcuts_controller_->register_operation(
        ShortcutActionId::open_playlist, "Open playlist", ShortcutScope::PlaylistView,
        QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O),
        [this]() { playlist_ctl_->load_playlist(); }, main_window_, true);

    shortcuts_controller_->register_operation(
        ShortcutActionId::play_pause, "Play / Pause", ShortcutScope::Application,
        QKeySequence(Qt::Key_Space),
        [this]() {
            if (playback_ctl_->state() == PlayingState::PLAYING) {
                playback_ctl_->pause();
            } else {
                playback_ctl_->play();
            }
        },
        main_window_, true);

    shortcuts_controller_->register_operation(
        ShortcutActionId::open_settings, "Open settings", ShortcutScope::MainWindow,
        QKeySequence(Qt::CTRL | Qt::Key_Comma), [this]() { open_settings_panel(); }, this, true);

    shortcuts_controller_->register_operation(
        ShortcutActionId::stop, "Stop", ShortcutScope::Application, QKeySequence(Qt::Key_S),
        [this]() { playback_ctl_->stop(); }, main_window_, true);

    shortcuts_controller_->register_operation(
        ShortcutActionId::open_search, "Open Search Panel", ShortcutScope::MainWindow,
        QKeySequence(Qt::CTRL | Qt::Key_F), [this]() { open_search_panel(); }, main_window_, true);

    shortcuts_controller_->register_operation(
        ShortcutActionId::show_hide_desktop_lyrics, "Show / Hide Desktop Lyrics",
        ShortcutScope::Application, QKeySequence(Qt::CTRL | Qt::Key_L),
        [this]() {
            auto* desktopLyrics = main_window_->desktop_lyrics_widget();
            if (!desktopLyrics) {
                return;
            }
            if (desktopLyrics->isVisible()) {
                desktopLyrics->hide();
            } else {
                if (desktopLyrics->parentWidget() != nullptr) {
                    desktopLyrics->setParent(nullptr); // 独立窗口(Wayland 安全)
                }
                desktopLyrics->show();
            }
        },
        main_window_, true);

    has_shortcuts_registered_ = true;
}

void PanelCoordinator::ensure_search_panel()
{
    if (search_panel_) {
        return;
    }

    search_panel_ = new SearchPanel(&ConfigManager::get_instance(), main_window_);
    search_panel_->setWindowFlag(Qt::Window, true);
    search_panel_->setAttribute(Qt::WA_DeleteOnClose, true);
    search_panel_->set_search_backend(search_backend_);
    search_backend_->warmup(playlist_ctl_->current_playlist_id());

    connect(playlist_ctl_->view_model(), &QAbstractItemModel::modelReset, search_panel_, [this]() {
        if (search_panel_) {
            const QJsonObject sub_obj =
                ConfigManager::get_instance().read_sub_config(search_panel_->config_sub_key());
            const QByteArray header_state =
                QByteArray::fromBase64(sub_obj.value("header_state").toString().toUtf8());
            search_panel_->apply_header_state_deferred(header_state);
        }
        if (search_backend_) {
            search_backend_->invalidate(PlaylistId{});
            search_backend_->warmup(playlist_ctl_->current_playlist_id());
        }
    });

    // 双击结果:播放列表条目/外部条目直接按路径播放(定位回当前列表)
    connect(search_panel_, &SearchPanel::sgnRequestPlayFile, main_window_,
            [this](const QString& filepath) {
                if (filepath.isEmpty()) {
                    return;
                }
                emit playlist_ctl_->sgn_request_play(filepath);
            });

    // 库级曲目身份(库引用条目兜底):经库解析后播放
    connect(search_panel_, &SearchPanel::sgnRequestPlayTrack, main_window_,
            [this](const TrackId& id) {
                if (id.isNull()) {
                    return;
                }
                const auto lib_track = library_mgr_->track_by_id(id);
                if (!lib_track || lib_track->missing) {
                    return;
                }
                emit playlist_ctl_->sgn_request_play(lib_track->filepath);
            });

    connect(search_panel_, &QObject::destroyed, this, [this]() { search_panel_ = nullptr; });
}
