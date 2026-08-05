# 阶段 1:曲目身份改造方案

## 1. 目标与边界

**目标**:建立"库级曲目身份 + 播放列表条目身份"分离的数据模型,为阶段 2(音乐库/SQLite)铺好类型地基;升级持久化格式并自动迁移旧缓存。

**明确不做**(属于后续阶段):

| 不做的事 | 归属阶段 |
|---|---|
| SQLite 音乐库、扫描、文件监控 | 阶段 2 |
| 播放列表按库解析、去重、元数据走库缓存 | 阶段 3 |
| 搜索后端换 FTS5 | 阶段 4 |
| AppController 拆分、信号链清理、`next_track` 返回 TrackId 等 API 改造 | 阶段 5 |

---

## 2. 身份模型设计

### 2.1 身份类型命名(PascalCase)

自定义类型一律 PascalCase;旧 camelCase(`trackId` / `playlistId`)为设计失误,本次一并修正。

```cpp
using TrackId    = QUuid;   // 库级曲目身份:全局唯一;首次入库时由库分配(阶段 2 落地,阶段 1 先保留类型)
using EntryId    = QUuid;   // 播放列表条目身份:播放列表内唯一(替代当前 trackId 的全部使用处)
using PlaylistId = QUuid;   // 播放列表身份(由 playlistId 更名)
```

> 关键决策:**全库机械重命名 `trackId` → `EntryId`、`playlistId` → `PlaylistId`(一次到位,删除旧别名)**。这样编译器全程兜底——漏改一处就编译失败。若同时保留新旧两个别名(类型相同),编译器不再报错,反而失去保护。

### 2.2 `Track` 结构(上移到 `core/types.h`)

`Track` 将同时被音乐库与播放列表使用,故从 `model/playlist/playlist.h` 上移到 `core/types.h`:

```cpp
enum class TrackSource {
    external,   // 外部条目:文件不在库中,元数据内联(当前所有条目的状态)
    library     // 库条目:引用库中曲目(阶段 3 后出现)
};

struct Track {
    EntryId  entry_id;            // 条目身份(播放列表内唯一)
    TrackId  library_track_id;    // 库级身份;外部条目为空
    TrackSource source = TrackSource::external;
    QString filepath;             // 规范化路径(强制不变量,见 2.3)
    TrackMetaData meta;
    bool missing = false;         // 文件缺失标记(阶段 3 使用,先落字段)

    static Track from_filepath(const QString& filepath);           // 新建外部条目
    static Track from_entry(EntryId eid, const QString& filepath); // 加载缓存用
};
```

- 用静态工厂收敛 `addTrack` / `addTrackWithId` 里重复的初始化逻辑。
- `meta.filepath` 与 `Track::filepath` 冗余:阶段 1 保留(标签编辑链路还在用),但建立**不变量:二者必须一致**,阶段 3 清理。
- 字段改名:`Track::tid` → `entry_id`、`SearchHint::track_id` → `entry_id`(语义对齐)。

### 2.3 路径规范化

`core/utils/path.hpp` 已有 `normalize_path()`(解析符号链接),阶段 1 增强:

```cpp
namespace utils::path {
QString canonical_path(const QString& p);  // 解析符号链接(已有)
QString case_fold(const QString& p);       // Windows: toLower;其他平台原样
QString normalize_path(const QString& p);  // canonical + case_fold + 分隔符统一
}
```

**强制不变量**:所有 `Track::filepath` 在 `add_track*` / `loadList` 入口必须过 `normalize_path()`,由 `Track::from_filepath` 集中保证。

---

## 3. 子步骤划分(每步可编译可运行)

### 步骤 1.1 — 类型层 + 全库重命名(核心)

1. `core/types.h`:新增 `TrackId` / `EntryId` / `PlaylistId`、`TrackSource`、`Track` 新字段与静态工厂;**删除旧别名 `trackId` / `playlistId`**。
2. 全库把使用处改名(机械操作,编译器兜底):

