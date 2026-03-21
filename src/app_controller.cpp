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
      m_playbackController(playbackController),
      m_playlistManager(std::make_unique<PlaylistManager>()),
      m_playlistController(std::make_unique<PlaylistController>(m_playlistManager.get(), nullptr, this)),
      m_mainWindow(std::make_unique<MainWindow>(m_playbackController, m_playlistController.get())),
      m_desktopLyricsSection(std::make_unique<DesktopLyricsSection>()),
      m_libraryViewSection(std::make_unique<LibraryViewSection>()),
      m_playbackConfigSection(std::make_unique<PlaybackConfigSection>()),
      m_searchPanelSection(std::make_unique<SearchPanelSection>()),
      m_windowConfigSection(std::make_unique<WindowConfigSection>()),
      m_settingsPanelSection(std::make_unique<SettingsPanelSection>()),
      m_shortcutsSection(std::make_unique<ShortcutsSection>()),
      m_desktopLyricsBinder(std::make_unique<DesktopLyricsBinder>()),
      m_libraryViewBinder(std::make_unique<LibraryViewBinder>()),
      m_playbackConfigBinder(std::make_unique<PlaybackConfigBinder>()),
      m_searchPanelBinder(std::make_unique<SearchPanelBinder>()),
      m_windowConfigBinder(std::make_unique<WindowConfigBinder>()),
      m_settingsPanelBinder(std::make_unique<SettingsPanelBinder>()),
      m_shortcutsBinder(std::make_unique<ShortcutsBinder>()),
      m_playbackRestoreCoordinator(std::make_unique<PlaybackRestoreCoordinator>(
                    m_playbackConfigSection.get(), m_playlistController.get(), m_playbackController, this))
{
    SortRule defaultRule;
    defaultRule.type = SortType::album;
    m_playlistController->viewModel()->setSingleGrouping(defaultRule);

    initializeConfig();
    ensureShortcutsController();
    m_desktop_lyrics_visible_cache = m_desktopLyricsSection->is_visible;
    applyConfig();
    initializeCoreConnections();
    configureDesktopLyricsWindowRelation();

    connect(m_mainWindow.get(), &MainWindow::sgnOpenSearchPanelRequested,
        this, &AppController::onOpenSearchPanelRequested);
    connect(m_mainWindow.get(), &MainWindow::sgnOpenSettingsPanelRequested,
        this, &AppController::onOpenSettingsPanelRequested);
    connect(m_mainWindow.get(), &MainWindow::sgnShowDesktopLyricsRequested,
        this, &AppController::handleShowDesktopLyricsRequested);

    connect(m_mainWindow.get(), &MainWindow::sgnAboutToClose,
            this, &AppController::saveConfig);

    connect(qApp, &QCoreApplication::aboutToQuit,
            this, &AppController::saveConfig);
}

AppController::~AppController() = default;

void AppController::showMainWindow() {
    if (m_mainWindow) {
        m_mainWindow->show();
    }
}

