# WusicPlayer 架构设计

> 面向新读者与维护者的架构说明. 历史设计决策与演进记录见 [`docs/history/`](history/). 

## 1. 总览

WusicPlayer 采用受 MVC 启发的**分层架构**, 自底向上为: 

```
view -> controller -> service -> model -> core
```

- **core**: 不依赖业务的最底层基础设施 (类型, 音频引擎, 配置, 主题, 工具) 
- **model**: 纯数据与视图模型, 无 UI 依赖
- **service**: 跨模块业务逻辑 (播放编排, 库交互, 标签写回, 主题应用, 恢复) 
- **controller**: 视图与模型/服务之间的桥梁 (播放, 列表, 快捷键, 搜索后端) 
- **view**: Qt Widgets UI 组件

顶层另有**组合根** `AppController` (`src/app_controller.*`) 与 **面板编排** `PanelCoordinator` (`src/panel_coordinator.*`) , 不属于任何库, 编译进可执行目标. 

```
src/
├── app_controller.*        # 组合根:new + connect
├── panel_coordinator.*     # 浮动面板(设置/搜索/EQ/快捷键)生命周期与快捷键注册
├── controller/             # wusic_controller
├── core/                   # wusic_core
├── model/                  # wusic_model
├── service/                # wusic_service
├── view/                   # wusic_view
└── static/                 # Qt 资源(图标/图片)
include/
└── plugin/                 # 外部插件接口(仅对外暴露的头文件)
thirdparty/
├── headeronly/             # 手动管理的 header-only 库(如 miniaudio.h)
├── lrc-parser/             # LRC 解析(子模块)
└── magic_enum/             # 枚举反射(子模块)
```

## 2. 分层细节

### core (`src/core/`) 
- `types.h` - 全局身份类型与数据结构
  - `TrackId` (库级曲目身份) , `EntryId` (播放列表条目身份) , `PlaylistId` (播放列表身份) 
  - `Track` / `TrackMetaData`, `SearchQueryMode`, 拖拽 MIME 常量等
- `player/` - 播放引擎 (FFmpeg 解码 + miniaudio 输出) : `player` / `player_engine` / `decoder` / `device`
- `config_manager/` - JSON 配置持久化 (`IConfigurable` 接口 + `ConfigManager` 单例) 
- `theme/` - 主题引擎 (QStyle 渲染 + 插件) : `ThemeManager`, `WusicProxyStyle`, 内置主题
- `lyrics_fetcher/` - 网络歌词获取 (网易云) 
- `utils/` - `audio.hpp` (TagLib 标签/封面解析) , `path.hpp` (路径规范化) 

### model (`src/model/`) 
- `playlist/` - 播放列表数据层
  - `Playlist` (条目容器) , `PlaylistRepo` (列表存储/缓存) , `PlaylistManager` (门面, 含跨列表添加/排序) , `PlaylistViewModel` (分组树视图模型, 含拖拽 mimeData) 
- `library/` - 音乐库
  - `LibraryRepo` (SQLite + FTS5) , `LibraryManager` (门面) , `LibraryScanner` (增量扫描) , `LibraryFileWatcher`, `LibraryBrowseModel` (媒体库分组浏览, 含拖拽 mimeData) 
- `playback_queue/` - 现在播放队列 (`PlaybackQueue` + `PlaybackQueueService`) 
- `search_model/`, `shortcuts_view_model/` - 搜索与快捷键的视图模型

### service (`src/service/`) 
- `playback_service` - 播放生命周期: `handle_play_track_request` (读文件 -> 更新封面/歌词/元数据面板) 
- `playback_restore_service` - 启动时恢复播放状态
- `library_interaction_service` - 播放列表树 / 歌曲表右键动作, 批量操作, 拖拽落点接线
- `tag_writeback_service` - 元数据写回文件
- `theme_service` - 主题应用与外部插件扫描

### controller (`src/controller/`) 
- `playback_controller` - 播放编排 (状态/位置/音量/设备/EQ) 
- `playlist_controller` - 播放列表增删改查, 播放, 跨列表复制, 拖动排序, 内联重命名
- `shortcuts_controller` / `status_bar_controller`
- `search_backend/` - 搜索后端抽象 (`ISearchBackend`) 与实现 (`InMemorySearchBackend` 播放列表内, `LibrarySearchBackend` 库) 

