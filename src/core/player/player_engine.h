#pragma once

#include "config.h"
#include "types.h"

#include "device.h"
#include "decoder.h"
#include "ring_buffer.hpp"

#include <atomic>
#include <unordered_map>
#include <memory>
#include <functional>
#include <string>
#include <vector>

class PlayerEngine
{
public:
    enum class PlayingState
    {
        STOP = 0,   // as IDLE
        PAUSE,
        PLAYING
    };
    using PlayingState = PlayerEngine::PlayingState;

public:
    PlayerEngine();
    ~PlayerEngine();

    // use this function before setUrl()
    bool startDevice();

    void setWatcdog();
    // local filepath only
    void setUrl(const std::string& url);
    void resume();
    void pause();
    void stop();
    PlayerEngine::PlayingState state();
    void seek(int64_t pos_ms);
    void setVolume(float volume);
    void setEQ(struct gains_t gains);
    std::unordered_map<std::string, std::string> metadata();
    float volume();
    int64_t position();
    void setPlaybackFinishedCallback(std::function<void()> func);
    size_t get_recent_audio_frames(F32StereoFrame* out_buffer, size_t count);
    std::vector<std::string> outputDevices() const;
    std::string currentOutputDeviceName() const;
    bool setOutputDeviceByName(const std::string& name);
    
private:
    std::unique_ptr<Decoder> m_decoder = nullptr;
    std::unique_ptr<SPSCRingBuffer<F32StereoFrame, RING_BUFFER_CAPACITY>> m_buffer;
    std::unique_ptr<Device> m_device = nullptr;
    
    std::atomic_bool m_decode_finished{false};
    std::atomic_bool m_abort_watchdog{false};
    
    std::function<void()> m_playback_callback;
    std::thread m_watchdog;

    std::atomic<PlayingState> m_state;
};