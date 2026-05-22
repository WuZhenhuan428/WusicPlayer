#include "app_controller.h"

#include <QCoreApplication>
#include <QListWidgetItem>
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QTreeView>
#include <QMessageBox>
#include <QTimer>
#include <QKeySequence>
#include <QFileInfo>
#include <QShortcut>
#include <QThread>

#include "core/types.h"

#include "model/ShortcutsViewModel/shortcuts_types.hpp"
#include "view/MainWindow.h"
#include "view/playlist/playlist_widgets.h"
#include "view/search_panel/search_panel.h"
#include "view/SettingsPanel/SettingsPanel.h"
#include "view/SettingsPanel/ShortcutsPanel/ShortcutsPanel.h"
#include "view/eq_widget/eq_widget.h"

#include "controller/PlaybackController.h"
#include "controller/shortcuts_controller.h"
#include "controller/PlaylistController.h"
#include "controller/search_backend/in_memory_search_backend.h"
#include "model/playlist/playlist_manager.h"

#include "core/ConfigManager/ConfigManager.h"
#include "core/ConfigManager/IConfigurable.h"
#include <QJsonObject>
#include "service/playback_restore_service.h"

#include "view/SettingsPanel/lyrics_setting_panel/lyrics_setting_panel.h"
#include "view/SettingsPanel/ThemeSettingsPage/ThemeSettingsPage.h"

#include "service/playback_service.h"
#include "service/library_interaction_service.h"
#include "service/tag_writeback_service.h"
#include "service/theme_service.h"

AppController::AppController(PlaybackController* playbackController, QObject* parent)
    : QObject(parent),
      m_playback_controller(playbackController),
      m_playlist_manager(std::make_unique<PlaylistManager>()),
      m_playlist_controller(std::make_unique<PlaylistController>(m_playlist_manager.get(), nullptr, this)),
      m_search_backend(std::make_unique<InMemorySearchBackend>(m_playlist_controller.get())),
      m_main_window(std::make_unique<MainWindow>(m_playback_controller, m_playlist_controller.get())),
      m_playback_service(std::make_unique<PlaybackService>(m_main_window.get(), m_playback_controller, m_playlist_controller.get(), this)),
      m_playback_restore_service(std::make_unique<PlaybackRestoreService>(
          m_playlist_controller.get(), m_playback_controller, this
      )),
      m_library_interaction_serivce(std::make_unique<LibraryInteractionService>(
          m_main_window->libraryPanel(), m_playback_controller, m_playlist_controller.get(), this
      )),
      m_tag_writeback_service(std::make_unique<TagWritebackService>(
          m_playlist_controller.get(), m_playback_controller, m_playlist_manager.get(), m_main_window.get(), this)),
      m_theme_service(std::make_unique<ThemeService>(this))
{
    ensureShortcutsController();
    initializeConfig();
    m_playback_restore_service->restore();
    initializeCoreConnections();
    configureDesktopLyricsWindowRelation();

    connect(m_main_window.get(), &MainWindow::sgnOpenSearchPanelRequested,
        this, &AppController::onOpenSearchPanelRequested);
    connect(m_main_window.get(), &MainWindow::sgnOpenSettingsPanelRequested,
        this, &AppController::onOpenSettingsPanelRequested);
    connect(m_main_window.get(), &MainWindow::sgnShowDesktopLyricsRequested,
        this, &AppController::handleShowDesktopLyricsRequested);

    connect(m_main_window.get(), &MainWindow::sgnAboutToClose,
            this, &AppController::saveConfig);

    connect(qApp, &QCoreApplication::aboutToQuit,
            this, &AppController::saveConfig);
}

AppController::~AppController() = default;

void AppController::showMainWindow() {
    if (m_main_window) {
        m_main_window->show();
    }
}

