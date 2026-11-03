#include "panel_coordinator.h"

#include "app_context.h"
#include "controller/playback_controller.h"
#include "controller/playlist_controller.h"
#include "controller/plugin_controller/plugin_controller.h"
#include "controller/search_backend/in_memory_search_backend.h"
#include "controller/shortcuts_controller.h"
#include "core/config_manager/config_manager.h"
#include "core/config_manager/language_settings.h"
#include "core/types.h"
#include "model/library/library_manager.h"
#include "model/shortcuts_view_model/shortcuts_types.hpp"
// #include "service/theme_service.h"
#include "view/dialogs/log_viewer_dialog.h"
#include "view/eq_widget/eq_widget.h"
#include "view/main_window.h"
#include "view/search_panel/search_panel.h"
#include "view/settings_panel/language_settings_page.h"
#include "view/settings_panel/library_settings_page.h"
#include "view/settings_panel/lyrics_setting_panel/lyrics_setting_panel.h"
#include "view/settings_panel/plugin_panel/plugin_panel.h"
#include "view/settings_panel/settings_panel.h"
#include "view/settings_panel/shortcuts_panel/shortcuts_panel.h"
#include "view/settings_panel/theme_settings_page/theme_settings_page.h"

#include <QAbstractItemModel>
#include <QEvent>
#include <QJsonObject>
#include <QKeySequence>
#include <QListWidgetItem>
#include <QShortcut>

PanelCoordinator::PanelCoordinator(AppContext& ctx, QObject* parent) : QObject(parent), ctx_(ctx)
{
    ensure_shortcuts_controller();
}

ShortcutsController* PanelCoordinator::shortcuts_controller()
{
    ensure_shortcuts_controller();
    return shortcuts_controller_;
}

void PanelCoordinator::save_panel_configs()
{
    ConfigManager& cm = ConfigManager::get_instance();
    if (settings_panel_) {
        cm.write_sub_config(SettingsPanel::config_sub_key(), settings_panel_->save_to_json());
    }
    if (shortcuts_panel_) {
        cm.write_sub_config(ShortcutsPanel::config_sub_key(), shortcuts_panel_->save_to_json());
    }
    if (search_panel_) {
        cm.write_sub_config(SearchPanel::config_sub_key(), search_panel_->save_to_json());
    }
}

