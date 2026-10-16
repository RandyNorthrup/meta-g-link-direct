// SPDX-License-Identifier: AGPL-3.0-only
#ifndef MGL_DISCOVERY_HPP
#define MGL_DISCOVERY_HPP

#include <cstdint>

namespace mgl {
struct BluetoothDiscovery {
    std::uint32_t advertisement_reports;
    std::uint32_t meta_advertisement_reports;
    std::uint32_t hresult;
    std::uint32_t bluetooth_error;
    bool scan_completed;
};

// One passive, ten-second scan. Counts packets, not unique devices or authenticated glasses.
// No pairing, device names/addresses, payload storage, or network changes.
[[nodiscard]] BluetoothDiscovery inspectBluetoothAdvertisements();
} // namespace mgl
#endif
