# 阶段 6:拆分 AppController + 清理信号链

> 对应 `0-todolist.md` 阶段 6;目标:`AppController` 降级为纯组合根,面板管理独立为 `PanelCoordinator`,信号链不超过一层转发。

---

## 1. 拆分前的问题

`AppController`(683 行)承担了六类职责:

1. **组合根**:创建/注入所有 manager、controller、service、widget
2. **核心内容接线**:播放/队列/媒体库/歌词/定位(Tab)
3. **菜单请求处理**:排序规则、列管理、About、桌面歌词
4. **配置**:模块注册(`initializeConfig`)、保存(`saveConfig`)
5. **面板管理**:设置面板/搜索面板/快捷键面板/EQ/标签编辑的创建、显示、生命周期
6. **快捷键注册**:`registerDefaultShortcuts`(14 个操作)
7. **状态栏**:播放状态、曲目数

其中 5/6 与"组合根"无关,混在一起导致 AppController 臃肿、职责不清。

## 2. 修改后的代码架构

```mermaid
graph TD
    subgraph AC[AppController(组合根)]
        AC1[创建 services/managers/widgets]
        AC2[核心接线:播放/队列/歌词/定位]
        AC3[菜单处理:排序/列/About/桌面歌词]
        AC4[配置:initializeConfig/saveConfig]
        AC5[状态栏]
    end

    subgraph PC[PanelCoordinator(面板编排)]
        PC1[SettingsPanel + 子页<br/>Lyrics/Theme/Media Library/Shortcuts]
        PC2[SearchPanel]
        PC3[EQWidget]
        PC4[ShortcutsController 注册]
        PC5[面板生命周期 + eventFilter]
        PC6[savePanelConfigs]
    end

    MW[MainWindow] -->|sgnOpenSettingsPanelRequested| PC
    MW -->|sgnOpenSearchPanelRequested| PC
    MW -->|sgnOpenEQWidgetRequested| PC
    LB[LibraryBrowserWidget] -->|sgnOpenLibrarySettingsRequested| PC
    SP[SidePanel] -->|sgnDesktopLyricsConfigRequested| PC
    AC -->|创建并注入| PC
```

## 3. PanelCoordinator(新增,`src/controller/panel_coordinator.{h,cpp}`)

**职责**:所有浮动面板/对话框的创建、显示、生命周期 + 快捷键注册 + 设置面板事件过滤。

```cpp
class PanelCoordinator : public QObject
{
    Q_OBJECT
public:
    PanelCoordinator(MainWindow*, PlaybackController*, PlaylistController*,
                     LibraryManager*, ThemeService*, InMemorySearchBackend*,
                     QObject* parent = nullptr);
    ShortcutsController* shortcutsController() const;
    void savePanelConfigs(); // 设置/快捷键/搜索面板 sub config 写入

public slots:
    void openSettingsPanel();                    // 默认页
    void openSettingsPanelPage(const QString&);  // 指定页("Media Library"/"Lyrics")
    void openSearchPanel();
    void openEQWidget();

protected:
    bool eventFilter(QObject*, QEvent*) override; // 设置面板隐藏 → 激活主窗口

private:
    void ensureSettingsPanel();
    void ensureShortcutsController();
    void ensureShortcutsPage();
    void registerDefaultShortcuts();
    void ensureSearchPanel();

    // 非拥有依赖
    MainWindow* main_window_;
    PlaybackController* playback_ctl_;
    PlaylistController* playlist_ctl_;
    LibraryManager* library_mgr_;
    ThemeService* theme_service_;
    InMemorySearchBackend* search_backend_;
    // 拥有(QPointer,懒创建)
    QPointer<SettingsPanel> settings_panel_;
    QPointer<ShortcutsPanel> shortcuts_panel_;
    QPointer<ShortcutsController> shortcuts_controller_;
    QPointer<SearchPanel> search_panel_;
    QPointer<LyricsSettingPanel> lyrics_settings_panel_;
    QPointer<ThemeSettingsPage> theme_settings_page_;
    QPointer<LibrarySettingsPage> library_settings_page_;
    QPointer<EQWidget> eq_widget_;
    bool has_shortcuts_registered_ = false;
};
```

