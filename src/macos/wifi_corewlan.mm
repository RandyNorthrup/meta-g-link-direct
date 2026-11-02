// SPDX-License-Identifier: AGPL-3.0-only
// The Wi-Fi station on macOS. Listing adapters needs no permission, so it happens here; the join
// itself happens in Meta G Link Helper, which holds the Location permission that macOS asks for
// (see wifi_helper.mm). The passphrase reaches it over its private socket, not a command line.
#include "helper_client.hpp"
#include "helper_protocol.hpp"
#include "interface_address.hpp"
#include "mgl/identity.hpp"
#include "mgl/wifi_station.hpp"
#include "wifi_credentials.hpp"

#import <CoreWLAN/CoreWLAN.h>
#import <Foundation/Foundation.h>

#include <sys/socket.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace mgl {
namespace {
using namespace std::chrono_literals;

// How long DHCP may take after the join, and the helper to leave the network and exit.
constexpr auto AddressWait = 15s;
constexpr auto AddressPollInterval = 100ms;
constexpr auto LeaveWait = 8s;

std::vector<std::string> interfaceNames() {
    @autoreleasepool {
        std::vector<std::string> names;
        for (NSString* name in CWWiFiClient.sharedWiFiClient.interfaceNames) {
            names.emplace_back(name.UTF8String);
        }
        std::ranges::sort(names);
        return names;
    }
}

// Holds the helper's socket: closing it is what makes the helper leave the network.
class StationImpl final : public WifiStationConnection {
  public:
    explicit StationImpl(helper::Descriptor peer) : peer_(std::move(peer)) {}
    ~StationImpl() override {
        (void)shutdown(peer_.get(), SHUT_WR);
        helper::awaitEnd(peer_.get(), LeaveWait);
    }
    StationImpl(const StationImpl&) = delete;
    StationImpl& operator=(const StationImpl&) = delete;
    StationImpl(StationImpl&&) = delete;
    StationImpl& operator=(StationImpl&&) = delete;

  private:
    helper::Descriptor peer_;
};

bool fitsOnOneLine(const std::string& value) { return value.find('\n') == std::string::npos; }
} // namespace

std::vector<WifiAdapter> listWifiAdapters() {
    std::vector<WifiAdapter> adapters;
    for (const auto& name : interfaceNames()) {
        adapters.push_back({.id = name, .description = "Wi-Fi (" + name + ")"});
    }
    return adapters;
}

std::string wifiNetworkName(const std::string& fresh) {
    const auto directory = identity::storageDirectory();
    if (!directory) {
        return fresh;
    }
    const std::string path = *directory + "/wifi-network-name";
    std::string kept;
    if (std::ifstream stored(path); stored >> kept && kept.starts_with(GlassesAccessPointPrefix) &&
                                    kept.size() == fresh.size()) {
        return kept;
    }
    // A failed write only means the next session picks a name again.
    std::ofstream(path, std::ios::trunc) << fresh;
    return fresh;
}

std::unique_ptr<WifiStationConnection> connectWifiStation(const std::string& ssid,
                                                          const std::string& passphrase,
                                                          const std::string& adapter_id,
                                                          const std::string& peer_address,
                                                          WifiStation& out) {
    constexpr std::size_t MaxSsidLength = 32;
    if (ssid.empty() || ssid.size() > MaxSsidLength || !fitsOnOneLine(ssid) ||
        !fitsOnOneLine(passphrase)) {
        out.status = WifiStationStatus::ProfileFailed;
        return nullptr;
    }
    if (!validIpv4(peer_address)) {
        out.status = WifiStationStatus::BadPeer;
        return nullptr;
    }
    const auto names = interfaceNames();
    if (names.empty()) {
        out.status = WifiStationStatus::NoAdapter;
        return nullptr;
    }
    const std::string interface_name = adapter_id.empty() ? names.front() : adapter_id;
    if (std::ranges::find(names, interface_name) == names.end()) {
        out.status = WifiStationStatus::AdapterMissing;
        return nullptr;
    }

    const auto requested = std::chrono::steady_clock::now();
    const auto since_request = [&] {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - requested);
    };
    int error = 0;
    helper::Descriptor peer = helper::startHelper(helper::WifiRole, error);
    if (!peer.valid()) {
        out.status = WifiStationStatus::ConnectFailed;
        out.detail = static_cast<std::uint32_t>(error);
        out.detail_text = "Meta G Link Helper did not start";
        return nullptr;
    }
    const std::string request = interface_name + "\n" + ssid + "\n" + passphrase + "\n";
    const auto answer =
        helper::sendText(peer.get(), request) ? helper::readAnswer(peer.get()) : std::nullopt;
    if (!answer) {
        out.status = WifiStationStatus::ConnectFailed;
        out.detail = static_cast<std::uint32_t>(ETIMEDOUT);
        out.detail_text = "Meta G Link Helper did not answer";
        return nullptr;
    }
    if (answer->status != helper::StatusOk) {
        out.detail = static_cast<std::uint32_t>(answer->detail);
        if (answer->status == helper::StatusNoPermission) {
            out.status = WifiStationStatus::NoPermission;
            out.detail_text = "allow Location for Meta G Link Helper in System Settings, Privacy & "
                              "Security, Location Services";
        } else if (answer->status == helper::StatusNotVisible) {
            out.status = WifiStationStatus::ConnectFailed;
            out.detail_text = "the network was not visible";
        } else {
            out.status = WifiStationStatus::ConnectFailed;
        }
        return nullptr;
    }
    out.associated_after = since_request();

    // Wait for DHCP from the glasses.
    const auto deadline = std::chrono::steady_clock::now() + AddressWait;
    bool addressed = false;
    while (!addressed && std::chrono::steady_clock::now() < deadline) {
        addressed = interfaceOnSubnetOf(interface_name, peer_address);
        if (!addressed) {
            std::this_thread::sleep_for(AddressPollInterval);
        }
    }
    out.addressed_after = since_request();
    auto station = std::make_unique<StationImpl>(std::move(peer));
    if (!addressed) {
        out.status = WifiStationStatus::NoAddress;
        return nullptr; // the handle's end leaves the network again
    }
    out.status = WifiStationStatus::Ok;
    return station;
}
} // namespace mgl
