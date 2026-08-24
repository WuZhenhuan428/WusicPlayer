#include "decoder.h"

#include <cstdio>
#include <cstdlib>

#include "core/logger/logger_manager.h"
namespace
{
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

Decoder::Decoder(const std::string& filepath)
{
    m_filepath = filepath;
    m_has_init = this->init_decoder();
    m_has_init = (this->init_filters() == 0);
    if (!m_has_init) {
        logger->error("Error: failed to init decoder/filters");
    }
}

Decoder::~Decoder()
{
    av_frame_free(&m_frame);
    av_packet_free(&m_pkt);
    avcodec_free_context(&m_codec_ctx);
    avformat_close_input(&m_fmt_ctx);
    avfilter_graph_free(&m_filter_graph);
}

bool Decoder::init_decoder()
{
    if (avformat_open_input(&m_fmt_ctx, m_filepath.c_str(), nullptr, nullptr) != 0) {
        logger->error("could not open file {}", m_filepath);
        return false;
    }

    if (avformat_find_stream_info(m_fmt_ctx, nullptr) < 0) {
        logger->error("could not find stream info");
        return false;
    }

    // find audio stream
    m_audio_stream_index = -1;
    for (unsigned int i = 0; i < m_fmt_ctx->nb_streams; ++i) {
        if (m_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            m_audio_stream_index = i;
            break;
        }
    }

    if (m_audio_stream_index == -1) {
        logger->error("No audio stream found");
        return false;
    }

    m_codecpar           = m_fmt_ctx->streams[m_audio_stream_index]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(m_codecpar->codec_id);
    if (!codec) {
        logger->error("could not find AVCodec");
        return false;
    }
    logger->info("AVCodec type: {}", (int)codec->type);

    m_codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_codec_ctx, m_codecpar);
    if (avcodec_open2(m_codec_ctx, codec, nullptr) < 0) {
        logger->error("Could not open codec");
        return false;
    }

    m_pkt   = av_packet_alloc();
    m_frame = av_frame_alloc();

    this->parse_tag();

    logger->info("Initialization completed...");
    return true;
}

void Decoder::work(SPSCRingBuffer<F32StereoFrame, RING_BUFFER_CAPACITY>* buffer,
                   std::atomic_bool* decode_finished)
{
    logger->info("thread start work");
    m_thread = std::thread(
        [this, buffer, decode_finished]() { this->thread_decode(buffer, decode_finished); });
}

void Decoder::join()
{
    logger->info("thread join");
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void Decoder::stop()
{
    // 向后台线程发送终止信号
    m_abort_request.store(true, std::memory_order_release);
}

void Decoder::decode(SPSCRingBuffer<F32StereoFrame, RING_BUFFER_CAPACITY>* buffer, bool is_flush)
{
    int ret = avcodec_send_packet(m_codec_ctx, is_flush ? nullptr : m_pkt);

    if (!is_flush && m_pkt) {
        av_packet_unref(m_pkt);
    }

    if (ret < 0) {
        logger->error("Error submitting the packet to the decoder");
        return;
    }
    while (ret >= 0) {
        ret = avcodec_receive_frame(m_codec_ctx, m_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return;
        } else if (ret < 0) {
            logger->error("Error occured during decoding: code={}", ret);
            return;
        }

        if (m_frame->best_effort_timestamp != AV_NOPTS_VALUE) {
            const AVRational stream_tb = m_fmt_ctx->streams[m_audio_stream_index]->time_base;
            const int64_t pos_ms =
                av_rescale_q(m_frame->best_effort_timestamp, stream_tb, AVRational{1, 1000});
            m_pos_ms.store(pos_ms, std::memory_order_release);
        }

        this->eq_check_and_update();

        int ret_push =
            av_buffersrc_add_frame_flags(m_buffersrc_ctx, m_frame, AV_BUFFERSRC_FLAG_KEEP_REF);
        if (ret_push < 0) {
            logger->error("some error occured when processing frame with EQ");
            break;
        }

        while (true) {
            AVFrame* filtered_frame = av_frame_alloc();
            ret                     = av_buffersink_get_frame(m_buffersink_ctx, filtered_frame);
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
                if (m_abort_request.load(std::memory_order_acquire) ||
                    m_seek_req_ms.load(std::memory_order_acquire) >= 0) {
                    break;
                }
                written += buffer->write(frames + written, total - written);
                if (written < total) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }

            av_frame_free(&filtered_frame);

            if (m_abort_request.load(std::memory_order_acquire) ||
                m_seek_req_ms.load(std::memory_order_acquire) >= 0) {
                break;
            }
        }

        if (m_abort_request.load(std::memory_order_acquire) ||
            m_seek_req_ms.load(std::memory_order_acquire) >= 0)
            break;
    }
}