void PanelCoordinator::open_settings_panel()
{
    ensure_settings_panel();
    ensure_shortcuts_page();

    // 歌词设置页(依赖桌面歌词控件颜色/字体)
    auto& main_window = this->ctx_.main_window;

    if (!lyrics_settings_panel_) {
        lyrics_settings_panel_ =
            new LyricsSettingPanel(main_window->desktop_lyrics_widget()->get_active_line_color(),
                                   main_window->desktop_lyrics_widget()->get_inactive_line_color());
        lyrics_settings_panel_->set_line_edit_text(
            main_window->desktop_lyrics_widget()->get_font());

        connect(lyrics_settings_panel_, &LyricsSettingPanel::sgn_active_color_changed, this,
                [&main_window](rgb_t rgb) {
                    main_window->desktop_lyrics_widget()->set_active_line_color(rgb);
                });
        connect(lyrics_settings_panel_, &LyricsSettingPanel::sgn_inactive_color_changed, this,
                [&main_window](rgb_t rgb) {
                    main_window->desktop_lyrics_widget()->set_inactive_line_color(rgb);
                });
        connect(lyrics_settings_panel_, &LyricsSettingPanel::sgn_display_mode_changed, this,
                [&main_window](bool is_two_line) {
                    main_window->desktop_lyrics_widget()->set_display_mode(
                        is_two_line ? DisplayMode::TwoLine : DisplayMode::OneLine);
                });
        connect(lyrics_settings_panel_, &LyricsSettingPanel::sgn_font_changed,
                main_window->desktop_lyrics_widget(), &DesktopLyricsWidget::set_lrc_font);

        settings_panel_->register_widget(lyrics_settings_panel_->get_title_item(),
                                         lyrics_settings_panel_);
    }

    // 主题设置页
    if (!theme_settings_page_) {
        theme_settings_page_ = new ThemeSettingsPage(this->ctx_.theme_service, settings_panel_);
        settings_panel_->register_widget(theme_settings_page_->get_title_item(),
                                         theme_settings_page_);
    }

    // 媒体库设置页(watched folders 唯一管理入口 + 添加解析策略配置)
    if (!library_settings_page_) {
        library_settings_page_ =
            new LibrarySettingsPage(this->ctx_.library_manager, settings_panel_);
        library_settings_page_->set_add_file_policy(
            this->ctx_.playlist_controller->add_file_policy());
        connect(library_settings_page_, &LibrarySettingsPage::sgn_add_file_policy_changed, this,
                [this](int policy) {
                    this->ctx_.playlist_controller->set_add_file_policy(
                        static_cast<AddFilePolicy>(policy));
                });
        settings_panel_->register_widget(library_settings_page_->get_title_item(),
                                         library_settings_page_);
    }

    auto& plugin_controller = this->ctx_.plugin_controller;
    if (!plugin_panel_) {
        plugin_panel_ = new PluginPanel(plugin_controller->descriptors(), main_window);
        plugin_panel_->setWindowFlag(Qt::Window, true);

        connect(plugin_panel_, &PluginPanel::sgn_request_refresh_plugin, this,
                [this]() { plugin_panel_->refresh(this->ctx_.plugin_controller->descriptors()); });
        connect(plugin_panel_, &PluginPanel::sgn_request_import_plugin, plugin_controller,
                &PluginController::import);
        connect(plugin_panel_, &PluginPanel::sgn_request_remove_plugin, plugin_controller,
                &PluginController::remove);
        connect(plugin_controller, &PluginController::sgn_data_updated, this,
                [this]() { plugin_panel_->refresh(this->ctx_.plugin_controller->descriptors()); });
        settings_panel_->register_widget(plugin_panel_->get_title_item(), plugin_panel_);
    }
    // 语言设置页(English / 简体中文, 切换后重启生效)
    if (!language_settings_page_) {
        language_settings_page_ =
            new LanguageSettingsPage(&LanguageSettings::instance(), settings_panel_);
        settings_panel_->register_widget(language_settings_page_->get_title_item(),
                                         language_settings_page_);
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

void PanelCoordinator::open_log_viewer()
{
    ensure_log_viewer();
    log_viewer_->show();
    log_viewer_->raise();
    log_viewer_->activateWindow();
}

void PanelCoordinator::ensure_log_viewer()
{
    if (log_viewer_) {
        return;
    }
    if (!this->ctx_.log_sink_gui) {
        return;
    }
    log_viewer_ = new LogViewerDialog(this->ctx_.log_sink_gui);
    log_viewer_->setWindowFlag(Qt::Window, true);
    log_viewer_->setAttribute(Qt::WA_DeleteOnClose, true);
    connect(log_viewer_, &QObject::destroyed, this, [this]() { log_viewer_ = nullptr; });
}

void PanelCoordinator::open_eq_widget()
{
    auto& main_window = this->ctx_.main_window;
    if (!eq_widget_) {
        eq_widget_ = new EQWidget(this->ctx_, main_window);
        eq_widget_->setWindowFlag(Qt::Window, true);
        eq_widget_->setAttribute(Qt::WA_DeleteOnClose);

        // “跳转至设置页面” → 插件管理页
        connect(eq_widget_, &EQWidget::sgn_open_settings_requested, this,
                [this]() { this->open_settings_panel_page(QStringLiteral("Plugins")); });

        // QPointer auto-nulls on destroy; config saved via save_config() on exit
        connect(eq_widget_, &QObject::destroyed, this, []() {});
    }

    eq_widget_->show();
    eq_widget_->raise();
    eq_widget_->activateWindow();
}

bool PanelCoordinator::eventFilter(QObject* obj, QEvent* event)
{
    auto& main_window = this->ctx_.main_window;
    if (obj == settings_panel_ && event->type() == QEvent::Hide) {
        // 设置面板关闭后激活主窗口,防止菜单栏焦点丢失
        if (main_window) {
            main_window->activateWindow();
            main_window->raise();
        }
    }
    return QObject::eventFilter(obj, event);
}

void PanelCoordinator::ensure_settings_panel()
{
    if (settings_panel_) {
        return;
    }
    settings_panel_ = new SettingsPanel(&ConfigManager::get_instance(), this->ctx_.main_window);
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

        connect(shortcuts_panel_, &ShortcutsPanel::sgn_default_config, this, [this]() {
            if (shortcuts_controller_) {
                shortcuts_controller_->reset_all_to_default();
            }
        });

        connect(shortcuts_panel_, &ShortcutsPanel::sgn_restore_config, this, [this]() {
            if (!shortcuts_controller_) {
                return;
            }
            QJsonObject sub_obj =
                ConfigManager::read_sub_config(shortcuts_controller_->config_sub_key());
            QJsonObject root;
            root.insert(shortcuts_controller_->config_sub_key(), sub_obj);
            shortcuts_controller_->load_from_json(root);
        });

        connect(shortcuts_panel_, &ShortcutsPanel::sgn_apply_config, this,
                []() { ConfigManager::get_instance().save_all(); });

        settings_panel_->register_widget(shortcuts_panel_->get_list_item(), shortcuts_panel_);
    }
}

void PanelCoordinator::register_default_shortcuts()
{
    if (!shortcuts_controller_ || has_shortcuts_registered_) {
        return;
    }
    auto& main_window  = this->ctx_.main_window;
    auto& playlist_ctl = this->ctx_.playlist_controller;
    auto& playback_ctl = this->ctx_.playback_controller;

    shortcuts_controller_->set_scope_host(ShortcutScope::Application, main_window);
    shortcuts_controller_->set_scope_host(ShortcutScope::MainWindow, main_window);
    shortcuts_controller_->set_scope_host(ShortcutScope::DesktopLyrics, main_window);

    shortcuts_controller_->register_operation(
        ShortcutActionId::SavePlaylist, tr("Save Playlist"), ShortcutScope::PlaylistView,
        QKeySequence(Qt::CTRL | Qt::Key_S), [&playlist_ctl]() { playlist_ctl->save_playlist(); },
        main_window, true);

    shortcuts_controller_->register_operation(
        ShortcutActionId::OpenFile, tr("Open File"), ShortcutScope::PlaylistView,
        QKeySequence(Qt::CTRL | Qt::Key_O), [&playlist_ctl]() { playlist_ctl->import_files(); },
        main_window, true);

    shortcuts_controller_->register_operation(
        ShortcutActionId::OpenPlaylist, tr("Open playlist"), ShortcutScope::PlaylistView,
        QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O),
        [playlist_ctl]() { playlist_ctl->load_playlist(); }, main_window, true);

    shortcuts_controller_->register_operation(
        ShortcutActionId::PlayPause, tr("Play / Pause"), ShortcutScope::Application,
        QKeySequence(Qt::Key_Space),
        [&playback_ctl]() {
            if (playback_ctl->state() == PlayingState::Playing) {
                playback_ctl->pause();
            } else {
                playback_ctl->play();
            }
        },
        main_window, true);

    shortcuts_controller_->register_operation(
        ShortcutActionId::OpenSettings, tr("Open settings"), ShortcutScope::MainWindow,
        QKeySequence(Qt::CTRL | Qt::Key_Comma), [this]() { open_settings_panel(); }, this, true);

    shortcuts_controller_->register_operation(
        ShortcutActionId::Stop, tr("Stop"), ShortcutScope::Application, QKeySequence(Qt::Key_S),
        [&playback_ctl]() { playback_ctl->stop(); }, main_window, true);

    shortcuts_controller_->register_operation(
        ShortcutActionId::OpenSearch, tr("Open Search Panel"), ShortcutScope::MainWindow,
        QKeySequence(Qt::CTRL | Qt::Key_F), [this]() { open_search_panel(); }, main_window, true);

    shortcuts_controller_->register_operation(
        ShortcutActionId::ShowHideDesktopLyrics, tr("Show / Hide Desktop Lyrics"),
        ShortcutScope::Application, QKeySequence(Qt::CTRL | Qt::Key_L),
        [&main_window]() {
            auto* desktop_lyrics = main_window->desktop_lyrics_widget();
            if (!desktop_lyrics) {
                return;
            }
            if (desktop_lyrics->isVisible()) {
                desktop_lyrics->hide();
            } else {
                if (desktop_lyrics->parentWidget() != nullptr) {
                    desktop_lyrics->setParent(nullptr); // 独立窗口(Wayland 安全)
                }
                desktop_lyrics->show();
            }
        },
        main_window, true);

    has_shortcuts_registered_ = true;
}

