#pragma once

#include <QObject>
#include <QPointer>

class EQWidget;
class InMemorySearchBackend;
class LibraryManager;
class LibrarySettingsPage;
class LogViewerDialog;
class LyricsSettingPanel;
class MainWindow;
class PlaybackController;
class PlaylistController;
class SearchPanel;
class SettingsPanel;
class ShortcutsController;
class ShortcutsPanel;
class ThemeService;
class ThemeSettingsPage;

class LogSinkGui;

/**
 * @brief UI 面板编排:所有浮动面板/对话框的创建、显示、生命周期,
 *        快捷键注册,以及设置面板的事件过滤。
 *
 * 依赖均为非拥有注入;面板对象为 QPointer 懒创建(可复用的 hide/show)。
 * AppController(组合根)只负责创建本对象并连接入口信号。
 */
class PanelCoordinator : public QObject
{
    Q_OBJECT
public:
    PanelCoordinator(MainWindow* main_window, PlaybackController* playback_ctl,
                     PlaylistController* playlist_ctl, LibraryManager* library_mgr,
                     ThemeService* theme_service, InMemorySearchBackend* search_backend,
                     LogSinkGui* gui_sink = nullptr, QObject* parent = nullptr);
    ~PanelCoordinator() override;

    // 懒创建并返回快捷键控制器(供配置注册)
    ShortcutsController* shortcuts_controller();

    // 设置/快捷键/搜索面板的 sub config 写入(应用退出时调用)
    void save_panel_configs();

public slots:
    void open_settings_panel();                          // 默认页
    void open_settings_panel_page(const QString& title); // 指定页(如 "Media Library"/"Lyrics")
    void open_search_panel();
    void open_eq_widget();
    void open_log_viewer();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void ensure_settings_panel();
    void ensure_shortcuts_controller();
    void ensure_shortcuts_page();
    void register_default_shortcuts();
    void ensure_search_panel();
    void ensure_log_viewer();

    // ---- 非拥有依赖 ----
    MainWindow* main_window_               = nullptr;
    PlaybackController* playback_ctl_      = nullptr;
    PlaylistController* playlist_ctl_      = nullptr;
    LibraryManager* library_mgr_           = nullptr;
    ThemeService* theme_service_           = nullptr;
    InMemorySearchBackend* search_backend_ = nullptr;
    LogSinkGui* gui_sink_                  = nullptr;

    // ---- 拥有(懒创建,可复用) ----
    QPointer<SettingsPanel> settings_panel_;
    QPointer<ShortcutsPanel> shortcuts_panel_;
    QPointer<ShortcutsController> shortcuts_controller_;
    QPointer<SearchPanel> search_panel_;
    QPointer<LyricsSettingPanel> lyrics_settings_panel_;
    QPointer<ThemeSettingsPage> theme_settings_page_;
    QPointer<LibrarySettingsPage> library_settings_page_;
    QPointer<LogViewerDialog> log_viewer_;
    QPointer<EQWidget> eq_widget_;
    bool has_shortcuts_registered_ = false;
};
