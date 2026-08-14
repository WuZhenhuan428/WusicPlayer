#include "app_controller.h"

#include "controller/playback_controller.h"
#include "controller/playlist_controller.h"
#include "controller/search_backend/in_memory_search_backend.h"
#include "controller/shortcuts_controller.h"
#include "controller/status_bar_controller.h"
#include "core/config_manager/config_manager.h"
#include "core/config_manager/i_configurable.h"
#include "core/logger/log_sink_gui.h"
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
#include "view/main_window.h"
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

#include "core/logger/logger_manager.h"
namespace
{
Logger* logger = LoggerManager::file_logger("app_controller", {"console", "gui"});
}

// 构造顺序: 独立组件 -> Controller -> (AppContext) -> Service
AppController::AppController(PlaybackController* playback_controller, QObject* parent) :
    QObject(parent), playback_controller_(playback_controller),
    playlist_manager_(std::make_unique<PlaylistManager>()),
    playlist_controller_(
        std::make_unique<PlaylistController>(playlist_manager_.get(), nullptr, this)),
    library_manager_(std::make_unique<LibraryManager>()),
    // 搜索面板:搜索当前播放列表(数据库 FTS5 仅媒体库控件使用)
    search_backend_(std::make_unique<InMemorySearchBackend>(playlist_controller_.get())),
    main_window_(std::make_unique<MainWindow>(playback_controller_, playlist_controller_.get())),
    status_bar_controller_(std::make_unique<StatusBarController>(main_window_->statusBar(), this))
{
    // 先构造 AppContext 引用的服务, 再构建上下文——
    // theme_service_ 必须在 app_context_ 之前构造, 否则上下文里是空指针
    // (PanelCoordinator/ThemeSettingsPage 会 connect/解引用 null → 段错误)。
    theme_service_     = std::make_unique<ThemeService>(this);

    // 其他资源初始化
    this->app_context_ = {
        .main_window_              = this->main_window_.get(),
        .playback_controller_      = this->playback_controller_,
        .playlist_controller_      = this->playlist_controller_.get(),
        .playlist_manager_         = this->playlist_manager_.get(),
        .theme_service_            = this->theme_service_.get(),
        .in_memory_search_backend_ = this->search_backend_.get(),
        .log_sink_gui_ =
            dynamic_cast<LogSinkGui*>(LoggerManager::instance().get_sink_by_name("gui").get()),
        .library_manager_ = this->library_manager_.get(),
    };
    // 面板编排:设置/搜索/EQ/快捷键(构造时注册默认快捷键)
    panel_coordinator_           = std::make_unique<PanelCoordinator>(app_context_, this);
    // Services 初始化
    playback_service_            = std::make_unique<PlaybackService>(app_context_, this);
    playback_restore_service_    = std::make_unique<PlaybackRestoreService>(app_context_, this);
    library_interaction_service_ = std::make_unique<LibraryInteractionService>(app_context_, this);
    tag_writeback_service_       = std::make_unique<TagWritebackService>(app_context_, this);
    playback_queue_service_      = std::make_unique<PlaybackQueueService>(app_context_, this);

    // 媒体库控件(左侧播放列表下方):注入库与队列服务
    main_window_->library_browser()->set_library_manager(library_manager_.get());
    main_window_->library_browser()->set_playback_queue_service(playback_queue_service_.get());

    // 音乐库:注入播放列表(解析曲目引用)、初始化数据库、启动初始扫描
    playlist_manager_->set_library_manager(library_manager_.get());
    const QString data_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(data_dir);
    if (library_manager_->initialize(data_dir + "/library.db")) {
        library_manager_->start_scan();
    }

    initialize_config();
    playback_restore_service_->restore();
    initialize_core_connections();
    configure_desktop_lyrics_window_relation();

    // 面板入口信号 → PanelCoordinator(组合根不再中转)
    connect(main_window_.get(), &MainWindow::sgnOpenSearchPanelRequested, panel_coordinator_.get(),
            &PanelCoordinator::open_search_panel);
    connect(main_window_.get(), &MainWindow::sgnOpenLogViewerRequested, panel_coordinator_.get(),
            &PanelCoordinator::open_log_viewer);
    connect(main_window_.get(), &MainWindow::sgnOpenSettingsPanelRequested,
            panel_coordinator_.get(), &PanelCoordinator::open_settings_panel);
    connect(main_window_.get(), &MainWindow::sgnShowDesktopLyricsRequested, this,
            &AppController::handle_show_desktop_lyrics_requested);
    connect(main_window_.get(), &MainWindow::sgnAboutToClose, this, &AppController::save_config);

    connect(qApp, &QCoreApplication::aboutToQuit, this, &AppController::save_config);

    this->setup_status_bar_connections();
}

