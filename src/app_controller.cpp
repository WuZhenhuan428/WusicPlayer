#include "app_controller.h"

#include "controller/PlaybackController.h"
#include "controller/PlaylistController.h"
#include "controller/search_backend/in_memory_search_backend.h"
#include "controller/shortcuts_controller.h"
#include "controller/status_bar_controller.h"
#include "core/ConfigManager/ConfigManager.h"
#include "core/ConfigManager/IConfigurable.h"
#include "core/types.h"
#include "model/ShortcutsViewModel/shortcuts_types.hpp"
#include "model/library/library_manager.h"
#include "model/playback_queue/playback_queue_service.h"
#include "model/playlist/playlist_manager.h"
#include "service/library_interaction_service.h"
#include "service/playback_restore_service.h"
#include "service/playback_service.h"
#include "service/tag_writeback_service.h"
#include "service/theme_service.h"
#include "view/MainWindow.h"
#include "view/SettingsPanel/SettingsPanel.h"
#include "view/SettingsPanel/ShortcutsPanel/ShortcutsPanel.h"
#include "view/SettingsPanel/ThemeSettingsPage/ThemeSettingsPage.h"
#include "view/SettingsPanel/library_settings_page.h"
#include "view/SettingsPanel/lyrics_setting_panel/lyrics_setting_panel.h"
#include "view/eq_widget/eq_widget.h"
#include "view/playlist/playlist_widgets.h"
#include "view/search_panel/search_panel.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QDir>
#include <QFileInfo>
#include <QJsonObject>
#include <QKeySequence>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QShortcut>
#include <QStandardPaths>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QTreeView>
#include <format>
#include <string>

AppController::AppController(PlaybackController* playbackController, QObject* parent) :
    QObject(parent), playback_controller_(playbackController),
    playlist_manager_(std::make_unique<PlaylistManager>()),
    playlist_controller_(
        std::make_unique<PlaylistController>(playlist_manager_.get(), nullptr, this)),
    library_manager_(std::make_unique<LibraryManager>()),
    // 搜索面板:搜索当前播放列表(数据库 FTS5 仅媒体库控件使用)
    search_backend_(std::make_unique<InMemorySearchBackend>(playlist_controller_.get())),
    main_window_(std::make_unique<MainWindow>(playback_controller_, playlist_controller_.get())),
    status_bar_controller_(std::make_unique<StatusBarController>(main_window_->statusBar(), this)),
    playback_service_(std::make_unique<PlaybackService>(main_window_.get(), playback_controller_,
                                                        playlist_controller_.get(), this)),
    playback_restore_service_(std::make_unique<PlaybackRestoreService>(playlist_controller_.get(),
                                                                       playback_controller_, this)),
    library_interaction_serivce_(std::make_unique<LibraryInteractionService>(
        main_window_->libraryPanel(), playback_controller_, playlist_controller_.get(), this)),
    tag_writeback_service_(
        std::make_unique<TagWritebackService>(playlist_controller_.get(), playback_controller_,
                                              playlist_manager_.get(), main_window_.get(), this)),
    theme_service_(std::make_unique<ThemeService>(this)),
    playback_queue_service_(std::make_unique<PlaybackQueueService>(this))
{
    // 现在播放队列:注入数据源(积累期,媒体库控件入队即播)
    playback_queue_service_->set_playlist_manager(playlist_manager_.get());
    playback_queue_service_->set_library_manager(library_manager_.get());
    // 媒体库控件(左侧播放列表下方):注入库与队列服务
    main_window_->libraryPanel()->setLibraryManager(library_manager_.get());
    main_window_->libraryPanel()->setPlaybackQueueService(playback_queue_service_.get());

    // 音乐库:注入播放列表(解析曲目引用)、初始化数据库、启动初始扫描
    playlist_manager_->set_library_manager(library_manager_.get());
    const QString data_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(data_dir);
    if (library_manager_->initialize(data_dir + "/library.db")) {
        library_manager_->start_scan();
    }

    ensureShortcutsController();
    initializeConfig();
    playback_restore_service_->restore();
    initializeCoreConnections();
    configureDesktopLyricsWindowRelation();

    connect(main_window_.get(), &MainWindow::sgnOpenSearchPanelRequested, this,
            &AppController::onOpenSearchPanelRequested);
    connect(main_window_.get(), &MainWindow::sgnOpenSettingsPanelRequested, this,
            &AppController::onOpenSettingsPanelRequested);
    connect(main_window_.get(), &MainWindow::sgnShowDesktopLyricsRequested, this,
            &AppController::handleShowDesktopLyricsRequested);

    connect(main_window_.get(), &MainWindow::sgnAboutToClose, this, &AppController::saveConfig);

    connect(qApp, &QCoreApplication::aboutToQuit, this, &AppController::saveConfig);

    this->setup_status_bar_connections();
}

