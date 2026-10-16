// SPDX-License-Identifier: AGPL-3.0-only
#ifndef MGL_PRIVATE_INPUT_HPP
#define MGL_PRIVATE_INPUT_HPP

#include <cstdint>
#include <optional>
#include <string_view>

namespace mgl {
struct PrivateAddressInput {
    std::optional<std::uint64_t> value;
    std::uint32_t error = 0;
};

// Bounded stdin, hidden console echo with exact restoration. Never logs the input.
[[nodiscard]] PrivateAddressInput readPrivateAddress();
// Only exports when caller explicitly supplies MGL_PROBE_REPORT. Contains redacted JSON only.
[[nodiscard]] bool writeRequestedReport(std::string_view redacted_json);
} // namespace mgl
#endif
