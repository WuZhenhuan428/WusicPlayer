#include "device.h"

#include <cstdio>
#include <cstring>
#include "../player_types.h"

namespace {
std::vector<std::string> enum_playback_names()
{
    std::vector<std::string> result;

    ma_context context;
    if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
        return result;
    }

    ma_device_info* playback_infos = nullptr;
    ma_uint32 playback_count = 0;
    if (ma_context_get_devices(&context, &playback_infos, &playback_count, nullptr, nullptr) == MA_SUCCESS) {
        result.reserve(playback_count);
        for (ma_uint32 i = 0; i < playback_count; ++i) {
            result.emplace_back(playback_infos[i].name);
        }
    }

    ma_context_uninit(&context);
    return result;
}

bool find_device_id_by_name(const std::string& name, ma_device_id* out_id)
{
    if (name.empty() || !out_id) {
        return false;
    }

    ma_context context;
    if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
        return false;
    }

    bool found = false;
    ma_device_info* playback_infos = nullptr;
    ma_uint32 playback_count = 0;
    if (ma_context_get_devices(&context, &playback_infos, &playback_count, nullptr, nullptr) == MA_SUCCESS) {
        for (ma_uint32 i = 0; i < playback_count; ++i) {
            if (name == playback_infos[i].name) {
                *out_id = playback_infos[i].id;
                found = true;
                break;
            }
        }
    }

    ma_context_uninit(&context);
    return found;
}
}

Device::Device(SPSCRingBuffer<F32StereoFrame, RING_BUFFER_CAPACITY>* buffer)
{
    this->init(buffer);
    m_vis_buffer.resize(8192);
}

Device::~Device()
{
    if (m_initialized) {
        ma_device_uninit(&m_device);
    }
}

bool Device::init(SPSCRingBuffer<F32StereoFrame, RING_BUFFER_CAPACITY>* buffer)
{
    m_buffer = buffer; // Store the original buffer pointer in our class
    fprintf(stdout, "[AUDIO][Device] init requested. selected='%s'\n", m_selected_device_name.c_str());

    m_device_config = ma_device_config_init(ma_device_type_playback);
    m_device_config.playback.format = ma_format_f32;
    m_device_config.playback.channels = 2;
    m_device_config.sampleRate = 44100;
    m_device_config.dataCallback = data_callback;
    m_device_config.pUserData = this; // Pass 'this' instead of 'buffer'

    ma_device_id selected_id;
    if (find_device_id_by_name(m_selected_device_name, &selected_id)) {
        m_device_config.playback.pDeviceID = &selected_id;
        fprintf(stdout, "[AUDIO][Device] using explicit playback device: %s\n", m_selected_device_name.c_str());
    }

    if (ma_device_init(nullptr, &m_device_config, &m_device) != MA_SUCCESS) {
        fprintf(stderr, "[AUDIO][Device] Error: failed to init ma_device\n");
        return false;
    }

    m_active_device_name = m_device.playback.name;
    fprintf(stdout, "[AUDIO][Device] init success. active='%s'\n", m_active_device_name.c_str());
    m_initialized = true;
    return true;
}

bool Device::start()
{
    if (!m_initialized) {
        fprintf(stderr, "[AUDIO][Device] Error: ma_device not initialized\n");
        return false;
    }

    if (ma_device_start(&m_device) != MA_SUCCESS) {
        fprintf(stderr, "[AUDIO][Device] ma_device_start failed\n");
        return false;
    }
    fprintf(stdout, "[AUDIO][Device] started. active='%s'\n", m_active_device_name.c_str());
    m_started = true;
    return true;
}

bool Device::pause()
{
    if (!m_initialized) {
        fprintf(stderr, "[AUDIO][Device] Error: ma_device not initialized\n");
        return false;
    }
    if (ma_device_stop(&m_device) != MA_SUCCESS) {
        fprintf(stderr, "[AUDIO][Device] Error: failed to stop ma_device\n");
        return false;
    }
    fprintf(stdout, "[AUDIO][Device] paused/stopped. active='%s'\n", m_active_device_name.c_str());
    m_started = false;
    return true;
}

bool Device::resume()
{
    return this->start();
}

