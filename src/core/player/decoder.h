#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}

#include "types.h"
#include "config.h"
#include "ring_buffer.hpp"

#include <string>
#include <thread>
#include <atomic>
#include <unordered_map>

class Decoder
{
public:
    Decoder(const std::string& filepath);
    ~Decoder();

    void work(SPSCRingBuffer<F32StereoFrame, RING_BUFFER_CAPACITY>* buffer, std::atomic_bool* decode_finished);
    void join();
    void stop();
    void seek(int64_t position_ms);
    const std::unordered_map<std::string, std::string> metadata();
    void set_eq_gain(struct gains_t gains);
    int64_t position();

private:
    bool parse_tag();
    bool init_decoder();
    void decode(SPSCRingBuffer<F32StereoFrame, RING_BUFFER_CAPACITY>* buffer, bool is_flush);
    void thread_decode(SPSCRingBuffer<F32StereoFrame, RING_BUFFER_CAPACITY>* buffer, std::atomic_bool* decode_finished);
    int init_filters();
    int process_frame_with_eq(AVFrame* frame);

    void eq_check_and_update();

private:
    std::atomic_int64_t m_seek_req_ms{-1};
    std::atomic_bool m_abort_request{false};
    bool m_has_init = false;
    std::unordered_map<std::string, std::string> m_meta;

    std::thread m_thread;
    std::string m_filepath;
    AVFormatContext* m_fmt_ctx = nullptr;
    int m_audio_stream_index;
    AVCodecParameters* m_codecpar = nullptr;
    AVCodecContext* m_codec_ctx = nullptr;
    AVPacket* m_pkt = nullptr;
    AVFrame* m_frame = nullptr;

    // eq filter resources
    AVFilterGraph* m_filter_graph = nullptr;
    AVFilterContext* m_buffersrc_ctx = nullptr;
    AVFilterContext* m_buffersink_ctx = nullptr;

    AVFilterContext* m_eq_ctx_31 = nullptr;
    AVFilterContext* m_eq_ctx_63 = nullptr;
    AVFilterContext* m_eq_ctx_125 = nullptr;
    AVFilterContext* m_eq_ctx_250 = nullptr;
    AVFilterContext* m_eq_ctx_500 = nullptr;
    AVFilterContext* m_eq_ctx_1k = nullptr;
    AVFilterContext* m_eq_ctx_2k = nullptr;
    AVFilterContext* m_eq_ctx_4k = nullptr;
    AVFilterContext* m_eq_ctx_8k = nullptr;
    AVFilterContext* m_eq_ctx_16k = nullptr;

    std::atomic<struct gains_t> m_gains;
    // ^ m_gains is not lock free, but it doesn't matter QAQ
    std::atomic_bool m_has_eq_changed{false};
    struct gains_t m_gains_old{};

    std::atomic<int64_t> m_pos_ms{0};
};