void AppController::initializeCoreConnections()
{
    PlaylistController* playlistController = m_playlist_controller.get();
    PlaybackController* playbackController = m_playback_controller;
    LibraryWidget* libraryPanel = m_main_window->libraryPanel();
    SidePanel* sidePanel = m_main_window->sidePanel();
    // DesktopLyricsWidget* desktopLyrics = m_main_window->desktopLyricsWidget();

    m_playback_service->bind();
    connect(m_playback_service.get(), &PlaybackService::sgnLocateCurrentTrack, this, &AppController::locateCurrentTrackInView);

    m_library_interaction_serivce->bind();
    connect(libraryPanel, &LibraryWidget::sgnTrackPropertyRequested, m_tag_writeback_service.get(), &TagWritebackService::requestTrackProperty);

    auto* lyricsModel = qobject_cast<WLyricsModel*>(sidePanel->getLyricsPanel()->model());
    connect(playbackController, &PlaybackController::sgnPositionChanged, sidePanel->getLyricsPanel(), &WLyricsPanel::ScrollByPosition);
    connect(sidePanel, &SidePanel::sgnDesktopLyricsConfigRequested, this, [this]() {
        onOpenSettingsPanelRequested();
        if (m_settings_panel) {
            m_settings_panel->switchToPageByTitle("Lyrics");
        }
    });

    if (lyricsModel) {
        m_lyrics_follow_conn = connect(playbackController, &PlaybackController::sgnPositionChanged,
                                       lyricsModel, &WLyricsModel::setCurrentPosition);
        connect(lyricsModel, &WLyricsModel::currentLineChanged, this, [this](const QString& currText, const QString& nextText) {
            m_main_window->desktopLyricsWidget()->setLrcLine(currText, nextText);
        });
        connect(lyricsModel, &WLyricsModel::currentLineChanged, this, [this](){
            m_main_window->desktopLyricsWidget()->updateLineColor();
        });
        connect(lyricsModel, &WLyricsModel::sgnUseTimelineFollow, this, [this, playbackController, lyricsModel](bool enable) {
            if (enable) {
                if (!m_lyrics_follow_conn) {
                    m_lyrics_follow_conn = connect(playbackController, &PlaybackController::sgnPositionChanged,
                                                   lyricsModel, &WLyricsModel::setCurrentPosition);
                }
                return;
            }

            if (m_lyrics_follow_conn) {
                disconnect(m_lyrics_follow_conn);
                m_lyrics_follow_conn = QMetaObject::Connection{};
            }
        });
    }

    connect(m_main_window.get(), &MainWindow::sgnImportFilesRequested, this, [playlistController]() { playlistController->importFiles(); });
    connect(m_main_window.get(), &MainWindow::sgnImportFolderRequested, this, [playlistController]() { playlistController->importDir(); });
    connect(m_main_window.get(), &MainWindow::sgnCreatePlaylistRequested, playlistController, &PlaylistController::createNewPlaylist);
    connect(m_main_window.get(), &MainWindow::sgnLoadPlaylist, playlistController, &PlaylistController::loadPlaylist);
    connect(m_main_window.get(), &MainWindow::sgnCopyPlaylistRequested, this, [playlistController]() { playlistController->copyPlaylist(); });
    connect(m_main_window.get(), &MainWindow::sgnRenamePlaylistRequested, this, [playlistController]() { playlistController->renamePlaylist(); });
    connect(m_main_window.get(), &MainWindow::sgnRemovePlaylistRequested, this, [playlistController]() { playlistController->removePlaylist(); });
    connect(m_main_window.get(), &MainWindow::sgnSavePlaylistRequested, this, [playlistController]() { playlistController->savePlaylist(); });
    connect(m_main_window.get(), &MainWindow::sgnSetSortRuleRequested, this, &AppController::handleSetSortRuleRequested);
    connect(m_main_window.get(), &MainWindow::sgnInsertColumnRequested, this, &AppController::handleInsertColumnRequested);
    connect(m_main_window.get(), &MainWindow::sgnRemoveColumnRequested, this, &AppController::handleRemoveColumnRequested);
    connect(m_main_window.get(), &MainWindow::sgnShowAboutMessagebox, this, &AppController::handleShowAboutMessagebox);
    connect(m_main_window.get(), &MainWindow::sgnOpenEQWidgetRequested, this, &AppController::handleOpenEQRequested);


    auto* locateShortcut = new QShortcut(QKeySequence(Qt::Key_Tab), libraryPanel->songTreeView());
    locateShortcut->setContext(Qt::WidgetShortcut);
    connect(locateShortcut, &QShortcut::activated, this, [this]() {
        locateCurrentTrackInView();
    });
}

