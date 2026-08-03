# WusicPlayer 重构待办事项

> 阶段编号对应文件编号

## 阶段 0: 待办事项

- [x] 记录重构工作的全部待办项, 按阶段组织。原则与约束见 [`1-design-constraint.md`](1-design-constraint.md)。

## 阶段 1: 设计契约

- [x] 整理设计契约 -> `1-design-constraint.md`
- [x] 汇总待办事项 -> 本文档

## 阶段 2: 曲目身份改造(最底层,先做)

- [x] 步骤 1.1 类型层 + 全库重命名:`TrackId` / `EntryId` / `PlaylistId` 三类型分离;`Track` 上移 `core/types.h` 并新增字段与工厂;全库机械重命名完成(编译器兜底验证)
- [x] 步骤 1.2 路径规范化:`path` 增强(`canonical_path` / `case_fold` / `normalize_path`);`Track::from_filepath` / `from_entry` 与 `loadList*` 入口全部经过规范化
- [x] 步骤 1.3 `findTrackByID` 所有权:改为 `const Track*` + 生命周期注释,5 处调用点同步更新
- [x] 步骤 1.4 缓存格式不兼容更新(保持 `kSchemaVersion = 1`,无迁移;旧缓存按缺失字段降级处理)
- [x] 步骤 1.5 补 `test/playlist` 单测:`tb_playlist` 64 项断言(Track 身份/路径规范化/查找删除/元数据/round-trip/批处理加载/旧格式降级)

## 阶段 3: 新建音乐库模块 `model/library` + SQLite

- [x] CMake 引入 `Qt6::Sql`(顶层 COMPONENTS 与 `wusic_model` 链接均已就绪)
- [x] 设计并实现 `library` 模块:`library`(内存索引)/ `library_repo`(SQLite)/ `library_scanner`(worker 线程)/ `library_file_watcher` / `library_manager`(门面)
- [x] SQLite schema:`tracks`、`watched_folders`、FTS5 虚拟表(含同步触发器);对 `artist / title / album / missing` 建索引
- [x] 扫描器:递归遍历 + 标签解析 + 增量更新(size+mtime 快照对比,未变化跳过标签解析;missing 检测)
- [x] 文件监控:`QFileSystemWatcher`(根目录顶层)+ reconcile 定时扫描(默认 30 分钟)
- [x] 异步扫描 + 进度信号:worker 线程扫描,`sgn_scan_progress` / `sgn_scan_finished` / `sgn_library_changed`
- [x] 新增 `test/library` 单元测试:`tb_library` 63 项断言(内存索引/Repo round-trip/FTS5 搜索/增量扫描/missing/Manager 异步+DB 恢复)

> 决策落实:① `LibraryTrack` 独立实体(不复用 `Track`);② 内存索引 + SQLite 双写,只读走内存、搜索走 FTS5;③ Scanner 快照模式(不碰 DB,SQLite 单线程)。

## 阶段 4: 播放列表改为引用音乐库

- [x] `Playlist::add_track` 改为通过库解析:`PlaylistManager::resolve_track`(库中 → `source=library` 引用,库外 → 外部条目);`addFolder` 同步走解析;注入 `LibraryManager`(非拥有)
- [x] 外部条目支持(直接添加文件,不强制入库;`source=external` + 内联元数据)
- [x] 缺失文件处理:库变更(`sgn_library_changed`)时 `refreshLibraryTracks` 同步 meta/missing;视图 `Node::missing` 置灰;右键"Remove Missing Tracks"(`removeMissingTracks` 全链路)
- [x] 元数据解析优先走库缓存:解析时拷贝库元数据,布局层不再重复解析
- [x] 接线:`AppController` 创建并初始化 `LibraryManager`(AppDataLocation/library.db),注入 `PlaylistManager`,启动初始扫描
- [x] 测试:`tb_playlist` 新增库引用/缺失处理(75 项);`tb_library` 新增播放列表-库集成解析(77 项)

## 阶段 5: 搜索后端切换到 SQLite FTS5

- [x] 替换 `InMemorySearchBackend` → 新增 `LibrarySearchBackend`(直接搜音乐库);`in_memory_search_backend.*` 已移出 CMake(文件可删)
- [x] `search_model` 对接:搜索身份由条目 `EntryId` 改为库级 `TrackId`(`SearchHint::track_id`、`trackIdAt`、`sgnRequestPlayTrack` 全链路同步)
- [x] `LibraryRepo::search` / `LibraryManager::search` 支持 `SearchQueryMode`(Plain 短语 / Prefix+Fuzzy 单 token 前缀 `kw*`)+ `ORDER BY bm25` 排序
- [x] 搜索结果播放:双击 → `TrackId` → 库解析 `filepath` → `requestPlay` 直接播放(库曲目不在播放列表中)
- [x] 测试:`tb_library` 新增 FTS5 后端测试(Plain/Prefix/空关键字),87 项断言;FTS5 命中验证通过