| 文件                                                   | 影响点                                             |
|--------------------------------------------------------|----------------------------------------------------|
| `core/types.h`                                         | 类型别名定义、`Track` 上移                         |
| `core/search_types.h`                                  | `SearchHint::track_id` → `entry_id`,类型 `EntryId` |
| `model/playlist/playlist.h/.cpp`                       | `Track::tid` → `entry_id`,工厂方法,`PlaylistId`    |
| `model/playlist/playlist_context.h/.cpp`               | `setPlayTrack` / `getPlayTrackId` / 信号           |
| `model/playlist/playlist_layout.h/.cpp`                | `Node.id` 类型                                     |
| `model/playlist/playlist_view_model.h/.cpp`            | `setActiveTrack` / `trackAt` 等                    |
| `model/playlist/playlist_manager.h/.cpp`               | 转发接口                                           |
| `model/playlist/playlist_repo.h/.cpp`                  | JSON 读写                                          |
| `model/search_model/*`                                 | 行→EntryId 映射                                    |
| `controller/PlaylistController.h/.cpp`                 | `removeTrack` 等                                   |
| `controller/search_backend/in_memory_search_backend.*` | `IndexedTrack`                                     |
| `service/tag_writeback_service.*`                      | `requestTrackProperty`                             |
| `service/library_interaction_service.h`                | 信号签名                                           |
| `service/playback_restore_service.*`                   | `findQueueIndexByTrackId`                          |
| `view/LibraryWidget/LibraryWidget.h`                   | 信号签名                                           |
| 其余引用 `playlistId` 的文件                           | 类型 `PlaylistId`                                  |

3. **验证**:编译 + 启动 + 手动走一遍"添加文件→播放→搜索→切列表"。

### 步骤 1.2 — 路径规范化落地

1. 增强 `utils/path.hpp`。
2. `Track::from_filepath` 内部强制 `normalize_path()`;`Playlist::addTrack` / `addTrackWithId` 改走静态工厂。
3. `PlaylistRepo::loadJsonPlaylist` 与 `.m3u/.wpl` 导入入口(`loadList`/`loadListBatched`)也过 normalize(外部文件可能用相对路径)。
4. **验证**:手动添加含子目录/符号链接的文件夹,确认无重复条目。

### 步骤 1.3 — `findTrackByID` 所有权(过渡方案)

按契约 §3.3,阶段 1 采取**低成本过渡**,真正的 `shared_ptr` 化留到阶段 3(那时条目才可能指向库对象):

```cpp
// playlist.h
// 非拥有:返回指针生命周期由所属 Playlist 管理,Playlist 未被修改/析构前有效;调用方不得 delete
const Track* findTrackByID(const EntryId& eid) const;
```

同步更新 5 处调用点(`playlist_manager.cpp` ×4、`tag_writeback_service.cpp` ×1)的 const 性 + 就近注释生命周期约束。

### 步骤 1.4 — 持久化格式不兼容更新(保持 kSchemaVersion = 1)

> 决策:不做兼容与迁移。`kSchemaVersion` 保持 `1`,直接破坏性改变格式;旧缓存读入时按缺失字段降级处理(`entry_id` 缺失 → 重新分配,`source` 缺失 → external)。

```jsonc
// 当前格式(不兼容旧格式)
{
  "schemaVersion": 1,
  "id": "...",
  "name": "...",
  "tracks": [
    {
      "entry_id": "...",          // 原 "id"
      "library_track_id": "",     // 可空
      "source": "external",       // "external" | "library"
      "filepath": "/path/normalized",
      "missing": false,
      "meta": { ... }
    }
  ]
}
```

- 写读统一走新格式(`writeJsonPlaylist` / `loadJsonPlaylist` / `loadListBatched`)。
- 旧缓存文件(含 `"id"` 字段)加载时:`entry_id` 缺失 → `from_filepath` 重新分配身份,其余字段按默认值处理。

### 步骤 1.5 — 测试

`test/playlist/tb_playlist.cpp` 目前是空的,补基础单测:

- `add_track` 生成唯一 `entry_id`、路径已规范化
- `from_filepath` 工厂(相对/符号链接路径 → 规范化结果)
- `find_track_by_id` 命中/未命中
- repo v2 序列化 round-trip
- **v1 → v2 迁移**:手工构造 v1 JSON → load 后字段正确

---

## 4. 风险与注意事项

1. **改名必须一次到位**:`types.h` 不能同时保留新旧两个别名,否则编译器失去兜底能力。
2. **缓存不可回退**:v2 写入后旧版本程序无法读——WIP 项目可接受,但提交说明要注明。
3. **第三方导入路径**:`.m3u/.wpl` 可能是相对路径,`loadList*` 入口必须 normalize。
4. **`meta.filepath` 冗余**:保持"与 `filepath` 一致"不变量,阶段 3 清理,不要在阶段 1 扩大范围。

---

## 5. 验收标准

- [ ] 全库无旧语义 `trackId` / `playlistId` 残留,条目身份统一 `EntryId`
- [ ] `Track` 上移至 `core/types.h`,含 `library_track_id` / `source` / `missing` 字段与静态工厂
- [ ] 所有 `add_track*` / 导入入口路径已规范化
- [ ] `findTrackByID` 返回 const 指针 + 生命周期注释
- [ ] 缓存 v2 可读写、v1 自动迁移
- [ ] 每个子步骤:编译通过 + 程序可启动 + 播放/列表/搜索基本功能正常
- [ ] `test/playlist` 有基础单测且通过
