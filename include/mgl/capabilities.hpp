// SPDX-License-Identifier: AGPL-3.0-only
#ifndef MGL_CAPABILITIES_HPP
#define MGL_CAPABILITIES_HPP

#include <cstdint>

namespace mgl {
struct WirelessCapabilities {
    std::uint32_t wifi_interfaces;
    std::uint32_t wifi_connected_interfaces;
    bool bluetooth_radio;
    std::uint32_t wifi_error;
    std::uint32_t bluetooth_error;
};

// Enumerates local radios only. Does not scan for, pair, connect to or change a device.
[[nodiscard]] WirelessCapabilities inspectWirelessCapabilities() noexcept;
} // namespace mgl
#endif
