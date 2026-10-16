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
    std::uint32_t uncached_peer_queries;
    std::uint32_t uncached_peers_resolved;
    std::uint32_t uncached_service_queries;
    std::uint32_t uncached_query_failures;
    std::uint32_t uncached_bluetooth_error;
    bool uncached_requested;
    bool uncached_complete;
    bool uncached_limit_reached;
    std::uint32_t pairing_error;
    bool pairing_dialog_requested;
    bool windows_bonded;
    bool inquiry_completed;
    bool rfcomm_timed_out;
    bool rfcomm_ambiguous;
    bool rfcomm_connection_attempted;
    bool rfcomm_connected;
};

// Discovery performs Classic inquiry plus Windows-registered Meta service lookup.
// Explicit connect skips inquiry, requires one paired service, opens/closes its socket
// with a deadline and sends no application frames. No pairing or ownership changes.
enum class ClassicMode : std::uint8_t {
    RegisteredDiscovery,
    RegisteredConnect,
    LiveDiscovery,
    LiveConnect,
    PairAndConnect
};

// Live modes query the Meta UUID directly on inquiry peers using Uncached SDP.
// Peer addresses stay inside the implementation. Limits/incomplete queries are explicit.
[[nodiscard]] ClassicDiscovery inspectClassicRfcomm(ClassicMode mode);
} // namespace mgl
#endif
