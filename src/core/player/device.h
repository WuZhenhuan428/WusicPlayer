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
    Device(SPSCRingBuffer<F32StereoFrame, RING_BUFFER_CAPACITY>* buffer);
    ~Device();

    bool start();
    bool pause();
    bool resume();
    bool set_volume(float volume); // 0.0 ~ 1.0
    float get_volume();
    std::vector<std::string> list_playback_devices() const;
    /// 系统默认播放设备名(ma_device_info.isDefault), 空表示无
    std::string default_playback_device_name() const;
    std::string current_playback_device_name() const;
    bool switch_playback_device_by_name(const std::string& device_name, bool start_after_switch);

    size_t get_recent_audio_frames(F32StereoFrame* out_buffer, size_t count);

private:
    bool init(SPSCRingBuffer<F32StereoFrame, RING_BUFFER_CAPACITY>* buffer);
    static void data_callback(ma_device* device, void* output, const void* input,
                              ma_uint32 frame_count);

private:
    ma_device_config m_device_config;
    ma_device m_device;
    bool m_initialized                                             = false;
    bool m_started                                                 = false;

    // Store the buffer here so the callback can access it through the Device instance
    SPSCRingBuffer<F32StereoFrame, RING_BUFFER_CAPACITY>* m_buffer = nullptr;
    std::string m_selected_device_name;
    std::string m_active_device_name;

    // visualizer buffer, ring buffer, overridable
    std::vector<F32StereoFrame> m_vis_buffer;
    std::atomic<std::size_t> m_vis_wrote_pos{0};
};
