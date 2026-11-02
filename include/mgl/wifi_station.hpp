// SPDX-License-Identifier: AGPL-3.0-only
// Connects a Wi-Fi adapter, as an ordinary station, to the access point the glasses host (see
// stream::buildCreateSoftApNetwork). Joining an AP is the robust direction of Wi-Fi: unlike a
// SoftAP, a station connect is well supported on every adapter. The handle holds the connection and
// restores the adapter's previous network when it is released.
#ifndef MGL_WIFI_STATION_HPP
#define MGL_WIFI_STATION_HPP

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mgl {
enum class WifiStationStatus : std::uint8_t {
    Ok,
    NoAdapter,      // no Wi-Fi adapter is present
    AdapterMissing, // the adapter chosen by id is not present
    ProfileFailed,  // the system refused the temporary WPA2 network
    ConnectFailed,  // the adapter did not associate in time
    NoAddress,      // associated but DHCP gave no address on the glasses' subnet in time
    BadPeer,        // the glasses' reported address is not an IPv4 address
    NoPermission,   // the system did not allow this app to join (macOS: Location for the helper)
};

// A Wi-Fi adapter that can join the glasses' network.
struct WifiAdapter {
    std::string id;          // Windows: the interface GUID, "{...}"; elsewhere the interface name
    std::string description; // the adapter's name, as the system shows it
};

// The Wi-Fi adapters the system lists, in its order. Empty when there are none or the system's
// Wi-Fi service is unavailable.
[[nodiscard]] std::vector<WifiAdapter> listWifiAdapters();

// Whether the adapter with this WifiAdapter::id is present now.
[[nodiscard]] bool wifiAdapterPresent(const std::string& adapter_id);

// The name to give the glasses' network on this computer: fresh, a new random name, unless the
// system keeps something for every network it joins that this program cannot delete. macOS keeps a
// keychain item per name, so there the first name stays and only one item ever collects.
[[nodiscard]] std::string wifiNetworkName(const std::string& fresh);

struct WifiStation {
    WifiStationStatus status = WifiStationStatus::ConnectFailed;
    std::uint32_t detail = 0; // the system's error code when not Ok
    std::string detail_text;  // the system's reason in words, where it gives one; never a secret
    // From the connect request: when the adapter associated, and when it had its DHCP address.
    std::chrono::milliseconds associated_after{0};
    std::chrono::milliseconds addressed_after{0};
};

// Keeps the station connected to the glasses' AP. Releasing it removes the temporary network, which
// disconnects the adapter; the system then returns it to its usual networks. A failed connect
// removes the network too.
class WifiStationConnection {
  public:
    WifiStationConnection() = default;
    virtual ~WifiStationConnection() = default;
    WifiStationConnection(const WifiStationConnection&) = delete;
    WifiStationConnection& operator=(const WifiStationConnection&) = delete;
    WifiStationConnection(WifiStationConnection&&) = delete;
    WifiStationConnection& operator=(WifiStationConnection&&) = delete;
};

// Connects a Wi-Fi adapter to the given WPA2-PSK network and waits for a DHCP address on the same
// subnet as peer_address, the glasses' IPv4 address on that network. An address the adapter still
// holds from its previous network does not count. adapter_id is a WifiAdapter::id; empty uses the
// first adapter the system lists. An id that matches no adapter fails with AdapterMissing rather
// than falling back to another adapter. Returns null and fills out on failure; otherwise out.status
// is Ok and the connection handle is returned.
[[nodiscard]] std::unique_ptr<WifiStationConnection>
connectWifiStation(const std::string& ssid, const std::string& passphrase,
                   const std::string& adapter_id, const std::string& peer_address,
                   WifiStation& out);
} // namespace mgl
#endif
