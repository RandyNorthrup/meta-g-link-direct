// SPDX-License-Identifier: AGPL-3.0-only
// How Meta G Link Helper and its caller talk over a Unix socket. The helper is started with one
// role flag and the socket's path, and answers with one status line: a word and a number, ending in
// a newline.
//
// Bluetooth role: after "OK" the socket carries the RFCOMM bytes both ways.
// Wi-Fi role: the caller first sends the adapter's name, the network's name and its passphrase, a
// line each. After "OK" the helper stays on the network until the caller closes its end, then
// leaves the network and exits.
#ifndef MGL_MACOS_HELPER_PROTOCOL_HPP
#define MGL_MACOS_HELPER_PROTOCOL_HPP

#include <string>

namespace mgl::helper {
inline constexpr const char* BluetoothRole = "--socket";
inline constexpr const char* WifiRole = "--wifi-socket";

inline constexpr const char* StatusOk = "OK";
inline constexpr const char* StatusNoPermission = "NO_PERMISSION";
inline constexpr const char* StatusNoMetaService = "NO_META_SERVICE";
inline constexpr const char* StatusAmbiguous = "AMBIGUOUS";
inline constexpr const char* StatusConnectFailed = "CONNECT_FAILED";
inline constexpr const char* StatusBadRequest = "BAD_REQUEST";
inline constexpr const char* StatusNotVisible = "NOT_VISIBLE";

[[nodiscard]] inline std::string statusLine(const char* status, int detail) {
    return std::string(status) + " " + std::to_string(detail) + "\n";
}
} // namespace mgl::helper
#endif
