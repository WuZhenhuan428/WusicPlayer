# WusicPlayer 音频后端

本文档描述音频播放链路、解码器与 EQ(均衡器)子系统的架构与数据流。

## 1. 架构概览

```
┌─────────────────────────────  应用层  ────────────────────────────────┐
│  EQWidget(固定窗口容器) ─ IEqPlugin 插件 UI                           │
│      │ apply / instant / restore                                      │
│      ▼                                                                │
│  PlaybackController ─ EqConfig(任意 band, 实际限制取决于 ffmpeg 后端) │
└─────────────────────────────────┬─────────────────────────────────────┘
                                  ▼
┌─────────────────────────────  播放层  ─────────────────────────────┐
│  Player(QObject 门面) -> PlayerEngine(状态机/线程协调)             │
│      │ set_url()                                                   │
│      ▼                                                             │
│  Decoder(FFmpeg 解码 + EQ filter graph, 独立工作线程)              │
│      │ F32StereoFrame 帧流                                         │
│      ▼                                                             │
│  SPSCRingBuffer(无锁环形缓冲, 满时阻塞)                            │
│      ▲                                                             │
│  Device(miniaudio 输出, 回调拉取)                                  │
└────────────────────────────────────────────────────────────────────┘
```

### 各层职责

| 组件                 | 职责                                                                                                  |
|----------------------|-------------------------------------------------------------------------------------------------------|
| `PlaybackController` | 门面:播放/暂停/进度/音量/EQ 配置;持久化(`playback.eq`);缓存当前 `EqConfig`                            |
| `Player`             | QObject 门面, 广播信号(`sgn_state_changed` 等), 内部持有 `PlayerEngine`                               |
| `PlayerEngine`       | 播放状态机(STOP/PAUSE/PLAYING)、解码器生命周期、100ms 预填充、watchdog 守护线程、缓存待应用的 EQ 配置 |
| `Decoder`            | FFmpeg 解码(本地文件)+ EQ filter graph; 单工作线程                                                    |
| `SPSCRingBuffer`     | 解码线程 -> 输出线程的无锁帧缓冲                                                                      |
| `Device`             | miniaudio 输出, 回调 `data_callback()` 拉取帧                                                         |

## 2. 解码流程(Decoder)

- `PlayerEngine::set_url(url)` -> 停止旧解码器 -> 创建 `Decoder(url)` -> 应用缓存的 EQ 配置(`m_pending_eq`) -> `decoder->work()` 启动工作线程。
- 工作线程循环:读取 packet -> 解码 frame -> **若 EQ 启用则 `process_frame_with_eq()` 经 filter graph 处理** -> 写入环形缓冲;解码完成置 `m_decode_finished`。
- 预填充 100ms(约 4410 帧)后开始播放, 减少启动卡顿。
- `m_abort_request` / `stop()` + `join()` 保证线程安全退出。

## 3. EQ 子系统

### 3.1 数据结构

```cpp
enum class EqFilterType { Parametric, Peak, LowShelf, HighShelf, LowPass, HighPass };

struct EqBand {
    EqFilterType type;    // 滤波器类型
    double       freq;    // 中心频率(Hz)
    double       q;       // Q 因子(带宽)
    float        gain_db; // 增益(dB), 无上下限(由 FFmpeg 后端限制)
};

struct EqConfig {
    bool enabled = false;
    QVector<EqBand> bands;   // 任意数量
};
```

> 由 `IEqPlugin::eq_config()` 输出; 旧固定十段 ±12dB 模型已移除, 后端支持任意 band。

### 3.2 Filter Graph

```
abuffer --> aformat(planar fltp/44100/stereo) --> eq_0 --> eq_1 --> … --> eq_N
                                                                           |
abuffersink <------------- aformat(packed) <-------------------------------*
```

- `abuffer` 注入解码后的 frame; `aformat` 先转 planar(FFmpeg EQ 滤波器要求), 处理完转回 packed。
- EQ 链按 `EqConfig.bands` 顺序构建, 实例名 `eq_0, eq_1, …`(`eq_instance_name()` 辅助生成)。
- 每 band 由 `eq_filter_name()` / `eq_filter_args()` 生成对应滤波器(`equalizer`、`bass`/`treble`、`highpass`/`lowpass` 等)。

