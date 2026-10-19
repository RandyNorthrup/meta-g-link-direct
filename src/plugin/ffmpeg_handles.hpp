// SPDX-License-Identifier: AGPL-3.0-only
// Owning handles for the FFmpeg objects the video and audio decoders share. The FFmpeg runtime
// ships with OBS.
#ifndef MGL_PLUGIN_FFMPEG_HANDLES_HPP
#define MGL_PLUGIN_FFMPEG_HANDLES_HPP

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavutil/frame.h>
}

#include <memory>

namespace mgl::plugin {
struct CodecContextFree {
    void operator()(AVCodecContext* context) const noexcept { avcodec_free_context(&context); }
};
struct PacketFree {
    void operator()(AVPacket* packet) const noexcept { av_packet_free(&packet); }
};
struct FrameFree {
    void operator()(AVFrame* frame) const noexcept { av_frame_free(&frame); }
};

using CodecContextPtr = std::unique_ptr<AVCodecContext, CodecContextFree>;
using PacketPtr = std::unique_ptr<AVPacket, PacketFree>;
using FramePtr = std::unique_ptr<AVFrame, FrameFree>;
} // namespace mgl::plugin
#endif
