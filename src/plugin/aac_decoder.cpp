// SPDX-License-Identifier: AGPL-3.0-only
#include "aac_decoder.hpp"

#include "ffmpeg_handles.hpp"
#include "mgl/bytes.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/codec.h>
#include <libavcodec/codec_id.h>
#include <libavcodec/defs.h>
#include <libavcodec/packet.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavutil/rational.h>
#include <libavutil/samplefmt.h>
}

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <utility>

namespace mgl::plugin {
namespace {
constexpr int MicrosPerSecond = 1'000'000;
} // namespace

struct AacDecoder::Impl {
    CodecContextPtr context;
    PacketPtr packet;
    FramePtr frame;
    Bytes config; // the AudioSpecificConfig the open decoder was built for
};

AacDecoder::AacDecoder() : impl_(std::make_unique<Impl>()) {}
AacDecoder::~AacDecoder() = default;

bool AacDecoder::ready() const noexcept { return impl_->context != nullptr; }

bool AacDecoder::configure(ByteView audio_specific_config) {
    if (ready() && std::ranges::equal(impl_->config, audio_specific_config)) {
        return true;
    }
    impl_->context.reset();
    impl_->config.clear();
    if (audio_specific_config.empty()) {
        return false;
    }
    const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
    if (codec == nullptr) {
        return false;
    }
    CodecContextPtr context(avcodec_alloc_context3(codec));
    if (!context) {
        return false;
    }
    // FFmpeg reads the sample rate and channel layout from the AudioSpecificConfig. It owns the
    // extradata buffer and frees it with the context; the padding must be zeroed.
    const auto size = audio_specific_config.size();
    auto* extradata = static_cast<std::uint8_t*>(av_mallocz(size + AV_INPUT_BUFFER_PADDING_SIZE));
    if (extradata == nullptr) {
        return false;
    }
    std::ranges::copy(audio_specific_config, extradata);
    context->extradata = extradata;
    context->extradata_size = static_cast<int>(size);
    context->pkt_timebase = AVRational{.num = 1, .den = MicrosPerSecond};
    if (avcodec_open2(context.get(), codec, nullptr) < 0) {
        return false;
    }
    PacketPtr packet(av_packet_alloc());
    FramePtr frame(av_frame_alloc());
    if (!packet || !frame) {
        return false;
    }
    impl_->context = std::move(context);
    impl_->packet = std::move(packet);
    impl_->frame = std::move(frame);
    impl_->config.assign(audio_specific_config.begin(), audio_specific_config.end());
    return true;
}

void AacDecoder::decode(std::uint64_t pts_us, ByteView aac,
                        const std::function<void(const DecodedAudio&)>& sink) {
    if (!ready() || aac.empty()) {
        return;
    }
    // Reference the caller's bytes without copying; FFmpeg copies into a padded buffer on send.
    impl_->packet->data =
        const_cast<std::uint8_t*>(aac.data()); // NOLINT(cppcoreguidelines-pro-type-const-cast)
    impl_->packet->size = static_cast<int>(aac.size());
    impl_->packet->pts = static_cast<std::int64_t>(pts_us);
    impl_->packet->dts = static_cast<std::int64_t>(pts_us);
    if (avcodec_send_packet(impl_->context.get(), impl_->packet.get()) < 0) {
        return;
    }
    while (avcodec_receive_frame(impl_->context.get(), impl_->frame.get()) == 0) {
        const AVFrame& frame = *impl_->frame;
        const int channels = frame.ch_layout.nb_channels;
        const bool float_planar = static_cast<AVSampleFormat>(frame.format) == AV_SAMPLE_FMT_FLTP;
        if (float_planar && channels > 0 && std::cmp_less_equal(channels, MaxAudioChannels) &&
            frame.nb_samples > 0 && frame.sample_rate > 0) {
            DecodedAudio out;
            out.pts_us = frame.pts >= 0 ? static_cast<std::uint64_t>(frame.pts) : pts_us;
            out.sample_rate = static_cast<std::uint32_t>(frame.sample_rate);
            out.channels = static_cast<std::uint32_t>(channels);
            out.frames = static_cast<std::uint32_t>(frame.nb_samples);
            const std::span planes(frame.extended_data, static_cast<std::size_t>(channels));
            std::ranges::transform(planes, out.plane.begin(), [](const std::uint8_t* plane) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) — FLTP planes.
                return reinterpret_cast<const float*>(plane);
            });
            sink(out);
        }
        av_frame_unref(impl_->frame.get());
    }
}
} // namespace mgl::plugin