void AppController::locateCurrentTrackInView()
{
    auto* playlistController = m_playlist_controller.get();
    auto* libraryPanel = m_main_window ? m_main_window->libraryPanel() : nullptr;
    if (!playlistController || !libraryPanel) {
        return;
    }

    auto* view = libraryPanel->songTreeView();
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
    auto* playlistController = m_playlist_controller.get();
    WSortTypeSetDialog dialog;
    if (dialog.exec() == QDialog::Accepted) {
        QString input = dialog.getText();
        playlistController->viewModel()->setSortExpression(input);
    }
}

void AppController::handleInsertColumnRequested()
{
    auto* playlistController = m_playlist_controller.get();
    WInsertColumnDialog dialog;
    int maxIndex = playlistController->viewModel()->getColumns().size();
    dialog.setMaxIndex(maxIndex);
    dialog.setIndex(1);
    int result = dialog.exec();
    if (result == QDialog::Accepted) {
        TableColumn column = dialog.getRule();
        int index = dialog.index();
        playlistController->viewModel()->insertColumn(index, column);
    }
}

void AppController::handleRemoveColumnRequested()
{
    auto* playlistController = m_playlist_controller.get();
    WColumnIndexDialog dialog(QObject::tr("Remove column"), QObject::tr("Input the column index except 0"), m_main_window.get());
    int maxIndex = playlistController->viewModel()->getColumns().size() - 1;
    dialog.setMaxIndex(maxIndex);
    dialog.setIndex(1);
    if (dialog.exec() == QDialog::Accepted) {
        playlistController->viewModel()->removeColumn(dialog.index());
    }
}

void AppController::handleShowAboutMessagebox() {
    QMessageBox* msg = new QMessageBox(m_main_window.get());
    msg->setWindowTitle("About");
    msg->setText("This is a ABOUT message box.");
    msg->setIcon(QMessageBox::Information);
    msg->setStandardButtons(QMessageBox::Ok);
    msg->show();
    msg->setAttribute(Qt::WA_DeleteOnClose);
}

void AppController::handleShowDesktopLyricsRequested()
{
    auto* desktopLyrics = m_main_window->desktopLyricsWidget();
    if (desktopLyrics) {
        configureDesktopLyricsWindowRelation();
        desktopLyrics->show();
    }
}


void AppController::configureDesktopLyricsWindowRelation()
{
    auto* desktopLyrics = m_main_window ? m_main_window->desktopLyricsWidget() : nullptr;
    if (!desktopLyrics) {
        return;
    }

    if (desktopLyrics->parentWidget() != nullptr) {
        desktopLyrics->setParent(nullptr);
    }
}

void AppController::initializeConfig() {
    ConfigManager& cm = ConfigManager::getInstance();
    if (m_playback_controller) {
        cm.registerModule(m_playback_controller);
        qDebug() << "[CONFIG] register playback controller";
    }
    if (m_playlist_controller) {
        cm.registerModule(m_playlist_controller.get());
        qDebug() << "[CONFIG] register playlist controller";
    }
    if (m_main_window) {
        cm.registerModule(m_main_window.get());
        qDebug() << "[CONFIG] register main window";
        if (m_main_window->libraryPanel()) {
            cm.registerModule(m_main_window->libraryPanel());
            qDebug() << "[CONFIG] register library panel";
        }
        if (m_main_window->desktopLyricsWidget()) {
            cm.registerModule(m_main_window->desktopLyricsWidget());
            qDebug() << "[CONFIG] register desktop lyrics widget";
        }
    }
    if (m_shortcuts_controller) {
        cm.registerModule(m_shortcuts_controller);
        qDebug() << "[CONFIG] register shortcuts controller";
    }
    cm.loadAll();

    // misc: dependent data from other module,
    // but not sufficient to create interfaces seprately
    // current:
    //      WControlBar -> m_btn_mode->icon
    m_main_window.get()->controlBarWidget()->setPlayMode(m_playlist_controller->playMode());
}

