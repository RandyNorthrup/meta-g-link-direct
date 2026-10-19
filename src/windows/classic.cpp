// SPDX-License-Identifier: AGPL-3.0-only
// Meta service UUID: pinned Visor BluetoothRfcommTransport.kt; see THIRD_PARTY_NOTICES.md.
#include "apartment.hpp"
#include "mgl/discovery.hpp"
#include "winrt_bluetooth.hpp"

#include <windows.h> // IWYU pragma: keep — SDK umbrella prerequisites.

#include <bluetoothapis.h>
#include <errhandlingapi.h>
#include <minwindef.h>
#include <winerror.h>

#include <winrt/Windows.Devices.Bluetooth.Rfcomm.h> // IWYU pragma: keep
#include <winrt/Windows.Devices.Bluetooth.h>        // IWYU pragma: keep
#include <winrt/Windows.Devices.Enumeration.h>      // IWYU pragma: keep
#include <winrt/Windows.Foundation.Collections.h>   // IWYU pragma: keep
#include <winrt/Windows.Foundation.h>               // IWYU pragma: keep
#include <winrt/Windows.Networking.Sockets.h>       // IWYU pragma: keep
#include <winrt/Windows.Networking.h>               // IWYU pragma: keep
#include <winrt/base.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace mgl {
namespace {
// SDK public projections include generated impl types; LLVM 22 cannot map these
// declarations back to the included public headers. Same boundary as discovery.cpp.
using winrt::Windows::Devices::Bluetooth::BluetoothCacheMode;      // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Bluetooth::BluetoothDevice;         // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Bluetooth::BluetoothError;          // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Bluetooth::IBluetoothDeviceStatics; // NOLINT(misc-include-cleaner)
using winrt::Windows::Networking::Sockets::SocketProtectionLevel;  // NOLINT(misc-include-cleaner)
using winrt::Windows::Networking::Sockets::StreamSocket;           // NOLINT(misc-include-cleaner)
using winrt_bluetooth::AsyncStatus;                                // NOLINT(misc-include-cleaner)
using winrt_bluetooth::DeviceInformation;                          // NOLINT(misc-include-cleaner)
using winrt_bluetooth::IDeviceInformationStatics;                  // NOLINT(misc-include-cleaner)
using winrt_bluetooth::IRfcommDeviceServiceStatics;                // NOLINT(misc-include-cleaner)
using winrt_bluetooth::IRfcommServiceIdStatics;                    // NOLINT(misc-include-cleaner)
using winrt_bluetooth::MetaRfcommService;
using winrt_bluetooth::RfcommDeviceService; // NOLINT(misc-include-cleaner)
using winrt_bluetooth::RfcommServiceId;     // NOLINT(misc-include-cleaner)
constexpr std::uint32_t NullInterfaceHresult = 0x80004003;
constexpr std::uint32_t AccessDeniedHresult = 0x80070005;
constexpr std::uint32_t TimeoutHresult = 0x800705B4;

std::uint64_t privateAddress(const BLUETOOTH_DEVICE_INFO& device) noexcept {
    // Windows documents ullLong on its C ABI address union; no variant tag exists.
    return device.Address.ullLong; // NOLINT(cppcoreguidelines-pro-type-union-access)
}

struct SelectionCloser {
    ClassicDiscovery* result;
    void operator()(BLUETOOTH_SELECT_DEVICE_PARAMS* selection) const noexcept {
        if (BluetoothSelectDevicesFree(selection) == FALSE) {
            result->pairing_error = ERROR_INVALID_DATA;
        }
    }
};

std::vector<std::uint64_t> pairClassicDevice(ClassicDiscovery& result) {
    BLUETOOTH_SELECT_DEVICE_PARAMS selection{};
    selection.dwSize = sizeof(selection);
    auto instruction = std::to_array(
        L"Select only your Ray-Ban Meta glasses. Cancel if absent. No Meta ownership changes.");
    selection.pszInfo = instruction.data();
    selection.fForceAuthentication = TRUE;
    selection.fShowAuthenticated = TRUE;
    selection.fShowRemembered = TRUE;
    selection.fShowUnknown = TRUE;
    selection.fAddNewDeviceWizard = TRUE;
    selection.fSkipServicesPage = TRUE;
    selection.cNumDevices = 1;
    result.pairing_dialog_requested = true;
    if (BluetoothSelectDevices(&selection) == FALSE) {
        result.pairing_error = GetLastError();
        return {};
    }
    const std::unique_ptr<BLUETOOTH_SELECT_DEVICE_PARAMS, SelectionCloser> cleanup(
        &selection, SelectionCloser{&result});
    if (selection.cNumDevices != 1 || selection.pDevices == nullptr) {
        result.pairing_error = ERROR_INVALID_DATA;
        return {};
    }
    auto device = selection.pDevices[0];
    result.pairing_error = BluetoothGetDeviceInfo(nullptr, &device);
    result.windows_bonded = result.pairing_error == ERROR_SUCCESS && device.fAuthenticated != FALSE;
    return result.windows_bonded ? std::vector<std::uint64_t>{privateAddress(device)}
                                 : std::vector<std::uint64_t>{};
}

std::vector<std::uint64_t> inquireClassic(ClassicDiscovery& result, bool collect_addresses) {
    std::vector<std::uint64_t> addresses;
    BLUETOOTH_DEVICE_SEARCH_PARAMS parameters{};
    parameters.dwSize = sizeof(parameters);
    parameters.fReturnAuthenticated = TRUE;
    parameters.fReturnRemembered = TRUE;
    parameters.fReturnUnknown = TRUE;
    parameters.fReturnConnected = TRUE;
    parameters.fIssueInquiry = TRUE;
    constexpr UCHAR InquiryUnits = 4; // Windows defines each unit as 1.28 seconds.
    constexpr UCHAR LiveInquiryUnits = 16;
    parameters.cTimeoutMultiplier = collect_addresses ? LiveInquiryUnits : InquiryUnits;
    BLUETOOTH_DEVICE_INFO device{};
    device.dwSize = sizeof(device);
    auto* const raw_find = BluetoothFindFirstDevice(&parameters, &device);
    if (raw_find == nullptr) {
        const auto error = GetLastError();
        result.classic_error = error == ERROR_NO_MORE_ITEMS ? ERROR_SUCCESS : error;
        result.inquiry_completed = result.classic_error == ERROR_SUCCESS;
        return addresses;
    }
    const std::unique_ptr<void, decltype(&BluetoothFindDeviceClose)> find(
        raw_find, &BluetoothFindDeviceClose);
    constexpr std::uint32_t MaximumDeviceReports = 256;
    for (;;) {
        ++result.classic_devices;
        if (device.fAuthenticated != FALSE) {
            ++result.classic_paired_devices;
        }
        constexpr std::size_t MaximumLivePeers = 8;
        const auto address = privateAddress(device);
        if (collect_addresses && std::ranges::find(addresses, address) == addresses.end()) {
            if (addresses.size() < MaximumLivePeers) {
                addresses.push_back(address);
            } else {
                result.uncached_limit_reached = true;
            }
        }
        if (result.classic_devices == MaximumDeviceReports) {
            result.classic_error = ERROR_MORE_DATA;
            return addresses;
        }
        if (BluetoothFindNextDevice(find.get(), &device) == FALSE) {
            break;
        }
    }
    const auto error = GetLastError();
    result.classic_error = error == ERROR_NO_MORE_ITEMS ? ERROR_SUCCESS : error;
    result.inquiry_completed = result.classic_error == ERROR_SUCCESS;
    return addresses;
}

template <typename Operation>
bool awaitOperation(const Operation& operation, ClassicDiscovery& result,
                    std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
    if (operation.wait_for(timeout) == AsyncStatus::Started) {
        result.rfcomm_timed_out = true;
        result.rfcomm_hresult = TimeoutHresult;
        operation.Cancel();
        return false;
    }
    // GetResults in the caller propagates Error/Canceled rather than fabricating completion.
    return true;
}

void connectService(const RfcommDeviceService& service, ClassicDiscovery& result) {
    const auto peer = service.Device();
    if (!peer || !peer.DeviceInformation().Pairing().IsPaired()) {
        result.rfcomm_hresult = AccessDeniedHresult;
        return;
    }
    const auto factory = winrt::get_activation_factory<StreamSocket>();
    if (!factory) {
        result.rfcomm_hresult = NullInterfaceHresult;
        return;
    }
    const auto socket = factory.ActivateInstance<StreamSocket>();
    if (!socket) {
        result.rfcomm_hresult = NullInterfaceHresult;
        return;
    }
    try {
        result.rfcomm_connection_attempted = true;
        const auto connection =
            socket.ConnectAsync(service.ConnectionHostName(), service.ConnectionServiceName(),
                                SocketProtectionLevel::BluetoothEncryptionWithAuthentication);
        if (awaitOperation(connection, result)) {
            connection.GetResults();
            result.rfcomm_connected = true;
        }
    } catch (...) {
        result.rfcomm_hresult = static_cast<std::uint32_t>(winrt::to_hresult().value);
    }
    socket.Close();
}

void inspectServices(bool connect, ClassicDiscovery& result) {
    const auto id_factory =
        winrt::get_activation_factory<RfcommServiceId, IRfcommServiceIdStatics>();
    const auto service_factory =
        winrt::get_activation_factory<RfcommDeviceService, IRfcommDeviceServiceStatics>();
    const auto information_factory =
        winrt::get_activation_factory<DeviceInformation, IDeviceInformationStatics>();
    if (!id_factory || !service_factory || !information_factory) {
        result.rfcomm_hresult = NullInterfaceHresult;
        return;
    }
    const auto service_id = id_factory.FromUuid(MetaRfcommService);
    if (!service_id) {
        result.rfcomm_hresult = NullInterfaceHresult;
        return;
    }
    const auto selector = service_factory.GetDeviceSelector(service_id);
    const auto lookup = information_factory.FindAllAsync(selector);
    if (!awaitOperation(lookup, result)) {
        return;
    }
    const auto services = lookup.GetResults();
    if (!services) {
        result.rfcomm_hresult = NullInterfaceHresult;
        return;
    }
    result.meta_rfcomm_services = services.Size();
    result.rfcomm_ambiguous = services.Size() > 1;
    if (!connect || services.Size() != 1) {
        return;
    }
    const auto opening = service_factory.FromIdAsync(services.GetAt(0).Id());
    if (!awaitOperation(opening, result)) {
        return;
    }
    const auto service = opening.GetResults();
    if (!service) {
        result.rfcomm_hresult = AccessDeniedHresult;
        return;
    }
    try {
        connectService(service, result);
    } catch (...) {
        result.rfcomm_hresult = static_cast<std::uint32_t>(winrt::to_hresult().value);
    }
    service.Close();
}

template <typename Operation>
bool awaitLiveOperation(const Operation& operation, ClassicDiscovery& result,
                        std::chrono::steady_clock::time_point deadline) {
    const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
        deadline - std::chrono::steady_clock::now());
    constexpr auto OperationTimeout = std::chrono::milliseconds(5000);
    return awaitOperation(
        operation, result,
        std::clamp(remaining, std::chrono::milliseconds::zero(), OperationTimeout));
}

