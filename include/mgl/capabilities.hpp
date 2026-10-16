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
    std::uint32_t bluetooth_features_error;
    bool bluetooth_features_valid;
    bool bluetooth_le_supported;
    bool bluetooth_classic_supported;
    std::uint8_t bluetooth_hci_version;
    std::uint8_t bluetooth_lmp_version;
};

// Enumerates local radios only. Does not scan for, pair, connect to or change a device.
[[nodiscard]] WirelessCapabilities inspectWirelessCapabilities() noexcept;
} // namespace mgl
#endif
