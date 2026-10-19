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

// Before link setup completes, a first byte with bit 7 set is a bare DataX message. Once the
// AirShield keys are installed every SDU is an AirShield frame, so the same bit marks a COMMAND
// frame instead (Visor wiki, Transport-and-framing: "read bit 7 by session phase").
enum class LinkPhase : std::uint8_t { Plaintext, Encrypted };

// Largest SDU either framing can describe: a 14-bit DataX length plus its fixed header.
inline constexpr std::size_t MaximumSduSize = 4 + 16383;

// Reads only the framing prefix. Ready does not mean the complete frame has arrived.
// Invalid input must be rejected by the caller rather than skipped or treated as an empty frame.
[[nodiscard]] FramePrefix inspectFramePrefix(std::span<const std::uint8_t> head) noexcept;
[[nodiscard]] FramePrefix inspectFramePrefix(std::span<const std::uint8_t> head,
                                             LinkPhase phase) noexcept;

inline constexpr std::uint8_t AirshieldTypeMask = 0xC0;
inline constexpr std::uint8_t AirshieldData = 0x40;
inline constexpr std::uint8_t AirshieldCommand = 0x80;
} // namespace mgl
#endif
