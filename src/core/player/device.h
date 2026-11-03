#pragma once

#include "core/player/config.h"
#include "miniaudio.h"
#include "ring_buffer.hpp"

#include <atomic>
#include <string>
#include <vector>

class Device
{
public:
    Device(SPSCRingBuffer<F32StereoFrame, kRingBufferCapacity>* buffer);
    ~Device();

    // 拥有 ma_device 句柄需要捕获 `this`
    // 复制与移动会导致句柄悬空
    Device(const Device&)            = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&)                 = delete;
    Device& operator=(Device&&)      = delete;

    bool start();
    bool pause();
    bool resume();
    bool set_volume(float volume); // 0.0 ~ 1.0
    float get_volume();
    [[nodiscard]] std::vector<std::string> list_playback_devices() const;
    /// 系统默认播放设备名(ma_device_info.isDefault), 空表示无
    [[nodiscard]] std::string default_playback_device_name() const;
    [[nodiscard]] std::string current_playback_device_name() const;
    bool switch_playback_device_by_name(const std::string& device_name, bool start_after_switch);

    size_t get_recent_audio_frames(F32StereoFrame* out_buffer, size_t count);

private:
    bool init(SPSCRingBuffer<F32StereoFrame, kRingBufferCapacity>* buffer);
    static void data_callback(ma_device* device, void* output, const void* input,
                              ma_uint32 frame_count);

private:
    ma_device_config device_config_;
    ma_device device_;
    bool initialized_                                            = false;
    bool started_                                                = false;

    // Store the buffer here so the callback can access it through the Device instance
    SPSCRingBuffer<F32StereoFrame, kRingBufferCapacity>* buffer_ = nullptr;
    std::string selected_device_name_;
    std::string active_device_name_;

    // visualizer buffer, ring buffer, overridable
    std::vector<F32StereoFrame> vis_buffer_;
    std::atomic<std::size_t> vis_wrote_pos_{0};
};