AppController::~AppController() = default;

void AppController::show_main_window()
{
    if (main_window_) {
        main_window_->show();
    }
}

void AppController::initialize_core_connections()
{
    PlaylistController* playlist_controller = playlist_controller_.get();
    PlaybackController* playback_controller = playback_controller_;
    LibraryBrowserWidget* library_browser   = main_window_->library_browser();
    SongTableView* song_table_view          = main_window_->song_table_view();
    SidePanel* side_panel                   = main_window_->side_panel();
    // DesktopLyricsWidget* desktopLyrics = main_window_->desktop_lyrics_widget();

    playback_service_->bind();
    connect(playback_service_.get(), &PlaybackService::sgnLocateCurrentTrack, this,
            &AppController::locate_current_track_in_view);

    // 媒体库控件:双击曲目 → 队列入队即播(积累期经 play_track_in_ui 播放)
    connect(playback_queue_service_.get(), &PlaybackQueueService::sgn_play_requested, this,
            [this](const QueueItem& item) { main_window_->play_track_in_ui(item.filepath); });
    connect(
        library_browser, &LibraryBrowserWidget::sgnPlayRequested, this,
        [this](const TrackId& track_id) { playback_queue_service_->play_library_track(track_id); });
    connect(library_browser, &LibraryBrowserWidget::sgnPlayTracksRequested, this,
            [this](const QVector<TrackId>& track_ids) {
                for (const TrackId& tid : track_ids) {
                    playback_queue_service_->play_library_track(tid);
                }
            });
    connect(library_browser, &LibraryBrowserWidget::sgnAddTracksToPlaylist, this,
            [this](const PlaylistId& dst_pid, const QVector<TrackId>& track_ids) {
                playlist_controller_->add_library_tracks(dst_pid, track_ids);
            });
    connect(library_browser, &LibraryBrowserWidget::sgnRefreshLibraryRequested, this,
            [this]() { library_manager_->start_scan(); });
    // 媒体库拖入播放列表树 → 添加到对应列表;拖入歌曲表 → 添加到当前列表
    connect(main_window_->playlist_tree_widget(), &PlaylistTreeWidget::sgnLibraryTracksDropped,
            this, [this](const PlaylistId& pid, const QVector<TrackId>& track_ids) {
                playlist_controller_->add_library_tracks(pid, track_ids);
            });
    // 播放列表条目拖入播放列表树 → 复制到目标列表(列表→列表)
    connect(main_window_->playlist_tree_widget(), &PlaylistTreeWidget::sgnPlaylistEntriesDropped,
            this,
            [this](const PlaylistId& src_pid, const PlaylistId& dst_pid,
                   const QVector<EntryId>& entry_ids) {
                playlist_controller_->copy_tracks_to_playlist(src_pid, entry_ids, dst_pid);
            });
    connect(song_table_view, &SongTableView::sgnLibraryTracksDropped, this,
            [this](const QVector<TrackId>& track_ids) {
                playlist_controller_->add_library_tracks(
                    playlist_controller_->current_playlist_id(), track_ids);
            });
    // Add to Playlist 目标列表(含当前列表,库曲目可入任意列表)
    library_browser->set_playlist_list_provider([this]() {
        QVector<QPair<PlaylistId, QString>> lists;
        const auto all = playlist_controller_->playlists();
        for (const auto& pl : all) {
            if (pl) {
                lists.push_back({pl->id(), pl->name()});
            }
        }
        return lists;
    });
    connect(library_browser, &LibraryBrowserWidget::sgnOpenLibrarySettingsRequested, this,
            [this]() { panel_coordinator_->open_settings_panel_page("Media Library"); });

    library_interaction_service_->bind();
    connect(song_table_view, &SongTableView::sgnTrackPropertyRequested,
            tag_writeback_service_.get(), &TagWritebackService::request_track_property);

    auto* lyricsModel = qobject_cast<WLyricsModel*>(side_panel->get_lyrics_panel()->model());
    connect(playback_controller, &PlaybackController::sgn_position_changed,
            side_panel->get_lyrics_panel(), &WLyricsPanel::scroll_by_position);
    connect(side_panel, &SidePanel::sgnDesktopLyricsConfigRequested, this,
            [this]() { panel_coordinator_->open_settings_panel_page("Lyrics"); });

    if (lyricsModel) {
        lyrics_follow_conn_ =
            connect(playback_controller, &PlaybackController::sgn_position_changed, lyricsModel,
                    &WLyricsModel::set_current_position);
        connect(lyricsModel, &WLyricsModel::currentLineChanged, this,
                [this](const QString& currText, const QString& nextText) {
                    main_window_->desktop_lyrics_widget()->set_lrc_line(currText, nextText);
                });
        connect(lyricsModel, &WLyricsModel::currentLineChanged, this,
                [this]() { main_window_->desktop_lyrics_widget()->update_line_color(); });
        connect(lyricsModel, &WLyricsModel::sgnUseTimelineFollow, this,
                [this, playback_controller, lyricsModel](bool enable) {
                    if (enable) {
                        if (!lyrics_follow_conn_) {
                            lyrics_follow_conn_ = connect(
                                playback_controller, &PlaybackController::sgn_position_changed,
                                lyricsModel, &WLyricsModel::set_current_position);
                        }
                        return;
                    }

                    if (lyrics_follow_conn_) {
                        disconnect(lyrics_follow_conn_);
                        lyrics_follow_conn_ = QMetaObject::Connection{};
                    }
                });
    }

    connect(main_window_.get(), &MainWindow::sgnCreatePlaylistRequested, playlist_controller,
            &PlaylistController::create_new_playlist);
    connect(main_window_.get(), &MainWindow::sgnLoadPlaylist, playlist_controller,
            &PlaylistController::load_playlist);
    connect(main_window_.get(), &MainWindow::sgnSetSortRuleRequested, this,
            &AppController::handle_set_sort_rule_requested);
    connect(main_window_.get(), &MainWindow::sgnInsertColumnRequested, this,
            &AppController::handle_insert_column_requested);
    connect(main_window_.get(), &MainWindow::sgnRemoveColumnRequested, this,
            &AppController::handle_remove_column_requested);
    connect(main_window_.get(), &MainWindow::sgnShowAboutMessagebox, this,
            &AppController::handle_show_about_messagebox);
    connect(main_window_.get(), &MainWindow::sgnOpenEQWidgetRequested, panel_coordinator_.get(),
            &PanelCoordinator::open_eq_widget);

    auto* locateShortcut = new QShortcut(QKeySequence(Qt::Key_Tab), song_table_view->tree_view());
    locateShortcut->setContext(Qt::WidgetShortcut);
    connect(locateShortcut, &QShortcut::activated, this,
            [this]() { locate_current_track_in_view(); });
}

