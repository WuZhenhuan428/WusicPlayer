# WusicPlayer 重构设计契约

## 0. 文档说明

- **目的**: 本文件是重构工作的"契约", 规定所有新代码与重构代码必须遵守的设计约束
- **适用范围**: `src/` 下所有代码, 以及本次重构涉及的全部改动
- **与其他文档的关系**: `docs/design-document.md` 描述系统 *是什么* (架构、模块、数据流); 本文件规定 *怎么做* 的硬性约束(命名、所有权、依赖方向、接口约定)
- **修改流程**: 契约需要修改时, 先在讨论中确认再更新本文件, 并同步更新 `0-todolist.md` 中的相关阶段

---

## 1. 通用设计原则

### 1.1 避免抽象泄漏

抽象不应向调用方暴露实现细节

具体要求:

1. **接口签名不暴露内部存储结构**. 例如:播放列表/音乐库的对外接口返回 `trackId`, 不把文件路径字符串当作身份标识; 调用方需要路径时通过库显式解析
2. **信号参数不传递内部数据结构或裸指针**, 优先传值类型 / const 引用
3. **model 层不依赖任何 view 类型**; view 只能通过 model / controller 的公开接口交互
4. **主键由所属模块自己定义并对外提供解析方法**, 调用方不得假设其实现(如不能假设 `trackId` 就是文件路径)

### 1.2 最小更新原则

1. 每次完成一个功能的重构后, 必须保证程序**能编译运行**, 并至少进行最小验证 (启动程序、执行该功能对应的基本操作)
2. 禁止 big-bang 重写: 重构必须分阶段, 每步可编译、可运行、可独立提交
3. 重构期间只动结构, 不动行为;行为变化必须单独提交并附带验证
4. 每个阶段结束, 相关单元测试必须通过

### 1.3 分层与依赖方向

依赖方向单向, 禁止反向依赖与循环依赖:

```text
view -> controller -> service -> model -> core
```

- `core`: 不依赖任何上层; 只含类型定义、工具、独立引擎
- `model`: 只依赖 `core`; 不依赖 service / controller / view
- `service`: 依赖 model / core, 可被多个 controller 复用
- `controller`: 依赖 service / model / core
- `view`: 依赖 controller / model / core 的公开接口, 不直接操作 service 内部
- 音乐库与播放列表: **playlist 依赖 library, library 不依赖 playlist**
- 组合根(`AppController`)是唯一被允许 *认识所有模块* 的类, 但它只做组装

### 1.4 信号方向与转发

1. **数据信号自下而上**: model / service -> view (通知数据变化)
2. **命令信号自上而下**: view / controller -> model / service (请求执行操作)
3. 一个类不应同时充当 *命令接收方* 与 *数据广播方* 的双重角色, 避免依赖图混乱
4. 信号转发 (proxy signal) 最多允许一层; 出现第二层转发时必须重构 (直接连接或重新设计)
5. 信号统一 `sgn_` 前缀; 参数使用值类型 / const 引用, 禁止传递裸指针

### 1.5 组合根与"上帝类"约束

1. `AppController` 降级为**纯组合根**: 只负责 `new` + `connect`, 不承载业务逻辑与 UI 流程
2. UI 面板的开关、创建、生命周期管理抽取为独立的 `PanelCoordinator`
3. 跨模块编排逻辑下沉到 `service` 层
4. 任何类若同时承担"对象组装 + 业务编排 + UI 流程"三类职责, 视为违反契约, 必须拆分

---

## 2. 命名规范

### 2.1 标识符

| 类别                               | 规则                  | 示例                                         |
|------------------------------------|-----------------------|----------------------------------------------|
| 自定义类型(类、结构体、枚举、别名) | PascalCase            | `TrackMetaData`、`PlaylistManager`           |
| 命名空间                           | snake_case            | `core::player`、`model::library`             |
| 函数 / 类方法                      | snake_case            | `create_playlist()`、`get_playlist_id()`     |
| 局部变量                           | snake_case            | `playlist_id`                                |
| 成员变量                           | snake_case + `_` 后缀 | `m_context`、`m_repo`                        |
| 常量                               | `k` 前缀 + PascalCase | `kSchemaVersion`                             |
| 枚举值                             | snake_case(全小写)    | `SortType::not_sorted`、`PlayMode::in_order` |

**例外(必须保留 Qt 原始命名)**

- 覆写 Qt 虚函数时保留原签名与命名: `data()`、`rowCount()`、`index()`、`parent()`、`headerData()`、`sort()`、`paintEvent()`、`eventFilter()` 等
- 连接 Qt 原生信号 / 槽时使用 Qt 提供的名字

