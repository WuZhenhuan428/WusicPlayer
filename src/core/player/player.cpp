#include "core/player/player.h"

#include <QMetaObject>
#include <algorithm>

#include "core/logger/logger_manager.h"
namespace
{
// NOLINTNEXTLINE
Logger* logger = LoggerManager::file_logger("player", {"console", "gui"});
} // namespace

Player::Player(QObject* parent) :
    QObject(parent), player_engine_(std::make_unique<PlayerEngine>()), min_db_(-50.0)
{
    if (!player_engine_->start_device()) {
        return;
    }

    player_engine_->set_playback_finished_callback([this](PlayerEngine::StopReason reason) {
        QMetaObject::invokeMethod(
            this,
            [this, reason]() {
                if (reason == PlayerEngine::StopReason::NaturalEof) {
                    emit sgn_state_changed(player_engine_->state());
                    emit sgn_playback_natural_end();
                } /*else ...  */
            },
            Qt::QueuedConnection);
    });
    player_engine_->set_watchdog();

    refresh_device_cache();

    // 设备热插拔轮询(miniaudio 无变化信号, 每 2s 检测一次)
    device_poll_timer_ = new QTimer(this);
    device_poll_timer_->setInterval(2000);
    connect(device_poll_timer_, &QTimer::timeout, this, &Player::poll_devices);
    device_poll_timer_->start();

    position_timer_ = new QTimer(this);
    position_timer_->setInterval(100);
    connect(position_timer_, &QTimer::timeout, this, [this]() {
        if (!player_engine_) {
            return;
        }
        const PlayingState curr_state = player_engine_->state();
        if (curr_state == PlayingState::Playing || curr_state == PlayingState::Pause) {
            emit sgn_position_changed(this->position());
        }
    });
    position_timer_->start();
}

PlayingState Player::state() const
{
    if (!player_engine_) {
        return PlayingState::Stop;
    }
    return const_cast<PlayerEngine*>(player_engine_.get())->state();
}

void Player::read(const QString& filepath)
{
    if (!player_engine_ || filepath.isEmpty()) {
        return;
    }

    loaded_track_path_ = filepath;

    player_engine_->set_url(filepath.toStdString());
    player_engine_->resume();
    const auto meta  = player_engine_->metadata();
    auto duration_it = meta.find("DURATION_MS");
    if (duration_it != meta.end()) {
        emit sgn_duration_changed(QString::fromStdString(duration_it->second).toLongLong());
    }
    emit sgn_state_changed(player_engine_->state());
    emit sgn_position_changed(this->position());
}

void Player::play()
{
    if (!player_engine_) {
        return;
    }

    if (player_engine_->state() == PlayingState::Stop) {
        if (loaded_track_path_.isEmpty()) {
            logger->info("play ignored: no loaded track while in STOP state.");
            emit sgn_state_changed(player_engine_->state());
            emit sgn_position_changed(0);
            return;
        }

        player_engine_->set_url(loaded_track_path_.toStdString());
        const auto meta  = player_engine_->metadata();
        auto duration_it = meta.find("DURATION_MS");
        if (duration_it != meta.end()) {
            emit sgn_duration_changed(QString::fromStdString(duration_it->second).toLongLong());
        }
    }

    player_engine_->resume();
    emit sgn_state_changed(player_engine_->state());
}

void Player::pause()
{
    if (!player_engine_) {
        return;
    }
    player_engine_->pause();
    emit sgn_state_changed(player_engine_->state());
}

// sgn_state_changed & sgn_position_changed仅用于改变控制栏按键状态
void Player::stop()
{
    if (!player_engine_) {
        return;
    }

    emit sgn_state_changed(PlayingState::Stop);
    emit sgn_position_changed(0);

    const PlayingState prev_state = player_engine_->state();
    if (prev_state == PlayingState::Stop) {
        return;
    }
    player_engine_->stop();
}

void Player::seek(qint64 pos_ms)
{
    if (!player_engine_) {
        return;
    }
    player_engine_->seek(static_cast<int64_t>(pos_ms));
    emit sgn_position_changed(this->position());
}

bool Player::is_muted() const
{
    return is_mute_;
}

void Player::set_mute(bool mute)
{
    if (!player_engine_ || is_mute_ == mute) {
        return;
    }

    if (is_mute_) { // && mute = off (recover)
        player_engine_->set_volume(old_volume_);
    } else { // mute = on (mute)
        old_volume_ = player_engine_->volume();
        player_engine_->set_volume(0.0f);
    }

    is_mute_ = mute;
}