void AppController::initializeCoreConnections()
{
    auto* playlistController = m_playlistController.get();
    auto* playbackController = m_playbackController;
    auto* controlBar = m_mainWindow->controlBarWidget();
    auto* libraryPanel = m_mainWindow->libraryPanel();
    auto* sidePanel = m_mainWindow->sidePanel();
    auto* desktopLyrics = m_mainWindow->desktopLyricsWidget();

        connect(m_mainWindow.get(), &MainWindow::sgnPlayTrackRequested,
            this, &AppController::handlePlayTrackRequest);
    connect(desktopLyrics, &DesktopLyricsWidget::sgnVisibilityChanged, this, [this](bool visible) {
        m_desktop_lyrics_visible_cache = visible;
        if (m_desktopLyricsSection) {
            m_desktopLyricsSection->is_visible = visible;
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
            m_mainWindow->desktopLyricsWidget()->setLrcLine(currText, nextText);
        });
        connect(lyricsModel, &WLyricsModel::currentLineChanged, this, [this](){
            m_mainWindow->desktopLyricsWidget()->updateLineColor();
        });
    }

    connect(m_mainWindow.get(), &MainWindow::sgnImportFilesRequested, this, [playlistController]() { playlistController->importFiles(); });
    connect(m_mainWindow.get(), &MainWindow::sgnImportFolderRequested, this, [playlistController]() { playlistController->importDir(); });
    connect(m_mainWindow.get(), &MainWindow::sgnCreatePlaylistRequested, playlistController, &PlaylistController::createNewPlaylist);
    connect(m_mainWindow.get(), &MainWindow::sgnLoadPlaylist, playlistController, &PlaylistController::loadPlaylist);
    connect(m_mainWindow.get(), &MainWindow::sgnCopyPlaylistRequested, this, [playlistController]() { playlistController->copyPlaylist(); });
    connect(m_mainWindow.get(), &MainWindow::sgnRenamePlaylistRequested, this, [playlistController]() { playlistController->renamePlaylist(); });
    connect(m_mainWindow.get(), &MainWindow::sgnRemovePlaylistRequested, this, [playlistController]() { playlistController->removePlaylist(); });
    connect(m_mainWindow.get(), &MainWindow::sgnSavePlaylistRequested, this, [playlistController]() { playlistController->savePlaylist(); });
    connect(m_mainWindow.get(), &MainWindow::sgnSetSortRuleRequested, this, &AppController::handleSetSortRuleRequested);
    connect(m_mainWindow.get(), &MainWindow::sgnInsertColumnRequested, this, &AppController::handleInsertColumnRequested);
    connect(m_mainWindow.get(), &MainWindow::sgnRemoveColumnRequested, this, &AppController::handleRemoveColumnRequested);
    connect(m_mainWindow.get(), &MainWindow::sgnShowAboutMessagebox, this, &AppController::handleShowAboutMessagebox);

    connect(controlBar, &WControlBar::sgnBtnNextClicked, this, [this, playlistController, playbackController]() {
        QString nextTrack = playlistController->nextTrack(playbackController->playMode());
        if (!nextTrack.isEmpty()) {
            m_mainWindow->playTrackInUi(nextTrack);
        }
    });

    connect(controlBar, &WControlBar::sgnBtnPrevClicked, this, [this, playlistController, playbackController]() {
        QString prevTrack = playlistController->prevTrack(playbackController->playMode());
        if (!prevTrack.isEmpty()) {
            m_mainWindow->playTrackInUi(prevTrack);
        }
    });

    connect(playbackController, &PlaybackController::sgnMediaStateChanged,
            this, [this, playlistController, playbackController](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::MediaStatus::EndOfMedia) {
            QString nextTrack = playlistController->nextTrack(playbackController->playMode());
            if (!nextTrack.isEmpty()) {
                m_mainWindow->playTrackInUi(nextTrack);
            }
        }
    });

    connect(playlistController, &PlaylistController::requestPlay,
            this, [this](const QString& filepath) {
        m_mainWindow->playTrackInUi(filepath);
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

    auto* playlistController = m_playlistController.get();
    auto* libraryPanel = m_mainWindow->libraryPanel();
    auto* sidePanel = m_mainWindow->sidePanel();
    auto* playbackController = m_playbackController;

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
    auto* playlistController = m_playlistController.get();
    WSortTypeSetDialog dialog;
    if (dialog.exec() == QDialog::Accepted) {
        QString input = dialog.getText();
        playlistController->viewModel()->setSortExpression(input);
    }
}

void AppController::handleInsertColumnRequested()
{
    auto* playlistController = m_playlistController.get();
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
    auto* playlistController = m_playlistController.get();
    WColumnIndexDialog dialog(QObject::tr("Remove column"), QObject::tr("Input the column index except 0"), m_mainWindow.get());
    int maxIndex = playlistController->viewModel()->getColumns().size() - 1;
    dialog.setMaxIndex(maxIndex);
    dialog.setIndex(1);
    if (dialog.exec() == QDialog::Accepted) {
        playlistController->viewModel()->removeColumn(dialog.index());
    }
}

void AppController::handleShowAboutMessagebox() {
    QMessageBox* msg = new QMessageBox(m_mainWindow.get());
    msg->setWindowTitle("About");
    msg->setText("This is a ABOUT message box.");
    msg->setIcon(QMessageBox::Information);
    msg->setStandardButtons(QMessageBox::Ok);
    msg->show();
    msg->setAttribute(Qt::WA_DeleteOnClose);
}

void AppController::handleShowDesktopLyricsRequested()
{
    auto* desktopLyrics = m_mainWindow->desktopLyricsWidget();
    if (desktopLyrics) {
        m_desktop_lyrics_visible_cache = true;
        if (m_desktopLyricsSection) {
            m_desktopLyricsSection->is_visible = true;
        }
        configureDesktopLyricsWindowRelation();
        desktopLyrics->show();
    }
}

void AppController::configureDesktopLyricsWindowRelation()
{
    auto* desktopLyrics = m_mainWindow ? m_mainWindow->desktopLyricsWidget() : nullptr;
    if (!desktopLyrics) {
        return;
    }

    if (desktopLyrics->parentWidget() != nullptr) {
        desktopLyrics->setParent(nullptr);
    }
}

void AppController::refreshPlaylistView()
{
    auto* playlistController = m_playlistController.get();
    auto* libraryPanel = m_mainWindow->libraryPanel();

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
    cm.registerSection(m_windowConfigSection.get());
    cm.registerSection(m_playbackConfigSection.get());
    cm.registerSection(m_libraryViewSection.get());
    cm.registerSection(m_searchPanelSection.get());
    cm.registerSection(m_desktopLyricsSection.get());
    cm.registerSection(m_settingsPanelSection.get());
    cm.registerSection(m_shortcutsSection.get());
    cm.loadAll();

    m_binders.push_back(m_desktopLyricsBinder.get());
    m_binders.push_back(m_libraryViewBinder.get());
    m_binders.push_back(m_playbackConfigBinder.get());
    m_binders.push_back(m_searchPanelBinder.get());
    m_binders.push_back(m_windowConfigBinder.get());
    m_binders.push_back(m_settingsPanelBinder.get());
    m_binders.push_back(m_shortcutsBinder.get());
}

MainWindowConfigContext AppController::buildConfigContext() const {
    MainWindowConfigContext ctx;
    ctx.mainWindow = m_mainWindow.get();
    ctx.appController = const_cast<AppController*>(this);
    ctx.playbackController = m_playbackController;
    ctx.playlistController = m_playlistController.get();
    ctx.libraryPanel = m_mainWindow->libraryPanel();
    ctx.controlBar = m_mainWindow->controlBarWidget();
    ctx.searchPanel = m_mainWindow->searchPanelWidget();
    ctx.desktopLyrics = m_mainWindow->desktopLyricsWidget();
    ctx.settingsPanel = m_settingsPanel;
    ctx.shortcutsController = m_shortcutsController;

    ctx.windowsSec = m_windowConfigSection.get();
    ctx.playbackSec = m_playbackConfigSection.get();
    ctx.librarySec = m_libraryViewSection.get();
    ctx.searchSec = m_searchPanelSection.get();
    ctx.desktopSec = m_desktopLyricsSection.get();
    ctx.settingsSec = m_settingsPanelSection.get();
    ctx.shortcutsSec = m_shortcutsSection.get();
    return ctx;
}

void AppController::applyConfig() {
    MainWindowConfigContext ctx = buildConfigContext();
    for (IConfigBinder* b : m_binders) {
        if (b) {
            b->apply(ctx);
        }
    }
    m_playbackRestoreCoordinator->restorePlaybackState();
}

void AppController::saveConfig() {
    if (!m_mainWindow) {
        return;
    }

    if (m_has_saved_config_on_exit) {
        return;
    }
    m_has_saved_config_on_exit = true;

    if (m_desktopLyricsSection) {
        m_desktopLyricsSection->is_visible = m_desktop_lyrics_visible_cache;
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
            m_mainWindow->desktopLyricsWidget()->getActiveLineColor(),
            m_mainWindow->desktopLyricsWidget()->getInactiveLineColor());
    }
    connect(m_lyrics_settings_panel, &LyricsSettingPanel::sgnActiveColorChanged, this, [this](rgb_t rgb){
        m_mainWindow->desktopLyricsWidget()->setActiveLineColor(rgb);
    });
    connect(m_lyrics_settings_panel, &LyricsSettingPanel::sgnInactiveColorChanged, this, [this](rgb_t rgb){
        m_mainWindow->desktopLyricsWidget()->setInactiveLineColor(rgb);
    });
    connect(m_lyrics_settings_panel, &LyricsSettingPanel::sgnDisplayModeChanged, this, [this](bool is_two_line){
        m_mainWindow->desktopLyricsWidget()->setDisplayMode( is_two_line ? DisplayMode::TwoLine : DisplayMode::OneLine );
    });
    connect(m_lyrics_settings_panel, &LyricsSettingPanel::sgnFontChanged, m_mainWindow->desktopLyricsWidget(), &DesktopLyricsWidget::setLrcFont);

    m_settingsPanel->registerWidget(m_lyrics_settings_panel->getTitleItem(), m_lyrics_settings_panel);

    m_settingsPanel->show();
    m_settingsPanel->raise();
    m_settingsPanel->activateWindow();
}

