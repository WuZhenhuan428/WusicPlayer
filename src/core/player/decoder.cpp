#include "decoder.h"

#include <cstdio>
#include <cstdlib>

#include "core/logger/logger_manager.h"
namespace
{
// NOLINTNEXTLINE
Logger* logger = LoggerManager::file_logger("decoder", {"console", "gui"});
} // namespace

namespace
{
// ---- EQ band → FFmpeg filter 映射 ----
const char* eq_filter_name(EqFilterType type)
{
    switch (type) {
    case EqFilterType::Parametric:
    case EqFilterType::Peak:
        return "equalizer";
    case EqFilterType::LowShelf:
        return "bass";
    case EqFilterType::HighShelf:
        return "treble";
    case EqFilterType::LowPass:
        return "lowpass";
    case EqFilterType::HighPass:
        return "highpass";
    }
    return "equalizer";
}

void eq_filter_args(const EqBand& band, char* buf, size_t size)
{
    // q 统一为 Q factor(width_type=q); 低通/高通不使用 gain
    switch (band.type) {
    case EqFilterType::LowPass:
    case EqFilterType::HighPass:
        std::snprintf(buf, size, "frequency=%.2f:width_type=q:width=%.3f", band.freq, band.q);
        break;
    case EqFilterType::Parametric:
    case EqFilterType::Peak:
    case EqFilterType::LowShelf:
    case EqFilterType::HighShelf:
    default:
        std::snprintf(buf, size, "frequency=%.2f:width_type=q:width=%.3f:gain=%.2f", band.freq,
                      band.q, band.gain_db);
        break;
    }
}

void eq_instance_name(int index, char* buf, size_t size)
{
    std::snprintf(buf, size, "eq_%d", index);
}
} // namespace

Decoder::Decoder(std::string filepath)
{
    filepath_ = std::move(filepath);
    // init_decoder() 失败(文件缺失等)时不得继续 init_filters():
    // filter 图依赖已打开的 codec 上下文, 空指针上构建会直接崩溃。
    has_init_ = this->init_decoder();
    if (has_init_) {
        has_init_ = (this->init_filters() == 0);
    }
    if (!has_init_) {
        logger->error("Error: failed to init decoder/filters");
    }
}

Decoder::~Decoder()
{
    av_frame_free(&frame_);
    av_packet_free(&pkt_);
    avcodec_free_context(&codec_ctx_);
    avformat_close_input(&fmt_ctx_);
    avfilter_graph_free(&filter_graph_);
}

bool Decoder::init_decoder()
{
    if (avformat_open_input(&fmt_ctx_, filepath_.c_str(), nullptr, nullptr) != 0) {
        logger->error("could not open file {}", filepath_);
        return false;
    }

    if (avformat_find_stream_info(fmt_ctx_, nullptr) < 0) {
        logger->error("could not find stream info");
        return false;
    }

    // find audio stream
    audio_stream_index_ = -1;
    for (unsigned int i = 0; i < fmt_ctx_->nb_streams; ++i) {
        if (fmt_ctx_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index_ = static_cast<int>(i);
            break;
        }
    }

    if (audio_stream_index_ == -1) {
        logger->error("No audio stream found");
        return false;
    }

    codecpar_            = fmt_ctx_->streams[audio_stream_index_]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar_->codec_id);
    if (!codec) {
        logger->error("could not find AVCodec");
        return false;
    }
    logger->info("AVCodec type: {}", static_cast<int>(codec->type));

    codec_ctx_ = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx_, codecpar_);
    if (avcodec_open2(codec_ctx_, codec, nullptr) < 0) {
        logger->error("Could not open codec");
        return false;
    }

    pkt_   = av_packet_alloc();
    frame_ = av_frame_alloc();

    this->parse_tag();

    logger->info("Initialization completed...");
    return true;
}

void Decoder::work(SPSCRingBuffer<F32StereoFrame, kRingBufferCapacity>* buffer,
                   std::atomic_bool* decode_finished)
{
    logger->info("thread start work");
    thread_ = std::thread(
        [this, buffer, decode_finished]() { this->thread_decode(buffer, decode_finished); });
}

