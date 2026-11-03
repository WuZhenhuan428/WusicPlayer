#include "device.h"

#include <cstdio>
#include <cstring>

#include "core/logger/logger_manager.h"
namespace
{
// NOLINTNEXTLINE
Logger* logger = LoggerManager::file_logger("player_device", {"console", "gui"});

std::vector<std::string> enum_playback_names()
{
    std::vector<std::string> result;

    ma_context context;
    if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
        return result;
    }

    ma_device_info* playback_infos = nullptr;
    ma_uint32 playback_count       = 0;
    if (ma_context_get_devices(&context, &playback_infos, &playback_count, nullptr, nullptr) ==
        MA_SUCCESS) {
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

    bool found                     = false;
    ma_device_info* playback_infos = nullptr;
    ma_uint32 playback_count       = 0;
    if (ma_context_get_devices(&context, &playback_infos, &playback_count, nullptr, nullptr) ==
        MA_SUCCESS) {
        for (ma_uint32 i = 0; i < playback_count; ++i) {
            if (name == playback_infos[i].name) {
                *out_id = playback_infos[i].id;
                found   = true;
                break;
            }
        }
    }

    ma_context_uninit(&context);
    return found;
}
} // namespace

// 二段构造
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
Device::Device(SPSCRingBuffer<F32StereoFrame, kRingBufferCapacity>* buffer)
{
    this->init(buffer);
    vis_buffer_.resize(8192);
}

Device::~Device()
{
    if (initialized_) {
        ma_device_uninit(&device_);
    }
}

bool Device::init(SPSCRingBuffer<F32StereoFrame, kRingBufferCapacity>* buffer)
{
    buffer_ = buffer; // Store the original buffer pointer in our class
    logger->info("init requested. selected={}", selected_device_name_);

    device_config_                   = ma_device_config_init(ma_device_type_playback);
    device_config_.playback.format   = ma_format_f32;
    device_config_.playback.channels = 2;
    device_config_.sampleRate        = 44100;
    device_config_.dataCallback      = data_callback;
    device_config_.pUserData         = this; // Pass 'this' instead of 'buffer'

    ma_device_id selected_id;
    if (find_device_id_by_name(selected_device_name_, &selected_id)) {
        device_config_.playback.pDeviceID = &selected_id;
        logger->info("using explicit playback device: {}", selected_device_name_);
    }

    if (ma_device_init(nullptr, &device_config_, &device_) != MA_SUCCESS) {
        logger->info("failed to init ma_device");
        return false;
    }

    active_device_name_ = device_.playback.name;
    logger->info("init success. active='{}'", active_device_name_);
    initialized_ = true;
    return true;
}

bool Device::start()
{
    if (!initialized_) {
        logger->error("ma_device not initialized");
        return false;
    }

    if (ma_device_start(&device_) != MA_SUCCESS) {
        logger->error("ma_device_start failed");
        return false;
    }
    logger->info("started, active='{}'", active_device_name_);
    started_ = true;
    return true;
}

bool Device::pause()
{
    if (!initialized_) {
        logger->error("ma_device not initialized");
        return false;
    }
    if (ma_device_stop(&device_) != MA_SUCCESS) {
        logger->error("failed to stop ma_device");
        return false;
    }
    logger->info("paused/stopped. active='{}'", active_device_name_);
    started_ = false;
    return true;
}

bool Device::resume()
{
    return this->start();
}

bool Device::set_volume(float volume)
{
    if (!initialized_) {
        return false;
    }
    if (ma_device_set_master_volume(&device_, volume) != MA_SUCCESS) {
        return false;
    }
    return true;
}

