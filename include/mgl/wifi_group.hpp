// SPDX-License-Identifier: AGPL-3.0-only
// A phone-style Wi-Fi Direct group: Windows acts as the group owner / legacy access point, and the
// glasses join it as an ordinary WPA2 client (Visor's JoinAccessPoint model). The group stays up
// for the lifetime of the handle.
#ifndef MGL_WIFI_GROUP_HPP
#define MGL_WIFI_GROUP_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace mgl {
enum class WifiGroupStatus : std::uint8_t {
    Ok,
    NoAdapterSupport, // the Wi-Fi adapter/driver will not host a legacy group owner
    StartFailed,      // the advertisement could not start
    Aborted           // the group stopped unexpectedly
};

struct WifiGroup {
    WifiGroupStatus status = WifiGroupStatus::StartFailed;
    std::uint32_t detail = 0; // HRESULT or publisher error when not Ok
    std::string ssid;         // the group name the glasses are told to join
    std::string passphrase;   // the WPA2 passphrase
};

// Starts a legacy Wi-Fi Direct group owner with a random SSID/passphrase and keeps it running. The
// returned handle owns the group; destroying it stops the advertisement.
class WifiGroupOwner {
  public:
    [[nodiscard]] static std::unique_ptr<WifiGroupOwner> start(WifiGroup& out);
    virtual ~WifiGroupOwner() = default;
    WifiGroupOwner(const WifiGroupOwner&) = delete;
    WifiGroupOwner& operator=(const WifiGroupOwner&) = delete;
    WifiGroupOwner(WifiGroupOwner&&) = delete;
    WifiGroupOwner& operator=(WifiGroupOwner&&) = delete;

  protected:
    WifiGroupOwner() = default;
};

// A 2.4 GHz SoftAP via the Mobile Hotspot API (band-pinned, which the glasses need). Used first by
// WifiGroupOwner::start, with the WiFiDirect legacy AP as a fallback.
[[nodiscard]] std::unique_ptr<WifiGroupOwner> startHotspotGroup(WifiGroup& out);
} // namespace mgl
#endif