## 回归修复:搜索面板无效 + 恢复记忆丢失

- [x] **搜索无效根因**:库从未被填充(无被监控目录)→ FTS5 无结果。修复:`PlaylistManager::addFolder` 同时把目录加入库监控(`add_watched_folder` → 异步扫描);`Playlist::upgradeExternalTracks` 在库变更时把已入库的外部条目升级为库引用(共享元数据 + 缺失跟踪)
- [x] **恢复记忆根因**:阶段 1 缓存格式破坏性改变后,旧缓存(缺 `entry_id`)加载时被**重新分配**条目 id → 配置保存的 `last_track_id` 永远匹配不上。修复:`loadJsonPlaylist`/`loadListBatched` 旧字段 `"id"` 回退复用为 `entry_id`(旧 `"id"` 语义即条目身份);旧缓存加载后一次性重写为新格式,稳定身份
- [x] 测试:`tb_playlist` 旧格式断言改为"id 保留" + 新增 `upgradeExternalTracks` 测试(83 项);`tb_library` 新增 `addFolder→库填充→条目升级` 端到端测试(98 项)

## 阶段 6: 拆分 AppController + 清理信号链

- [x] 抽取 `PanelCoordinator`(UI 面板开关、创建、生命周期):见 [`6-appcontroller-split.md`](6-appcontroller-split.md);设置/搜索/EQ/快捷键面板与子页(歌词/主题/媒体库/快捷键)全部移交,懒创建 + QPointer 复用;依赖 view 组件,置于顶层可执行目标(不可入 wusic_controller 库,避免 view→controller 循环依赖)
- [x] `AppController` 降级为纯组合根(只负责 `new` + `connect`):683 → 388 行;保留核心接线/菜单处理/配置/状态栏
- [x] 清理 proxy signal 转发链:移除 `LibraryInteractionService::sgnTrackPropertyRequested` 死转发(无消费方,TagWriteback 已由 AppController 直连 SongTableView);删除 `AppController::tag_edit_widget_` 死成员;面板打开信号(MainWindow/LibraryBrowserWidget/SidePanel)直连 PanelCoordinator
- [x] API 改造:`PlaylistManager::nextTrack/prevTrack` 与 `PlaylistController::nextTrack/prevTrack` 返回条目身份 `EntryId`(不再返回 filepath;导航+`setPlayTrack` 逻辑保留);新增 `PlaylistController::trackFilePath(EntryId)` 由播放层解析路径(为空则忽略);`PlaybackService` 上/下曲与自然结束 3 处改为「身份 → 解析 → 播放」。说明:返回 `EntryId` 而非库级 `TrackId`(播放列表上下文,外部条目无库级身份)。测试:`tb_playlist` 补 nextTrack/prevTrack 身份与导航断言;`tb_search_backend` 补 trackFilePath 解析

## 阶段 7+: 媒体库交互体系(设计见 `3-interaction-design.md`)

> 背景:搜索/库直接播放绕过播放列表,导致主面板信息不显示、Tab 定位失效。根治方案 = 三层内容模型(媒体库 / 播放列表 / 现在播放队列)。

