#include "core/player/player_engine.h"

#include <chrono>
#include <thread>

#include "core/logger/logger_manager.h"

namespace
{
// NOLINTNEXTLINE
Logger* logger = LoggerManager::file_logger("player_engine", {"console", "gui"});
} // namespace

PlayerEngine::PlayerEngine()
{
    buffer_ = std::make_unique<SPSCRingBuffer<F32StereoFrame, kRingBufferCapacity>>();
    device_ = std::make_unique<Device>(buffer_.get());
    decode_finished_.store(false, std::memory_order_relaxed);
    state_ = PlayingState::Stop;
}

PlayerEngine::~PlayerEngine()
{
    abort_watchdog_.store(true, std::memory_order_release);
    if (watchdog_.joinable()) {
        watchdog_.join();
    }

    if (decoder_) {
        decoder_->stop();
        decoder_->join();
    }
}

bool PlayerEngine::start_device()
{
    if (!device_->start()) {
        logger->info("ma_device_start");
        decode_finished_.store(true, std::memory_order_release);
        if (decoder_) {
            decoder_->join();
        }
        return false;
    }

    return true;
}

std::unordered_map<std::string, std::string> PlayerEngine::metadata()
{
    if (!decoder_) {
        return {};
    }
    return decoder_->metadata();
}

void PlayerEngine::set_watchdog()
{
    watchdog_ = std::thread([this]() {
        bool has_notified = false;
        while (!abort_watchdog_.load(std::memory_order_acquire)) {
            // If decoder exists, completed reading, AND buffer drained completely:
            if (decoder_ && decode_finished_.load(std::memory_order_acquire) && buffer_->empty()) {
                if (!has_notified) {
                    PlayingState curr_state =
                        state_.exchange(PlayingState::Stop, std::memory_order_acq_rel);
                    if (curr_state != PlayingState::Stop) {
                        logger->info("Playback finished autonomously");
                        if (playback_callback_) {
                            playback_callback_(StopReason::NaturalEof);
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
    buffer_->clear();
    bool was_paused = (state_ == PlayingState::Pause);
    if (device_ && !was_paused) {
        device_->pause();
    }

    if (decoder_) {
        decoder_->stop();
        decoder_->join();
        decoder_.reset();
    }
    decode_finished_.store(false, std::memory_order_release);

    decoder_ = std::make_unique<Decoder>(url);
    if (!decoder_->is_valid()) {
        // 打开失败(文件缺失/无音频流/解码器不可用): 进入 STOP, 不启动解码线程,
        // 避免后续 av_read_frame 等在空上下文上崩溃。
        logger->error("set_url: cannot open '{}', decoder init failed", url);
        decoder_.reset();
        decode_finished_.store(true, std::memory_order_release);
        state_.store(PlayingState::Stop, std::memory_order_release);
        return;
    }
    if (pending_eq_) {
        decoder_->set_eq_config(pending_eq_); // apply cached EQ config
    }
    decoder_->work(buffer_.get(), &decode_finished_);

    // pre-filling 100ms buffer
    constexpr size_t prebuffer_frames = 44100 / 10; // 100ms
    while (!decode_finished_.load(std::memory_order_acquire)) {
        if (buffer_->size() >= prebuffer_frames) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (!was_paused) {
        device_->resume();
        state_ = PlayingState::Playing;
    }
}

void PlayerEngine::resume()
{
    if (device_ && state_ == PlayingState::Pause) {
        device_->resume();
        state_ = PlayingState::Playing;
    }
}

void PlayerEngine::pause()
{
    if (device_ && state_ == PlayingState::Playing) {
        device_->pause();
        state_ = PlayingState::Pause;
    }
}

void PlayerEngine::stop()
{
    // 1. 抢占并锁住 STOP，阻断看门狗对自然结束的判断
    PlayingState old_state = state_.exchange(PlayingState::Stop, std::memory_order_acq_rel);
    if (decoder_) {
        decoder_->stop();
    }
    if (device_) {
        device_->pause();
    }
    if (buffer_) {
        buffer_->clear();
    }

    // 2. 重置解码完成标志
    decode_finished_.store(false, std::memory_order_release);
    if (old_state != PlayingState::Stop && playback_callback_) {
        playback_callback_(StopReason::ManualStop);
    }
}

void PlayerEngine::set_volume(float volume)
{
    if (volume < 0 || volume > 1.0f) {
        logger->warn("Volume must be in range from 0 to 1");
        return;
    }
    if (device_) {
        device_->set_volume(volume);
    }
}

void PlayerEngine::seek(int64_t pos_ms)
{
    if (!device_ || !decoder_) {
        logger->warn("device or decoder does not exist (may be initializing)");
        return;
    }

    if (state_ == PlayingState::Stop) {
        return;
    }

    if (state_ == PlayingState::Playing) {
        device_->pause();
    }

    decoder_->seek(pos_ms);

    if (state_ == PlayingState::Playing) {
        device_->resume();
    }
}

void PlayerEngine::set_playback_finished_callback(std::function<void(StopReason)> func)
{
    playback_callback_ = std::move(func);
}

PlayerEngine::PlayingState PlayerEngine::state()
{
    return state_;
}

void PlayerEngine::set_eq_config(std::shared_ptr<const EqConfig> cfg)
{
    pending_eq_ = std::move(cfg);
    if (decoder_) {
        decoder_->set_eq_config(pending_eq_);
    }
}

std::shared_ptr<const EqConfig> PlayerEngine::eq_config() const
{
    if (decoder_) {
        return decoder_->eq_config();
    }
    return pending_eq_;
}

size_t PlayerEngine::get_recent_audio_frames(F32StereoFrame* out_buffer, size_t count)
{
    return device_->get_recent_audio_frames(out_buffer, count);
}

std::vector<std::string> PlayerEngine::output_devices() const
{
    if (!device_) {
        return {};
    }
    return device_->list_playback_devices();
}

std::string PlayerEngine::default_output_device_name() const
{
    if (!device_) {
        return {};
    }
    return device_->default_playback_device_name();
}

std::string PlayerEngine::current_output_device_name() const
{
    if (!device_) {
        return {};
    }
    return device_->current_playback_device_name();
}

bool PlayerEngine::set_output_device_by_name(const std::string& name)
{
    if (!device_) {
        return false;
    }

    const bool should_start = (state_.load(std::memory_order_acquire) == PlayingState::Playing);
    const bool ok           = device_->switch_playback_device_by_name(name, should_start);
    if (!ok) {
        return false;
    }

    if (should_start) {
        state_.store(PlayingState::Playing, std::memory_order_release);
    } else {
        state_.store(PlayingState::Pause, std::memory_order_release);
    }
    return true;
}

float PlayerEngine::volume()
{
    if (!device_) {
        return 0.0f;
    }
    return device_->get_volume();
}

int64_t PlayerEngine::position()
{
    if (!decoder_) {
        return 0;
    }
    return decoder_->position();
}