void PanelCoordinator::ensure_search_panel()
{
    if (search_panel_) {
        return;
    }

    auto& main_window    = this->ctx_.main_window;
    auto& playlist_ctl   = this->ctx_.playlist_controller;
    auto& search_backend = this->ctx_.in_memory_search_backend;

    this->search_panel_  = new SearchPanel(&ConfigManager::get_instance(), main_window);
    this->search_panel_->setWindowFlag(Qt::Window, true);
    this->search_panel_->setAttribute(Qt::WA_DeleteOnClose, true);
    this->search_panel_->set_search_backend(search_backend);
    search_backend->warmup(playlist_ctl->current_playlist_id());

    connect(playlist_ctl->view_model(), &QAbstractItemModel::modelReset, search_panel_, [this]() {
        auto& search_backend = this->ctx_.in_memory_search_backend;
        if (search_panel_) {
            const QJsonObject sub_obj =
                ConfigManager::read_sub_config(SearchPanel::config_sub_key());
            const QByteArray header_state =
                QByteArray::fromBase64(sub_obj.value("header_state").toString().toUtf8());
            search_panel_->apply_header_state_deferred(header_state);
        }
        if (search_backend) {
            search_backend->invalidate(PlaylistId{});
            search_backend->warmup(this->ctx_.playlist_controller->current_playlist_id());
        }
    });

    // 双击结果:播放列表条目/外部条目直接按路径播放(定位回当前列表)
    connect(search_panel_, &SearchPanel::sgn_request_play_file, main_window,
            [this](const QString& filepath) {
                if (filepath.isEmpty()) {
                    return;
                }
                emit this->ctx_.playlist_controller->sgn_request_play(filepath);
            });

    // 库级曲目身份(库引用条目兜底):经库解析后播放
    connect(search_panel_, &SearchPanel::sgn_request_play_track, main_window,
            [this](const TrackId& id) {
                if (id.is_null()) {
                    return;
                }
                const auto lib_track = this->ctx_.library_manager->track_by_id(id);
                if (!lib_track || lib_track->missing) {
                    return;
                }
                emit this->ctx_.playlist_controller->sgn_request_play(lib_track->filepath);
            });

    connect(search_panel_, &QObject::destroyed, this, [this]() { search_panel_ = nullptr; });
}
