// SPDX-License-Identifier: AGPL-3.0-only
// Legacy Wi-Fi Direct group owner via the Windows WiFiDirect advertisement publisher. The glasses
// join this as a plain WPA2 access point, mirroring Visor's PhoneGroup/JoinAccessPoint model.
#include "mgl/wifi_group.hpp"
#include "apartment.hpp"

#include <windows.h> // IWYU pragma: keep — SDK umbrella prerequisites.

#include <combaseapi.h>

#include <winrt/Windows.Devices.WiFiDirect.h>   // IWYU pragma: keep
#include <winrt/Windows.Foundation.h>           // IWYU pragma: keep
#include <winrt/Windows.Security.Credentials.h> // IWYU pragma: keep
#include <winrt/base.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace mgl {
namespace {
using winrt::Windows::Devices::WiFiDirect::
    WiFiDirectAdvertisementPublisher; // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::WiFiDirect::
    WiFiDirectAdvertisementPublisherStatus; // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::WiFiDirect::
    WiFiDirectConnectionListener;                                // NOLINT(misc-include-cleaner)
using winrt::Windows::Security::Credentials::PasswordCredential; // NOLINT(misc-include-cleaner)

std::string narrow(const winrt::hstring& text) { return winrt::to_string(text); }

std::string randomSuffix(std::size_t length) {
    static constexpr std::string_view Alphabet =
        "abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    std::array<std::uint8_t, 64> bytes{};
    const auto take = std::min(length, bytes.size());
    // BCryptGenRandom via the crypto module would add a dependency cycle; the group name and
    // passphrase are not security-critical beyond being unpredictable, and the OS RNG is used
    // through the UUID helper below instead.
    GUID guid{};
    (void)CoCreateGuid(&guid);
    const auto* raw = reinterpret_cast<const std::uint8_t*>(
        &guid); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    std::string out;
    for (std::size_t index = 0; index < take; ++index) {
        out.push_back(Alphabet[raw[index % sizeof(guid)] % Alphabet.size()]);
    }
    return out;
}

class WifiGroupOwnerImpl final : public WifiGroupOwner {
  public:
    WifiGroupOwnerImpl(Apartment apartment, WiFiDirectAdvertisementPublisher publisher,
                       WiFiDirectConnectionListener listener)
        : apartment_(std::move(apartment)), publisher_(std::move(publisher)),
          listener_(std::move(listener)) {}
    ~WifiGroupOwnerImpl() override {
        try {
            publisher_.Stop();
        } catch (...) { // NOLINT(bugprone-empty-catch) — teardown must not throw.
        }
    }
    WifiGroupOwnerImpl(const WifiGroupOwnerImpl&) = delete;
    WifiGroupOwnerImpl& operator=(const WifiGroupOwnerImpl&) = delete;
    WifiGroupOwnerImpl(WifiGroupOwnerImpl&&) = delete;
    WifiGroupOwnerImpl& operator=(WifiGroupOwnerImpl&&) = delete;

  private:
    Apartment apartment_;
    WiFiDirectAdvertisementPublisher publisher_;
    WiFiDirectConnectionListener listener_;
};
} // namespace

std::unique_ptr<WifiGroupOwner> WifiGroupOwner::start(WifiGroup& out) {
    // Prefer the band-pinned 2.4 GHz hotspot; the glasses' Wi-Fi firmware needs it.
    if (auto hotspot = startHotspotGroup(out)) {
        return hotspot;
    }
    Apartment apartment;
    try {
        WiFiDirectAdvertisementPublisher publisher;
        WiFiDirectConnectionListener listener;
        auto advertisement = publisher.Advertisement();
        // Legacy AP mode makes the device a group owner on its own; the autonomous-GO and
        // discoverability flags conflict with it (E_ILLEGAL_METHOD_CALL), so they stay unset.
        const auto ssid = "DIRECT-mgl-" + randomSuffix(6);
        const auto passphrase = randomSuffix(20);
        auto legacy = advertisement.LegacySettings();
        legacy.IsEnabled(true);
        legacy.Ssid(winrt::to_hstring(ssid));
        PasswordCredential credential;
        credential.Password(winrt::to_hstring(passphrase));
        legacy.Passphrase(credential);

        std::mutex mutex;
        std::condition_variable changed;
        std::optional<WiFiDirectAdvertisementPublisherStatus> status;
        std::uint32_t error = 0;
        const auto token = publisher.StatusChanged([&](const auto&, const auto& args) {
            const std::scoped_lock lock(mutex);
            status = args.Status();
            error = static_cast<std::uint32_t>(args.Error());
            changed.notify_all();
        });
        publisher.Start();

        {
            std::unique_lock lock(mutex);
            constexpr auto StartTimeout = std::chrono::seconds(12);
            changed.wait_for(lock, StartTimeout, [&] { return status.has_value(); });
            if (!status) {
                publisher.StatusChanged(token);
                publisher.Stop();
                out.status = WifiGroupStatus::StartFailed;
                return nullptr;
            }
            if (*status != WiFiDirectAdvertisementPublisherStatus::Started) {
                publisher.StatusChanged(token);
                publisher.Stop();
                // Aborted with an error usually means the adapter/driver cannot host a group owner.
                out.status =
                    error != 0 ? WifiGroupStatus::NoAdapterSupport : WifiGroupStatus::StartFailed;
                out.detail = error;
                return nullptr;
            }
        }
        publisher.StatusChanged(token);

        out.status = WifiGroupStatus::Ok;
        out.ssid = ssid;
        out.passphrase = passphrase;
        return std::make_unique<WifiGroupOwnerImpl>(std::move(apartment), std::move(publisher),
                                                    std::move(listener));
    } catch (const winrt::hresult_error& failure) {
        out.status = WifiGroupStatus::StartFailed;
        out.detail = static_cast<std::uint32_t>(failure.code().value);
        return nullptr;
    }
}
} // namespace mgl