### 3.3 双路径更新(eq_check_and_update)

解码线程消费 `m_pending_eq`(atomic), 与 `m_applied_eq`(解码线程独占)比较:

- **结构未变**(enabled/band 的 type/freq/q 均相同): 逐 band 用
  `avfilter_graph_send_command(graph, "eq_i", "gain", "<db>", …)` 就地修改增益, 无重建开销。
- **结构变化/首次**: `init_filters()` 重建整个 graph(含 EQ 链)。

### 3.4 线程模型

| 变量               | 访问方                | 说明                          |
|--------------------|-----------------------|-------------------------------|
| `m_pending_eq`     | atomic, 任意线程写/读 | 最新请求的配置                |
| `m_applied_eq`     | 解码线程独占          | 当前实际生效配置              |
| `m_has_eq_changed` | atomic 标志           | 通知解码线程有新的 `EqConfig` |

`PlayerEngine` 在无解码器时缓存 `m_pending_eq`; `set_url()` 创建 `Decoder` 后立即应用,
保证"播放前设置的 EQ 配置"在任意歌曲加载时生效。

## 4. EQ 插件化数据流

```
EQWidget(容器)
  ├─ 插件列表:  PluginManager::plugins<IEqPlugin>()
  ├─ Apply:     plugin->apply_current() + compose_config()(enabled 取自勾选框)
  ├─ Instant:   plugin->sgn_config_changed -> apply_to_backend()
  ├─ Reset:     plugin->reset()
  ├─ Cancel:    plugin->revert()
  └─ 打开窗口:  按持久化 plugin_id 恢复选中; restore_from_config() 同步后端配置到插件 UI

IEqPlugin 接口(v1.0):
  create_eq_widget(parent) / eq_config() / apply_current() / revert() / reset() / restore_from_config(cfg)
  信号: sgn_config_changed(即时模式)
```

配置下发链路:

```
PlaybackController::set_eq_config(EqConfig)
  -> Player::set_eq_config(shared_ptr<const EqConfig>)
  -> PlayerEngine::set_eq_config()    [缓存 m_pending_eq + 有解码器则转发]
  -> Decoder::set_eq_config()         [atomic 存储 + m_has_eq_changed 置位]
  -> 解码线程 eq_check_and_update()   [按需重建或发 gain 命令]
```

## 5. EQ 配置持久化

`PlaybackController`(IConfigurable, key=`playback`)在 `WusicPlayer.json` 中保存:

```json
{
  "playback": {
    "eq": {
      "plugin_id": "wusic.eq.builtin",
      "enabled": true,
      "bands": [
        { "type": 0, "freq": 31, "q": 1.414, "gain_db": -3.0 },
        { "type": 0, "freq": 63, "q": 1.414, "gain_db": -2.0 }
      ]
    }
  }
}
```

- 启动 `load_from_json()` 重建 `EqConfig` -> `set_eq_config()`, 在文件加载前即缓存待生效。
- `EQWidget` 打开时按 `plugin_id` 恢复选中插件, 用 `eq_config()` 同步启用状态与插件 UI。

## 6. 关键文件

```
src/controller/playback_controller.h/cpp   # 门面 + EQ 配置持久化
src/core/player/player.h/cpp               # QObject 门面
src/core/player/player_engine.h/cpp        # 状态机 / 线程协调 / m_pending_eq
src/core/player/decoder.h/cpp              # FFmpeg 解码 + EQ filter graph
src/core/player/device.h/cpp               # miniaudio 输出
src/core/player/ring_buffer.hpp            # 无锁环形缓冲
include/plugin/eq_types.h                  # EqConfig / EqBand / EqFilterType
include/plugin/i_eq_plugin.h               # EQ 插件接口
src/core/eq/builtin/builtin_eq_plugin.h/cpp# 内建十段 EQ 插件
src/view/eq_widget/eq_widget.h/cpp         # EQ 固定窗口容器
```
