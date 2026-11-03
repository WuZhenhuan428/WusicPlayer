#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
}

#include "core/player/config.h"
#include "plugin/eq_types.h"
#include "ring_buffer.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

class Decoder
{
public:
    Decoder(std::string filepath);
    ~Decoder();

    // 持有线程与 FFmpeg 资源, 禁止拷贝与移动
    Decoder(const Decoder&)            = delete;
    Decoder& operator=(const Decoder&) = delete;
    Decoder(Decoder&&)                 = delete;
    Decoder& operator=(Decoder&&)      = delete;

    void work(SPSCRingBuffer<F32StereoFrame, kRingBufferCapacity>* buffer,
              std::atomic_bool* decode_finished);
    void join();
    void stop();
    void seek(int64_t position_ms);
    /// 文件/解码器是否初始化成功(文件缺失/无音频流/解码器不可用 → false)
    bool is_valid() const
    {
        return has_init_;
    }
    const std::unordered_map<std::string, std::string>& metadata();
    /// 插件路径: 任意 band, 无增益上下限
    void set_eq_config(std::shared_ptr<const EqConfig> cfg);
    /// 查询当前请求的 EQ 配置
    std::shared_ptr<const EqConfig> eq_config() const;
    int64_t position();

private:
    bool parse_tag();
    bool init_decoder();
    void decode(SPSCRingBuffer<F32StereoFrame, kRingBufferCapacity>* buffer, bool is_flush);
    void thread_decode(SPSCRingBuffer<F32StereoFrame, kRingBufferCapacity>* buffer,
                       std::atomic_bool* decode_finished);
    int init_filters();
    int process_frame_with_eq(AVFrame* frame);

    void eq_check_and_update();
    static bool eq_structure_changed(const EqConfig& applied, const EqConfig& next);

private:
    std::atomic_int64_t seek_req_ms_{-1};
    std::atomic_bool abort_request_{false};
    bool has_init_ = false;
    std::unordered_map<std::string, std::string> meta_;

    std::thread thread_;
    std::string filepath_;
    AVFormatContext* fmt_ctx_        = nullptr;
    int audio_stream_index_          = -1; // 默认无效
    AVCodecParameters* codecpar_     = nullptr;
    AVCodecContext* codec_ctx_       = nullptr;
    AVPacket* pkt_                   = nullptr;
    AVFrame* frame_                  = nullptr;

    // eq filter resources
    AVFilterGraph* filter_graph_     = nullptr;
    AVFilterContext* buffersrc_ctx_  = nullptr;
    AVFilterContext* buffersink_ctx_ = nullptr;

    // 由 EqConfig 动态构建的 band 滤波器链(替代固定十段)
    std::vector<AVFilterContext*> eq_ctxs_;

    std::atomic<std::shared_ptr<const EqConfig>> pending_eq_{nullptr};
    std::shared_ptr<const EqConfig> applied_eq_; // 解码线程独占
    std::atomic_bool has_eq_changed_{false};

    std::atomic<int64_t> pos_ms_{0};
};
