// SPDX-License-Identifier: AGPL-3.0-only
// HEVC (H.265) decoder over FFmpeg's libavcodec, turning the glasses' Annex-B access units into
// planar YUV frames for OBS. The FFmpeg runtime ships with OBS, so this lives in the plugin.
#ifndef MGL_PLUGIN_HEVC_DECODER_HPP
#define MGL_PLUGIN_HEVC_DECODER_HPP

#include "mgl/bytes.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>

namespace mgl::plugin {
// Planar YUV 4:2:0 carries Y, U and V planes.
inline constexpr std::size_t PlaneCount = 3;

struct DecodedFrame {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint64_t pts_us = 0;
    std::array<const std::uint8_t*, PlaneCount> plane{};
    std::array<std::uint32_t, PlaneCount> linesize{};
    bool full_range = false;
};

// Not thread-safe; drive it from one thread (the media worker).
class HevcDecoder {
  public:
    HevcDecoder();
    ~HevcDecoder();
    HevcDecoder(const HevcDecoder&) = delete;
    HevcDecoder& operator=(const HevcDecoder&) = delete;
    HevcDecoder(HevcDecoder&&) = delete;
    HevcDecoder& operator=(HevcDecoder&&) = delete;

    [[nodiscard]] bool open();
    // Remember the latest VPS/SPS/PPS; they are prepended to keyframes so IDR frames decode.
    void setParameters(ByteView annex_b_parameter_sets);
    // Decode one access unit; the sink is called for each frame produced (usually one).
    void decode(std::uint64_t pts_us, ByteView annex_b, bool keyframe,
                const std::function<void(const DecodedFrame&)>& sink);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace mgl::plugin
#endif
