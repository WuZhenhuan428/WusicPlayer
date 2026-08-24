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
    Decoder(const std::string& filepath);
    ~Decoder();

    void work(SPSCRingBuffer<F32StereoFrame, RING_BUFFER_CAPACITY>* buffer,
              std::atomic_bool* decode_finished);
    void join();
    void stop();
    void seek(int64_t position_ms);
    const std::unordered_map<std::string, std::string> metadata();
    /// 插件路径: 任意 band, 无增益上下限
    void set_eq_config(std::shared_ptr<const EqConfig> cfg);
    /// 查询当前请求的 EQ 配置
    std::shared_ptr<const EqConfig> eq_config() const;
    int64_t position();

private:
    bool parse_tag();
    bool init_decoder();
    void decode(SPSCRingBuffer<F32StereoFrame, RING_BUFFER_CAPACITY>* buffer, bool is_flush);
    void thread_decode(SPSCRingBuffer<F32StereoFrame, RING_BUFFER_CAPACITY>* buffer,
                       std::atomic_bool* decode_finished);
    int init_filters();
    int process_frame_with_eq(AVFrame* frame);

    void eq_check_and_update();
    bool eq_structure_changed(const EqConfig& applied, const EqConfig& next) const;

private:
    std::atomic_int64_t m_seek_req_ms{-1};
    std::atomic_bool m_abort_request{false};
    bool m_has_init = false;
    std::unordered_map<std::string, std::string> m_meta;

    std::thread m_thread;
    std::string m_filepath;
    AVFormatContext* m_fmt_ctx = nullptr;
    int m_audio_stream_index;
    AVCodecParameters* m_codecpar     = nullptr;
    AVCodecContext* m_codec_ctx       = nullptr;
    AVPacket* m_pkt                   = nullptr;
    AVFrame* m_frame                  = nullptr;

    // eq filter resources
    AVFilterGraph* m_filter_graph     = nullptr;
    AVFilterContext* m_buffersrc_ctx  = nullptr;
    AVFilterContext* m_buffersink_ctx = nullptr;

    // 由 EqConfig 动态构建的 band 滤波器链(替代固定十段)
    std::vector<AVFilterContext*> m_eq_ctxs;

    std::atomic<std::shared_ptr<const EqConfig>> m_pending_eq{nullptr};
    std::shared_ptr<const EqConfig> m_applied_eq; // 解码线程独占
    std::atomic_bool m_has_eq_changed{false};

    std::atomic<int64_t> m_pos_ms{0};
};