void AppController::onOpenSearchPanelRequested() {
    ensureSearchPanel();

    m_searchPanel->show();
    m_searchPanel->raise();
    m_searchPanel->activateWindow();
}

void AppController::ensureSettingsPanel() {
    if (m_settingsPanel) {
        return;
    }

    m_settingsPanel = new SettingsPanel;
    m_settingsPanel->setWindowFlag(Qt::Window, true);
    m_settingsPanel->setAttribute(Qt::WA_DeleteOnClose, true);

    const QByteArray geo_cache = m_settingsPanelGeoCache;
    if (!geo_cache.isEmpty()) {
        m_settingsPanel->restoreGeometry(geo_cache);
    }

    connect(m_settingsPanel, &SettingsPanel::sgnStateSnapshot, this,
            [this](const QByteArray& geometry) {
                m_settingsPanelGeoCache = geometry;
            });

    connect(m_settingsPanel, &QObject::destroyed, this, [this]() {
        m_settingsPanel = nullptr;
        m_shortcutsPanel = nullptr;
    });
}

void AppController::ensureShortcutsPage() {
    ensureShortcutsController();

    if (!m_shortcutsPanel) {
        m_shortcutsPanel = new ShortcutsPanel(m_mainWindow.get());
        m_shortcutsPanel->setViewModel(m_shortcutsController->viewModel());

        connect(m_shortcutsPanel, &ShortcutsPanel::sgnDefaultConfig, this, [this]() {
            if (m_shortcutsController) {
                m_shortcutsController->resetAllToDefault();
            }
        });

        connect(m_shortcutsPanel, &ShortcutsPanel::sgnRestoreConfig, this, [this]() {
            if (m_shortcutsController && m_shortcutsSection) {
                m_shortcutsController->applyBindings(m_shortcutsSection->bindings);
            }
        });

        connect(m_shortcutsPanel, &ShortcutsPanel::sgnApplyConfig, this, [this]() {
            MainWindowConfigContext ctx = buildConfigContext();
            if (m_shortcutsBinder) {
                m_shortcutsBinder->collect(ctx);
            }
            ConfigManager::getInstance().saveAll();
        });
    }

    m_settingsPanel->registerWidget(m_shortcutsPanel->getListItem(), m_shortcutsPanel);
}

