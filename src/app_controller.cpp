#include "app_controller.h"

#include "controller/PlaybackController.h"
#include "controller/PlaylistController.h"
#include "controller/search_backend/in_memory_search_backend.h"
#include "controller/shortcuts_controller.h"
#include "controller/status_bar_controller.h"
#include "core/ConfigManager/ConfigManager.h"
#include "core/ConfigManager/IConfigurable.h"
#include "core/types.h"
#include "model/library/library_manager.h"
#include "model/playback_queue/playback_queue_service.h"
#include "model/playlist/playlist_manager.h"
#include "panel_coordinator.h"
#include "service/library_interaction_service.h"
#include "service/playback_restore_service.h"
#include "service/playback_service.h"
#include "service/tag_writeback_service.h"
#include "service/theme_service.h"
#include "view/MainWindow.h"
#include "view/playlist/playlist_widgets.h"

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
        main_window_->playlistTreeWidget(), main_window_->songTableView(), playback_controller_,
        playlist_controller_.get(), this)),
    tag_writeback_service_(
        std::make_unique<TagWritebackService>(playlist_controller_.get(), playback_controller_,
                                              playlist_manager_.get(), main_window_.get(), this)),
    theme_service_(std::make_unique<ThemeService>(this)),
    playback_queue_service_(std::make_unique<PlaybackQueueService>(this)),
    // 面板编排:设置/搜索/EQ/快捷键(构造时注册默认快捷键)
    panel_coordinator_(std::make_unique<PanelCoordinator>(
        main_window_.get(), playback_controller_, playlist_controller_.get(),
        library_manager_.get(), theme_service_.get(), search_backend_.get(), this))
{
    // 现在播放队列:注入数据源(积累期,媒体库控件入队即播)
    playback_queue_service_->set_playlist_manager(playlist_manager_.get());
    playback_queue_service_->set_library_manager(library_manager_.get());
    // 媒体库控件(左侧播放列表下方):注入库与队列服务
    main_window_->libraryBrowser()->set_library_manager(library_manager_.get());
    main_window_->libraryBrowser()->set_playback_queue_service(playback_queue_service_.get());

    // 音乐库:注入播放列表(解析曲目引用)、初始化数据库、启动初始扫描
    playlist_manager_->set_library_manager(library_manager_.get());
    const QString data_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(data_dir);
    if (library_manager_->initialize(data_dir + "/library.db")) {
        library_manager_->start_scan();
    }

    initializeConfig();
    playback_restore_service_->restore();
    initializeCoreConnections();
    configureDesktopLyricsWindowRelation();

    // 面板入口信号 → PanelCoordinator(组合根不再中转)
    connect(main_window_.get(), &MainWindow::sgnOpenSearchPanelRequested, panel_coordinator_.get(),
            &PanelCoordinator::openSearchPanel);
    connect(main_window_.get(), &MainWindow::sgnOpenSettingsPanelRequested,
            panel_coordinator_.get(), &PanelCoordinator::openSettingsPanel);
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
    LibraryBrowserWidget* libraryBrowser   = main_window_->libraryBrowser();
    SongTableView* songTableView           = main_window_->songTableView();
    SidePanel* sidePanel                   = main_window_->sidePanel();
    // DesktopLyricsWidget* desktopLyrics = main_window_->desktopLyricsWidget();

    playback_service_->bind();
    connect(playback_service_.get(), &PlaybackService::sgnLocateCurrentTrack, this,
            &AppController::locateCurrentTrackInView);

    // 媒体库控件:双击曲目 → 队列入队即播(积累期经 playTrackInUi 播放)
    connect(playback_queue_service_.get(), &PlaybackQueueService::sgn_play_requested, this,
            [this](const QueueItem& item) { main_window_->playTrackInUi(item.filepath); });
    connect(
        libraryBrowser, &LibraryBrowserWidget::sgnPlayRequested, this,
        [this](const TrackId& track_id) { playback_queue_service_->play_library_track(track_id); });
    connect(libraryBrowser, &LibraryBrowserWidget::sgnOpenLibrarySettingsRequested, this,
            [this]() { panel_coordinator_->openSettingsPanelPage("Media Library"); });

    library_interaction_serivce_->bind();
    connect(songTableView, &SongTableView::sgnTrackPropertyRequested, tag_writeback_service_.get(),
            &TagWritebackService::requestTrackProperty);

    auto* lyricsModel = qobject_cast<WLyricsModel*>(sidePanel->getLyricsPanel()->model());
    connect(playbackController, &PlaybackController::sgnPositionChanged,
            sidePanel->getLyricsPanel(), &WLyricsPanel::ScrollByPosition);
    connect(sidePanel, &SidePanel::sgnDesktopLyricsConfigRequested, this,
            [this]() { panel_coordinator_->openSettingsPanelPage("Lyrics"); });

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
    connect(main_window_.get(), &MainWindow::sgnSetSortRuleRequested, this,
            &AppController::handleSetSortRuleRequested);
    connect(main_window_.get(), &MainWindow::sgnInsertColumnRequested, this,
            &AppController::handleInsertColumnRequested);
    connect(main_window_.get(), &MainWindow::sgnRemoveColumnRequested, this,
            &AppController::handleRemoveColumnRequested);
    connect(main_window_.get(), &MainWindow::sgnShowAboutMessagebox, this,
            &AppController::handleShowAboutMessagebox);
    connect(main_window_.get(), &MainWindow::sgnOpenEQWidgetRequested, panel_coordinator_.get(),
            &PanelCoordinator::openEQWidget);

    auto* locateShortcut = new QShortcut(QKeySequence(Qt::Key_Tab), songTableView->treeView());
    locateShortcut->setContext(Qt::WidgetShortcut);
    connect(locateShortcut, &QShortcut::activated, this, [this]() { locateCurrentTrackInView(); });
}

void AppController::locateCurrentTrackInView()
{
    auto* playlistController = playlist_controller_.get();
    auto* songTableView      = main_window_ ? main_window_->songTableView() : nullptr;
    if (!playlistController || !songTableView) {
        return;
    }

    auto* view  = songTableView->treeView();
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
        if (main_window_->songTableView()) {
            cm.registerModule(main_window_->songTableView());
            qDebug() << "[CONFIG] register song table view";
        }
        if (main_window_->libraryBrowser()) {
            cm.registerModule(main_window_->libraryBrowser());
            qDebug() << "[CONFIG] register library browser";
        }
        if (main_window_->desktopLyricsWidget()) {
            cm.registerModule(main_window_->desktopLyricsWidget());
            qDebug() << "[CONFIG] register desktop lyrics widget";
        }
    }
    if (panel_coordinator_->shortcutsController()) {
        cm.registerModule(panel_coordinator_->shortcutsController());
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
    panel_coordinator_->savePanelConfigs();
    cm.saveAll();
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
            // 双击列表项目时触发 &PlaylistTreeWidget::sgnSwitchPlaylist
            // CRUD 时触发 `&PlaylistController::playlistChanged`
            connect(playlist_controller_.get(), &PlaylistController::playlistChanged, this,
                    show_track_count);
            connect(main_window_.get()->playlistTreeWidget(),
                    &PlaylistTreeWidget::sgnSwitchPlaylist, this, show_track_count);
        },
        Qt::SingleShotConnection);
}
