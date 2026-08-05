# 媒体库控件(Library Browser)设计文档

> 编号对应 `0-todolist.md` 阶段 7+ ②;依据 `3-interaction-design.md` §4。
> 状态:设计已与用户确认(2026-08-03),进入实现。

---

## 1. 目标与定位

- 侧边栏**播放列表下方的独立浏览面板**:库可浏览 / 分组 / 实时 FTS5 搜索 / 入队即播。
- 文件添加(注册 watched folder)= **媒体库唯一入口**,由"配置"按钮进入设置面板的"Media Library"页管理。
- 与现有 SearchPanel 职责切分:**Ctrl-F 搜索面板改回搜索当前播放列表**;数据库(FTS5)搜索仅在本控件使用。

## 2. 用户确认的 UI 结构

每一行是一个 `QHBoxLayout`,整体组合为 `QVBoxLayout`:

```text
[QComboBox 分类:artist/album/genre/folder/year] [QPushButton 设置(DSL 后续)] [QPushButton 配置→设置面板-媒体库]
[QLineEdit 搜索关键字(实时 FTS5,结果按当前分类分组)]
[QTreeView 浏览/搜索结果(分组节点默认折叠)]
```

已确认决策:
- **放置位置**:集成进 `LibraryWidget` 左侧面板(播放列表树下方,垂直 splitter)。
- **DSL 范围**:初版只支持预设分类(ComboBox);DSL 自定义规则与主视图 `SortRule` 体系后续统一改造。
- **播放行为**:双击库曲目 → `PlaybackQueueService` 入队并设为当前 → `sgn_play_requested` 驱动播放。

## 3. 模块结构

```text
src/model/library/library_browse_model.h/cpp   # 浏览模型(分组树 + 搜索)
src/view/LibraryWidget/LibraryBrowserWidget.h/cpp # 控件(顶栏 + 搜索 + 树)
src/view/SettingsPanel/library_settings_page.h/cpp # 设置面板"Media Library"页
```

## 4. LibraryBrowseModel(模型层)

### 4.1 分类枚举

```cpp
enum class LibraryGrouping
{
    none = 0, // 平铺(单组)
    artist, album, genre, folder, year
};
```

分组键映射:`artist→meta.artist`、`album→meta.album`、`genre→meta.genre`、
`folder→QFileInfo(filepath).absolutePath()`、`year→meta.year`;空值归入 "(Unknown)"。

### 4.2 数据流

| 状态 | 数据源 |
|---|---|
| 无关键字 | `LibraryManager::index()`(全量,内存索引) |
| 有关键字 | `LibraryManager::search(keyword, Plain, 500)`(FTS5) |

结果统一构建为分组树:`根 → 分组节点(组名+计数) → 曲目行(title/artist/album/duration)`。

- 组内排序:track_number → title;组间按 key 排序(year 按数值)。
- 分组节点默认折叠。
- `LibraryManager::sgn_library_changed` → 自动 `refresh()`(重建,积累期简单处理)。
- 列:0=Title、1=Artist、2=Album、3=Duration。

### 4.3 模型接口

```cpp
class LibraryBrowseModel : public QAbstractItemModel
{
public:
    explicit LibraryBrowseModel(LibraryManager* lib, QObject* parent = nullptr);
    void set_grouping(LibraryGrouping g);   // 重建
    void set_keyword(const QString& kw);    // 重建(空=全量)
    void refresh();                          // 重新拉取库数据
    std::optional<TrackId> track_id_at(const QModelIndex& index) const; // 叶节点
    // QAbstractItemModel 标准接口(index/parent/rowCount/data/headerData)
};
```

## 5. LibraryBrowserWidget(视图层)

- 持有 `LibraryBrowseModel`(自有)+ `LibraryManager*`(非拥有)。
- 顶栏:分类 ComboBox(切换 `set_grouping`)、设置按钮(禁用占位,DSL 后续)、配置按钮。
- 搜索:`QTimer` 防抖(200ms)→ `set_keyword`,实时 FTS5。
- 双击:
  - 曲目行 → `emit sgnPlayRequested(track_id)`(由 AppController 走队列入队即播)
  - 分组节点 → 展开/折叠
- 信号:`sgnPlayRequested(TrackId)`、`sgnOpenLibrarySettingsRequested()`。

## 6. LibraryWidget 集成

左侧改为垂直 splitter:

```text
QSplitter(Horizontal)
├── QSplitter(Vertical)          # 左侧
│   ├── 播放列表树(上,可折叠)
│   └── LibraryBrowserWidget(下,可折叠)
└── 歌曲表(右)
```

新增方法(转发给媒体库控件):`setLibraryManager(LibraryManager*)`、`setPlaybackQueueService(PlaybackQueueService*)`。
新增转发信号:`sgnLibraryPlayRequested(TrackId)`、`sgnOpenLibrarySettingsRequested()`。

## 7. PlaybackQueueService 播放入口(积累期)

```cpp
bool play_library_track(const TrackId& track_id);   // 入队 + set_current + emit sgn_play_requested
int  play_external(const QString& filepath, const TrackMetaData& meta = {});
signals:
    void sgn_play_requested(const QueueItem& item);  // AppController 接 → playTrackInUi(filepath)
```

切断点时 `sgn_play_requested` 改接 `PlaybackService`,无需改视图。

## 8. 设置面板"Media Library"页

- 标题 "Media Library";`LibraryManager*`(非拥有)。
- UI:watched folders 列表(`QListWidget`)+ "Add folder" / "Remove" 按钮。
- 添加 → `LibraryManager::add_watched_folder`(去重)→ 触发异步扫描。
- 配置按钮:打开设置面板并 `switchToPageByTitle("Media Library")`。

## 9. 测试计划(`test/library/tb_library_browse.cpp` 或并入 tb_library)

1. 分组键映射:artist/album/genre/folder/year 及空值 "(Unknown)"。
2. 分组树结构:组数、组内曲目数、平铺(none)单组。
3. 排序:组内 track_number → title。
4. 关键字:FTS5 搜索结果分组;空关键字回退全量。
5. `track_id_at` 叶节点解析正确。

## 10. 持久化(已实现)

- `LibraryBrowserWidget` 实现 `IConfigurable`(`configSubKey = "library_browser"`):分类、关键字、树表头状态。
- `LibraryWidget` 增加左侧垂直 splitter 状态(`left_splitter_state`)。
- `AppController::initializeConfig` 注册 libraryBrowser 模块。

## 11. 搜索面板职责切分(已实现)

- Ctrl-F 搜索面板改回**搜索当前播放列表**:`AppController::search_backend_` 恢复为 `InMemorySearchBackend`(按列表建索引,Plain/Prefix/Fuzzy 打分)。
- `SearchHint` 增加 `filepath`(播放依据);`InMemorySearchBackend::rebuildIndex` 适配阶段 5 语义:`track_id` = 库引用条目的 `library_track_id`(外部条目为空)、`filepath` = 条目路径。
- 双击:优先 `sgnRequestPlayFile(filepath)`(直接 `requestPlay` 定位回列表),库身份兜底 `sgnRequestPlayTrack`。
- 数据库(FTS5)搜索仅媒体库控件使用(直接走 `LibraryManager::search`)。

## 12. 后续(不在本阶段)

- DSL 自定义分组规则 + 分类设置弹窗(与主视图 SortRule 统一改造)。
- 拖拽文件/文件夹 → 注册 watched folder。
- 右键菜单(播放/加入队列/添加到列表/编辑标签/打开所在文件夹/重新扫描)。
