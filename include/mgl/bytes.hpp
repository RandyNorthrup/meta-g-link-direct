// SPDX-License-Identifier: AGPL-3.0-only
#ifndef MGL_BYTES_HPP
#define MGL_BYTES_HPP

#include <cstdint>
#include <span>
#include <vector>

namespace mgl {
using Bytes = std::vector<std::uint8_t>;
using ByteView = std::span<const std::uint8_t>;

inline void append(Bytes& out, ByteView data) { out.insert(out.end(), data.begin(), data.end()); }
} // namespace mgl
#endif
