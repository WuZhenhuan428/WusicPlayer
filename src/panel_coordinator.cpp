#include "panel_coordinator.h"

#include "controller/PlaybackController.h"
#include "controller/PlaylistController.h"
#include "controller/search_backend/in_memory_search_backend.h"
#include "controller/shortcuts_controller.h"
#include "core/ConfigManager/ConfigManager.h"
#include "core/types.h"
#include "model/ShortcutsViewModel/shortcuts_types.hpp"
#include "model/library/library_manager.h"
#include "service/theme_service.h"
#include "view/MainWindow.h"
#include "view/SettingsPanel/SettingsPanel.h"
#include "view/SettingsPanel/ShortcutsPanel/ShortcutsPanel.h"
#include "view/SettingsPanel/ThemeSettingsPage/ThemeSettingsPage.h"
#include "view/SettingsPanel/library_settings_page.h"
#include "view/SettingsPanel/lyrics_setting_panel/lyrics_setting_panel.h"
#include "view/eq_widget/eq_widget.h"
#include "view/search_panel/search_panel.h"

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
    ensureShortcutsController();
}

PanelCoordinator::~PanelCoordinator() = default;

ShortcutsController* PanelCoordinator::shortcutsController()
{
    ensureShortcutsController();
    return shortcuts_controller_;
}

void PanelCoordinator::savePanelConfigs()
{
    ConfigManager& cm = ConfigManager::getInstance();
    if (settings_panel_) {
        cm.writeSubConfig(settings_panel_->configSubKey(), settings_panel_->saveToJson());
    }
    if (shortcuts_panel_) {
        cm.writeSubConfig(shortcuts_panel_->configSubKey(), shortcuts_panel_->saveToJson());
    }
    if (search_panel_) {
        cm.writeSubConfig(search_panel_->configSubKey(), search_panel_->saveToJson());
    }
}

void PanelCoordinator::openSettingsPanel()
{
    ensureSettingsPanel();
    ensureShortcutsPage();

    // 歌词设置页(依赖桌面歌词控件颜色/字体)
    if (!lyrics_settings_panel_) {
        lyrics_settings_panel_ =
            new LyricsSettingPanel(main_window_->desktopLyricsWidget()->getActiveLineColor(),
                                   main_window_->desktopLyricsWidget()->getInactiveLineColor());
        lyrics_settings_panel_->setLineEditText(main_window_->desktopLyricsWidget()->getFont());

        connect(
            lyrics_settings_panel_, &LyricsSettingPanel::sgnActiveColorChanged, this,
            [this](rgb_t rgb) { main_window_->desktopLyricsWidget()->setActiveLineColor(rgb); });
        connect(
            lyrics_settings_panel_, &LyricsSettingPanel::sgnInactiveColorChanged, this,
            [this](rgb_t rgb) { main_window_->desktopLyricsWidget()->setInactiveLineColor(rgb); });
        connect(lyrics_settings_panel_, &LyricsSettingPanel::sgnDisplayModeChanged, this,
                [this](bool is_two_line) {
                    main_window_->desktopLyricsWidget()->setDisplayMode(
                        is_two_line ? DisplayMode::TwoLine : DisplayMode::OneLine);
                });
        connect(lyrics_settings_panel_, &LyricsSettingPanel::sgnFontChanged,
                main_window_->desktopLyricsWidget(), &DesktopLyricsWidget::setLrcFont);

        settings_panel_->registerWidget(lyrics_settings_panel_->getTitleItem(),
                                        lyrics_settings_panel_);
    }

    // 主题设置页
    if (!theme_settings_page_) {
        theme_settings_page_ = new ThemeSettingsPage(theme_service_, settings_panel_);
        settings_panel_->registerWidget(theme_settings_page_->getTitleItem(), theme_settings_page_);
    }

    // 媒体库设置页(watched folders 唯一管理入口 + 添加解析策略配置)
    if (!library_settings_page_) {
        library_settings_page_ = new LibrarySettingsPage(library_mgr_, settings_panel_);
        library_settings_page_->set_add_file_policy(playlist_ctl_->addFilePolicy());
        connect(library_settings_page_, &LibrarySettingsPage::sgnAddFilePolicyChanged, this,
                [this](int policy) {
                    playlist_ctl_->setAddFilePolicy(static_cast<AddFilePolicy>(policy));
                });
        settings_panel_->registerWidget(library_settings_page_->getTitleItem(),
                                        library_settings_page_);
    }

    settings_panel_->show();
    settings_panel_->raise();
    settings_panel_->activateWindow();
}

void PanelCoordinator::openSettingsPanelPage(const QString& title)
{
    openSettingsPanel();
    if (settings_panel_) {
        settings_panel_->switchToPageByTitle(title);
    }
}

void PanelCoordinator::openSearchPanel()
{
    ensureSearchPanel();

    search_panel_->show();
    search_panel_->raise();
    search_panel_->activateWindow();
}