bool Device::set_volume(float volume)
{
    if (!m_initialized) {
        return false;
    }
    if (ma_device_set_master_volume(&m_device, volume) != MA_SUCCESS) {
        return false;
    }
    return true;
}

float Device::get_volume()
{
    if (!m_initialized) {
        return -1.0f;
    }
    float vol;
    if (ma_device_get_master_volume(&m_device, &vol) != MA_SUCCESS) {
        return -1.0f;
    }
    return vol;
}

std::vector<std::string> Device::list_playback_devices() const
{
    return enum_playback_names();
}

std::string Device::current_playback_device_name() const
{
    return m_active_device_name;
}

bool Device::switch_playback_device_by_name(const std::string& device_name, bool start_after_switch)
{
    if (!m_buffer) {
        fprintf(stderr, "[AUDIO][Device] switch ignored: null buffer\n");
        return false;
    }

    fprintf(stdout, "[AUDIO][Device] switch request from '%s' to '%s', start_after_switch=%d\n",
            m_active_device_name.c_str(), device_name.c_str(), start_after_switch ? 1 : 0);

    const float old_volume = get_volume();
    if (m_initialized) {
        ma_device_uninit(&m_device);
        m_initialized = false;
        m_started = false;
        fprintf(stdout, "[AUDIO][Device] old device uninitialized\n");
    }

    m_selected_device_name = device_name;
    if (!init(m_buffer)) {
        fprintf(stderr, "[AUDIO][Device] switch failed during init\n");
        return false;
    }

    if (old_volume >= 0.0f) {
        set_volume(old_volume);
        fprintf(stdout, "[AUDIO][Device] restored volume=%.3f\n", old_volume);
    }

    if (start_after_switch) {
        const bool started = start();
        fprintf(stdout, "[AUDIO][Device] switch result started=%d active='%s'\n",
                started ? 1 : 0, m_active_device_name.c_str());
        return started;
    }

    fprintf(stdout, "[AUDIO][Device] switch result active='%s' (not started)\n", m_active_device_name.c_str());

    return true;
}


size_t Device::get_recent_audio_frames(F32StereoFrame* out_buffer, size_t count)
{
    if (m_vis_buffer.empty() || count == 0)
        return 0;

    if (count > m_vis_buffer.size())
        count = m_vis_buffer.size();

    size_t current_pos = m_vis_wrote_pos.load(std::memory_order_acquire);
    size_t vis_size = m_vis_buffer.size();

    size_t read_start = (current_pos + vis_size - count) % vis_size;

    size_t first_part = vis_size - read_start;
    if (first_part >= count) {
        std::memcpy(out_buffer, &m_vis_buffer[read_start], count * sizeof(F32StereoFrame));
    } else {
        std::memcpy(out_buffer, &m_vis_buffer[read_start], first_part * sizeof(F32StereoFrame));
        std::memcpy(out_buffer + first_part, &m_vis_buffer[0], (count - first_part) * sizeof(F32StereoFrame));
    }

    return count;
}


void Device::data_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count)
{
    UNUSED(input);

    Device* self = static_cast<Device*>(device->pUserData);
    SPSCRingBuffer<F32StereoFrame, RING_BUFFER_CAPACITY>* buffer = self->m_buffer;
    
    F32StereoFrame* out_ptr = static_cast<F32StereoFrame*>(output);
    size_t copied = 0;

    copied = buffer->read(out_ptr, frame_count);
    if (copied < frame_count) {
        std::memset(out_ptr + copied, 0, (frame_count - copied) * sizeof(F32StereoFrame));
    }
    
    // Copy the exact data we are sending to the speakers directly over to the visualizer ring buffer
    size_t vis_size = self->m_vis_buffer.size();
    if (vis_size > 0 && copied > 0) {
        size_t current_pos = self->m_vis_wrote_pos.load(std::memory_order_relaxed);
        
        for (size_t i = 0; i < frame_count; ++i) {
            self->m_vis_buffer[current_pos] = out_ptr[i];
            current_pos = (current_pos + 1) % vis_size;
        }
        
        self->m_vis_wrote_pos.store(current_pos, std::memory_order_release);
    }
}