void queryLivePeer(std::uint64_t address, const RfcommServiceId& service_id,
                   std::vector<BluetoothDevice>& owners, std::vector<RfcommDeviceService>& found,
                   ClassicDiscovery& result, std::chrono::steady_clock::time_point deadline) {
    ++result.uncached_peer_queries;
    const auto factory = winrt::get_activation_factory<BluetoothDevice, IBluetoothDeviceStatics>();
    if (!factory) {
        result.rfcomm_hresult = NullInterfaceHresult;
        ++result.uncached_query_failures;
        return;
    }
    const auto opening = factory.FromBluetoothAddressAsync(address);
    if (!awaitLiveOperation(opening, result, deadline)) {
        ++result.uncached_query_failures;
        return;
    }
    const auto peer = opening.GetResults();
    if (!peer) {
        ++result.uncached_query_failures;
        return;
    }
    owners.push_back(peer);
    ++result.uncached_peers_resolved;
    ++result.uncached_service_queries;
    const auto query = peer.GetRfcommServicesForIdAsync(service_id, BluetoothCacheMode::Uncached);
    if (!awaitLiveOperation(query, result, deadline)) {
        ++result.uncached_query_failures;
        return;
    }
    const auto response = query.GetResults();
    if (!response) {
        result.rfcomm_hresult = NullInterfaceHresult;
        ++result.uncached_query_failures;
        return;
    }
    if (response.Error() != BluetoothError::Success) {
        result.uncached_bluetooth_error = static_cast<std::uint32_t>(response.Error());
        ++result.uncached_query_failures;
        return;
    }
    for (const auto& service : response.Services()) {
        if (!service) {
            result.rfcomm_hresult = NullInterfaceHresult;
            ++result.uncached_query_failures;
        } else {
            found.push_back(service);
        }
    }
}

