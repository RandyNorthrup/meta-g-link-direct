// SPDX-License-Identifier: AGPL-3.0-only
// Just enough HEVC (H.265) Annex-B parsing to recognise frames, keyframes and the picture size.
// Ported in behaviour from Visor core/media Hevc.kt at the revision in THIRD_PARTY_NOTICES.md.
#ifndef MGL_HEVC_HPP
#define MGL_HEVC_HPP

#include "mgl/bytes.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace mgl::hevc {
// The HEVC NAL unit types present in an Annex-B buffer, in order.
[[nodiscard]] std::vector<int> nalTypes(ByteView annex_b);

// True when the buffer carries an IRAP (keyframe) NAL, types 16..21.
[[nodiscard]] bool hasKeyframe(ByteView annex_b);

// VPS/SPS/PPS parameter-set NAL types are 32..34.
[[nodiscard]] bool hasParameterSets(ByteView annex_b);

struct Dimensions {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

// The picture size from the first SPS (type 33) in the buffer, if one parses.
[[nodiscard]] std::optional<Dimensions> spsDimensions(ByteView annex_b);
} // namespace mgl::hevc
#endif