float Device::get_volume()
{
    if (!initialized_) {
        return -1.0f;
    }
    float vol = -1.0f;
    if (ma_device_get_master_volume(&device_, &vol) != MA_SUCCESS) {
        return -1.0f;
    }
    return vol;
}
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
std::vector<std::string> Device::list_playback_devices() const
{
    return enum_playback_names();
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
std::string Device::default_playback_device_name() const
{
    ma_context context;
    if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
        return {};
    }
    std::string result;
    ma_device_info* playback_infos = nullptr;
    ma_uint32 playback_count       = 0;
    if (ma_context_get_devices(&context, &playback_infos, &playback_count, nullptr, nullptr) ==
        MA_SUCCESS) {
        for (ma_uint32 i = 0; i < playback_count; ++i) {
            if (playback_infos[i].isDefault) {
                result = playback_infos[i].name;
                break;
            }
        }
    }
    ma_context_uninit(&context);
    return result;
}

std::string Device::current_playback_device_name() const
{
    return active_device_name_;
}

bool Device::switch_playback_device_by_name(const std::string& device_name, bool start_after_switch)
{
    if (!buffer_) {
        logger->error("switch ignored: null buffer");
        return false;
    }

    logger->info("switch request from '{}' to '{}', start_after_switch={}", active_device_name_,
                 device_name, start_after_switch ? 1 : 0);

    const float old_volume = get_volume();
    if (initialized_) {
        ma_device_uninit(&device_);
        initialized_ = false;
        started_     = false;
        logger->info("old device uninitialized");
    }

    selected_device_name_ = device_name;
    if (!init(buffer_)) {
        logger->error("switch failed during init");
        return false;
    }

    if (old_volume >= 0.0f) {
        set_volume(old_volume);
        logger->info("restored volume={:.03f}", old_volume);
    }

    if (start_after_switch) {
        const bool started = start();
        logger->info("switch result started={} active='{}'", started ? 1 : 0, active_device_name_);
        return started;
    }

    logger->info("switch result active='{}' (not started)", active_device_name_);

    return true;
}

size_t Device::get_recent_audio_frames(F32StereoFrame* out_buffer, size_t count)
{
    if (vis_buffer_.empty() || count == 0) {
        return 0;
    }

    count              = std::min(count, vis_buffer_.size());

    size_t current_pos = vis_wrote_pos_.load(std::memory_order_acquire);
    size_t vis_size    = vis_buffer_.size();

    size_t read_start  = (current_pos + vis_size - count) % vis_size;

    size_t first_part  = vis_size - read_start;
    if (first_part >= count) {
        std::memcpy(out_buffer, &vis_buffer_[read_start], count * sizeof(F32StereoFrame));
    } else {
        std::memcpy(out_buffer, &vis_buffer_[read_start], first_part * sizeof(F32StereoFrame));
        std::memcpy(out_buffer + first_part, vis_buffer_.data(),
                    (count - first_part) * sizeof(F32StereoFrame));
    }

    return count;
}

void Device::data_callback(ma_device* device, void* output, [[maybe_unused]] const void* input,
                           ma_uint32 frame_count)
{
    Device* self = static_cast<Device*>(device->pUserData);
    SPSCRingBuffer<F32StereoFrame, kRingBufferCapacity>* buffer = self->buffer_;

    F32StereoFrame* out_ptr = static_cast<F32StereoFrame*>(output);
    size_t copied           = 0;

    copied                  = buffer->read(out_ptr, frame_count);
    if (copied < frame_count) {
        std::memset(out_ptr + copied, 0, (frame_count - copied) * sizeof(F32StereoFrame));
    }

    // Copy the exact data we are sending to the speakers directly over to the visualizer ring
    // buffer
    size_t vis_size = self->vis_buffer_.size();
    if (vis_size > 0 && copied > 0) {
        size_t current_pos = self->vis_wrote_pos_.load(std::memory_order_relaxed);

        for (size_t i = 0; i < frame_count; ++i) {
            self->vis_buffer_[current_pos] = out_ptr[i];
            current_pos                    = (current_pos + 1) % vis_size;
        }

        self->vis_wrote_pos_.store(current_pos, std::memory_order_release);
    }
}