AppController::~AppController() = default;

void AppController::showMainWindow()
{
    if (main_window_) {
        main_window_->show();
    }
}

void AppController::initializeCoreConnections()
{
    PlaylistController* playlistController = playlist_controller_.get();
    PlaybackController* playbackController = playback_controller_;
    LibraryWidget* libraryPanel            = main_window_->libraryPanel();
    SidePanel* sidePanel                   = main_window_->sidePanel();
    // DesktopLyricsWidget* desktopLyrics = main_window_->desktopLyricsWidget();

    playback_service_->bind();
    connect(playback_service_.get(), &PlaybackService::sgnLocateCurrentTrack, this,
            &AppController::locateCurrentTrackInView);

    // 媒体库控件:双击曲目 → 队列入队即播(积累期经 playTrackInUi 播放)
    connect(playback_queue_service_.get(), &PlaybackQueueService::sgn_play_requested, this,
            [this](const QueueItem& item) { main_window_->playTrackInUi(item.filepath); });
    connect(
        libraryPanel, &LibraryWidget::sgnLibraryPlayRequested, this,
        [this](const TrackId& track_id) { playback_queue_service_->play_library_track(track_id); });
    connect(libraryPanel, &LibraryWidget::sgnOpenLibrarySettingsRequested, this, [this]() {
        onOpenSettingsPanelRequested();
        if (settings_panel_) {
            settings_panel_->switchToPageByTitle("Media Library");
        }
    });

    library_interaction_serivce_->bind();
    connect(libraryPanel, &LibraryWidget::sgnTrackPropertyRequested, tag_writeback_service_.get(),
            &TagWritebackService::requestTrackProperty);

    auto* lyricsModel = qobject_cast<WLyricsModel*>(sidePanel->getLyricsPanel()->model());
    connect(playbackController, &PlaybackController::sgnPositionChanged,
            sidePanel->getLyricsPanel(), &WLyricsPanel::ScrollByPosition);
    connect(sidePanel, &SidePanel::sgnDesktopLyricsConfigRequested, this, [this]() {
        onOpenSettingsPanelRequested();
        if (settings_panel_) {
            settings_panel_->switchToPageByTitle("Lyrics");
        }
    });

    if (lyricsModel) {
        lyrics_follow_conn_ = connect(playbackController, &PlaybackController::sgnPositionChanged,
                                      lyricsModel, &WLyricsModel::setCurrentPosition);
        connect(lyricsModel, &WLyricsModel::currentLineChanged, this,
                [this](const QString& currText, const QString& nextText) {
                    main_window_->desktopLyricsWidget()->setLrcLine(currText, nextText);
                });
        connect(lyricsModel, &WLyricsModel::currentLineChanged, this,
                [this]() { main_window_->desktopLyricsWidget()->updateLineColor(); });
        connect(lyricsModel, &WLyricsModel::sgnUseTimelineFollow, this,
                [this, playbackController, lyricsModel](bool enable) {
                    if (enable) {
                        if (!lyrics_follow_conn_) {
                            lyrics_follow_conn_ =
                                connect(playbackController, &PlaybackController::sgnPositionChanged,
                                        lyricsModel, &WLyricsModel::setCurrentPosition);
                        }
                        return;
                    }

                    if (lyrics_follow_conn_) {
                        disconnect(lyrics_follow_conn_);
                        lyrics_follow_conn_ = QMetaObject::Connection{};
                    }
                });
    }

    connect(main_window_.get(), &MainWindow::sgnImportFilesRequested, this,
            [playlistController]() { playlistController->importFiles(); });
    connect(main_window_.get(), &MainWindow::sgnImportFolderRequested, this,
            [playlistController]() { playlistController->importDir(); });
    connect(main_window_.get(), &MainWindow::sgnCreatePlaylistRequested, playlistController,
            &PlaylistController::createNewPlaylist);
    connect(main_window_.get(), &MainWindow::sgnLoadPlaylist, playlistController,
            &PlaylistController::loadPlaylist);
    connect(main_window_.get(), &MainWindow::sgnCopyPlaylistRequested, this,
            [playlistController]() { playlistController->copyPlaylist(); });
    connect(main_window_.get(), &MainWindow::sgnRenamePlaylistRequested, this,
            [playlistController]() { playlistController->renamePlaylist(); });
    connect(main_window_.get(), &MainWindow::sgnRemovePlaylistRequested, this,
            [playlistController]() { playlistController->removePlaylist(); });
    connect(main_window_.get(), &MainWindow::sgnSavePlaylistRequested, this,
            [playlistController]() { playlistController->savePlaylist(); });
    connect(main_window_.get(), &MainWindow::sgnSetSortRuleRequested, this,
            &AppController::handleSetSortRuleRequested);
    connect(main_window_.get(), &MainWindow::sgnInsertColumnRequested, this,
            &AppController::handleInsertColumnRequested);
    connect(main_window_.get(), &MainWindow::sgnRemoveColumnRequested, this,
            &AppController::handleRemoveColumnRequested);
    connect(main_window_.get(), &MainWindow::sgnShowAboutMessagebox, this,
            &AppController::handleShowAboutMessagebox);
    connect(main_window_.get(), &MainWindow::sgnOpenEQWidgetRequested, this,
            &AppController::handleOpenEQRequested);

    auto* locateShortcut = new QShortcut(QKeySequence(Qt::Key_Tab), libraryPanel->songTreeView());
    locateShortcut->setContext(Qt::WidgetShortcut);
    connect(locateShortcut, &QShortcut::activated, this, [this]() { locateCurrentTrackInView(); });
}