- [x] **现在播放队列**(核心):设计见 [`4-playback-queue.md`](4-playback-queue.md);新增 `model/playback_queue/`(`QueueItem` 三态来源 / `PlaybackQueue` 容器+PlayMode 导航 / `PlaybackQueueService` 来源构建+JSON 持久化),与播放列表解耦;积累期纯新增不接线,切断点由 `sgn_play_requested` 接入播放。测试:`tb_playback_queue` 219 项断言(队列操作/当前项/导航/信号/持久化 round-trip/三种来源构建)
- [x] **媒体库控件**:设计见 [`5-library-browser.md`](5-library-browser.md);集成进 `LibraryWidget` 左侧面板(播放列表树下方,垂直 splitter);`LibraryBrowseModel`(预设分类 artist/album/genre/folder/year + FTS5 搜索 + 分组树默认折叠)、`LibraryBrowserWidget`(分类下拉 + 设置按钮占位 + 配置按钮 + 防抖搜索 + 树)、设置面板 "Media Library" 页(watched folders 唯一管理入口);双击 → `PlaybackQueueService::play_library_track` 入队即播。DSL 自定义规则后续与主视图 SortRule 统一改造。测试:`tb_library_browse` 50 项断言(分组/平铺/Unknown 归组/搜索/后注入)
- [x] **播放列表解析策略 + 配置**:`AddFilePolicy{by_operation,import_to_library,keep_external,always_ask}`;`PlaylistManager::addTrack/addFolder` 接受策略(by_operation 按操作类型:文件夹→同步入库、单文件→仅外部);import 时注册父目录/目录到库,扫描后 `upgradeExternalTracks` 自动升级为库引用;`PlaylistController` 决策(always_ask 弹窗)并持久化 `add_file_policy`(config key "playlist");设置面板 "Media Library" 页新增"添加未入库文件时"下拉。测试:`tb_add_file_policy`(keep_external/import/by_operation/全局策略/文件夹/库命中)17 项断言
- [~] **菜单栏清理**(第一步完成):移除与播放列表右键重复的 File 菜单项(Save/Copy/Rename/Remove current playlist,右键经 `LibraryInteractionService` 已接线);移除死代码 Help→Manual(从未连接);修复 View 菜单助记符冲突("Set sort rule (&R)" vs "Remove a column (&R)");同步清理 MainWindow 信号(`sgnSave/Copy/Rename/RemovePlaylistRequested`)与 AppController 对应连接。剩余(待各控件右键功能完善后):View 排序/列管理→播放列表右键、文件添加→媒体库控件右键、Playback EQ 归位
- [ ] **智能播放列表**(保存的查询):特化视图实时反映库变化,替代手工维护;"整库播放列表"由库控件承担

## 媒体库控件补充:持久化 + 搜索面板职责切分

- [x] 媒体库控件持久化:`LibraryBrowserWidget` 实现 `IConfigurable`(分类/关键字/树表头);`LibraryWidget` 持久化左侧 splitter 状态;`AppController` 注册 libraryBrowser 模块
- [x] Ctrl-F 搜索面板改回搜索当前播放列表:`AppController::search_backend_` 恢复 `InMemorySearchBackend`;`SearchHint` 增 `filepath`;`rebuildIndex` 适配阶段 5 `TrackId` 语义(库引用条目填 `library_track_id`,外部条目留空 + `filepath` 播放);双击优先 `sgnRequestPlayFile`。数据库 FTS5 仅媒体库控件使用。测试:`tb_search_backend` 17 项断言(库引用/外部条目/invalidate 重建)

## 跨阶段

- [x] **拆分 LibraryWidget**(组合根化):原 `LibraryWidget` 糅合播放列表导航/媒体库浏览/歌曲表三职责,已拆为 `view/playlist_tree/PlaylistTreeWidget`(列表树+双击切换+CRUD 右键)、`view/song_table/SongTableView`(歌曲表+列管理+播放右键+表头持久化)、`view/library_browser/LibraryBrowserWidget`(已有,已从 `LibraryWidget/` 目录迁出,旧目录删除);由 `MainWindow::buildCentralArea` 直接组合(左垂直:播放列表树+库控件;主水平:左+歌曲表);`LibraryWidget` 已删除;`LibraryInteractionService` 改注入两控件;`AppController` 全部 libraryPanel 引用迁移直连;splitter 状态持久化归 MainWindow("window"),表头归 SongTableView("song_table_view")
- [x] **修复布局回归(上中下排列)**:根因=`MainWindow::loadFromJson` 恢复 `splitter_orientation` 时 `toInt()` 默认 0,而 `Qt::Horizontal=1`/`Qt::Vertical=2`,配置缺失时强转出无效方向(QSplitter 按垂直处理)→ 主 splitter 变垂直。修复=`toInt(Qt::Horizontal)` 默认 + 仅当值为 1/2 时 `setOrientation`
- [ ] 全库 clang-format 格式化(已有 `.clang-format`,迁移完成后统一执行)
- [ ] 存量命名迁移: 函数/方法/变量 -> snake_case; 信号 -> `sgn_` 前缀;Qt 虚函数覆写除外
- [ ] 存量 widget 成员 -> 缩写前缀(如 `le_`、`btn_`),并尽量局部化到 `init_ui()`
- [ ] 迁移后全量回归验证 (编译 + 运行 + 测试)
- [ ] 整理代码结构: 插件头文件外置于`include/`路径, 新增 cmake 头文件库路径并迁移 `miniaudio` 以防止 clang-format影响
