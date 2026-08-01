# 一、引言

1. 项目背景与目标
   1. 背景：Linux缺少一个现代化、功能丰富的本地音乐播放器。DeaDBeeF功能丰富但是图形栈过时（GTK2+）且缺少实用插件，其他播放器（如Elisa）则在功能上有所缺失。
   2. 目标：本项目旨在实现一个类似于 Foobar2000 的、面向个人的音乐播放器，主要目标在于个人播放体验，如音乐库管理、播放列表管理，而非实现高级音频处理技术。

2. 适用范围与读者对象
   1. 需要对本软件进行开发或二次开发的用户。使用者只需要查看 [README](../README_zh.md) 即可

3. 术语与缩写定义（MVC、LRC、PCM、FFmpeg、miniaudio、QStyle 等）
   1. \[占位符\]

4. 参考文档
   1. [Qt6](https://doc.qt.io/qt-6/)
   2. [FFmpeg](https://ffmpeg.org/documentation.html)
   3. [midiaudio](https://miniaud.io/docs/index.html)

# 二、总体设计

1. 系统定位与核心需求
   1. 定位：以 Linux 桌面为主、支持多平台，提供面向用于使用体验的高性能桌面音频播放器。
   2. 核心需求：
      1. 基于FFmpeg, 支持播放基本的音频格式
      2. 面向较大规模（万级）的个人曲库管理
      3. 灵活的播放列表功能

2. 设计目标与约束
   1. 平台
      1. Linux 优先
      2. 优先适配 Wayland 环境，不反对 X11/XCB 依赖，但不应当作为首选方案
      3. 跨平台功能不支持时应当能进行屏蔽，保证其他功能跨平台不受影响
   2. 开发环境
      1. 编程语言：C++17
      2. Qt 版本：软件本体尽量跟随 Qt 6 的最新稳定版本（不是LTS）
      3. 当前标准：C++17 + Qt 6.5+
      4. 说明：Qt 有极好的向下兼容性，当前使用 Qt 6.5+ 基本能满足需求
   3. 模块化、可扩展、主题可插拔
      1. 当软件核心功能完善后，新功能应当尽量通过插件实现，以保证软件本体的精简稳定，同时提升功能开发效率

3. 系统架构概览
   1. 模块描述
      1. `core`：包含配置管理器、歌词获取、音频解码核心、主题管理器、工具文件（utils）、类型定义等内容。主要存放与 Qt GUI 关系较小的独立功能模块
      2. `view`：Qt 控件视图，同时作为用户交互的主要部分
      3. `model`：Qt View Model，是控件与数据交互的中间层
      4. `controller`：控制器，用于管理相对独立的大型功能模块
      5. `service`：服务层，用于编排跨模块复杂功能的逻辑
      6. `static`：静态资源
   2. 分层架构
      ```
      User input -> view (GUI) -> controller -> service -> model -> core
      ```
   3. 模块依赖关系图
      \[待定\]

4. 技术选型理由
   1. 为什么选 Qt Widgets 而非 QML
      1. 本软件目标在于实用功能而非视觉效果，Qt Widget (C++) 无论是兼容性、稳定性、（实用功能的）开发效率还是性能都明显优于 QML
   2. 为什么选 miniaudio 而非 Qt Multimedia 作为音频后端：
      1. Qt Multimedia 与 Qt 耦合程度较大且封装程度过高，实现自定义功能较为麻烦（如实现音频频谱可视化）
      2. 而在外部音频后端中，miniaudio 同时有跨平台、单头文件（无依赖）等优点，同时在 Unity 等软件中验证了可行性，适合本项目
   3. 为什么选 QStyle 而非 QSS 做主题
      1. QStyle 对于控件样式的覆盖面完整，且不存在 QSS 的性能问题，同时可深入定制，适用于本项目
   4. 为什么选 MVC 衍生模式而非标准 MVP/MVVM
      1. MVVM：Qt Widget 的 View/Model 设计虽然本身接近于 MVVM，但基于 C++ 底层开发，数据绑定功能不完善（需要大量手动管理），且软件本身不存在大量的数据更新，因此不选用 MVVM
      2. MVP：Qt Widget 的底层设计适配于 View/Model 通信，MVP 不适用于 Qt Widget架构
      3. MVC：Qt Widget 本身的功能可以降低 View/Model 交互的复杂度，Controller 只需要管理数据处理逻辑即可，且软件规模有限，MVC 足以承受系统复杂度

# 三、模块详细设计

1. Core 层
   1. 音频引擎（core/player/）
      1. FFmpeg 解码器设计（decoder.h/cpp）
         1. 解码：基于 FFmpeg/libav* 的解码器，工作于子线程
         2. 基本解码流程：`读取文件 -> 解析属性 -> loop(接收packet -> 接收frame -> filter处理(重采样&eq) -> buffer)`
         3. 缓冲区：基于 SPSC 结构的无锁环形缓冲区，满容量时新输入覆盖旧数据（体现为音频播放跳跃而非卡住）
         4. 控制：通过原子变量组成控制状态机
      2. miniaudio 设备抽象（device.h/cpp, miniaudio.h）
         1. miniaudio 设置参数通过约定与 ffmpeg 保持一致
         2. 通过数据回调函数获取音频数据，缓冲区数据不足时静音处理
         3. 通过保存配置的方式，保存最近使用的音频设备
            > 注意，当设备前后不一致时，选择miniaudio自动发现的默认设备，且当系统设备改变后，不会自动更新设备
      3. 播放器状态机（player.h/cpp）：`Idle -> Loading -> Playing -> Paused -> Stopped`
         > 播放后端存在 QtMediaPlayer 向 自定义后端的迁移，状态机设计可能存在不一致问题。目前使用没有问题。
      4. 环形缓冲区（ring_buffer.hpp）
         1. 基于`std::array`，通过位运算计算状态，因此需要保证容量为`2^n`，否则编译期报错。
         2. 基于音频播放目的的设计，缓冲区满时新数据直接覆盖，而非阻塞
         3. 通过`std::atomic`原子操作实现无锁控制。仅在 amd64 平台经过验证
      5. 配置（config.h）：定义一些可配置的常量，如缓冲区容量、eq 上下限等
   2. 主题系统（core/theme/）
      1. ThemePalette 数据结构设计
         1. 元数据：主题名称、作者、是否为暗色主题
      2. WusicProxyStyle（QProxyStyle 子类）绘制流程
         1. 先加载调色板，后执行绘制即可，没有特殊的逻辑
      3. ThemeManager 单例
         1. 发现：
            1. 系统主题：通过内置函数`QStyleFactory::keys()`自动查找
            2. 内置主题：通过事先写好的头文件载入即可
            3. 外部主题：通过`QPlug`查找指定文件夹下对应的`.so`/`.dll`插件，保存符合要求的文件路径
         2. 切换：
            1. 系统主题：通过内置函数`QStyle* QStyleFactory::create(const QString&)`查找主题
            2. 内置主题：通过内置主题头文件创建对应的`QStyle`
            3. 外部主题：通过工厂函数`ThemePalette createPalette() const`获取调色板，进而创建`QStyle`
            4. 应用：获取`QStyle`后，通过内置函数`qApp->setStyle()`与`qApp->setPalette()`应用主题，同时发送信号，手动刷新部分自定义控件
      4. 内置主题 vs 外部插件主题的加载机制
         1. 自定义主题调色板类（`ThemePalette`）,内置主要控件的颜色与外观描述信息，通过函数`QPalette toQPalette() const`生成`QPalette`文件
         2. 内置：在头文件中预先实现调色板类，直接编译进软件中，在软件启动时通过`ThemeManager::registerBuiltinPalette(const ThemePalette&)`注册
         3. 外部：实现对应接口后。在 GUI 使用中手动添加路径并手动刷新，通过函数`ThemeManager::scanExternalPlugins(const QString&)`注册
      5. IThemePlugin 接口规范
         1. 元数据：插件名称，作者，版本，描述；均为`QString`字符串
         2. 实现工厂方法`ThemePalette createPalette() const`，返回完整的`ThemePalette`结构
      6. 用户选项
         1. 考虑到系统兼容性、插件规范性、主题颜色的实际情况，暗色/亮色模式下默认图标颜色可能出现相反、对比度不足等问题，因此允许用户自由组合
   3. 配置管理（core/ConfigManager/）
      1. IConfigurable 接口：模块化配置注册
         1. 软件整体采用“分散注册、统一加载/读取”的策略，实现`IConfigurable`中对应的接口即可
         2. 对于临时控件，`ConfigManager`提供`readSubConfig()`与`writeSubConfig()`接口，用于直接读写配置文件。
      2. JSON 序列化/反序列化方案：使用`QJson*`相关库实现，基本结构为`{"scope_1": {}, "scope_2": {}, ...}`，其中`scope_?`为`IConfigurable`中注册时使用的id
      3. 配置文件的读写时机与顺序
         1. 控件长期存在：即生命周期与软件基本一致的情况下，软件启动时统一创建控件加载配置，软件关闭时统一保存配置
         2. 临时控件：创建/销毁控件时即时加载/保存配置
   4. 歌词获取器（core/LyricsFetcher/）
      1. 网易云音乐 API 对接（netease_qt6.h/cpp）
      2. 歌词管理器（lyrics_manager.h/cpp）
   5. 共享类型定义（core/*types.h）
      1. 元数据：TrackMetaData
      2. 排序参数：SortRule / SortType
      3. ID：trackId / playlistId（UUID 方案）
   6. 工具类（core/utils/）

3.2 Model 层
   1. 播放列表模型（model/playlist/）
      数据结构设计（树形/扁平）
      排序表达式语法与解析
      与视图的绑定方式（Qt Item Model 体系）
   2. 搜索模型（model/search_model/）
      内存搜索引擎设计
      实时过滤策略
      搜索结果排序与高亮
   3. 快捷键视图模型（model/ShortcutsViewModel/）
      快捷键配置的数据模型

3.3 Controller 层
   1. AppController：应用级协调器
      各子模块的初始化顺序
      信号槽连接拓扑图
      事件过滤器机制（eventFilter）
   2. PlaybackController：播放编排
      与 Player（音频引擎）的交互
      播放模式切换逻辑
   3. PlaylistController：播放列表增删改查
   4. ShortcutsController：快捷键注册与分发
   5. 搜索后端（controller/search_backend/）

3.4 Service 层
   1. PlaybackService：播放生命周期管理
   2. PlaybackRestoreService：启动恢复播放
   3. LibraryInteractionService：文件系统操作
   4. TagWritebackService：元数据写回
   5. ThemeService：主题应用与管理

3.5 View 层
   1. MainWindow：主窗口布局与区域划分
      菜单栏、工具栏、状态栏
      中央区域（可分割布局）
   2. WControlBar：播放控制栏
   3. LibraryWidget：音乐库视图（表格/列表）
   4. playlist/：播放列表面板
   5. search_panel/：搜索面板
   6. DesktopLyricsWidget：桌面歌词悬浮窗
   7. eq_widget/：均衡器面板
   8. tag_edit_widget/：标签编辑器
   9. SettingsPanel/：设置面板
      通用设置、快捷键设置、主题设置
   10. SidePanel/：侧边导航栏
   11. dialogs/：对话框集合