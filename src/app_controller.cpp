#include "app_controller.h"

#include <QCoreApplication>
#include <QListWidgetItem>
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QTreeView>
#include <QMessageBox>
#include <QTimer>
#include <qkeysequence.h>
#include <qnamespace.h>

#include "model/ShortcutsViewModel/shortcuts_types.hpp"
#include "view/MainWindow.h"
#include "view/playlist/playlist_widgets.h"
#include "view/search_panel/playlist_search_panel.h"
#include "view/SettingsPanel/SettingsPanel.hpp"
#include "view/SettingsPanel/ShortcutsPanel/ShortcutsPanel.hpp"

#include "controller/PlaybackController.h"
#include "controller/shortcuts_controller.h"
#include "controller/PlaylistController.h"
#include "controller/search_backend/in_memory_search_backend.h"
#include "model/playlist/playlist_manager.h"

#include "core/ConfigManager/ConfigManager.h"
#include "view/ConfigBinder/MainWindowConfigContext.hpp"
#include "view/ConfigBinder/IConfigBinder.hpp"
#include "view/ConfigBinder/DesktopLyricsSection.hpp"
#include "view/ConfigBinder/LibraryViewSection.hpp"
#include "view/ConfigBinder/PlaybackConfigSection.hpp"
#include "view/ConfigBinder/SearchPanelSection.hpp"
#include "view/ConfigBinder/WindowConfigSection.hpp"
#include "view/ConfigBinder/DesktopLyricsBinder.hpp"
#include "view/ConfigBinder/LibraryViewBinder.hpp"
#include "view/ConfigBinder/PlaybackConfigBinder.hpp"
#include "view/ConfigBinder/SearchPanelBinder.hpp"
#include "view/ConfigBinder/WindowConfigBinder.hpp"
#include "view/ConfigBinder/SettingsPanelSection.hpp"
#include "view/ConfigBinder/SettingsPanelBinder.hpp"
#include "view/ConfigBinder/ShortcutsSection.hpp"
#include "view/ConfigBinder/ShortcutsBinder.hpp"
#include "view/PlaybackRestoreCoordinator.hpp"

#include "view/SettingsPanel/lyrics_setting_panel/lyrics_setting_panel.h"

