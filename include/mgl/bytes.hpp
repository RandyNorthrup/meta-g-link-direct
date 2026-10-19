// SPDX-License-Identifier: AGPL-3.0-only
#ifndef MGL_BYTES_HPP
#define MGL_BYTES_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace mgl {
using Bytes = std::vector<std::uint8_t>;
using ByteView = std::span<const std::uint8_t>;

inline void append(Bytes& out, ByteView data) { out.insert(out.end(), data.begin(), data.end()); }

// Appends the low byte_count bytes of value (at most four), least significant first.
inline void appendLittleEndian(Bytes& out, std::uint32_t value,
                               std::size_t byte_count = sizeof(std::uint32_t)) {
    constexpr unsigned BitsPerByte = 8;
    constexpr std::uint32_t ByteMask = 0xFFU;
    const std::size_t count = byte_count < sizeof(value) ? byte_count : sizeof(value);
    for (std::size_t index = 0; index < count; ++index) {
        out.push_back(static_cast<std::uint8_t>((value >> (index * BitsPerByte)) & ByteMask));
    }
}
} // namespace mgl
#endif
