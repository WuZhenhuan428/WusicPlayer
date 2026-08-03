#pragma once

#include "core/player/config.h"
#include "core/player_types.h"
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
        STOP = 0, // as IDLE
        PAUSE,
        PLAYING
    };
    enum class StopReason
    {
        NATURAL_EOF,
        MANUAL_STOP
    };
    using PlayingState = PlayerEngine::PlayingState;

public:
    PlayerEngine();
    ~PlayerEngine();

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
    void set_eq(gains_t gains);
    const gains_t gains() const;
    std::unordered_map<std::string, std::string> metadata();
    float volume();
    int64_t position();
    void set_playback_finished_callback(std::function<void(StopReason)> func);
    size_t get_recent_audio_frames(F32StereoFrame* out_buffer, size_t count);
    std::vector<std::string> output_devices() const;
    std::string current_output_device_name() const;
    bool set_output_device_by_name(const std::string& name);

private:
    std::unique_ptr<Decoder> m_decoder = nullptr;
    std::unique_ptr<SPSCRingBuffer<F32StereoFrame, RING_BUFFER_CAPACITY>> m_buffer;
    std::unique_ptr<Device> m_device = nullptr;

    std::atomic_bool m_decode_finished{false};
    std::atomic_bool m_abort_watchdog{false};

    std::function<void(StopReason)> m_playback_callback;
    std::thread m_watchdog;

    std::atomic<PlayingState> m_state;

    // Cached EQ gains — applied to Decoder when it is created (set_url),
    // so EQ config loaded before any audio file can still take effect.
    gains_t m_pending_gains = {};
};