void Decoder::join()
{
    logger->info("thread join");
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Decoder::stop()
{
    // 向后台线程发送终止信号
    abort_request_.store(true, std::memory_order_release);
}

void Decoder::decode(SPSCRingBuffer<F32StereoFrame, kRingBufferCapacity>* buffer, bool is_flush)
{
    int ret = avcodec_send_packet(codec_ctx_, is_flush ? nullptr : pkt_);

    if (!is_flush && pkt_) {
        av_packet_unref(pkt_);
    }

    if (ret < 0) {
        logger->error("Error submitting the packet to the decoder");
        return;
    }
    while (ret >= 0) {
        ret = avcodec_receive_frame(codec_ctx_, frame_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        }
        if (ret < 0) {
            logger->error("Error occured during decoding: code={}", ret);
            return;
        }

        if (frame_->best_effort_timestamp != AV_NOPTS_VALUE) {
            const AVRational stream_tb = fmt_ctx_->streams[audio_stream_index_]->time_base;
            const int64_t pos_ms =
                av_rescale_q(frame_->best_effort_timestamp, stream_tb, AVRational{1, 1000});
            pos_ms_.store(pos_ms, std::memory_order_release);
        }

        this->eq_check_and_update();

        int ret_push =
            av_buffersrc_add_frame_flags(buffersrc_ctx_, frame_, AV_BUFFERSRC_FLAG_KEEP_REF);
        if (ret_push < 0) {
            logger->error("some error occured when processing frame with EQ");
            break;
        }

        while (true) {
            AVFrame* filtered_frame = av_frame_alloc();
            ret                     = av_buffersink_get_frame(buffersink_ctx_, filtered_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                av_frame_free(&filtered_frame);
                break;
            }
            if (ret < 0) {
                av_frame_free(&filtered_frame);
                break;
            }

            const F32StereoFrame* frames =
                reinterpret_cast<const F32StereoFrame*>(filtered_frame->data[0]);
            std::size_t written     = 0;
            const std::size_t total = static_cast<std::size_t>(filtered_frame->nb_samples);

            while (written < total) {
                if (abort_request_.load(std::memory_order_acquire) ||
                    seek_req_ms_.load(std::memory_order_acquire) >= 0) {
                    break;
                }
                written += buffer->write(frames + written, total - written);
                if (written < total) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }

            av_frame_free(&filtered_frame);

            if (abort_request_.load(std::memory_order_acquire) ||
                seek_req_ms_.load(std::memory_order_acquire) >= 0) {
                break;
            }
        }

        if (abort_request_.load(std::memory_order_acquire) ||
            seek_req_ms_.load(std::memory_order_acquire) >= 0) {
            break;
        }
    }
}

void Decoder::thread_decode(SPSCRingBuffer<F32StereoFrame, kRingBufferCapacity>* buffer,
                            std::atomic_bool* decode_finished)
{
    // 初始化失败(文件缺失等): 不触碰任何 FFmpeg 上下文, 直接标记结束
    if (!has_init_) {
        decode_finished->store(true, std::memory_order_release);
        return;
    }

    // 如果没有被 abort，才继续向后解封装读取数据包
    while (!abort_request_.load(std::memory_order_acquire)) {

        // 过滤 seek 请求并执行
        int64_t target_ms = seek_req_ms_.load(std::memory_order_acquire);
        if (target_ms >= 0) {
            int64_t timestamp = target_ms * (AV_TIME_BASE / 1000);

            avformat_seek_file(fmt_ctx_, -1, INT64_MIN, timestamp, INT64_MAX, 0);
            avcodec_flush_buffers(codec_ctx_);
            buffer->clear();

            seek_req_ms_.store(-1, std::memory_order_release);
            // 重置播放完成标志，防止刚播完或者快播完的时候拉进度条导致提前结束
            decode_finished->store(false, std::memory_order_release);

            continue;
        }

        if (av_read_frame(fmt_ctx_, pkt_) < 0) {
            // 这里判断一下，如果是真的读完了但是没有 seek，才跳出开始 flush
            // 但如果处于暂停等特定情况...这里简单起见，读完了就 break 准备收尾
            // 当然，即使读完了，用户也可能 seek，所以在 break 后面的处理需要留意
            break;
        }

        if (pkt_->stream_index != audio_stream_index_) {
            av_packet_unref(pkt_);
            continue;
        }

        decode(buffer, false);
    }

    // Flush decoder after demux reaches end so delayed frames are emitted.
    decode(buffer, true);
    decode_finished->store(true, std::memory_order_release);
}

const std::unordered_map<std::string, std::string>& Decoder::metadata()
{
    return meta_;
}

bool Decoder::parse_tag() // OK
{
    if (!fmt_ctx_) {
        return false;
    }

    const AVDictionaryEntry* tag = nullptr;
    while ((tag = av_dict_iterate(fmt_ctx_->metadata, tag))) {
        meta_.emplace(tag->key, tag->value);
    }

    // 手动提取运行时和文件属性相关的必须指标
    // 1. 时长 (Duration)
    if (fmt_ctx_->duration != AV_NOPTS_VALUE) {
        // fmt_ctx->duration 的单位是 AV_TIME_BASE (即 1,000,000 分之一秒)
        int64_t duration_ms = fmt_ctx_->duration / (AV_TIME_BASE / 1000);
        meta_.emplace("DURATION_MS", std::to_string(duration_ms));
    }

    // 2. 码率 (Bitrate)
    if (fmt_ctx_->bit_rate > 0) {
        meta_.emplace("BITRATE", std::to_string(fmt_ctx_->bit_rate));
    }

    // 3. 采样率, 声道数, 编码格式 (Sample Rate, Channels, Codec)
    if (codecpar_) {
        meta_.emplace("SAMPLE_RATE", std::to_string(codecpar_->sample_rate));
        meta_.emplace("CHANNELS", std::to_string(codecpar_->ch_layout.nb_channels));

        const AVCodec* codec = avcodec_find_decoder(codecpar_->codec_id);
        if (codec) {
            meta_.emplace("CODEC_NAME", codec->name);
        }
    }

    if (meta_.empty()) {
        return false;
    }
    return true;
}

void Decoder::seek(int64_t position_ms)
{
    seek_req_ms_.store(position_ms, std::memory_order_release);
}

int Decoder::init_filters()
{
    // 可重复调用: 重建前释放旧图(eq_check_and_update 结构变化时触发)
    if (filter_graph_) {
        avfilter_graph_free(&filter_graph_);
        filter_graph_   = nullptr;
        buffersrc_ctx_  = nullptr;
        buffersink_ctx_ = nullptr;
        eq_ctxs_.clear();
    }

    const AVFilter* src  = avfilter_get_by_name("abuffer");
    const AVFilter* sink = avfilter_get_by_name("abuffersink");
    filter_graph_        = avfilter_graph_alloc();

    // setup buffer args
    char args[512];
    char layout_name[256];
    av_channel_layout_describe(&codec_ctx_->ch_layout, layout_name, sizeof(layout_name));
    // get metadata before resample
    std::snprintf(args, sizeof(args),
                  "time_base=1/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%s",
                  codec_ctx_->sample_rate, codec_ctx_->sample_rate,
                  av_get_sample_fmt_name(codec_ctx_->sample_fmt), layout_name);

    int ret =
        avfilter_graph_create_filter(&buffersrc_ctx_, src, "in", args, nullptr, filter_graph_);

    if (ret < 0) {
        logger->error("failed to create abuffer filter");
        return ret;
    }

    // setup sink args
    ret = avfilter_graph_create_filter(&buffersink_ctx_, sink, "out", nullptr, nullptr,
                                       filter_graph_);

    if (ret < 0) {
        logger->error("failed to create abuffersink filter");
        return ret;
    }

    // filter chain: (buffer)src -> aformat(to planar) -> eq -> aformat(to packed) -> (buffer)sink
    AVFilterContext* aformat_planar_ctx = nullptr;
    AVFilterContext* aformat_packed_ctx = nullptr;

    // 1. convert to planar float (FLTP) to use eq
    ret                                 = avfilter_graph_create_filter(
        &aformat_planar_ctx, avfilter_get_by_name("aformat"), "fmt_planar",
        "sample_fmts=fltp:sample_rates=44100:channel_layouts=stereo", nullptr, filter_graph_);
    if (ret < 0) {
        logger->error("failed to create aformat_planar filter");
        return ret;
    }

    // 2. EQ: 由 EqConfig 动态构建任意数量的 band 滤波器
    auto eq_cfg = pending_eq_.load(std::memory_order_acquire);
    if (!eq_cfg) {
        eq_cfg = std::make_shared<EqConfig>(); // 默认空
    }
    if (eq_cfg->enabled) {
        char name_buf[32];
        for (int i = 0; i < eq_cfg->bands.size(); ++i) {
            const EqBand& band = eq_cfg->bands[i];
            char args_eq[256];
            eq_filter_args(band, args_eq, sizeof(args_eq));
            eq_instance_name(i, name_buf, sizeof(name_buf));

            AVFilterContext* eq_ctx = nullptr;
            ret = avfilter_graph_create_filter(&eq_ctx,
                                               avfilter_get_by_name(eq_filter_name(band.type)),
                                               name_buf, args_eq, nullptr, filter_graph_);
            if (ret < 0) {
                logger->error("failed to create eq filter #{}", i);
                return ret;
            }
            eq_ctxs_.push_back(eq_ctx);
        }
    }

    // 2. packed
    ret = avfilter_graph_create_filter(
        &aformat_packed_ctx, avfilter_get_by_name("aformat"), "fmt_packed",
        "sample_fmts=flt:sample_rates=44100:channel_layouts=stereo", nullptr, filter_graph_);
    if (ret < 0) {
        logger->error("failed to create aformat_packed filter");
        return ret;
    }

    avfilter_link(buffersrc_ctx_, 0, aformat_planar_ctx, 0);
    if (eq_ctxs_.empty()) {
        // 无 EQ band: planar 直连 packed
        avfilter_link(aformat_planar_ctx, 0, aformat_packed_ctx, 0);
    } else {
        avfilter_link(aformat_planar_ctx, 0, eq_ctxs_.front(), 0);
        for (size_t i = 0; i + 1 < eq_ctxs_.size(); ++i) {
            avfilter_link(eq_ctxs_[i], 0, eq_ctxs_[i + 1], 0);
        }
        avfilter_link(eq_ctxs_.back(), 0, aformat_packed_ctx, 0);
    }
    avfilter_link(aformat_packed_ctx, 0, buffersink_ctx_, 0);

    // apply config
    ret = avfilter_graph_config(filter_graph_, nullptr);
    if (ret < 0) {
        logger->error("failed to apply filter graph's config");
        return ret;
    }

    // 记录已应用的结构(供 eq_check_and_update 判断是否需重建)
    applied_eq_ = std::move(eq_cfg);
    return 0;
}

int Decoder::process_frame_with_eq(AVFrame* frame)
{
    int ret = av_buffersrc_add_frame_flags(buffersrc_ctx_, frame, AV_BUFFERSRC_FLAG_KEEP_REF);
    if (ret < 0) {
        logger->error("failed to process frame");
        return ret;
    }

    AVFrame* filtered_frame = av_frame_alloc();
    ret                     = av_buffersink_get_frame(buffersink_ctx_, filtered_frame);
    if (ret == AVERROR(EAGAIN)) {
        av_frame_free(&filtered_frame);
        return 0; // continue
    }
    if (ret < 0) {
        av_frame_free(&filtered_frame);
        return ret;
    }
    return 0;
}

void Decoder::set_eq_config(std::shared_ptr<const EqConfig> cfg)
{
    if (!cfg) {
        return;
    }
    pending_eq_.store(std::move(cfg), std::memory_order_release);
    has_eq_changed_.store(true, std::memory_order_release);
}

bool Decoder::eq_structure_changed(const EqConfig& applied, const EqConfig& next)
{
    if (applied.enabled != next.enabled) {
        return true;
    }
    if (applied.bands.size() != next.bands.size()) {
        return true;
    }
    for (int i = 0; i < applied.bands.size(); ++i) {
        if (applied.bands[i].type != next.bands[i].type) {
            return true;
        }
        if (applied.bands[i].freq != next.bands[i].freq) {
            return true;
        }
        if (applied.bands[i].q != next.bands[i].q) {
            return true;
        }
    }
    return false;
}

void Decoder::eq_check_and_update()
{
    if (!has_eq_changed_.load(std::memory_order_acquire)) {
        return;
    }
    has_eq_changed_.store(false, std::memory_order_release);

    auto new_cfg = pending_eq_.load(std::memory_order_acquire);
    if (!new_cfg) {
        return;
    }

    // 结构未变 → 仅按 band 发增益命令; 结构变化/首次 → 重建 filter graph
    const bool rebuild = !applied_eq_ || Decoder::eq_structure_changed(*applied_eq_, *new_cfg);
    if (rebuild) {
        if (this->init_filters() != 0) {
            logger->error("eq: failed to rebuild filter graph");
        }
        return; // init_filters 已按新配置建图并记录 m_applied_eq
    }

    char gain_buf[32];
    const int band_count = static_cast<int>(new_cfg->bands.size());
    const int ctx_count  = static_cast<int>(eq_ctxs_.size());
    for (int i = 0; (i < band_count) && (i < ctx_count); ++i) {
        char name_buf[32];
        eq_instance_name(i, name_buf, sizeof(name_buf));
        std::snprintf(gain_buf, sizeof(gain_buf), "%.2f", new_cfg->bands[i].gain_db);
        avfilter_graph_send_command(filter_graph_, name_buf, "gain", gain_buf, nullptr, 0, 0);
    }
    applied_eq_ = std::move(new_cfg);
}

int64_t Decoder::position()
{
    return pos_ms_.load(std::memory_order_acquire);
}

std::shared_ptr<const EqConfig> Decoder::eq_config() const
{
    return pending_eq_.load(std::memory_order_acquire);
}
