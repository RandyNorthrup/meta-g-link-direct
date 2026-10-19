// SPDX-License-Identifier: AGPL-3.0-only
#include "hevc_decoder.hpp"

#include "mgl/bytes.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
}

#include <cstdint>
#include <functional>
#include <memory>

namespace mgl::plugin {
namespace {
constexpr std::int64_t MicrosPerSecond = 1'000'000;
} // namespace

struct HevcDecoder::Impl {
    const AVCodec* codec = nullptr;
    AVCodecContext* context = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;
    Bytes parameters;
    Bytes scratch;

    ~Impl() {
        if (frame != nullptr) {
            av_frame_free(&frame);
        }
        if (packet != nullptr) {
            av_packet_free(&packet);
        }
        if (context != nullptr) {
            avcodec_free_context(&context);
        }
    }
};

HevcDecoder::HevcDecoder() : impl_(std::make_unique<Impl>()) {}
HevcDecoder::~HevcDecoder() = default;

bool HevcDecoder::open() {
    impl_->codec = avcodec_find_decoder(AV_CODEC_ID_HEVC);
    if (impl_->codec == nullptr) {
        return false;
    }
    impl_->context = avcodec_alloc_context3(impl_->codec);
    if (impl_->context == nullptr) {
        return false;
    }
    impl_->context->pkt_timebase = AVRational{1, static_cast<int>(MicrosPerSecond)};
    // Low-latency: no frame threading, decode and emit as soon as possible.
    impl_->context->thread_count = 1;
    if (avcodec_open2(impl_->context, impl_->codec, nullptr) < 0) {
        return false;
    }
    impl_->packet = av_packet_alloc();
    impl_->frame = av_frame_alloc();
    return impl_->packet != nullptr && impl_->frame != nullptr;
}

void HevcDecoder::setParameters(ByteView annex_b_parameter_sets) {
    impl_->parameters.assign(annex_b_parameter_sets.begin(), annex_b_parameter_sets.end());
}

void HevcDecoder::decode(std::uint64_t pts_us, ByteView annex_b, bool keyframe,
                         const std::function<void(const DecodedFrame&)>& sink) {
    // Prepend the stored parameter sets to keyframes so an IDR access unit carries its SPS/PPS.
    const std::uint8_t* data = annex_b.data();
    int size = static_cast<int>(annex_b.size());
    if (keyframe && !impl_->parameters.empty()) {
        impl_->scratch.clear();
        impl_->scratch.reserve(impl_->parameters.size() + annex_b.size());
        append(impl_->scratch, {impl_->parameters.data(), impl_->parameters.size()});
        append(impl_->scratch, annex_b);
        data = impl_->scratch.data();
        size = static_cast<int>(impl_->scratch.size());
    }
    // Reference the caller's bytes without copying; FFmpeg copies internally on send.
    impl_->packet->data =
        const_cast<std::uint8_t*>(data); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    impl_->packet->size = size;
    impl_->packet->pts = static_cast<std::int64_t>(pts_us);
    impl_->packet->dts = static_cast<std::int64_t>(pts_us);
    if (avcodec_send_packet(impl_->context, impl_->packet) < 0) {
        return;
    }
    while (avcodec_receive_frame(impl_->context, impl_->frame) == 0) {
        const auto format = static_cast<AVPixelFormat>(impl_->frame->format);
        const bool planar_yuv = format == AV_PIX_FMT_YUV420P || format == AV_PIX_FMT_YUVJ420P;
        if (planar_yuv && impl_->frame->width > 0 && impl_->frame->height > 0) {
            DecodedFrame out;
            out.width = static_cast<std::uint32_t>(impl_->frame->width);
            out.height = static_cast<std::uint32_t>(impl_->frame->height);
            out.pts_us =
                impl_->frame->pts >= 0 ? static_cast<std::uint64_t>(impl_->frame->pts) : pts_us;
            out.full_range =
                format == AV_PIX_FMT_YUVJ420P || impl_->frame->color_range == AVCOL_RANGE_JPEG;
            for (int plane = 0; plane < 3; ++plane) {
                out.plane[plane] = impl_->frame->data[plane];
                out.linesize[plane] = static_cast<std::uint32_t>(impl_->frame->linesize[plane]);
            }
            sink(out);
        }
        av_frame_unref(impl_->frame);
    }
}
} // namespace mgl::plugin