void AppController::locate_current_track_in_view()
{
    auto* playlist_controller = playlist_controller_.get();
    auto* song_table_view     = main_window_ ? main_window_->song_table_view() : nullptr;
    if (!playlist_controller || !song_table_view) {
        return;
    }

    auto* view  = song_table_view->tree_view();
    auto* model = playlist_controller->view_model();
    if (!view || !model) {
        return;
    }

    const QModelIndex index = model->get_current_track_index();
    if (!index.isValid()) {
        return;
    }

    view->scrollTo(index.siblingAtColumn(1), QAbstractItemView::PositionAtCenter);
    view->setCurrentIndex(index.siblingAtColumn(1));
}

void AppController::handle_set_sort_rule_requested()
{
    auto* playlist_controller = playlist_controller_.get();
    WSortTypeSetDialog dialog;
    if (dialog.exec() == QDialog::Accepted) {
        QString input = dialog.getText();
        playlist_controller->view_model()->set_sort_expression(input);
    }
}

void AppController::handle_insert_column_requested()
{
    auto* playlist_controller = playlist_controller_.get();
    WInsertColumnDialog dialog;
    int maxIndex = playlist_controller->view_model()->get_columns().size();
    dialog.set_max_index(maxIndex);
    dialog.set_index(1);
    int result = dialog.exec();
    if (result == QDialog::Accepted) {
        TableColumn column = dialog.get_rule();
        int index          = dialog.index();
        playlist_controller->view_model()->insert_column(index, column);
    }
}