void Player::set_volume(float vol)
{
    if (!player_engine_) {
        return;
    }

    const double normalized = std::clamp(static_cast<double>(vol) / 100.0, 0.0, 1.0);
    double audio_gain       = Player::map_slider_to_volume(normalized, min_db_);
    player_engine_->set_volume(static_cast<float>(audio_gain));

    if (!is_mute_) {
        old_volume_ = static_cast<float>(audio_gain);
    }
}

double Player::map_slider_to_volume(double value, double min_db)
{
    if (value <= 0.0) {
        return 0.0;
    }

    if (value >= 1.0) {
        return 1.0;
    }

    double db = min_db + ((0.0 - min_db) * value);
    return qPow(10.0, db / 20.0);
}

qint64 Player::position() const
{
    if (!player_engine_) {
        return 0;
    }

    if (player_engine_->state() == PlayingState::Stop) {
        return 0;
    }

    return static_cast<qint64>(player_engine_->position());
}

void Player::set_output_device(const AudioDeviceInfo& device)
{
    this->apply_output_device(device, /*manual=*/true);
}

void Player::apply_output_device(const AudioDeviceInfo& device, bool manual)
{
    if (!player_engine_ || device.id.isEmpty()) {
        logger->warn("apply_output_device ignored. m_player_engine/device invalid.");
        return;
    }

    logger->info("switching output device to {} id='{}'", device.description, device.id);

    const bool ok = player_engine_->set_output_device_by_name(device.description.toStdString());
    if (!ok) {
        logger->warn("backend switch failed for '{}'", device.description);
        return;
    }

    preferred_output_id_ = device.id;
    if (manual && follow_system_device_) {
        // 用户手动选择设备 → 退出“跟随系统”
        follow_system_device_ = false;
        emit sgn_follow_system_changed(false);
    }
    refresh_device_cache();
    logger->info("output switch applied. active={} id='{}'", current_output_device().description,
                 current_output_device().id);
    emit sgn_device_changed(current_output_device());
}

void Player::set_output_device_by_id(const QByteArray& id)
{
    if (id.isEmpty()) {
        logger->warn("set_output_device_by_id ignored. empty id.");
        return;
    }

    logger->info("request switch by id='{}'", id);

    for (const auto& dev : audio_devices_) {
        if (dev.id == id) {
            set_output_device(dev);
            return;
        }
    }

    logger->warn("no matching output device id found: {}", id);
}

QVector<AudioDeviceInfo> Player::devices() const
{
    return audio_devices_;
}

AudioDeviceInfo Player::current_output_device() const
{
    for (const auto& dev : audio_devices_) {
        if (dev.id == current_output_id_) {
            return dev;
        }
    }

    if (!audio_devices_.isEmpty()) {
        return audio_devices_.first();
    }

    return {};
}

void Player::refresh_device_cache()
{
    if (!player_engine_) {
        audio_devices_.clear();
        current_output_id_.clear();
        return;
    }

    // 枚举来自 miniaudio(miniaudio 的 ma_context_get_devices)
    const auto names = player_engine_->output_devices();
    QVector<AudioDeviceInfo> devices;
    devices.reserve(static_cast<qsizetype>(names.size()));
    for (const auto& n : names) {
        AudioDeviceInfo info;
        info.id          = QByteArray(n.data(), static_cast<int>(n.size()));
        info.description = QString::fromUtf8(n.data(), static_cast<int>(n.size()));
        devices.push_back(std::move(info));
    }

    // 标记系统默认设备
    const std::string default_name = player_engine_->default_output_device_name();
    default_device_id_.clear();
    if (!default_name.empty()) {
        for (auto& dev : devices) {
            if (dev.description.toStdString() == default_name) {
                dev.is_default     = true;
                default_device_id_ = dev.id;
                break;
            }
        }
    }
    audio_devices_ = std::move(devices);

    if (audio_devices_.isEmpty()) {
        current_output_id_.clear();
        logger->warn("refresh_device_cache got empty list.");
        return;
    }

    const std::string active_name = player_engine_->current_output_device_name();
    for (const auto& dev : audio_devices_) {
        if (dev.description.toStdString() == active_name) {
            current_output_id_ = dev.id;
            // 避免 log 刷屏, 暂时关闭
            // logger->info("active backend device mapped: {}", dev.description);
            return;
        }
    }

    if (!preferred_output_id_.isEmpty()) {
        for (const auto& dev : audio_devices_) {
            if (dev.id == preferred_output_id_) {
                current_output_id_ = dev.id;
                logger->info("use preferred device: {}", dev.description);
                return;
            }
        }
    }

    current_output_id_ = audio_devices_.first().id;
    logger->info("using first available output device: {}", audio_devices_.first().description);
}

