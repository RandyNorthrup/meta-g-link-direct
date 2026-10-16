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

struct ClassicDiscovery {
    std::uint32_t classic_devices;
    std::uint32_t classic_paired_devices;
    std::uint32_t classic_error;
    std::uint32_t meta_rfcomm_services;
    std::uint32_t rfcomm_hresult;
    bool inquiry_completed;
    bool rfcomm_timed_out;
    bool rfcomm_ambiguous;
    bool rfcomm_connection_attempted;
    bool rfcomm_connected;
};

// Discovery performs Classic inquiry plus Windows-registered Meta service lookup.
// Explicit connect skips inquiry, requires one paired service, opens/closes its socket
// with a deadline and sends no application frames. No pairing or ownership changes.
[[nodiscard]] ClassicDiscovery inspectClassicRfcomm(bool connect);
} // namespace mgl
#endif