void AppController::saveConfig() {
    if (!m_main_window) {
        return;
    }

    if (m_has_saved_config_on_exit) {
        return;
    }
    m_has_saved_config_on_exit = true;

    ConfigManager& cm = ConfigManager::getInstance();
    if (m_settings_panel) {
        cm.writeSubConfig(m_settings_panel->configSubKey(), m_settings_panel->saveToJson());
    }
    if (m_shortcuts_panel) {
        cm.writeSubConfig(m_shortcuts_panel->configSubKey(), m_shortcuts_panel->saveToJson());
    }
    if (m_search_panel) {
        cm.writeSubConfig(m_search_panel->configSubKey(), m_search_panel->saveToJson());
    }
    cm.saveAll();
}

void AppController::handleOpenEQRequested()
{
    if (!m_eq_widget) {
        m_eq_widget = new EQWidget(
            m_playback_controller->gains(),
            m_playback_controller->isEqEnabled(),
            false,
            nullptr  // no parent — WA_DeleteOnClose will clean up
        );
        m_eq_widget->setWindowFlag(Qt::Window, true);
        m_eq_widget->setAttribute(Qt::WA_DeleteOnClose);

        connect(m_eq_widget, &EQWidget::sgnGainChanged,
                m_playback_controller, &PlaybackController::setGains);
        connect(m_eq_widget, &EQWidget::sgnEqEnabledChanged,
                m_playback_controller, &PlaybackController::setEqEnabled);

        connect(m_eq_widget, &QObject::destroyed, this, [this]() {
            Q_UNUSED(this);
            // QPointer auto-nulls; config is saved on app close via saveConfig().
        });
    }

    m_eq_widget->show();
    m_eq_widget->raise();
    m_eq_widget->activateWindow();
}

void AppController::onOpenSettingsPanelRequested() {
    ensureSettingsPanel();
    ensureShortcutsPage();
    // ensure lyrics panel
    if (!m_lyrics_settings_panel) {
        m_lyrics_settings_panel = new LyricsSettingPanel(
            m_main_window->desktopLyricsWidget()->getActiveLineColor(),
            m_main_window->desktopLyricsWidget()->getInactiveLineColor());
        m_lyrics_settings_panel->setLineEditText(m_main_window->desktopLyricsWidget()->getFont());

        connect(m_lyrics_settings_panel, &LyricsSettingPanel::sgnActiveColorChanged, this, [this](rgb_t rgb){
            m_main_window->desktopLyricsWidget()->setActiveLineColor(rgb);
        });
        connect(m_lyrics_settings_panel, &LyricsSettingPanel::sgnInactiveColorChanged, this, [this](rgb_t rgb){
            m_main_window->desktopLyricsWidget()->setInactiveLineColor(rgb);
        });
        connect(m_lyrics_settings_panel, &LyricsSettingPanel::sgnDisplayModeChanged, this, [this](bool is_two_line){
            m_main_window->desktopLyricsWidget()->setDisplayMode( is_two_line ? DisplayMode::TwoLine : DisplayMode::OneLine );
        });
        connect(m_lyrics_settings_panel, &LyricsSettingPanel::sgnFontChanged, m_main_window->desktopLyricsWidget(), &DesktopLyricsWidget::setLrcFont);

        m_settings_panel->registerWidget(m_lyrics_settings_panel->getTitleItem(), m_lyrics_settings_panel);
    }

    // 主题设置页
    if (!m_theme_settings_page) {
        m_theme_settings_page = new ThemeSettingsPage(m_theme_service.get(), m_settings_panel);
        m_settings_panel->registerWidget(m_theme_settings_page->getTitleItem(), m_theme_settings_page);
    }

    m_settings_panel->show();
    m_settings_panel->raise();
    m_settings_panel->activateWindow();
}

