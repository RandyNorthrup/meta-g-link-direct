// SPDX-License-Identifier: AGPL-3.0-only
// Discovery markers follow Visor NearbyGlasses.kt at the revision in THIRD_PARTY_NOTICES.md.
#include "mgl/discovery.hpp"
#include "apartment.hpp"

#include <winrt/Windows.Devices.Bluetooth.Advertisement.h> // IWYU pragma: keep
#include <winrt/Windows.Devices.Bluetooth.h>               // IWYU pragma: keep
#include <winrt/Windows.Foundation.Collections.h>          // IWYU pragma: keep
#include <winrt/Windows.Foundation.h>                      // IWYU pragma: keep
#include <winrt/base.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>

namespace mgl {
namespace {
// The SDK public projection supplies these through generated impl/*.h files that
// clang-tidy 22 cannot map back to the public header. Keep the documented public API.
using winrt::Windows::Devices::Bluetooth::Advertisement::
    BluetoothLEAdvertisement; // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Bluetooth::Advertisement::
    BluetoothLEAdvertisementWatcher; // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Bluetooth::Advertisement::
    BluetoothLEScanningMode; // NOLINT(misc-include-cleaner)
constexpr winrt::guid MetaService{
    0x0000FD5F, 0x0000, 0x1000, {0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB}};

struct ScanState {
    std::mutex mutex;
    std::condition_variable changed;
    BluetoothDiscovery result{};
    bool stopped = false;
};

void incrementReport(std::uint32_t& count) noexcept {
    if (count < std::numeric_limits<std::uint32_t>::max()) {
        ++count;
    }
}

bool hasMetaMarker(const BluetoothLEAdvertisement& advertisement) {
    constexpr std::uint16_t MetaCompany = 0x01AB;
    const auto manufacturers = advertisement.ManufacturerData();
    const auto services = advertisement.ServiceUuids();
    return std::ranges::any_of(
               manufacturers, [](const auto& entry) { return entry.CompanyId() == MetaCompany; }) ||
           std::ranges::any_of(services,
                               [](const auto& service) { return service == MetaService; });
}

void scanAdvertisements(const std::shared_ptr<ScanState>& state) {
    constexpr std::uint32_t NullActivationHresult = 0x80004003;
    const auto factory = winrt::get_activation_factory<BluetoothLEAdvertisementWatcher>();
    if (!factory) {
        state->result.hresult = NullActivationHresult;
        return;
    }
    const auto watcher = factory.ActivateInstance<BluetoothLEAdvertisementWatcher>();
    if (!watcher) {
        state->result.hresult = NullActivationHresult;
        return;
    }
    watcher.ScanningMode(BluetoothLEScanningMode::Passive);
    const auto received =
        watcher.Received(winrt::auto_revoke, [state](const auto&, const auto& args) {
            try {
                const bool meta = hasMetaMarker(args.Advertisement());
                const std::scoped_lock lock(state->mutex);
                incrementReport(state->result.advertisement_reports);
                if (meta) {
                    incrementReport(state->result.meta_advertisement_reports);
                }
            } catch (...) {
                const std::scoped_lock lock(state->mutex);
                state->result.hresult = static_cast<std::uint32_t>(winrt::to_hresult().value);
                state->changed.notify_all();
            }
        });
    const auto stopped =
        watcher.Stopped(winrt::auto_revoke, [state](const auto&, const auto& args) {
            const std::scoped_lock lock(state->mutex);
            state->stopped = true;
            state->result.bluetooth_error = static_cast<std::uint32_t>(args.Error());
            state->changed.notify_all();
        });
    watcher.Start();
    try {
        std::unique_lock lock(state->mutex);
        constexpr auto ScanDuration = std::chrono::seconds(10);
        const bool ended_early = state->changed.wait_for(
            lock, ScanDuration, [&state] { return state->stopped || state->result.hresult != 0; });
        state->result.scan_completed = !ended_early;
    } catch (...) {
        watcher.Stop();
        throw;
    }
    watcher.Stop();
    std::unique_lock lock(state->mutex);
    constexpr auto StopTimeout = std::chrono::seconds(2);
    if (!state->changed.wait_for(lock, StopTimeout, [&state] { return state->stopped; })) {
        constexpr std::uint32_t StopTimeoutHresult = 0x800705B4;
        state->result.hresult = StopTimeoutHresult;
    }
    // Event revokers unregister before the watcher is released. Callbacks retain shared state.
}
} // namespace

BluetoothDiscovery inspectBluetoothAdvertisements() {
    const auto state = std::make_shared<ScanState>();
    try {
        const Apartment apartment;
        scanAdvertisements(state);
    } catch (const winrt::hresult_error& error) {
        const std::scoped_lock lock(state->mutex);
        state->result.hresult = static_cast<std::uint32_t>(error.code().value);
    }
    const std::scoped_lock lock(state->mutex);
    state->result.scan_completed = state->result.scan_completed && state->result.hresult == 0 &&
                                   state->result.bluetooth_error == 0;
    return state->result;
}
} // namespace mgl