### 2.2 Qt 信号与槽

- 信号统一 `sgn_` 前缀 + snake_case:`sgn_position_changed`、`sgn_track_property_requested`
- 槽使用 snake_case (不强制前缀)
- 现有混用命名 (`sgnTrackPropertyRequested`、`playlistChanged`) 在迁移阶段统一修改

### 2.3 Qt Widget 缩写前缀(局部使用)

widget 前缀仅用于**局部作用域**, 最典型的是 `init_ui()` 内的局部变量; 成员级 UI 控件应尽量减少暴露(封装在类内部)

常用缩写:

| 前缀    | 类型                      |
|---------|---------------------------|
| `le_`   | QLineEdit                 |
| `btn_`  | QPushButton               |
| `lbl_`  | QLabel                    |
| `cmb_`  | QComboBox                 |
| `chk_`  | QCheckBox                 |
| `rb_`   | QRadioButton              |
| `tv_`   | QTreeView                 |
| `lv_`   | QListView                 |
| `tw_`   | QTableWidget              |
| `tab_`  | QTabWidget                |
| `sl_`   | QSlider                   |
| `sp_`   | QSpinBox / QDoubleSpinBox |
| `pgb_`  | QProgressBar              |
| `grp_`  | QGroupBox                 |
| `fr_`   | QFrame                    |
| `act_`  | QAction                   |
| `menu_` | QMenu                     |

> 规则:缩写前缀只解决"局部 UI 构建代码的易读性", 不允许扩散到业务层.

### 2.4 格式:clang-format

- 格式由仓库根目录 `.clang-format` 统一管理 (基于 LLVM、4 空格缩进、Allman 大括号、ColumnLimit 100、PointerAlignment Left)
- 提交前必须执行 clang-format, 保证全库风格一致
- 命名迁移完成后统一跑一次全库格式化, 避免反复 diff

### 2.5 存量代码迁移

- 存量代码按阶段逐一迁移到上述命名规范 (见 `0-todolist.md` 跨阶段任务)
- 迁移只改命名与格式, 不改行为; 迁移后必须回归验证

---

## 3. 指针所有权与生命周期

### 3.1 所有权标注约定

| 形式                 | 语义             | 说明                                                                   |
|----------------------|------------------|------------------------------------------------------------------------|
| `std::unique_ptr<T>` | 独占所有权       | 成员对象首选, 类析构时释放                                             |
| `std::shared_ptr<T>` | 共享所有权       | 仅当确需共享(如 `Track` 同时被库与播放列表引用)                        |
| `QPointer<T>`        | Qt 对象安全引用  | 指向可能被销毁的 QObject, 自动失效                                     |
| 裸指针 `T*`          | 借用 / 非拥有    | **必须**在接口注释中标注"非拥有, 生命周期由 X 管理"; 禁止调用方 delete |
| QObject parent 体系  | 父对象拥有子对象 | 用 `new Foo(parent)` 创建的子对象归父对象管                            |

### 3.2 getter / API 返回约定

1. 优先返回**值类型或 const 引用**, 避免返回裸指针
2. 返回裸指针时, 必须在 getter 声明处 (头文件注释) 标注所有权与有效范围
3. 调用方在获取借用指针处, 应就近注释其生命周期约束 (如"非拥有, 勿 delete; 仅在本函数内有效")
4. 禁止返回内部容器的可变引用 / 指针 (如 `QVector<Track>&` 直接暴露)
5. 生命周期跨越异步调用时, 必须使用 `QPointer` / `shared_ptr` 或复制值, **禁止裸指针跨线程**

### 3.3 重点迁移项

- `Playlist::findTrackByID()` 返回裸 `Track*`: 改为返回 `std::shared_ptr<Track>` 或 const 引用 + 生命周期注释
- 库中曲目与播放列表条目共享元数据时, 统一走库的解析接口, 不复制裸指针

---

## 4. 曲目身份设计

> 身份类型统一 PascalCase 命名(自定义类型规则):`TrackId`(库级曲目身份)、`EntryId`(播放列表条目身份)、`PlaylistId`(播放列表身份)。旧 camelCase(`trackId`/`playlistId`)为设计失误,已在阶段 1 修正。

