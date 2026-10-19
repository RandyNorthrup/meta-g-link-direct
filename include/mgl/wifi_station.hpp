// SPDX-License-Identifier: AGPL-3.0-only
// Connects a Windows Wi-Fi adapter, as an ordinary station, to the access point the glasses host
// (see stream::buildCreateSoftApNetwork). Joining an AP is the robust direction of Wi-Fi: unlike a
// SoftAP, a station connect is well supported on every adapter. The handle holds the connection and
// restores the adapter's previous network when it is released.
#ifndef MGL_WIFI_STATION_HPP
#define MGL_WIFI_STATION_HPP

#include <cstdint>
#include <memory>
#include <string>

namespace mgl {
enum class WifiStationStatus : std::uint8_t {
    Ok,
    NoAdapter,     // no Wi-Fi adapter is present
    ProfileFailed, // the temporary WPA2 profile could not be set
    ConnectFailed, // the adapter did not associate in time
    NoAddress      // associated but DHCP gave no usable address in time
};

struct WifiStation {
    WifiStationStatus status = WifiStationStatus::ConnectFailed;
    std::uint32_t detail = 0; // Win32/WLAN error when not Ok
};

// Keeps the station connected to the glasses' AP. Releasing it deletes the temporary profile,
// which disconnects the adapter; Windows then returns it to its usual networks. A failed connect
// deletes the profile too.
class WifiStationConnection {
  public:
    WifiStationConnection() = default;
    virtual ~WifiStationConnection() = default;
    WifiStationConnection(const WifiStationConnection&) = delete;
    WifiStationConnection& operator=(const WifiStationConnection&) = delete;
    WifiStationConnection(WifiStationConnection&&) = delete;
    WifiStationConnection& operator=(WifiStationConnection&&) = delete;
};

// Connects the first Wi-Fi adapter to the given WPA2-PSK network and waits for an address. Returns
// null and fills out on failure; otherwise out.status is Ok and the connection handle is returned.
[[nodiscard]] std::unique_ptr<WifiStationConnection>
connectWifiStation(const std::string& ssid, const std::string& passphrase, WifiStation& out);
} // namespace mgl
#endif
