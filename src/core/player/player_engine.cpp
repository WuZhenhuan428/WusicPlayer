#include "core/player/player_engine.h"

#include <chrono>
#include <thread>

#include "core/logger/logger_manager.h"

namespace
{
Logger* logger = LoggerManager::file_logger("player_engine", {"console", "gui"});
}

PlayerEngine::PlayerEngine()
{
    m_buffer = std::make_unique<SPSCRingBuffer<F32StereoFrame, RING_BUFFER_CAPACITY>>();
    m_device = std::make_unique<Device>(m_buffer.get());
    m_decode_finished.store(false, std::memory_order_relaxed);
    m_state = PlayingState::STOP;
}

PlayerEngine::~PlayerEngine()
{
    m_abort_watchdog.store(true, std::memory_order_release);
    if (m_watchdog.joinable()) {
        m_watchdog.join();
    }

    if (m_decoder) {
        m_decoder->stop();
        m_decoder->join();
    }
}

bool PlayerEngine::start_device()
{
    if (!m_device->start()) {
        logger->info("ma_device_start");
        m_decode_finished.store(true, std::memory_order_release);
        if (m_decoder) {
            m_decoder->join();
        }
        return false;
    }

    return true;
}

std::unordered_map<std::string, std::string> PlayerEngine::metadata()
{
    if (!m_decoder) {
        return {};
    }
    return m_decoder->metadata();
}

void PlayerEngine::set_watchdog()
{
    m_watchdog = std::thread([this]() {
        bool has_notified = false;
        while (!m_abort_watchdog.load(std::memory_order_acquire)) {
            // If decoder exists, completed reading, AND buffer drained completely:
            if (m_decoder && m_decode_finished.load(std::memory_order_acquire) &&
                m_buffer->empty()) {
                if (!has_notified) {
                    PlayingState curr_state =
                        m_state.exchange(PlayingState::STOP, std::memory_order_acq_rel);
                    if (curr_state != PlayingState::STOP) {
                        logger->info("Playback finished autonomously");
                        if (m_playback_callback) {
                            m_playback_callback(StopReason::NATURAL_EOF);
                        }
                    }
                    has_notified = true;
                }
            } else {
                // If a new song starts or seek clears the finish flag, reset notification state
                has_notified = false;
            }
            // polling interval
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
}

void PlayerEngine::set_url(const std::string& url)
{
    m_buffer->clear();
    bool was_paused = (m_state == PlayingState::PAUSE);
    if (m_device && !was_paused) {
        m_device->pause();
    }

    if (m_decoder) {
        m_decoder->stop();
        m_decoder->join();
        m_decoder.reset();
    }
    m_decode_finished.store(false, std::memory_order_release);

    m_decoder = std::make_unique<Decoder>(url);
    if (m_pending_eq) {
        m_decoder->set_eq_config(m_pending_eq); // 插件路径: apply cached EQ config
    } else {
        m_decoder->set_eq_gain(m_pending_gains); // 兼容 shim: 旧十段路径
    }
    m_decoder->work(m_buffer.get(), &m_decode_finished);

    // pre-filling 100ms buffer
    constexpr size_t kPrebufferFrames = 44100 / 10; // 100ms
    while (!m_decode_finished.load(std::memory_order_acquire)) {
        if (m_buffer->size() >= kPrebufferFrames) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (!was_paused) {
        m_device->resume();
        m_state = PlayingState::PLAYING;
    }
}

void PlayerEngine::resume()
{
    if (m_device && m_state == PlayingState::PAUSE) {
        m_device->resume();
        m_state = PlayingState::PLAYING;
    }
}

void PlayerEngine::pause()
{
    if (m_device && m_state == PlayingState::PLAYING) {
        m_device->pause();
        m_state = PlayingState::PAUSE;
    }
}

void PlayerEngine::stop()
{
    // 1. 抢占并锁住 STOP，阻断看门狗对自然结束的判断
    PlayingState old_state = m_state.exchange(PlayingState::STOP, std::memory_order_acq_rel);
    if (m_decoder) {
        m_decoder->stop();
    }
    if (m_device) {
        m_device->pause();
    }
    if (m_buffer) {
        m_buffer->clear();
    }

    // 2. 重置解码完成标志
    m_decode_finished.store(false, std::memory_order_release);
    if (old_state != PlayingState::STOP && m_playback_callback) {
        m_playback_callback(StopReason::MANUAL_STOP);
    }
}

void PlayerEngine::set_volume(float volume)
{
    if (volume < 0 || volume > 1.0f) {
        logger->warn("Volume must be in range from 0 to 1");
        return;
    }
    if (m_device) {
        m_device->set_volume(volume);
    }
}

void PlayerEngine::seek(int64_t pos_ms)
{
    if (!m_device || !m_decoder) {
        logger->warn("device or decoder does not exist (may be initializing)");
        return;
    }

    if (m_state == PlayingState::STOP) {
        return;
    }

    if (m_state == PlayingState::PLAYING) {
        m_device->pause();
    }

    m_decoder->seek(pos_ms);

    if (m_state == PlayingState::PLAYING) {
        m_device->resume();
    }
}

void PlayerEngine::set_playback_finished_callback(std::function<void(StopReason)> func)
{
    m_playback_callback = func;
}

PlayerEngine::PlayingState PlayerEngine::state()
{
    return m_state;
}

void PlayerEngine::set_eq(gains_t gains)
{
    m_pending_gains = gains;
    if (m_decoder) {
        m_decoder->set_eq_gain(gains);
    }
}

void PlayerEngine::set_eq_config(std::shared_ptr<const EqConfig> cfg)
{
    m_pending_eq = std::move(cfg);
    if (m_decoder) {
        m_decoder->set_eq_config(m_pending_eq);
    }
}

std::shared_ptr<const EqConfig> PlayerEngine::eq_config() const
{
    if (m_decoder) {
        return m_decoder->eq_config();
    }
    return m_pending_eq;
}

size_t PlayerEngine::get_recent_audio_frames(F32StereoFrame* out_buffer, size_t count)
{
    return m_device->get_recent_audio_frames(out_buffer, count);
}

std::vector<std::string> PlayerEngine::output_devices() const
{
    if (!m_device) {
        return {};
    }
    return m_device->list_playback_devices();
}

std::string PlayerEngine::current_output_device_name() const
{
    if (!m_device) {
        return {};
    }
    return m_device->current_playback_device_name();
}

bool PlayerEngine::set_output_device_by_name(const std::string& name)
{
    if (!m_device) {
        return false;
    }

    const bool should_start = (m_state.load(std::memory_order_acquire) == PlayingState::PLAYING);
    const bool ok           = m_device->switch_playback_device_by_name(name, should_start);
    if (!ok) {
        return false;
    }

    if (should_start) {
        m_state.store(PlayingState::PLAYING, std::memory_order_release);
    } else {
        m_state.store(PlayingState::PAUSE, std::memory_order_release);
    }
    return true;
}

float PlayerEngine::volume()
{
    if (!m_device) {
        return 0.0f;
    }
    return m_device->get_volume();
}

int64_t PlayerEngine::position()
{
    if (!m_decoder) {
        return 0;
    }
    return m_decoder->position();
}

const gains_t PlayerEngine::gains() const
{
    if (!m_decoder) {
        return m_pending_gains;
    }
    return m_decoder->gains();
}
