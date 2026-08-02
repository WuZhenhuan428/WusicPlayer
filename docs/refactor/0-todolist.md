# WusicPlayer 重构待办事项

> 阶段编号对应文件编号

## 阶段 0: 待办事项

- [x] 记录重构工作的全部待办项, 按阶段组织。原则与约束见 [`1-design-constraint.md`](1-design-constraint.md)。

## 阶段 1: 设计契约

- [x] 整理设计契约 -> `1-design-constraint.md`
- [x] 汇总待办事项 -> 本文档

## 阶段 2: 曲目身份改造(最底层,先做)

- [x] 引入库级曲目身份:`TrackId`(库级)/ `EntryId`(条目级)/ `PlaylistId` 三类型分离,全库机械重命名完成(编译器兜底验证)
- [x] `core/types.h` 改造:`Track` 上移至此,新增 `library_track_id` / `source` / `missing` 字段与静态工厂 `from_filepath` / `from_entry`
- [x] 路径规范化:`Track::from_filepath` / `from_entry` 已强制 `normalize_path()`(步骤 1.2 的核心部分已随工厂落地;`path_utils` 的 `case_fold` 增强与 `loadList*` 入口规范化待补)
- [ ] 播放列表缓存 `kSchemaVersion` 升级与迁移(步骤 1.4)
- [ ] 同步更新 `test/playlist` 测试(步骤 1.5)

## 阶段 3: 新建音乐库模块 `model/library` + SQLite

- [ ] CMake 引入 `Qt6::Sql`
- [ ] 设计并实现 `library` 模块:`library` / `library_repo` / `library_scanner` / `library_file_watcher` / `library_manager`
- [ ] SQLite schema:`tracks`、`watched_folders`、FTS5 虚拟表;对 `path / artist / title / album / missing` 建索引
- [ ] 扫描器:递归遍历 + 标签解析 + 增量更新(比较 size+mtime)
- [ ] 文件监控:`QFileSystemWatcher`(根目录顶层)+ reconcile 定时扫描
- [ ] 异步扫描 + 进度信号
- [ ] 新增 `test/library` 单元测试(扫描、去重、移动检测、FTS5)

## 阶段 4: 播放列表改为引用音乐库

- [ ] `Playlist::add_track` 改为通过库解析
- [ ] 外部条目支持(直接添加文件,不强制入库)
- [ ] 缺失文件处理(保留条目标 `missing`,UI 置灰,提供移除/定位操作)
- [ ] 元数据解析优先走库缓存

## 阶段 5: 搜索后端切换到 SQLite FTS5

- [ ] 替换 `InMemorySearchBackend`
- [ ] `search_model` 对接
- [ ] 万级曲库搜索性能验证(目标亚秒级)

## 阶段 6: 拆分 AppController + 清理信号链

- [ ] 抽取 `PanelCoordinator`(UI 面板开关、创建、生命周期)
- [ ] `AppController` 降级为纯组合根(只负责 `new` + `connect`)
- [ ] 清理 proxy signal 转发链(转发不超过一层)
- [ ] API 改造:`next_track` / `prev_track` 返回 `trackId` 而非 filepath 等

## 跨阶段

- [ ] 全库 clang-format 格式化(已有 `.clang-format`,迁移完成后统一执行)
- [ ] 存量命名迁移: 函数/方法/变量 -> snake_case; 信号 -> `sgn_` 前缀;Qt 虚函数覆写除外
- [ ] 存量 widget 成员 -> 缩写前缀(如 `le_`、`btn_`),并尽量局部化到 `init_ui()`
- [ ] 迁移后全量回归验证 (编译 + 运行 + 测试)
- [ ] 整理代码结构: 插件头文件外置于`include/`路径, 新增 cmake 头文件库路径并迁移 `miniaudio` 以防止 clang-format影响