void Decoder::thread_decode(SPSCRingBuffer<F32StereoFrame, RING_BUFFER_CAPACITY>* buffer,
                            std::atomic_bool* decode_finished)
{
    // 如果没有被 abort，才继续向后解封装读取数据包
    while (!m_abort_request.load(std::memory_order_acquire)) {

        // 过滤 seek 请求并执行
        int64_t target_ms = m_seek_req_ms.load(std::memory_order_acquire);
        if (target_ms >= 0) {
            int64_t timestamp = target_ms * (AV_TIME_BASE / 1000);

            avformat_seek_file(m_fmt_ctx, -1, INT64_MIN, timestamp, INT64_MAX, 0);
            avcodec_flush_buffers(m_codec_ctx);
            buffer->clear();

            m_seek_req_ms.store(-1, std::memory_order_release);
            // 重置播放完成标志，防止刚播完或者快播完的时候拉进度条导致提前结束
            decode_finished->store(false, std::memory_order_release);

            continue;
        }

        if (av_read_frame(m_fmt_ctx, m_pkt) < 0) {
            // 这里判断一下，如果是真的读完了但是没有 seek，才跳出开始 flush
            // 但如果处于暂停等特定情况...这里简单起见，读完了就 break 准备收尾
            // 当然，即使读完了，用户也可能 seek，所以在 break 后面的处理需要留意
            break;
        }

        if (m_pkt->stream_index != m_audio_stream_index) {
            av_packet_unref(m_pkt);
            continue;
        }

        decode(buffer, false);
    }

    // Flush decoder after demux reaches end so delayed frames are emitted.
    decode(buffer, true);
    decode_finished->store(true, std::memory_order_release);
}

const std::unordered_map<std::string, std::string> Decoder::metadata()
{
    return m_meta;
}

bool Decoder::parse_tag() // OK
{
    if (!m_fmt_ctx) {
        return false;
    }

    const AVDictionaryEntry* tag = nullptr;
    while ((tag = av_dict_iterate(m_fmt_ctx->metadata, tag))) {
        m_meta.emplace(tag->key, tag->value);
    }

    // 手动提取运行时和文件属性相关的必须指标
    // 1. 时长 (Duration)
    if (m_fmt_ctx->duration != AV_NOPTS_VALUE) {
        // fmt_ctx->duration 的单位是 AV_TIME_BASE (即 1,000,000 分之一秒)
        int64_t duration_ms = m_fmt_ctx->duration / (AV_TIME_BASE / 1000);
        m_meta.emplace("DURATION_MS", std::to_string(duration_ms));
    }

    // 2. 码率 (Bitrate)
    if (m_fmt_ctx->bit_rate > 0) {
        m_meta.emplace("BITRATE", std::to_string(m_fmt_ctx->bit_rate));
    }

    // 3. 采样率, 声道数, 编码格式 (Sample Rate, Channels, Codec)
    if (m_codecpar) {
        m_meta.emplace("SAMPLE_RATE", std::to_string(m_codecpar->sample_rate));
        m_meta.emplace("CHANNELS", std::to_string(m_codecpar->ch_layout.nb_channels));

        const AVCodec* codec = avcodec_find_decoder(m_codecpar->codec_id);
        if (codec) {
            m_meta.emplace("CODEC_NAME", codec->name);
        }
    }

    if (m_meta.empty()) {
        return false;
    }
    return true;
}

void Decoder::seek(int64_t position_ms)
{
    m_seek_req_ms.store(position_ms, std::memory_order_release);
}