void AppController::locateCurrentTrackInView()
{
    auto* playlistController = playlist_controller_.get();
    auto* libraryPanel       = main_window_ ? main_window_->libraryPanel() : nullptr;
    if (!playlistController || !libraryPanel) {
        return;
    }

    auto* view  = libraryPanel->songTreeView();
    auto* model = playlistController->viewModel();
    if (!view || !model) {
        return;
    }

    const QModelIndex index = model->getCurrentTrackIndex();
    if (!index.isValid()) {
        return;
    }

    view->scrollTo(index.siblingAtColumn(1), QAbstractItemView::PositionAtCenter);
    view->setCurrentIndex(index.siblingAtColumn(1));
}

void AppController::handleSetSortRuleRequested()
{
    auto* playlistController = playlist_controller_.get();
    WSortTypeSetDialog dialog;
    if (dialog.exec() == QDialog::Accepted) {
        QString input = dialog.getText();
        playlistController->viewModel()->setSortExpression(input);
    }
}

void AppController::handleInsertColumnRequested()
{
    auto* playlistController = playlist_controller_.get();
    WInsertColumnDialog dialog;
    int maxIndex = playlistController->viewModel()->getColumns().size();
    dialog.setMaxIndex(maxIndex);
    dialog.setIndex(1);
    int result = dialog.exec();
    if (result == QDialog::Accepted) {
        TableColumn column = dialog.getRule();
        int index          = dialog.index();
        playlistController->viewModel()->insertColumn(index, column);
    }
}

void AppController::handleRemoveColumnRequested()
{
    auto* playlistController = playlist_controller_.get();
    WColumnIndexDialog dialog(QObject::tr("Remove column"),
                              QObject::tr("Input the column index except 0"), main_window_.get());
    int maxIndex = playlistController->viewModel()->getColumns().size() - 1;
    dialog.setMaxIndex(maxIndex);
    dialog.setIndex(1);
    if (dialog.exec() == QDialog::Accepted) {
        playlistController->viewModel()->removeColumn(dialog.index());
    }
}

void AppController::handleShowAboutMessagebox()
{
    QMessageBox* msg = new QMessageBox(main_window_.get());
    msg->setWindowTitle("About");
    msg->setText("This is a ABOUT message box.");
    msg->setIcon(QMessageBox::Information);
    msg->setStandardButtons(QMessageBox::Ok);
    msg->show();
    msg->setAttribute(Qt::WA_DeleteOnClose);
}

