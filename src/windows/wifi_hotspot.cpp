// SPDX-License-Identifier: AGPL-3.0-only
// A 2.4 GHz SoftAP via the Windows Mobile Hotspot (NetworkOperatorTethering) API, an alternative
// group owner to the WiFiDirect legacy AP. The glasses' Wi-Fi firmware is stable only on 2.4 GHz,
// which this API can pin; the WiFiDirect publisher cannot. The glasses join it as a WPA2 client.
#include "apartment.hpp"
#include "mgl/wifi_group.hpp"

#include <windows.h> // IWYU pragma: keep — SDK umbrella prerequisites.

#include <combaseapi.h>

#include <winrt/Windows.Foundation.h>                  // IWYU pragma: keep
#include <winrt/Windows.Networking.Connectivity.h>     // IWYU pragma: keep
#include <winrt/Windows.Networking.NetworkOperators.h> // IWYU pragma: keep
#include <winrt/base.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace mgl {
namespace {
using winrt::Windows::Networking::Connectivity::ConnectionProfile;  // NOLINT(misc-include-cleaner)
using winrt::Windows::Networking::Connectivity::NetworkInformation; // NOLINT(misc-include-cleaner)
using winrt::Windows::Networking::NetworkOperators::
    NetworkOperatorTetheringManager; // NOLINT(misc-include-cleaner)
using winrt::Windows::Networking::NetworkOperators::
    TetheringOperationStatus; // NOLINT(misc-include-cleaner)
using winrt::Windows::Networking::NetworkOperators::
    TetheringWiFiBand; // NOLINT(misc-include-cleaner)

std::string randomSuffixHotspot(std::size_t length) {
    static constexpr std::string_view Alphabet =
        "abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    GUID guid{};
    (void)CoCreateGuid(&guid);
    const auto* raw = reinterpret_cast<const std::uint8_t*>(
        &guid); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    std::string out;
    const auto take = std::min<std::size_t>(length, sizeof(guid));
    for (std::size_t index = 0; index < take; ++index) {
        out.push_back(Alphabet[raw[index] % Alphabet.size()]);
    }
    return out;
}

template <typename Operation> auto await(const Operation& operation) {
    return operation.get(); // blocking; the probe drives this on its own thread
}

class HotspotOwnerImpl final : public WifiGroupOwner {
  public:
    HotspotOwnerImpl(Apartment apartment, NetworkOperatorTetheringManager manager)
        : apartment_(std::move(apartment)), manager_(std::move(manager)) {}
    ~HotspotOwnerImpl() override {
        try {
            manager_.StopTetheringAsync().get();
        } catch (...) { // NOLINT(bugprone-empty-catch) — teardown must not throw.
        }
    }
    HotspotOwnerImpl(const HotspotOwnerImpl&) = delete;
    HotspotOwnerImpl& operator=(const HotspotOwnerImpl&) = delete;
    HotspotOwnerImpl(HotspotOwnerImpl&&) = delete;
    HotspotOwnerImpl& operator=(HotspotOwnerImpl&&) = delete;

  private:
    Apartment apartment_;
    NetworkOperatorTetheringManager manager_;
};
} // namespace

std::unique_ptr<WifiGroupOwner> startHotspotGroup(WifiGroup& out);

std::unique_ptr<WifiGroupOwner> startHotspotGroup(WifiGroup& out) {
    Apartment apartment;
    try {
        const auto profile = NetworkInformation::GetInternetConnectionProfile();
        if (!profile) {
            out.status = WifiGroupStatus::NoAdapterSupport;
            return nullptr;
        }
        auto manager = NetworkOperatorTetheringManager::CreateFromConnectionProfile(profile);
        auto config = manager.GetCurrentAccessPointConfiguration();
        const auto ssid = "DIRECT-mgl-" + randomSuffixHotspot(6);
        const auto passphrase = randomSuffixHotspot(16);
        config.Ssid(winrt::to_hstring(ssid));
        config.Passphrase(winrt::to_hstring(passphrase));
        try {
            config.Band(TetheringWiFiBand::TwoPointFourGigahertz);
        } catch (...) { // NOLINT(bugprone-empty-catch) — band fixed by the manager if unsettable.
        }
        await(manager.ConfigureAccessPointAsync(config));
        const auto result = await(manager.StartTetheringAsync());
        if (result.Status() != TetheringOperationStatus::Success) {
            out.status = WifiGroupStatus::StartFailed;
            out.detail = static_cast<std::uint32_t>(result.Status());
            return nullptr;
        }
        out.status = WifiGroupStatus::Ok;
        out.ssid = ssid;
        out.passphrase = passphrase;
        return std::make_unique<HotspotOwnerImpl>(std::move(apartment), std::move(manager));
    } catch (const winrt::hresult_error& failure) {
        out.status = WifiGroupStatus::StartFailed;
        out.detail = static_cast<std::uint32_t>(failure.code().value);
        return nullptr;
    }
}
} // namespace mgl
