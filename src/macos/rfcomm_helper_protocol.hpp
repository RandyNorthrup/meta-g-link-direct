// SPDX-License-Identifier: AGPL-3.0-only
// The status line Meta G Link Helper sends first on its socket: a word and a number, ending in a
// newline. After "OK" the socket carries the RFCOMM bytes.
#ifndef MGL_MACOS_RFCOMM_HELPER_PROTOCOL_HPP
#define MGL_MACOS_RFCOMM_HELPER_PROTOCOL_HPP

#include <string>

namespace mgl::helper {
inline constexpr const char* StatusOk = "OK";
inline constexpr const char* StatusNoPermission = "NO_PERMISSION";
inline constexpr const char* StatusNoMetaService = "NO_META_SERVICE";
inline constexpr const char* StatusAmbiguous = "AMBIGUOUS";
inline constexpr const char* StatusConnectFailed = "CONNECT_FAILED";

[[nodiscard]] inline std::string statusLine(const char* status, int detail) {
    return std::string(status) + " " + std::to_string(detail) + "\n";
}
} // namespace mgl::helper
#endif