void AppController::handleShowDesktopLyricsRequested()
{
    auto* desktopLyrics = main_window_->desktopLyricsWidget();
    if (desktopLyrics) {
        configureDesktopLyricsWindowRelation();
        desktopLyrics->show();
    }
}

void AppController::configureDesktopLyricsWindowRelation()
{
    auto* desktopLyrics = main_window_ ? main_window_->desktopLyricsWidget() : nullptr;
    if (!desktopLyrics) {
        return;
    }

    if (desktopLyrics->parentWidget() != nullptr) {
        desktopLyrics->setParent(nullptr);
    }
}

void AppController::initializeConfig()
{
    ConfigManager& cm = ConfigManager::getInstance();
    if (playback_controller_) {
        cm.registerModule(playback_controller_);
        qDebug() << "[CONFIG] register playback controller";
    }
    if (playlist_controller_) {
        cm.registerModule(playlist_controller_.get());
        qDebug() << "[CONFIG] register playlist controller";
    }
    if (main_window_) {
        cm.registerModule(main_window_.get());
        qDebug() << "[CONFIG] register main window";
        if (main_window_->libraryPanel()) {
            cm.registerModule(main_window_->libraryPanel());
            qDebug() << "[CONFIG] register library panel";
            if (main_window_->libraryPanel()->libraryBrowser()) {
                cm.registerModule(main_window_->libraryPanel()->libraryBrowser());
                qDebug() << "[CONFIG] register library browser";
            }
        }
        if (main_window_->desktopLyricsWidget()) {
            cm.registerModule(main_window_->desktopLyricsWidget());
            qDebug() << "[CONFIG] register desktop lyrics widget";
        }
    }
    if (shortcuts_controller_) {
        cm.registerModule(shortcuts_controller_);
        qDebug() << "[CONFIG] register shortcuts controller";
    }
    cm.loadAll();

    // misc: dependent data from other module,
    // but not sufficient to create interfaces seprately
    // current:
    //      WControlBar -> m_btn_mode->icon
    main_window_.get()->controlBarWidget()->setPlayMode(playlist_controller_->playMode());
}

void AppController::saveConfig()
{
    if (!main_window_) {
        return;
    }

    if (has_saved_config_on_exit_) {
        return;
    }
    has_saved_config_on_exit_ = true;

    ConfigManager& cm         = ConfigManager::getInstance();
    if (settings_panel_) {
        cm.writeSubConfig(settings_panel_->configSubKey(), settings_panel_->saveToJson());
    }
    if (shortcuts_panel_) {
        cm.writeSubConfig(shortcuts_panel_->configSubKey(), shortcuts_panel_->saveToJson());
    }
    if (search_panel_) {
        cm.writeSubConfig(search_panel_->configSubKey(), search_panel_->saveToJson());
    }
    cm.saveAll();
}

void AppController::handleOpenEQRequested()
{
    if (!eq_widget_) {
        eq_widget_ =
            new EQWidget(playback_controller_->gains(), playback_controller_->isEqEnabled(), false,
                         nullptr // no parent — WA_DeleteOnClose will clean up
            );
        eq_widget_->setWindowFlag(Qt::Window, true);
        eq_widget_->setAttribute(Qt::WA_DeleteOnClose);

        connect(eq_widget_, &EQWidget::sgnGainChanged, playback_controller_,
                &PlaybackController::setGains);
        connect(eq_widget_, &EQWidget::sgnEqEnabledChanged, playback_controller_,
                &PlaybackController::setEqEnabled);

        connect(eq_widget_, &QObject::destroyed, this, [this]() {
            Q_UNUSED(this);
            // QPointer auto-nulls; config is saved on app close via saveConfig().
        });
    }

    eq_widget_->show();
    eq_widget_->raise();
    eq_widget_->activateWindow();
}