void AppController::onOpenSearchPanelRequested() {
    ensureSearchPanel();

    m_search_panel->show();
    m_search_panel->raise();
    m_search_panel->activateWindow();
}

void AppController::ensureSettingsPanel() {
    if (m_settings_panel) {
        return;
    }

    m_settings_panel = new SettingsPanel(&ConfigManager::getInstance());
    m_settings_panel->setWindowFlag(Qt::Window, true);
    // 不使用 WA_DeleteOnClose：改为 hide/show 复用，避免销毁/重建循环中的状态不一致

    // 面板隐藏后激活主窗口，防止菜单栏焦点丢失导致不响应点击
    m_settings_panel->installEventFilter(this);
}

void AppController::ensureShortcutsPage() {
    ensureShortcutsController();

    if (!m_shortcuts_panel) {
        m_shortcuts_panel = new ShortcutsPanel(&ConfigManager::getInstance(), m_settings_panel);
        m_shortcuts_panel->setViewModel(m_shortcuts_controller->viewModel());

        connect(m_shortcuts_panel, &ShortcutsPanel::sgnDefaultConfig, this, [this]() {
            if (m_shortcuts_controller) {
                m_shortcuts_controller->resetAllToDefault();
            }
        });

        connect(m_shortcuts_panel, &ShortcutsPanel::sgnRestoreConfig, this, [this]() {
            if (!m_shortcuts_controller) {
                return;
            }
            QJsonObject sub_obj = ConfigManager::getInstance().readSubConfig(m_shortcuts_controller->configSubKey());
            QJsonObject root;
            root.insert(m_shortcuts_controller->configSubKey(), sub_obj);
            m_shortcuts_controller->loadFromJson(root);
        });

        connect(m_shortcuts_panel, &ShortcutsPanel::sgnApplyConfig, this, []() {
            ConfigManager::getInstance().saveAll();
        });

        m_settings_panel->registerWidget(m_shortcuts_panel->getListItem(), m_shortcuts_panel);
    }
}

void AppController::ensureShortcutsController()
{
    if (m_shortcuts_controller) {
        return;
    }

    m_shortcuts_controller = new ShortcutsController(this);
    registerDefaultShortcuts();
}