int Decoder::init_filters()
{
    // 可重复调用: 重建前释放旧图(eq_check_and_update 结构变化时触发)
    if (m_filter_graph) {
        avfilter_graph_free(&m_filter_graph);
        m_filter_graph   = nullptr;
        m_buffersrc_ctx  = nullptr;
        m_buffersink_ctx = nullptr;
        m_eq_ctxs.clear();
    }

    const AVFilter* src  = avfilter_get_by_name("abuffer");
    const AVFilter* sink = avfilter_get_by_name("abuffersink");
    m_filter_graph       = avfilter_graph_alloc();

    // setup buffer args
    char args[512];
    char layout_name[256];
    av_channel_layout_describe(&m_codec_ctx->ch_layout, layout_name, sizeof(layout_name));
    // get metadata before resample
    std::snprintf(args, sizeof(args),
                  "time_base=1/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%s",
                  m_codec_ctx->sample_rate, m_codec_ctx->sample_rate,
                  av_get_sample_fmt_name(m_codec_ctx->sample_fmt), layout_name);

    int ret =
        avfilter_graph_create_filter(&m_buffersrc_ctx, src, "in", args, nullptr, m_filter_graph);

    if (ret < 0) {
        logger->error("failed to create abuffer filter");
        return ret;
    }

    // setup sink args
    ret = avfilter_graph_create_filter(&m_buffersink_ctx, sink, "out", nullptr, nullptr,
                                       m_filter_graph);

    if (ret < 0) {
        logger->error("failed to create abuffersink filter");
        return ret;
    }

    // filter chain: (buffer)src -> aformat(to planar) -> eq -> aformat(to packed) -> (buffer)sink
    AVFilterContext *aformat_planar_ctx, *aformat_packed_ctx;

    // 1. convert to planar float (FLTP) to use eq
    ret = avfilter_graph_create_filter(
        &aformat_planar_ctx, avfilter_get_by_name("aformat"), "fmt_planar",
        "sample_fmts=fltp:sample_rates=44100:channel_layouts=stereo", nullptr, m_filter_graph);
    if (ret < 0) {
        logger->error("failed to create aformat_planar filter");
        return ret;
    }

    // 2. EQ: 由 EqConfig 动态构建任意数量的 band 滤波器
    auto eq_cfg = m_pending_eq.load(std::memory_order_acquire);
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
            ret                     = avfilter_graph_create_filter(&eq_ctx,
                                                                   avfilter_get_by_name(eq_filter_name(band.type)),
                                                                   name_buf, args_eq, nullptr, m_filter_graph);
            if (ret < 0) {
                logger->error("failed to create eq filter #{}", i);
                return ret;
            }
            m_eq_ctxs.push_back(eq_ctx);
        }
    }

    // 2. packed
    ret = avfilter_graph_create_filter(
        &aformat_packed_ctx, avfilter_get_by_name("aformat"), "fmt_packed",
        "sample_fmts=flt:sample_rates=44100:channel_layouts=stereo", nullptr, m_filter_graph);
    if (ret < 0) {
        logger->error("failed to create aformat_packed filter");
        return ret;
    }

    avfilter_link(m_buffersrc_ctx, 0, aformat_planar_ctx, 0);
    if (m_eq_ctxs.empty()) {
        // 无 EQ band: planar 直连 packed
        avfilter_link(aformat_planar_ctx, 0, aformat_packed_ctx, 0);
    } else {
        avfilter_link(aformat_planar_ctx, 0, m_eq_ctxs.front(), 0);
        for (size_t i = 0; i + 1 < m_eq_ctxs.size(); ++i) {
            avfilter_link(m_eq_ctxs[i], 0, m_eq_ctxs[i + 1], 0);
        }
        avfilter_link(m_eq_ctxs.back(), 0, aformat_packed_ctx, 0);
    }
    avfilter_link(aformat_packed_ctx, 0, m_buffersink_ctx, 0);

    // apply config
    ret = avfilter_graph_config(m_filter_graph, nullptr);
    if (ret < 0) {
        logger->error("failed to apply filter graph's config");
        return ret;
    }

    // 记录已应用的结构(供 eq_check_and_update 判断是否需重建)
    m_applied_eq = std::move(eq_cfg);
    return 0;
}