void AppController::onOpenSettingsPanelRequested()
{
    ensureSettingsPanel();
    ensureShortcutsPage();
    // ensure lyrics panel
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
        theme_settings_page_ = new ThemeSettingsPage(theme_service_.get(), settings_panel_);
        settings_panel_->registerWidget(theme_settings_page_->getTitleItem(), theme_settings_page_);
    }

    // 媒体库设置页(watched folders 唯一管理入口 + 添加解析策略配置)
    if (!library_settings_page_) {
        library_settings_page_ = new LibrarySettingsPage(library_manager_.get(), settings_panel_);
        library_settings_page_->set_add_file_policy(playlist_controller_->addFilePolicy());
        connect(library_settings_page_, &LibrarySettingsPage::sgnAddFilePolicyChanged, this,
                [this](int policy) {
                    playlist_controller_->setAddFilePolicy(static_cast<AddFilePolicy>(policy));
                });
        settings_panel_->registerWidget(library_settings_page_->getTitleItem(),
                                        library_settings_page_);
    }

    settings_panel_->show();
    settings_panel_->raise();
    settings_panel_->activateWindow();
}

void AppController::onOpenSearchPanelRequested()
{
    ensureSearchPanel();

    search_panel_->show();
    search_panel_->raise();
    search_panel_->activateWindow();
}

void AppController::ensureSettingsPanel()
{
    if (settings_panel_) {
        return;
    }

    settings_panel_ = new SettingsPanel(&ConfigManager::getInstance());
    settings_panel_->setWindowFlag(Qt::Window, true);
    // 不使用 WA_DeleteOnClose：改为 hide/show 复用，避免销毁/重建循环中的状态不一致

    // 面板隐藏后激活主窗口，防止菜单栏焦点丢失导致不响应点击
    settings_panel_->installEventFilter(this);
}

void AppController::ensureShortcutsPage()
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

void AppController::ensureShortcutsController()
{
    if (shortcuts_controller_) {
        return;
    }

    shortcuts_controller_ = new ShortcutsController(this);
    registerDefaultShortcuts();
}

void AppController::registerDefaultShortcuts()
{
    if (!shortcuts_controller_ || has_shortcuts_registered_) {
        return;
    }

    shortcuts_controller_->setScopeHost(ShortcutScope::Application, main_window_.get());
    shortcuts_controller_->setScopeHost(ShortcutScope::MainWindow, main_window_.get());
    shortcuts_controller_->setScopeHost(ShortcutScope::DesktopLyrics, main_window_.get());

    shortcuts_controller_->registerOperation(
        ShortcutActionId::save_playlist, "Save Playlist", ShortcutScope::PlaylistView,
        QKeySequence(Qt::CTRL | Qt::Key_S),
        [this]() { playlist_controller_.get()->savePlaylist(); }, main_window_.get(), true);

    shortcuts_controller_->registerOperation(
        ShortcutActionId::open_file, "Open File", ShortcutScope::PlaylistView,
        QKeySequence(Qt::CTRL | Qt::Key_O), [this]() { playlist_controller_.get()->importFiles(); },
        main_window_.get(), true);

    shortcuts_controller_->registerOperation(
        ShortcutActionId::open_playlist, "Open playlist", ShortcutScope::PlaylistView,
        QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O),
        [this]() { playlist_controller_.get()->loadPlaylist(); }, main_window_.get(), true);

    shortcuts_controller_->registerOperation(
        ShortcutActionId::play_pause, "Play / Pause", ShortcutScope::Application,
        QKeySequence(Qt::Key_Space),
        [this]() {
            if (playback_controller_->state() == PlayingState::PLAYING) {
                playback_controller_->pause();
            } else {
                playback_controller_->play();
            }
        },
        main_window_.get(), true);

    shortcuts_controller_->registerOperation(
        ShortcutActionId::open_settings, "Open settings", ShortcutScope::MainWindow,
        QKeySequence(Qt::CTRL | Qt::Key_Comma), [this]() { onOpenSettingsPanelRequested(); }, this,
        true);

    shortcuts_controller_->registerOperation(
        ShortcutActionId::stop, "Stop", ShortcutScope::Application, QKeySequence(Qt::Key_S),
        [this]() { playback_controller_->stop(); }, main_window_.get(), true);

    shortcuts_controller_->registerOperation(
        ShortcutActionId::open_search, "Open Search Panel", ShortcutScope::MainWindow,
        QKeySequence(Qt::CTRL | Qt::Key_F), [this]() { onOpenSearchPanelRequested(); },
        main_window_.get(), true);

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
                configureDesktopLyricsWindowRelation();
                desktopLyrics->show();
            }
        },
        main_window_.get(), true);

    has_shortcuts_registered_ = true;
}