void AppController::registerDefaultShortcuts()
{
    if (!m_shortcuts_controller || m_shortcuts_registered) {
        return;
    }

    m_shortcuts_controller->setScopeHost(ShortcutScope::Application, m_main_window.get());
    m_shortcuts_controller->setScopeHost(ShortcutScope::MainWindow, m_main_window.get());
    m_shortcuts_controller->setScopeHost(ShortcutScope::DesktopLyrics, m_main_window.get());

    m_shortcuts_controller->registerOperation(
        ShortcutActionId::save_playlist,
        "Save Playlist",
        ShortcutScope::PlaylistView,
        QKeySequence(Qt::CTRL | Qt::Key_S),
        [this](){
            m_playlist_controller.get()->savePlaylist();
        },
        m_main_window.get(),
        true
    );

    m_shortcuts_controller->registerOperation(
        ShortcutActionId::open_file,
        "Open File",
        ShortcutScope::PlaylistView,
        QKeySequence(Qt::CTRL | Qt::Key_O),
        [this](){ m_playlist_controller.get()->importFiles(); },
        m_main_window.get(),
        true
    );

    m_shortcuts_controller->registerOperation(
        ShortcutActionId::open_playlist,
        "Open playlist",
        ShortcutScope::PlaylistView,
        QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O),
        [this](){m_playlist_controller.get()->loadPlaylist();},
        m_main_window.get(),
        true
    );

    m_shortcuts_controller->registerOperation(
        ShortcutActionId::play_pause,
        "Play / Pause",
        ShortcutScope::Application,
        QKeySequence(Qt::Key_Space),
        [this]() {
            if (m_playback_controller->state() == PlayingState::PLAYING) {
                m_playback_controller->pause();
            } else {
                m_playback_controller->play();
            }
        },
        m_main_window.get(),
        true
    );

    m_shortcuts_controller->registerOperation(
        ShortcutActionId::open_settings,
        "Open settings",
        ShortcutScope::MainWindow,
        QKeySequence(Qt::CTRL | Qt::Key_Comma),
        [this]() {
            onOpenSettingsPanelRequested();
        },
        this,
        true
    );

    m_shortcuts_controller->registerOperation(
        ShortcutActionId::stop,
        "Stop",
        ShortcutScope::Application,
        QKeySequence(Qt::Key_S),
        [this]() {
            m_playback_controller->stop();
        },
        m_main_window.get(),
        true
    );

    m_shortcuts_controller->registerOperation(
        ShortcutActionId::open_search,
        "Open Search Panel",
        ShortcutScope::MainWindow,
        QKeySequence(Qt::CTRL | Qt::Key_F),
        [this]() {
            onOpenSearchPanelRequested();
        },
        m_main_window.get(),
        true
    );

    m_shortcuts_controller->registerOperation(
        ShortcutActionId::show_hide_desktop_lyrics,
        "Show / Hide Desktop Lyrics",
        ShortcutScope::Application,
        QKeySequence(Qt::CTRL | Qt::Key_L),
        [this]() {
            auto* desktopLyrics = m_main_window->desktopLyricsWidget();
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
        m_main_window.get(),
        true
    );

    m_shortcuts_registered = true;
}

void AppController::ensureSearchPanel() {
    if (m_search_panel) {
        return;
    }

    m_search_panel = new SearchPanel(&ConfigManager::getInstance());
    m_search_panel->setWindowFlag(Qt::Window, true);
    m_search_panel->setAttribute(Qt::WA_DeleteOnClose, true);
    m_search_panel->setSearchBackend(m_search_backend.get());
    m_search_backend->warmup(m_playlist_controller->currentPlaylist());

    connect(m_playlist_controller->viewModel(), &QAbstractItemModel::modelReset,
            m_search_panel, [this]() {
        if (m_search_panel) {
            const QJsonObject sub_obj = ConfigManager::getInstance().readSubConfig(m_search_panel->configSubKey());
            const QByteArray header_state = QByteArray::fromBase64(sub_obj.value("header_state").toString().toUtf8());
            m_search_panel->applyHeaderStateDeferred(header_state);
        }
        if (m_search_backend) {
            m_search_backend->invalidate(playlistId{});
            m_search_backend->warmup(m_playlist_controller->currentPlaylist());
        }
    });

    connect(m_search_panel, &SearchPanel::sgnRequestPlayTrack,
            m_main_window.get(), [this](const trackId& id) {
        auto* model = m_playlist_controller->viewModel();
        if (!model) return;
        if (id.isNull()) return;

        int queueIndex = model->playbackQueue().indexOf(id);
        if (queueIndex >= 0) {
            m_locate_on_next_play_request = true;
            m_playlist_controller->play(queueIndex);

            this->locateCurrentTrackInView();
        }
    });

    connect(m_search_panel, &QObject::destroyed, this, [this]() {
        m_search_panel = nullptr;
    });
}

bool AppController::eventFilter(QObject* obj, QEvent* event) {
    if (obj == m_settings_panel && event->type() == QEvent::Hide) {
        // 设置面板关闭后激活主窗口，防止菜单栏焦点丢失
        if (m_main_window) {
            m_main_window->activateWindow();
            m_main_window->raise();
        }
    }
    return QObject::eventFilter(obj, event);
}