void Player::poll_devices()
{
    const QVector<AudioDeviceInfo> previous = audio_devices_;
    const QByteArray prev_default           = default_device_id_;
    this->refresh_device_cache();

    bool changed = (previous.size() != audio_devices_.size());
    if (!changed) {
        for (qsizetype i = 0; i < audio_devices_.size(); ++i) {
            if (previous[i].id != audio_devices_[i].id) {
                changed = true;
                break;
            }
        }
    }

    // 设备恢复: 因丢失而静音后, 设备重新出现则解除静音
    if (mute_due_device_loss_) {
        bool back = false;
        for (const auto& dev : audio_devices_) {
            if (dev.id == current_output_id_) {
                back = true;
                break;
            }
        }
        if (back && !audio_devices_.isEmpty()) {
            mute_due_device_loss_ = false;
            this->set_mute(false);
            logger->info("device recovered, unmuting");
        }
    }

    if (changed) {
        this->handle_devices_changed();
        return;
    }

    // 跟随系统: 系统默认设备变化 → 自动切换
    if (follow_system_device_ && !prev_default.isEmpty() && default_device_id_ != prev_default) {
        logger->info("system default device changed: {} -> {}", prev_default, default_device_id_);
        if (const AudioDeviceInfo* def = this->find_default_device()) {
            this->apply_output_device(*def, /*manual=*/false);
        }
    }
}

void Player::handle_devices_changed()
{
    const QByteArray old_id = current_output_id_;
    logger->info("output devices changed. old_id='{}'", old_id);
    logger->info("refreshed outputs count={} current_id='{}'", audio_devices_.size(),
                 current_output_id_);

    if (audio_devices_.isEmpty()) {
        current_output_id_.clear();
        mute_due_device_loss_ = true;
        this->set_mute(true); // 全部设备丢失 → 静音
        logger->warn("no available output devices after hot-plug; muting.");
        emit sgn_device_changed(AudioDeviceInfo{});
        return;
    }

    bool old_still_exists = false;
    for (const auto& dev : audio_devices_) {
        if (dev.id == old_id) {
            old_still_exists = true;
            break;
        }
    }

    if (!old_still_exists) {
        logger->info("current output device removed.");
        if (follow_system_device_) {
            // 跟随系统: 切到当前系统默认(或第一个可用)
            const AudioDeviceInfo* target = this->find_default_device();
            if (!target) {
                target = &audio_devices_.first();
            }
            logger->info("following system default: {}", target->description);
            this->apply_output_device(*target, /*manual=*/false);
        } else {
            // 非跟随: 当前设备丢失 → 静音, 等待设备恢复
            logger->warn("device lost while not following system; muting.");
            mute_due_device_loss_ = true;
            this->set_mute(true);
        }
        return;
    }

    logger->info("output device still valid: {}", current_output_device().description);
    emit sgn_device_changed(current_output_device());
}

bool Player::follow_system_device() const
{
    return follow_system_device_;
}

void Player::set_follow_system_device(bool on)
{
    if (follow_system_device_ == on) {
        return;
    }
    follow_system_device_ = on;
    emit sgn_follow_system_changed(on);
    if (on) {
        // 开启跟随 → 立即切到系统默认设备
        logger->info("follow system device enabled");
        if (const AudioDeviceInfo* def = this->find_default_device()) {
            this->apply_output_device(*def, /*manual=*/false);
        }
    } else {
        // 关闭跟随 → 固定当前设备
        logger->info("follow system device disabled, pinning current device");
        if (!current_output_id_.isEmpty()) {
            preferred_output_id_ = current_output_id_;
        }
    }
}

const AudioDeviceInfo* Player::find_default_device() const
{
    for (const auto& dev : audio_devices_) {
        if (dev.is_default) {
            return &dev;
        }
    }
    return audio_devices_.isEmpty() ? nullptr : &audio_devices_.first();
}

void Player::set_eq_config(std::shared_ptr<const EqConfig> cfg)
{
    player_engine_->set_eq_config(std::move(cfg));
}

std::shared_ptr<const EqConfig> Player::eq_config() const
{
    return player_engine_->eq_config();
}

float Player::volume() const
{
    return player_engine_->volume();
}