void AppController::ensureSearchPanel()
{
    if (search_panel_) {
        return;
    }

    search_panel_ = new SearchPanel(&ConfigManager::getInstance());
    search_panel_->setWindowFlag(Qt::Window, true);
    search_panel_->setAttribute(Qt::WA_DeleteOnClose, true);
    search_panel_->setSearchBackend(search_backend_.get());
    search_backend_->warmup(playlist_controller_->currentPlaylistId());

    connect(playlist_controller_->viewModel(), &QAbstractItemModel::modelReset, search_panel_,
            [this]() {
                if (search_panel_) {
                    const QJsonObject sub_obj =
                        ConfigManager::getInstance().readSubConfig(search_panel_->configSubKey());
                    const QByteArray header_state =
                        QByteArray::fromBase64(sub_obj.value("header_state").toString().toUtf8());
                    search_panel_->applyHeaderStateDeferred(header_state);
                }
                if (search_backend_) {
                    search_backend_->invalidate(PlaylistId{});
                    search_backend_->warmup(playlist_controller_->currentPlaylistId());
                }
            });

    // 双击结果:播放列表条目/外部条目直接按路径播放(定位回当前列表)
    connect(search_panel_, &SearchPanel::sgnRequestPlayFile, main_window_.get(),
            [this](const QString& filepath) {
                if (filepath.isEmpty())
                    return;
                emit playlist_controller_->requestPlay(filepath);
            });

    // 库级曲目身份(库引用条目兜底):经库解析后播放
    connect(search_panel_, &SearchPanel::sgnRequestPlayTrack, main_window_.get(),
            [this](const TrackId& id) {
                if (id.isNull())
                    return;
                const auto lib_track = library_manager_->track_by_id(id);
                if (!lib_track || lib_track->missing)
                    return;
                // 库曲目不在播放列表中,直接按路径播放
                emit playlist_controller_->requestPlay(lib_track->filepath);
            });

    connect(search_panel_, &QObject::destroyed, this, [this]() { search_panel_ = nullptr; });
}

bool AppController::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == settings_panel_ && event->type() == QEvent::Hide) {
        // 设置面板关闭后激活主窗口，防止菜单栏焦点丢失
        if (main_window_) {
            main_window_->activateWindow();
            main_window_->raise();
        }
    }
    return QObject::eventFilter(obj, event);
}

void AppController::setup_status_bar_connections()
{
    status_bar_controller_->register_item("play_state");
    connect(playback_controller_, &PlaybackController::sgnPlaybackStateChanged, this,
            [this](PlayingState state) {
                QString hint = "";
                switch (state) {
                case PlayingState::PAUSE:
                    hint = "PAUSE";
                    break;
                case PlayingState::PLAYING:
                    hint = "PLAYING";
                    break;
                case PlayingState::STOP:
                    hint = "STOP";
                    break;
                }
                status_bar_controller_->update_item_by_id("play_state", hint);
            });

    // NOTE: 通过cacheLoadFinished信号，确保播放列表已经创建完成，防止悬垂引用
    connect(
        playlist_controller_.get(), &PlaylistController::cacheLoadFinished, this,
        [this]() {
            // 播放列表可能为空(如删除最后一个列表),需空指针保护
            auto show_track_count = [this]() {
                const auto pl = playlist_controller_->current_playlist();
                const std::string tracks =
                    pl ? std::format("{} track(s)", pl->track_count()) : "0 track(s)";
                status_bar_controller_->update_item_by_id("playlist_track_num", tracks.c_str());
            };
            {
                const auto pl = playlist_controller_->current_playlist();
                const std::string tracks =
                    pl ? std::format("{} track(s)", pl->track_count()) : "0 track(s)";
                status_bar_controller_->register_item("playlist_track_num", tracks.c_str());
            }
            // 双击列表项目时触发 &LibraryWidget::sgnSwitchPlaylist
            // CRUD 时触发 `&PlaylistController::playlistChanged`
            connect(playlist_controller_.get(), &PlaylistController::playlistChanged, this,
                    show_track_count);
            connect(main_window_.get()->libraryPanel(), &LibraryWidget::sgnSwitchPlaylist, this,
                    show_track_count);
        },
        Qt::SingleShotConnection);
}