void AppController::ensureShortcutsController()
{
    if (m_shortcutsController) {
        return;
    }

    m_shortcutsController = new ShortcutsController(this);
    registerDefaultShortcuts();
}

void AppController::registerDefaultShortcuts()
{
    if (!m_shortcutsController || m_shortcuts_registered) {
        return;
    }

    m_shortcutsController->setScopeHost(ShortcutScope::Application, m_mainWindow.get());
    m_shortcutsController->setScopeHost(ShortcutScope::MainWindow, m_mainWindow.get());
    m_shortcutsController->setScopeHost(ShortcutScope::DesktopLyrics, m_mainWindow.get());

    m_shortcutsController->registerOperation(
        ShortcutActionId::save_playlist,
        "Save Playlist",
        ShortcutScope::PlaylistView,
        QKeySequence(Qt::CTRL | Qt::Key_S),
        [this](){
            m_playlistController.get()->savePlaylist();
        },
        m_mainWindow.get(),
        true
    );

    m_shortcutsController->registerOperation(
        ShortcutActionId::open_file,
        "Open File",
        ShortcutScope::PlaylistView,
        QKeySequence(Qt::CTRL | Qt::Key_O),
        [this](){ m_playlistController.get()->importFiles(); },
        m_mainWindow.get(),
        true
    );

    m_shortcutsController->registerOperation(
        ShortcutActionId::open_playlist,
        "Open playlist",
        ShortcutScope::PlaylistView,
        QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O),
        [this](){m_playlistController.get()->loadPlaylist();},
        m_mainWindow.get(),
        true
    );

    m_shortcutsController->registerOperation(
        ShortcutActionId::play_pause,
        "Play / Pause",
        ShortcutScope::Application,
        QKeySequence(Qt::Key_Space),
        [this]() {
            const QMediaPlayer* player = m_playbackController->getMediaPlayer();
            if (player && player->playbackState() == QMediaPlayer::PlayingState) {
                m_playbackController->pause();
            } else {
                m_playbackController->play();
            }
        },
        m_mainWindow.get(),
        true
    );

    m_shortcutsController->registerOperation(
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

    m_shortcutsController->registerOperation(
        ShortcutActionId::stop,
        "Stop",
        ShortcutScope::Application,
        QKeySequence(Qt::Key_S),
        [this]() {
            m_playbackController->stop();
        },
        m_mainWindow.get(),
        true
    );

    m_shortcutsController->registerOperation(
        ShortcutActionId::open_search,
        "Open Search Panel",
        ShortcutScope::MainWindow,
        QKeySequence(Qt::CTRL | Qt::Key_F),
        [this]() {
            onOpenSearchPanelRequested();
        },
        m_mainWindow.get(),
        true
    );

    m_shortcutsController->registerOperation(
        ShortcutActionId::show_hide_desktop_lyrics,
        "Show / Hide Desktop Lyrics",
        ShortcutScope::Application,
        QKeySequence(Qt::CTRL | Qt::Key_L),
        [this]() {
            auto* desktopLyrics = m_mainWindow->desktopLyricsWidget();
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
        m_mainWindow.get(),
        true
    );

    m_shortcuts_registered = true;
}

void AppController::ensureSearchPanel() {
    if (m_searchPanel) {
        return;
    }

    m_searchPanel = new PlaylistSearchPanel;
    m_searchPanel->setWindowFlag(Qt::Window, true);
    m_searchPanel->setAttribute(Qt::WA_DeleteOnClose, true);
    m_searchPanel->setSourceModel(m_playlistController->viewModel());

    const QByteArray geoCache = m_mainWindow->searchPanelGeometryCache();
    if (!geoCache.isEmpty()) {
        m_searchPanel->restoreGeometry(geoCache);
    }

    const QByteArray headerCache = m_mainWindow->searchPanelHeaderStateCache();
    m_searchPanel->applyHeaderStateDeferred(headerCache);

    connect(m_playlistController->viewModel(), &QAbstractItemModel::modelReset,
            m_searchPanel, [this]() {
        if (m_searchPanel) {
            m_searchPanel->applyHeaderStateDeferred(m_mainWindow->searchPanelHeaderStateCache());
        }
    }, Qt::SingleShotConnection);

    connect(m_searchPanel, &PlaylistSearchPanel::sgnRequestPlayTrack,
            m_mainWindow.get(), [this](const QModelIndex &source_index) {
        auto* model = m_playlistController->viewModel();
        if (!model) return;
        trackId id = model->trackAt(source_index);
        if (id.isNull()) return;

        int queueIndex = model->playbackQueue().indexOf(id);
        if (queueIndex >= 0) {
            m_playlistController->play(queueIndex);
        }
    });

    connect(m_searchPanel, &PlaylistSearchPanel::sgnStateSnapshot,
            m_mainWindow.get(), [this](const QByteArray& geometry, const QByteArray& header) {
        m_mainWindow->setSearchPanelStateCache(geometry, header);
    });

    connect(m_searchPanel, &QObject::destroyed, this, [this]() {
        m_mainWindow->setSearchPanel(nullptr);
        m_searchPanel = nullptr;
    });

    m_mainWindow->setSearchPanel(m_searchPanel);
}
