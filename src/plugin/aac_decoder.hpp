// SPDX-License-Identifier: AGPL-3.0-only
// AAC decoder over FFmpeg's libavcodec, turning the glasses' raw AAC access units into 32-bit
// float planar samples for OBS.
#ifndef MGL_PLUGIN_AAC_DECODER_HPP
#define MGL_PLUGIN_AAC_DECODER_HPP

#include "mgl/bytes.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>

namespace mgl::plugin {
// The most channels handed on; OBS takes up to eight planes.
inline constexpr std::size_t MaxAudioChannels = 8;

struct DecodedAudio {
    std::uint64_t pts_us = 0;
    std::uint32_t sample_rate = 0;
    std::uint32_t channels = 0;
    std::uint32_t frames = 0; // samples per channel
    std::array<const float*, MaxAudioChannels> plane{};
};

// Not thread-safe; drive it from one thread (the media worker).
class AacDecoder {
  public:
    AacDecoder();
    ~AacDecoder();
    AacDecoder(const AacDecoder&) = delete;
    AacDecoder& operator=(const AacDecoder&) = delete;
    AacDecoder(AacDecoder&&) = delete;
    AacDecoder& operator=(AacDecoder&&) = delete;

    // Opens the decoder for an AudioSpecificConfig, or reopens it when the config changes. False
    // if FFmpeg cannot open that format; the decoder is then not ready.
    [[nodiscard]] bool configure(ByteView audio_specific_config);
    [[nodiscard]] bool ready() const noexcept;
    // Decodes one raw AAC access unit; the sink is called for each block of samples produced.
    // Units that arrive before a successful configure are dropped.
    void decode(std::uint64_t pts_us, ByteView aac,
                const std::function<void(const DecodedAudio&)>& sink);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace mgl::plugin
#endif