1. **库内曲目身份**:首次扫描时分配 `TrackId`;规范化路径作为查找索引;库负责路径 ↔ `TrackId` 双向解析。
2. **播放列表引用**:播放列表条目以 `EntryId` 为条目身份,引用库的 `TrackId`;条目可带"外部条目"标志(文件不在库中,`Track::source == TrackSource::external`)。
3. **路径规范化**:统一 canonical path(Windows 大小写不敏感、符号链接解析),作为去重与匹配的基础;`Track::filepath` 强制规范化(由 `Track::from_filepath` 保证)。
4. **文件重命名检测**:通过"路径变化但 size+mtime 一致"的启发式将 `TrackId` 迁移到新路径,播放列表引用不失效。
5. 对外 API(如 `next_track`)只暴露 `EntryId`/`TrackId`,路径由调用方按需解析。

---

## 5. 数据存储设计

### 5.1 音乐库 -> SQLite

- 使用 `QtSql` 的 QSQLITE 驱动 (CMake 增加 `Qt6::Sql`)
- 表结构
  - `tracks(id INTEGER PK,  uuid TEXT UNIQUE,  path TEXT UNIQUE,  size,  mtime,  duration_ms,  missing,  artist,  title,  album,  genre,  ...)`, 对 `path / artist / title / album / missing` 建索引 `watched_folders(path TEXT PRIMARY KEY)`
  - FTS5 虚拟表 (`tracks_fts`) 用于全文搜索
- 注意确认目标平台 Qt 内置 SQLite 是否编译 FTS5

### 5.2 播放列表 -> JSON 缓存(保持现状)

- 播放列表规模小、人可读、需导出, 维持 JSON 缓存, 不进 SQLite
- 库索引与播放列表缓存**分开存储**, 互不耦合
- 格式变化做**不兼容更新**(保持 `kSchemaVersion` 不变, 阶段 1 决策); 旧缓存读入时按缺失字段降级处理 (如 `entry_id` 缺失则重新分配身份)

---

## 6. 播放列表与外部条目

1. **无"临时/匿名音乐库"概念**: 播放列表条目 = 曲目引用 + 可选内联元数据
2. 文件在库中 -> 引用库 UUID, 元数据实时同步
3. 文件不在库中 (直接添加) -> 外部条目, 元数据按需解析并缓存; 可(可选)懒导入库
4. **缺失文件**: 文件被删除 / 移动后, 条目保留并标记 `missing`, UI 置灰; 提供"移除缺失条目"与"重新定位文件"操作

---

## 7. 文件变化检测

1. 混合策略:
   - `QFileSystemWatcher` 监控用户添加的根目录(顶层), 提供增量即时反馈
   - 启动时 + 定时 **reconcile 扫描**:比较目录 mtime, 跳过未变化的目录
2. 事件分类处理:
   - 新增 -> 入库
   - 删除 -> 标记 `missing`
   - 改动 -> 重读标签(注意 `tag_writeback_service` 写回也会触发事件, 不得误删条目)
3. 扫描必须异步执行, 通过进度信号回报, 不阻塞 UI 线程

---

## 8. 错误处理

1. Qt 依赖部分不使用 C++ 异常; 用返回值 (bool / 结果枚举)、错误信号、日志组合表达
2. 无 Qt 依赖部分尽量不使用异常，至少保证异常不外抛
3. 错误分类: 文件 I/O, 标签解析, 网络, 解码, 设备
4. 错误信息通过日志 (qWarning / qCritical) 记录; 需要用户感知的通过界面信号上抛
5. 异步操作的失败必须通过信号明确告知调用方, 不得静默吞掉

---

## 9. 线程与并发

1. UI 线程不执行耗时操作(扫描、标签解析、网络请求)
2. 后台任务通过 Qt 信号/槽跨线程传递结果, **禁止跨线程共享裸指针**
3. 数据归属明确: 库/播放列表的数据读写集中在所属线程(默认主线程);扫描器只产出结果再投递
4. 竞态由模块内部封装, 不向调用方暴露锁

---

## 10. 测试要求

1. 每个重构阶段: 编译通过 + 程序可启动 + 相关功能最小验证 + 已有测试通过
2. 新增 `test/library` 测试, 重点覆盖: 扫描、去重、文件重命名迁移、缺失文件标记、FTS5 搜索
3. 播放列表身份改造后, `test/playlist` 同步更新
4. 性能目标: 万级曲库下, 扫描、搜索、过滤保持可用(搜索建议亚秒级)

---

## 11. 技术栈约束

1. C++23, Qt 6.5+, GCC
2. 使用 include 而非 module
3. 不新增重型第三方依赖; 确需引入时必须先评估并在契约中记录
4. 新增依赖以 header-only 或系统包 / 子模块方式纳入 (参照现有 magic_enum、lrc-parser 的做法)

> Qt 6.5+ 正式支持的 C++ 版本为 C++17,  对于无 Qt 依赖的代码允许使用 C++20 / C++23 的特性,  但是需要保证 GCC 已经正式支持