## 4. AppController 瘦身后

**保留**(组合根职责):
- 创建/注入全部 services/managers/widgets;创建 `PanelCoordinator` 并注入
- `initializeCoreConnections`(播放/队列/媒体库/歌词/定位)+ `locateCurrentTrackInView`
- 菜单处理:`handleSetSortRuleRequested`/`handleInsertColumnRequested`/`handleRemoveColumnRequested`/`handleShowAboutMessagebox`/`handleShowDesktopLyricsRequested`
- 配置:`initializeConfig`(注册模块,shortcuts 经 `panel_coordinator_->shortcutsController()`)、`saveConfig`(调 `panel_coordinator_->savePanelConfigs()`)
- `setup_status_bar_connections`

**移除**(移交 PanelCoordinator):`onOpenSettingsPanelRequested`/`onOpenSearchPanelRequested`/`ensureSettingsPanel`/`ensureShortcutsPage`/`ensureShortcutsController`/`registerDefaultShortcuts`/`ensureSearchPanel`/`handleOpenEQRequested`/`eventFilter`,以及对应 9 个 QPointer 成员(含死成员 `tag_edit_widget_`,实际由 `TagWritebackService` 管理)。

## 5. 信号链清理

| 清理点 | 处理 |
|---|---|
| `LibraryInteractionService::sgnTrackPropertyRequested` 转发 | **死转发**(无消费方);`TagWritebackService` 已由 AppController 直连 `SongTableView::sgnTrackPropertyRequested`。移除转发信号 + connect |
| `AppController::tag_edit_widget_` | 死成员(实际由 `TagWritebackService::m_tag_edit_widget` 管理),删除 |
| 面板打开信号 | `MainWindow`/`LibraryBrowserWidget`/`SidePanel` → `PanelCoordinator`(一层),不再经 AppController 中转 |

## 6. 验证

- `-Werror` 全量编译零警告
- ctest 全部通过(无 view 单测,靠编译 + 启动验证)
- 启动:7 个 CONFIG 注册 + 面板打开/关闭正常

## 7. 实现结果(2026-08-03)

- `AppController`:683 → **388 行**;`PanelCoordinator`:**340 行**(`src/panel_coordinator.{h,cpp}`,顶层可执行目标)。
- **PanelCoordinator 归属**:因依赖 view 组件(ShortcutsPanel/SearchPanel/SettingsPanel/EQWidget),不能放入 `wusic_controller` 库(会触发 view→controller 循环依赖,链接报 `ShortcutsPanel::staticMetaObject` relocation),故置于 `WusicPlayer` 可执行目标(与 `app_controller` 同层)。
- 信号直连:MainWindow 的搜索/设置/EQ 信号、LibraryBrowserWidget 的媒体库配置信号、SidePanel 的歌词配置信号均直连 `PanelCoordinator`。
- 死转发清理:移除 `LibraryInteractionService::sgnTrackPropertyRequested`(无消费方;TagWriteback 已由 AppController 直连 SongTableView);删除 `AppController::tag_edit_widget_` 死成员。
- **API 改造(第 4 项)**:`PlaylistManager::nextTrack/prevTrack`、`PlaylistController::nextTrack/prevTrack` 返回 `EntryId`(条目身份,不再返回 filepath);新增 `PlaylistController::trackFilePath(EntryId)`;`PlaybackService` 上/下曲与自然结束改为「身份 → 解析 → 播放」。返回 `EntryId` 而非库级 `TrackId`(播放列表上下文;外部条目无库级身份)。
- 测试:`tb_playlist` 补 nextTrack/prevTrack 身份+导航断言;`tb_search_backend` 补 trackFilePath 解析;ctest 8/8 通过。