void PanelCoordinator::openEQWidget()
{
    if (!eq_widget_) {
        eq_widget_ =
            new EQWidget(playback_ctl_->gains(), playback_ctl_->isEqEnabled(), false, nullptr);
        eq_widget_->setWindowFlag(Qt::Window, true);
        eq_widget_->setAttribute(Qt::WA_DeleteOnClose);

        connect(eq_widget_, &EQWidget::sgnGainChanged, playback_ctl_,
                &PlaybackController::setGains);
        connect(eq_widget_, &EQWidget::sgnEqEnabledChanged, playback_ctl_,
                &PlaybackController::setEqEnabled);

        // QPointer auto-nulls on destroy; config saved via saveConfig() on exit
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

void PanelCoordinator::ensureSettingsPanel()
{
    if (settings_panel_) {
        return;
    }
    settings_panel_ = new SettingsPanel(&ConfigManager::getInstance());
    settings_panel_->setWindowFlag(Qt::Window, true);
    // 不使用 WA_DeleteOnClose：改为 hide/show 复用，避免销毁/重建循环中的状态不一致
    settings_panel_->installEventFilter(this);
}

void PanelCoordinator::ensureShortcutsController()
{
    if (shortcuts_controller_) {
        return;
    }
    shortcuts_controller_ = new ShortcutsController(this);
    registerDefaultShortcuts();
}

void PanelCoordinator::ensureShortcutsPage()
{
    ensureShortcutsController();

    if (!shortcuts_panel_) {
        shortcuts_panel_ = new ShortcutsPanel(&ConfigManager::getInstance(), settings_panel_);
        shortcuts_panel_->setViewModel(shortcuts_controller_->viewModel());

        connect(shortcuts_panel_, &ShortcutsPanel::sgnDefaultConfig, this, [this]() {
            if (shortcuts_controller_) {
                shortcuts_controller_->resetAllToDefault();
            }
        });

        connect(shortcuts_panel_, &ShortcutsPanel::sgnRestoreConfig, this, [this]() {
            if (!shortcuts_controller_) {
                return;
            }
            QJsonObject sub_obj =
                ConfigManager::getInstance().readSubConfig(shortcuts_controller_->configSubKey());
            QJsonObject root;
            root.insert(shortcuts_controller_->configSubKey(), sub_obj);
            shortcuts_controller_->loadFromJson(root);
        });

        connect(shortcuts_panel_, &ShortcutsPanel::sgnApplyConfig, this,
                []() { ConfigManager::getInstance().saveAll(); });

        settings_panel_->registerWidget(shortcuts_panel_->getListItem(), shortcuts_panel_);
    }
}

void PanelCoordinator::registerDefaultShortcuts()
{
    if (!shortcuts_controller_ || has_shortcuts_registered_) {
        return;
    }

    shortcuts_controller_->setScopeHost(ShortcutScope::Application, main_window_);
    shortcuts_controller_->setScopeHost(ShortcutScope::MainWindow, main_window_);
    shortcuts_controller_->setScopeHost(ShortcutScope::DesktopLyrics, main_window_);

    shortcuts_controller_->registerOperation(
        ShortcutActionId::save_playlist, "Save Playlist", ShortcutScope::PlaylistView,
        QKeySequence(Qt::CTRL | Qt::Key_S), [this]() { playlist_ctl_->savePlaylist(); },
        main_window_, true);

    shortcuts_controller_->registerOperation(
        ShortcutActionId::open_file, "Open File", ShortcutScope::PlaylistView,
        QKeySequence(Qt::CTRL | Qt::Key_O), [this]() { playlist_ctl_->importFiles(); },
        main_window_, true);

    shortcuts_controller_->registerOperation(
        ShortcutActionId::open_playlist, "Open playlist", ShortcutScope::PlaylistView,
        QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O), [this]() { playlist_ctl_->loadPlaylist(); },
        main_window_, true);

    shortcuts_controller_->registerOperation(
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

    shortcuts_controller_->registerOperation(
        ShortcutActionId::open_settings, "Open settings", ShortcutScope::MainWindow,
        QKeySequence(Qt::CTRL | Qt::Key_Comma), [this]() { openSettingsPanel(); }, this, true);

    shortcuts_controller_->registerOperation(
        ShortcutActionId::stop, "Stop", ShortcutScope::Application, QKeySequence(Qt::Key_S),
        [this]() { playback_ctl_->stop(); }, main_window_, true);

    shortcuts_controller_->registerOperation(
        ShortcutActionId::open_search, "Open Search Panel", ShortcutScope::MainWindow,
        QKeySequence(Qt::CTRL | Qt::Key_F), [this]() { openSearchPanel(); }, main_window_, true);

    shortcuts_controller_->registerOperation(
        ShortcutActionId::show_hide_desktop_lyrics, "Show / Hide Desktop Lyrics",
        ShortcutScope::Application, QKeySequence(Qt::CTRL | Qt::Key_L),
        [this]() {
            auto* desktopLyrics = main_window_->desktopLyricsWidget();
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

void PanelCoordinator::ensureSearchPanel()
{
    if (search_panel_) {
        return;
    }

    search_panel_ = new SearchPanel(&ConfigManager::getInstance());
    search_panel_->setWindowFlag(Qt::Window, true);
    search_panel_->setAttribute(Qt::WA_DeleteOnClose, true);
    search_panel_->setSearchBackend(search_backend_);
    search_backend_->warmup(playlist_ctl_->currentPlaylistId());

    connect(playlist_ctl_->viewModel(), &QAbstractItemModel::modelReset, search_panel_, [this]() {
        if (search_panel_) {
            const QJsonObject sub_obj =
                ConfigManager::getInstance().readSubConfig(search_panel_->configSubKey());
            const QByteArray header_state =
                QByteArray::fromBase64(sub_obj.value("header_state").toString().toUtf8());
            search_panel_->applyHeaderStateDeferred(header_state);
        }
        if (search_backend_) {
            search_backend_->invalidate(PlaylistId{});
            search_backend_->warmup(playlist_ctl_->currentPlaylistId());
        }
    });

    // 双击结果:播放列表条目/外部条目直接按路径播放(定位回当前列表)
    connect(search_panel_, &SearchPanel::sgnRequestPlayFile, main_window_,
            [this](const QString& filepath) {
                if (filepath.isEmpty()) {
                    return;
                }
                emit playlist_ctl_->requestPlay(filepath);
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
                emit playlist_ctl_->requestPlay(lib_track->filepath);
            });

    connect(search_panel_, &QObject::destroyed, this, [this]() { search_panel_ = nullptr; });
}
