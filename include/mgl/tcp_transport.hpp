// SPDX-License-Identifier: AGPL-3.0-only
// A TCP transport to the glasses over the Wi-Fi group. The glasses carry the same SDUs as on
// RFCOMM, with no length prefix.
#ifndef MGL_TCP_TRANSPORT_HPP
#define MGL_TCP_TRANSPORT_HPP

#include "mgl/transport.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace mgl {
struct TcpOpen {
    bool ok = false;
    std::uint32_t detail = 0; // Winsock error when not ok
    std::unique_ptr<Transport> transport;
};

// Connects to the glasses' Wi-Fi address and port from their SoftApNetworkInfo.
[[nodiscard]] TcpOpen openTcp(const std::string& address, std::uint16_t port);
} // namespace mgl
#endif