### view (`src/view/`) 
- `main_window.*` - 主窗口外壳与菜单栏
- `playlist_tree/` - 播放列表树 (单击选中 / 双击切换 / 慢单击内联重命名 / 右键 / 拖动排序 / 接收拖入) 
- `song_table/` - 当前列表歌曲表 (单选/多选右键 / 组节点箭头 / 接收拖入 / 拖拽源) 
- `library_browser/` - 媒体库浏览 (右键 / 拖拽源 / 双击播放) 
- `control_bar/`, `side_panel/`, `desktop_lyrics_widget/`, `eq_widget/`, `settings_panel/`, `search_panel/`, `dialogs/`, `tag_edit_widget/`

## 3. 关键机制

### 3.1 播放请求链路
所有播放最终汇入 `MainWindow::sgnPlayTrackRequested`: 

```
来源(双击/搜索/上下一首/打开文件)
  -> PlaybackService::handle_play_track_request(filepath)
  -> PlaybackController::read(filepath)          // 实际播放
  -> SidePanel::load_cover / load_meta_data      // 面板更新
```

- **列表内播放**: 先 `PlaylistController::locate_filepath` (更新 context -> song table 高亮) , 再播放
- **库直播 / 外部播放**: 曲目不在列表, 元数据走 `parse_to_local_meta` 文件标签兜底
- **现在播放队列** (`PlaybackQueueService`) 作为多播放来源 (媒体库/外部) 的入队即播入口, emit `sgn_play_requested`

### 3.2 音乐库搜索 (FTS5) 
`LibraryRepo::search` 两段式: 
1. **FTS5 前缀优先**: 按空白 (含全角) 分词, 每 token `"tok"*` AND, bm25 排序
2. **LIKE 子串兜底**: 无命中时每 token `%tok%` 在文本列 (title/artist/album/album_artist/genre) 子串匹配

这使 CJK (unicode61 下连续字符为单 token) 与英文均支持前缀 + 子串. 

### 3.3 拖拽 (列表与媒体库互通) 
- **MIME**: 
  - `application/x-wusic-library-tracks` - 库曲目 (TrackId JSON 数组) , 源为 `LibraryBrowseModel`
  - `application/x-wusic-playlist-entries` - 列表条目 (`{src, ids}`) , 源为 `PlaylistViewModel`
- **目标**: `PlaylistTreeDropView` (落到列表项 -> 添加/复制；背景忽略) , `SongTableDropView` (落到 -> 添加到当前列表) 
- 库浏览多选: 全组节点 或 全曲目行 有效；混合 -> 拒绝并记录日志

### 3.4 配置持久化
`IConfigurable` 接口 (`load_from_json` / `save_to_json` / `config_sub_key`) , `ConfigManager` 统一读写 `WusicPlayer.json`. 注册的模块见各 `config_sub_key()` 返回值 (`window`, `playback`, `playlist`, `song_table_view`, `library_browser`, `search_panel` 等) . 

### 3.5 主题系统
`ThemeManager` + `WusicProxyStyle` (QStyle 渲染, 非 QSS) . 外部主题通过实现 `IThemePlugin` (`include/plugin/` 对外接口) 编译为 `.so` 放入 `plugins/themes/`. 详见 [`THEME_SYSTEM.md`](THEME_SYSTEM.md). 

## 4. 命名约定

| 类别                            | 约定                                  | 示例                                      |
|---------------------------------|---------------------------------------|-------------------------------------------|
| 类 / 结构体 / 枚举              | PascalCase                            | `PlaybackController`, `TrackMetaData`     |
| 命名空间 / 函数 / 方法 / 变量   | snake_case                            | `add_library_tracks`, `m_lib`             |
| 信号                            | `sgn_` 前缀 + snake_case              | `sgn_playlist_changed`                    |
| 常量                            | `k` + PascalCase                      | `kLeafBase`                               |
| 枚举值                          | 全小写 snake                          | `SearchQueryMode::prefix`                 |
| Qt 虚函数覆写                   | 保留原名                              | `data()`, `paintEvent()`                  |
| 文件 / 路径                     | snake_case                            | `playlist_manager.cpp`, `settings_panel/` |

## 5. 构建与测试

- 依赖与平台构建: 见 [`BUILDING.md`](BUILDING.md)
- 测试: `cmake --preset debug && cd build/debug && ctest` (8 个测试模块) 
