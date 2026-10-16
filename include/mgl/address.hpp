// SPDX-License-Identifier: AGPL-3.0-only
#ifndef MGL_ADDRESS_HPP
#define MGL_ADDRESS_HPP

#include <cstdint>
#include <optional>
#include <string_view>

namespace mgl {
// Exactly six octets, compact or consistently colon/dash separated. No logging.
// Reject malformed, zero and broadcast values; does not establish device identity.
[[nodiscard]] std::optional<std::uint64_t> parseBluetoothAddress(std::string_view input) noexcept;
} // namespace mgl
#endif
