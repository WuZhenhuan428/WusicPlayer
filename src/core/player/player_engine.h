#pragma once

#include "core/player/config.h"
#include "core/player/decoder.h"
#include "core/player/device.h"
#include "core/player/ring_buffer.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class PlayerEngine
{
public:
    enum class PlayingState
    {
        Stop = 0, // as Idle
        Pause,
        Playing,
    };
    enum class StopReason
    {
        NaturalEof,
        ManualStop,
    };
    using PlayingState = PlayerEngine::PlayingState;

public:
    PlayerEngine();
    ~PlayerEngine();

    PlayerEngine(const PlayerEngine&)            = delete;
    PlayerEngine& operator=(const PlayerEngine&) = delete;
    PlayerEngine(PlayerEngine&&)                 = delete;
    PlayerEngine& operator=(PlayerEngine&&)      = delete;

    // use this function before set_url()
    bool start_device();

    void set_watchdog();
    // local filepath only
    void set_url(const std::string& url);
    void resume();
    void pause();
    void stop();
    PlayerEngine::PlayingState state();
    void seek(int64_t pos_ms);
    void set_volume(float volume);
    /// 插件路径: 任意 band 的 EQ 配置(无增益上下限)
    void set_eq_config(std::shared_ptr<const EqConfig> cfg);
    /// 查询当前请求的 EQ 配置
    [[nodiscard]] std::shared_ptr<const EqConfig> eq_config() const;
    std::unordered_map<std::string, std::string> metadata();
    float volume();
    int64_t position();
    void set_playback_finished_callback(std::function<void(StopReason)> func);
    size_t get_recent_audio_frames(F32StereoFrame* out_buffer, size_t count);
    [[nodiscard]] std::vector<std::string> output_devices() const;
    /// 系统默认输出设备名(空表示无)
    [[nodiscard]] std::string default_output_device_name() const;
    [[nodiscard]] std::string current_output_device_name() const;
    bool set_output_device_by_name(const std::string& name);

private:
    std::unique_ptr<Decoder> decoder_ = nullptr;
    std::unique_ptr<SPSCRingBuffer<F32StereoFrame, kRingBufferCapacity>> buffer_;
    std::unique_ptr<Device> device_ = nullptr;

    std::atomic_bool decode_finished_{false};
    std::atomic_bool abort_watchdog_{false};

    std::function<void(StopReason)> playback_callback_;
    std::thread watchdog_;

    std::atomic<PlayingState> state_;

    // 缓存 EQ 配置 — set_url 创建 Decoder 时应用,
    // 使任何音频文件加载前设置/恢复的 EQ 配置都能生效。
    std::shared_ptr<const EqConfig> pending_eq_;
};