void AppController::handle_remove_column_requested()
{
    auto* playlist_controller = playlist_controller_.get();
    WColumnIndexDialog dialog(QObject::tr("Remove column"),
                              QObject::tr("Input the column index except 0"), main_window_.get());
    int maxIndex = playlist_controller->view_model()->get_columns().size() - 1;
    dialog.set_max_index(maxIndex);
    dialog.set_index(1);
    if (dialog.exec() == QDialog::Accepted) {
        playlist_controller->view_model()->remove_column(dialog.index());
    }
}

void AppController::handle_show_about_messagebox()
{
    QMessageBox* msg = new QMessageBox(main_window_.get());
    msg->setWindowTitle("About");
    msg->setText("This is a ABOUT message box.");
    msg->setIcon(QMessageBox::Information);
    msg->setStandardButtons(QMessageBox::Ok);
    msg->show();
    msg->setAttribute(Qt::WA_DeleteOnClose);
}

void AppController::handle_show_desktop_lyrics_requested()
{
    auto* desktopLyrics = main_window_->desktop_lyrics_widget();
    if (desktopLyrics) {
        configure_desktop_lyrics_window_relation();
        desktopLyrics->show();
    }
}

void AppController::configure_desktop_lyrics_window_relation()
{
    auto* desktopLyrics = main_window_ ? main_window_->desktop_lyrics_widget() : nullptr;
    if (!desktopLyrics) {
        return;
    }

    if (desktopLyrics->parentWidget() != nullptr) {
        desktopLyrics->setParent(nullptr);
    }
}

void AppController::initialize_config()
{
    ConfigManager& cm = ConfigManager::get_instance();
    if (playback_controller_) {
        cm.register_module(playback_controller_);
        logger->debug("[CONFIG] register playback controller");
    }
    if (playlist_controller_) {
        cm.register_module(playlist_controller_.get());
        logger->debug("[CONFIG] register playlist controller");
    }
    if (main_window_) {
        cm.register_module(main_window_.get());
        logger->debug("[CONFIG] register main window");
        if (main_window_->song_table_view()) {
            cm.register_module(main_window_->song_table_view());
            logger->debug("[CONFIG] register song table view");
        }
        if (main_window_->library_browser()) {
            cm.register_module(main_window_->library_browser());
            logger->debug("[CONFIG] register library browser");
        }
        if (main_window_->desktop_lyrics_widget()) {
            cm.register_module(main_window_->desktop_lyrics_widget());
            logger->debug("[CONFIG] register desktop lyrics widget");
        }
    }
    if (panel_coordinator_->shortcuts_controller()) {
        cm.register_module(panel_coordinator_->shortcuts_controller());
        logger->debug("[CONFIG] register shortcuts controller");
    }
    cm.load_all();

    // misc: dependent data from other module,
    // but not sufficient to create interfaces seprately
    // current:
    //      WControlBar -> m_btn_mode->icon
    main_window_.get()->control_bar_widget()->set_play_mode(playlist_controller_->play_mode());
}

void AppController::save_config()
{
    if (!main_window_) {
        return;
    }

    if (has_saved_config_on_exit_) {
        return;
    }
    has_saved_config_on_exit_ = true;

    ConfigManager& cm         = ConfigManager::get_instance();
    panel_coordinator_->save_panel_configs();
    cm.save_all();
}

void AppController::setup_status_bar_connections()
{
    status_bar_controller_->register_item("play_state");
    connect(playback_controller_, &PlaybackController::sgn_playback_state_changed, this,
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

    // NOTE: 通过sgn_cache_load_finished信号，确保播放列表已经创建完成，防止悬垂引用
    connect(
        playlist_controller_.get(), &PlaylistController::sgn_cache_load_finished, this,
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
            // CRUD 时触发 `&PlaylistController::sgn_playlist_changed`
            connect(playlist_controller_.get(), &PlaylistController::sgn_playlist_changed, this,
                    show_track_count);
            connect(main_window_.get()->playlist_tree_widget(),
                    &PlaylistTreeWidget::sgnSwitchPlaylist, this, show_track_count);
        },
        Qt::SingleShotConnection);
}