int Decoder::process_frame_with_eq(AVFrame* frame)
{
    int ret = av_buffersrc_add_frame_flags(m_buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF);
    if (ret < 0) {
        logger->error("failed to process frame");
        return ret;
    }

    AVFrame* filtered_frame = av_frame_alloc();
    ret                     = av_buffersink_get_frame(m_buffersink_ctx, filtered_frame);
    if (ret == AVERROR(EAGAIN)) {
        av_frame_free(&filtered_frame);
        return 0; // continue
    } else if (ret < 0) {
        av_frame_free(&filtered_frame);
        return ret;
    }
    return 0;
}

void Decoder::set_eq_gain(gains_t gains)
{
    // 兼容 shim: 保留原十段 ±12dB 行为, 转换为 EqConfig 走统一后端
    auto cfg               = std::make_shared<EqConfig>();
    cfg->enabled           = true;

    const double freqs[10] = {31, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
    const double vals[10]  = {gains._31, gains._63, gains._125, gains._250, gains._500,
                              gains._1k, gains._2k, gains._4k,  gains._8k,  gains._16k};
    for (int i = 0; i < 10; ++i) {
        double g = vals[i];
        if (g > EQ_UPPER_LIMIT_DB)
            g = EQ_UPPER_LIMIT_DB;
        if (g < EQ_LOWER_LIMIT_DB)
            g = EQ_LOWER_LIMIT_DB;
        // Q≈1.414 对应原 width_type=o:width=1(1 octave 带宽)
        cfg->bands.push_back(EqBand{EqFilterType::Parametric, freqs[i], 1.414, g});
    }

    m_gains.store(gains, std::memory_order_release); // 兼容 gains() 查询
    this->set_eq_config(std::move(cfg));
}

void Decoder::set_eq_config(std::shared_ptr<const EqConfig> cfg)
{
    if (!cfg) {
        return;
    }
    m_pending_eq.store(std::move(cfg), std::memory_order_release);
    m_has_eq_changed.store(true, std::memory_order_release);
}

bool Decoder::eq_structure_changed(const EqConfig& applied, const EqConfig& next) const
{
    if (applied.enabled != next.enabled)
        return true;
    if (applied.bands.size() != next.bands.size())
        return true;
    for (int i = 0; i < applied.bands.size(); ++i) {
        if (applied.bands[i].type != next.bands[i].type)
            return true;
        if (applied.bands[i].freq != next.bands[i].freq)
            return true;
        if (applied.bands[i].q != next.bands[i].q)
            return true;
    }
    return false;
}

void Decoder::eq_check_and_update()
{
    if (!m_has_eq_changed.load(std::memory_order_acquire)) {
        return;
    }
    m_has_eq_changed.store(false, std::memory_order_release);

    auto new_cfg = m_pending_eq.load(std::memory_order_acquire);
    if (!new_cfg) {
        return;
    }

    // 结构未变 → 仅按 band 发增益命令; 结构变化/首次 → 重建 filter graph
    const bool rebuild = !m_applied_eq || this->eq_structure_changed(*m_applied_eq, *new_cfg);
    if (rebuild) {
        if (this->init_filters() != 0) {
            logger->error("eq: failed to rebuild filter graph");
        }
        return; // init_filters 已按新配置建图并记录 m_applied_eq
    }

    char gain_buf[32];
    for (int i = 0; i < new_cfg->bands.size() && i < static_cast<int>(m_eq_ctxs.size()); ++i) {
        char name_buf[32];
        eq_instance_name(i, name_buf, sizeof(name_buf));
        std::snprintf(gain_buf, sizeof(gain_buf), "%.2f", new_cfg->bands[i].gain_db);
        avfilter_graph_send_command(m_filter_graph, name_buf, "gain", gain_buf, nullptr, 0, 0);
    }
    m_applied_eq = std::move(new_cfg);
}

int64_t Decoder::position()
{
    return m_pos_ms.load(std::memory_order_acquire);
}

const gains_t Decoder::gains() const
{
    gains_t gains = m_gains.load(std::memory_order_acquire);
    return gains;
}