AppController::AppController(PlaybackController* playbackController, QObject* parent)
    : QObject(parent),
      m_playback_controller(playbackController),
      m_playlist_manager(std::make_unique<PlaylistManager>()),
      m_playlist_controller(std::make_unique<PlaylistController>(m_playlist_manager.get(), nullptr, this)),
    m_search_backend(std::make_unique<InMemorySearchBackend>(m_playlist_controller.get())),
      m_main_window(std::make_unique<MainWindow>(m_playback_controller, m_playlist_controller.get())),
      m_desktop_lyrics_section(std::make_unique<DesktopLyricsSection>()),
      m_library_view_section(std::make_unique<LibraryViewSection>()),
      m_playback_config_section(std::make_unique<PlaybackConfigSection>()),
      m_search_panel_section(std::make_unique<SearchPanelSection>()),
      m_window_config_section(std::make_unique<WindowConfigSection>()),
      m_settings_panel_section(std::make_unique<SettingsPanelSection>()),
      m_shortcuts_section(std::make_unique<ShortcutsSection>()),
      m_desktop_lyrics_binder(std::make_unique<DesktopLyricsBinder>()),
      m_library_view_binder(std::make_unique<LibraryViewBinder>()),
      m_playback_config_binder(std::make_unique<PlaybackConfigBinder>()),
      m_search_panel_binder(std::make_unique<SearchPanelBinder>()),
      m_window_config_binder(std::make_unique<WindowConfigBinder>()),
      m_settings_sanel_binder(std::make_unique<SettingsPanelBinder>()),
      m_shortcuts_binder(std::make_unique<ShortcutsBinder>()),
      m_playback_restore_coordinator(std::make_unique<PlaybackRestoreCoordinator>(
                    m_playback_config_section.get(), m_playlist_controller.get(), m_playback_controller, this))
{
    SortRule defaultRule;
    defaultRule.type = SortType::album;
    m_playlist_controller->viewModel()->setSingleGrouping(defaultRule);

    initializeConfig();
    ensureShortcutsController();
    m_desktop_lyrics_visible_cache = m_desktop_lyrics_section->is_visible;
    applyConfig();
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
    auto* playlistController = m_playlist_controller.get();
    auto* playbackController = m_playback_controller;
    auto* controlBar = m_main_window->controlBarWidget();
    auto* libraryPanel = m_main_window->libraryPanel();
    auto* sidePanel = m_main_window->sidePanel();
    auto* desktopLyrics = m_main_window->desktopLyricsWidget();

        connect(m_main_window.get(), &MainWindow::sgnPlayTrackRequested,
            this, &AppController::handlePlayTrackRequest);
    connect(desktopLyrics, &DesktopLyricsWidget::sgnVisibilityChanged, this, [this](bool visible) {
        m_desktop_lyrics_visible_cache = visible;
        if (m_desktop_lyrics_section) {
            m_desktop_lyrics_section->is_visible = visible;
        }
    });

    connect(controlBar, &WControlBar::sgnBtnPlayClicked, playbackController, &PlaybackController::play);
    connect(controlBar, &WControlBar::sgnBtnPauseClicked, playbackController, &PlaybackController::pause);
    connect(controlBar, &WControlBar::sgnBtnStopClicked, playbackController, &PlaybackController::stop);
    connect(controlBar, &WControlBar::sgnBtnMuteClicked, playbackController, &PlaybackController::flipMute);

    connect(controlBar, &WControlBar::sgnInOrder, this, [playbackController]() {
        playbackController->setPlayMode(PlayMode::in_order);
    });
    connect(controlBar, &WControlBar::sgnLoop, this, [playbackController]() {
        playbackController->setPlayMode(PlayMode::loop);
    });
    connect(controlBar, &WControlBar::sgnShuffle, this, [playbackController]() {
        playbackController->setPlayMode(PlayMode::shuffle);
    });
    connect(controlBar, &WControlBar::sgnOutOfOrderTrack, this, [playbackController]() {
        playbackController->setPlayMode(PlayMode::out_of_order_track);
    });
    connect(controlBar, &WControlBar::sgnOutOfOrderGroup, this, [playbackController]() {
        playbackController->setPlayMode(PlayMode::out_of_order_group);
    });
    connect(controlBar, &WControlBar::sgnSliderPositionReleased, this, [playbackController](int percent) {
        playbackController->setPosition(percent * 1000);
    });
    connect(controlBar, &WControlBar::sgnSliderVolumeReleased, playbackController, &PlaybackController::setVolume);
    connect(controlBar, &WControlBar::sgnSliderVolumeMoved, playbackController, &PlaybackController::setVolume);
    connect(playbackController, &PlaybackController::sgnDevicesChanged, controlBar, &WControlBar::setDevice);
    connect(controlBar, &WControlBar::sgnSelectDeviceId, playbackController, &PlaybackController::setDeviceById);
    connect(playbackController, &PlaybackController::sgnPositionChanged, controlBar, &WControlBar::updatePosition);
    connect(playbackController, &PlaybackController::sgnPlaybackStateChanged, controlBar, &WControlBar::onPlayerStateChanged);
    connect(playbackController, &PlaybackController::sgnDurationChanged, controlBar, &WControlBar::updateDuration);
    connect(playbackController, &PlaybackController::sgnPlayModeChanged, this, [controlBar](PlayMode mode) {
        controlBar->setPlayMode(mode);
    });

    connect(playlistController->viewModel(), &QAbstractItemModel::modelReset, this, [libraryPanel]() {
        QTreeView* view = libraryPanel->songTreeView();
        if (!view || !view->model()) {
            return;
        }
        QAbstractItemModel* model = view->model();
        for (int i = 0; i < model->rowCount(); ++i) {
            QModelIndex idx = model->index(i, 0);
            if (model->hasChildren(idx)) {
                view->setFirstColumnSpanned(i, QModelIndex(), true);
                view->setExpanded(idx, true);
            }
        }
    });

    auto* lyricsModel = dynamic_cast<WLyricsModel*>(sidePanel->getLyricsPanel()->model());
    connect(playbackController, &PlaybackController::sgnPositionChanged, sidePanel->getLyricsPanel(), &WLyricsPanel::ScrollByPosition);

    if (lyricsModel) {
        connect(playbackController, &PlaybackController::sgnPositionChanged, lyricsModel, &WLyricsModel::setCurrentPosition);
        connect(lyricsModel, &WLyricsModel::currentLineChanged, this, [this](const QString& currText, const QString& nextText) {
            m_main_window->desktopLyricsWidget()->setLrcLine(currText, nextText);
        });
        connect(lyricsModel, &WLyricsModel::currentLineChanged, this, [this](){
            m_main_window->desktopLyricsWidget()->updateLineColor();
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

    connect(controlBar, &WControlBar::sgnBtnNextClicked, this, [this, playlistController, playbackController]() {
        QString nextTrack = playlistController->nextTrack(playbackController->playMode());
        if (!nextTrack.isEmpty()) {
            m_main_window->playTrackInUi(nextTrack);
        }
    });

    connect(controlBar, &WControlBar::sgnBtnPrevClicked, this, [this, playlistController, playbackController]() {
        QString prevTrack = playlistController->prevTrack(playbackController->playMode());
        if (!prevTrack.isEmpty()) {
            m_main_window->playTrackInUi(prevTrack);
        }
    });

    connect(playbackController, &PlaybackController::sgnMediaStateChanged,
            this, [this, playlistController, playbackController](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::MediaStatus::EndOfMedia) {
            QString nextTrack = playlistController->nextTrack(playbackController->playMode());
            if (!nextTrack.isEmpty()) {
                m_main_window->playTrackInUi(nextTrack);
            }
        }
    });

    connect(playlistController, &PlaylistController::requestPlay,
            this, [this](const QString& filepath) {
        m_main_window->playTrackInUi(filepath);
    });

    connect(playlistController, &PlaylistController::playlistChanged,
            this, &AppController::refreshPlaylistView);

    connect(libraryPanel, &LibraryWidget::sgnImportFiles, playlistController, &PlaylistController::importFiles);
    connect(libraryPanel, &LibraryWidget::sgnImportDir, playlistController, &PlaylistController::importDir);
    connect(libraryPanel, &LibraryWidget::sgnSwitchPlaylist, playlistController, &PlaylistController::switchToPlaylist);
    connect(libraryPanel, &LibraryWidget::sgnRenamePlaylist, playlistController, &PlaylistController::renamePlaylist);
    connect(libraryPanel, &LibraryWidget::sgnRemovePlaylist, playlistController, &PlaylistController::removePlaylist);
    connect(libraryPanel, &LibraryWidget::sgnSavePlaylist, playlistController, &PlaylistController::savePlaylist);
    connect(libraryPanel, &LibraryWidget::sgnCopyPlaylist, playlistController, &PlaylistController::copyPlaylist);

    connect(libraryPanel, &LibraryWidget::sgnPlayTrackByModelIndex,
        this, [playlistController](const QModelIndex& index) {
        auto* model = playlistController->viewModel();
        if (!model) return;
        trackId id = model->trackAt(index);
        if (id.isNull()) return;
        int queueIndex = model->playbackQueue().indexOf(id);
        if (queueIndex >= 0) {
            playlistController->play(queueIndex);
        }
    });
}

void AppController::handlePlayTrackRequest(const QString& filepath)
{
    if (filepath.isEmpty()) {
        return;
    }

    auto* playlistController = m_playlist_controller.get();
    auto* libraryPanel = m_main_window->libraryPanel();
    auto* sidePanel = m_main_window->sidePanel();
    auto* playbackController = m_playback_controller;

    playbackController->read(filepath);
    sidePanel->loadCover(filepath);

    QModelIndex index = playlistController->viewModel()->getCurrentTrackIndex();
    if (index.isValid()) {
        libraryPanel->songTreeView()->scrollTo(index.siblingAtColumn(1), QAbstractItemView::PositionAtCenter);
    }

    TrackMetaData meta = playlistController->currentMetadata();
    sidePanel->loadLyrics(meta);
    sidePanel->loadMetaData(meta);
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
        m_desktop_lyrics_visible_cache = true;
        if (m_desktop_lyrics_section) {
            m_desktop_lyrics_section->is_visible = true;
        }
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

void AppController::refreshPlaylistView()
{
    auto* playlistController = m_playlist_controller.get();
    auto* libraryPanel = m_main_window->libraryPanel();

    QVector<QPair<playlistId, QString>> items;
    const auto& lists = playlistController->playlists();
    items.reserve(static_cast<int>(lists.size()));
    for (const auto& list : lists) {
        items.push_back({list->id(), list->name()});
    }
    libraryPanel->setPlaylists(items);
}

void AppController::initializeConfig() {
    ConfigManager& cm = ConfigManager::getInstance();
    cm.registerSection(m_window_config_section.get());
    cm.registerSection(m_playback_config_section.get());
    cm.registerSection(m_library_view_section.get());
    cm.registerSection(m_search_panel_section.get());
    cm.registerSection(m_desktop_lyrics_section.get());
    cm.registerSection(m_settings_panel_section.get());
    cm.registerSection(m_shortcuts_section.get());
    cm.loadAll();

    m_binders.push_back(m_desktop_lyrics_binder.get());
    m_binders.push_back(m_library_view_binder.get());
    m_binders.push_back(m_playback_config_binder.get());
    m_binders.push_back(m_search_panel_binder.get());
    m_binders.push_back(m_window_config_binder.get());
    m_binders.push_back(m_settings_sanel_binder.get());
    m_binders.push_back(m_shortcuts_binder.get());
}

MainWindowConfigContext AppController::buildConfigContext() const {
    MainWindowConfigContext ctx;
    ctx.mainWindow = m_main_window.get();
    ctx.appController = const_cast<AppController*>(this);
    ctx.playbackController = m_playback_controller;
    ctx.playlistController = m_playlist_controller.get();
    ctx.libraryPanel = m_main_window->libraryPanel();
    ctx.controlBar = m_main_window->controlBarWidget();
    ctx.searchPanel = m_main_window->searchPanelWidget();
    ctx.desktopLyrics = m_main_window->desktopLyricsWidget();
    ctx.settingsPanel = m_settings_panel;
    ctx.shortcutsController = m_shortcuts_controller;

    ctx.windowsSec = m_window_config_section.get();
    ctx.playbackSec = m_playback_config_section.get();
    ctx.librarySec = m_library_view_section.get();
    ctx.searchSec = m_search_panel_section.get();
    ctx.desktopSec = m_desktop_lyrics_section.get();
    ctx.settingsSec = m_settings_panel_section.get();
    ctx.shortcutsSec = m_shortcuts_section.get();
    return ctx;
}

void AppController::applyConfig() {
    MainWindowConfigContext ctx = buildConfigContext();
    for (IConfigBinder* b : m_binders) {
        if (b) {
            b->apply(ctx);
        }
    }
    m_playback_restore_coordinator->restorePlaybackState();
}

void AppController::saveConfig() {
    if (!m_main_window) {
        return;
    }

    if (m_has_saved_config_on_exit) {
        return;
    }
    m_has_saved_config_on_exit = true;

    if (m_desktop_lyrics_section) {
        m_desktop_lyrics_section->is_visible = m_desktop_lyrics_visible_cache;
    }

    MainWindowConfigContext ctx = buildConfigContext();
    for (IConfigBinder* b : m_binders) {
        if (b) {
            b->collect(ctx);
        }
    }
    ConfigManager::getInstance().saveAll();
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
    }
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

    m_settings_panel = new SettingsPanel;
    m_settings_panel->setWindowFlag(Qt::Window, true);
    m_settings_panel->setAttribute(Qt::WA_DeleteOnClose, true);

    const QByteArray geo_cache = m_settings_panel_geo_cache;
    if (!geo_cache.isEmpty()) {
        m_settings_panel->restoreGeometry(geo_cache);
    }

    connect(m_settings_panel, &SettingsPanel::sgnStateSnapshot, this,
            [this](const QByteArray& geometry) {
                m_settings_panel_geo_cache = geometry;
            });

    connect(m_settings_panel, &QObject::destroyed, this, [this]() {
        m_settings_panel = nullptr;
        m_shortcuts_panel = nullptr;
    });
}

void AppController::ensureShortcutsPage() {
    ensureShortcutsController();

    if (!m_shortcuts_panel) {
        m_shortcuts_panel = new ShortcutsPanel(m_main_window.get());
        m_shortcuts_panel->setViewModel(m_shortcuts_controller->viewModel());

        connect(m_shortcuts_panel, &ShortcutsPanel::sgnDefaultConfig, this, [this]() {
            if (m_shortcuts_controller) {
                m_shortcuts_controller->resetAllToDefault();
            }
        });

        connect(m_shortcuts_panel, &ShortcutsPanel::sgnRestoreConfig, this, [this]() {
            if (m_shortcuts_controller && m_shortcuts_section) {
                m_shortcuts_controller->applyBindings(m_shortcuts_section->bindings);
            }
        });

        connect(m_shortcuts_panel, &ShortcutsPanel::sgnApplyConfig, this, [this]() {
            MainWindowConfigContext ctx = buildConfigContext();
            if (m_shortcuts_binder) {
                m_shortcuts_binder->collect(ctx);
            }
            ConfigManager::getInstance().saveAll();
        });
    }

    m_settings_panel->registerWidget(m_shortcuts_panel->getListItem(), m_shortcuts_panel);
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
            const QMediaPlayer* player = m_playback_controller->getMediaPlayer();
            if (player && player->playbackState() == QMediaPlayer::PlayingState) {
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

    m_search_panel = new PlaylistSearchPanel;
    m_search_panel->setWindowFlag(Qt::Window, true);
    m_search_panel->setAttribute(Qt::WA_DeleteOnClose, true);
    m_search_panel->setSearchBackend(m_search_backend.get());
    m_search_backend->warmup(m_playlist_controller->currentPlaylist());

    const QByteArray geoCache = m_main_window->searchPanelGeometryCache();
    if (!geoCache.isEmpty()) {
        m_search_panel->restoreGeometry(geoCache);
    }

    const QByteArray headerCache = m_main_window->searchPanelHeaderStateCache();
    m_search_panel->applyHeaderStateDeferred(headerCache);

    connect(m_playlist_controller->viewModel(), &QAbstractItemModel::modelReset,
            m_search_panel, [this]() {
        if (m_search_panel) {
            m_search_panel->applyHeaderStateDeferred(m_main_window->searchPanelHeaderStateCache());
        }
        if (m_search_backend) {
            m_search_backend->invalidate(playlistId{});
            m_search_backend->warmup(m_playlist_controller->currentPlaylist());
        }
    });

    connect(m_search_panel, &PlaylistSearchPanel::sgnRequestPlayTrack,
            m_main_window.get(), [this](const trackId& id) {
        auto* model = m_playlist_controller->viewModel();
        if (!model) return;
        if (id.isNull()) return;

        int queueIndex = model->playbackQueue().indexOf(id);
        if (queueIndex >= 0) {
            m_playlist_controller->play(queueIndex);
        }
    });

    connect(m_search_panel, &PlaylistSearchPanel::sgnStateSnapshot,
            m_main_window.get(), [this](const QByteArray& geometry, const QByteArray& header) {
        m_main_window->setSearchPanelStateCache(geometry, header);
    });

    connect(m_search_panel, &QObject::destroyed, this, [this]() {
        m_main_window->setSearchPanel(nullptr);
        m_search_panel = nullptr;
    });

    m_main_window->setSearchPanel(m_search_panel);
}