template <typename Resource>
void closeResources(const std::vector<Resource>& resources, ClassicDiscovery& result) noexcept {
    for (const auto& resource : resources) {
        try {
            resource.Close();
        } catch (...) {
            result.rfcomm_hresult = static_cast<std::uint32_t>(winrt::to_hresult().value);
        }
    }
}

void inspectLiveServices(bool connect, const std::vector<std::uint64_t>& addresses,
                         ClassicDiscovery& result) {
    result.uncached_requested = true;
    std::vector<BluetoothDevice> owners;
    std::vector<RfcommDeviceService> found;
    try {
        const auto factory =
            winrt::get_activation_factory<RfcommServiceId, IRfcommServiceIdStatics>();
        const auto service_id =
            factory ? factory.FromUuid(MetaRfcommService) : RfcommServiceId{nullptr};
        if (!service_id) {
            result.rfcomm_hresult = NullInterfaceHresult;
        } else {
            constexpr auto LiveQueryBudget = std::chrono::seconds(20);
            const auto deadline = std::chrono::steady_clock::now() + LiveQueryBudget;
            for (const auto address : addresses) {
                if (std::chrono::steady_clock::now() >= deadline) {
                    result.uncached_limit_reached = true;
                    break;
                }
                try {
                    queryLivePeer(address, service_id, owners, found, result, deadline);
                } catch (...) {
                    result.rfcomm_hresult = static_cast<std::uint32_t>(winrt::to_hresult().value);
                    ++result.uncached_query_failures;
                }
            }
            result.uncached_complete = (result.inquiry_completed || result.windows_bonded) &&
                                       !result.uncached_limit_reached &&
                                       result.uncached_query_failures == 0 &&
                                       result.uncached_service_queries == addresses.size();
            result.meta_rfcomm_services = static_cast<std::uint32_t>(found.size());
            result.rfcomm_ambiguous = found.size() > 1;
            if (connect && found.size() == 1 && result.rfcomm_hresult == 0) {
                connectService(found.front(), result);
            }
        }
    } catch (...) {
        result.rfcomm_hresult = static_cast<std::uint32_t>(winrt::to_hresult().value);
    }
    // Keep peer owners alive until all dependent services/sockets have been closed.
    closeResources(found, result);
    closeResources(owners, result);
}
} // namespace

ClassicDiscovery inspectClassicRfcomm(ClassicMode mode) {
    ClassicDiscovery result{};
    const bool pairing = mode == ClassicMode::PairAndConnect;
    const bool connect =
        pairing || mode == ClassicMode::RegisteredConnect || mode == ClassicMode::LiveConnect;
    const bool live =
        pairing || mode == ClassicMode::LiveDiscovery || mode == ClassicMode::LiveConnect;
    std::vector<std::uint64_t> addresses;
    if (pairing) {
        addresses = pairClassicDevice(result);
        if (!result.windows_bonded || result.pairing_error != 0) {
            return result;
        }
    } else if (!connect || live) {
        addresses = inquireClassic(result, live);
    }
    try {
        const Apartment apartment;
        if (live) {
            inspectLiveServices(connect, addresses, result);
        } else {
            inspectServices(connect, result);
        }
    } catch (...) {
        result.rfcomm_hresult = static_cast<std::uint32_t>(winrt::to_hresult().value);
    }
    return result;
}
} // namespace mgl
