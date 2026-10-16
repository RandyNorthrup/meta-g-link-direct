// SPDX-License-Identifier: AGPL-3.0-only
// Meta service UUID: pinned Visor BluetoothRfcommTransport.kt; see THIRD_PARTY_NOTICES.md.
#include "apartment.hpp"
#include "mgl/discovery.hpp"

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

#include <chrono>
#include <cstdint>
#include <memory>

namespace mgl {
namespace {
// SDK public projections include generated impl types; LLVM 22 cannot map these
// declarations back to the included public headers. Same boundary as discovery.cpp.
using winrt::Windows::Devices::Bluetooth::Rfcomm::
    IRfcommDeviceServiceStatics; // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Bluetooth::Rfcomm::
    IRfcommServiceIdStatics; // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Bluetooth::Rfcomm::
    RfcommDeviceService;                                           // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Bluetooth::Rfcomm::RfcommServiceId; // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Enumeration::DeviceInformation;     // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Enumeration::
    IDeviceInformationStatics;                                    // NOLINT(misc-include-cleaner)
using winrt::Windows::Foundation::AsyncStatus;                    // NOLINT(misc-include-cleaner)
using winrt::Windows::Networking::Sockets::SocketProtectionLevel; // NOLINT(misc-include-cleaner)
using winrt::Windows::Networking::Sockets::StreamSocket;          // NOLINT(misc-include-cleaner)
constexpr winrt::guid MetaRfcommService{
    0x8B0D2687, 0x42A4, 0x44CB, {0x94, 0x36, 0xFB, 0xA3, 0xB9, 0xB9, 0x6D, 0xE2}};
constexpr std::uint32_t NullInterfaceHresult = 0x80004003;
constexpr std::uint32_t AccessDeniedHresult = 0x80070005;
constexpr std::uint32_t TimeoutHresult = 0x800705B4;

void inquireClassic(ClassicDiscovery& result) {
    BLUETOOTH_DEVICE_SEARCH_PARAMS parameters{};
    parameters.dwSize = sizeof(parameters);
    parameters.fReturnAuthenticated = TRUE;
    parameters.fReturnRemembered = TRUE;
    parameters.fReturnUnknown = TRUE;
    parameters.fReturnConnected = TRUE;
    parameters.fIssueInquiry = TRUE;
    constexpr UCHAR InquiryUnits = 4; // Windows defines each unit as 1.28 seconds.
    parameters.cTimeoutMultiplier = InquiryUnits;
    BLUETOOTH_DEVICE_INFO device{};
    device.dwSize = sizeof(device);
    auto* const raw_find = BluetoothFindFirstDevice(&parameters, &device);
    if (raw_find == nullptr) {
        const auto error = GetLastError();
        result.classic_error = error == ERROR_NO_MORE_ITEMS ? ERROR_SUCCESS : error;
        result.inquiry_completed = result.classic_error == ERROR_SUCCESS;
        return;
    }
    const std::unique_ptr<void, decltype(&BluetoothFindDeviceClose)> find(
        raw_find, &BluetoothFindDeviceClose);
    constexpr std::uint32_t MaximumDeviceReports = 256;
    for (;;) {
        ++result.classic_devices;
        if (device.fAuthenticated != FALSE) {
            ++result.classic_paired_devices;
        }
        if (result.classic_devices == MaximumDeviceReports) {
            result.classic_error = ERROR_MORE_DATA;
            return;
        }
        if (BluetoothFindNextDevice(find.get(), &device) == FALSE) {
            break;
        }
    }
    const auto error = GetLastError();
    result.classic_error = error == ERROR_NO_MORE_ITEMS ? ERROR_SUCCESS : error;
    result.inquiry_completed = result.classic_error == ERROR_SUCCESS;
}

template <typename Operation>
bool awaitOperation(const Operation& operation, ClassicDiscovery& result) {
    constexpr auto OperationTimeout = std::chrono::seconds(5);
    if (operation.wait_for(OperationTimeout) == AsyncStatus::Started) {
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
} // namespace

ClassicDiscovery inspectClassicRfcomm(bool connect) {
    ClassicDiscovery result{};
    if (!connect) {
        inquireClassic(result);
    }
    try {
        const Apartment apartment;
        inspectServices(connect, result);
    } catch (...) {
        result.rfcomm_hresult = static_cast<std::uint32_t>(winrt::to_hresult().value);
    }
    return result;
}
} // namespace mgl
