// SPDX-License-Identifier: AGPL-3.0-only
#ifndef MGL_SDU_HPP
#define MGL_SDU_HPP

#include <cstddef>
#include <cstdint>
#include <span>

namespace mgl {
enum class PrefixStatus : std::uint8_t { NeedMore, Ready, Invalid };

struct FramePrefix {
    PrefixStatus status;
    std::size_t frame_size;
};

// Reads only the framing prefix. Ready does not mean the complete frame has arrived.
// Invalid input must be rejected by the caller rather than skipped or treated as an empty frame.
[[nodiscard]] FramePrefix inspectFramePrefix(std::span<const std::uint8_t> head) noexcept;
} // namespace mgl
#endif
