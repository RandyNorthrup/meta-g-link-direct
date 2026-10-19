// SPDX-License-Identifier: AGPL-3.0-only
// Synchronous RFCOMM transport. Service resolution follows Visor BluetoothRfcommTransport.kt at
// the revision in THIRD_PARTY_NOTICES.md; the socket I/O is Winsock AF_BTH via SduSocket.
#include "mgl/rfcomm_transport.hpp"

#include "apartment.hpp"
#include "mgl/transport.hpp"
#include "sdu_socket.hpp"

#include <windows.h> // IWYU pragma: keep — SDK umbrella prerequisites.

#include <winsock2.h>
#include <ws2bth.h>

#include <winrt/Windows.Devices.Bluetooth.Rfcomm.h> // IWYU pragma: keep
#include <winrt/Windows.Devices.Bluetooth.h>        // IWYU pragma: keep
#include <winrt/Windows.Devices.Enumeration.h>      // IWYU pragma: keep
#include <winrt/Windows.Foundation.Collections.h>   // IWYU pragma: keep
#include <winrt/Windows.Foundation.h>               // IWYU pragma: keep
#include <winrt/base.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

namespace mgl {
namespace {
using winrt::Windows::Devices::Bluetooth::BluetoothDevice; // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Bluetooth::Rfcomm::
    IRfcommDeviceServiceStatics; // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Bluetooth::Rfcomm::
    IRfcommServiceIdStatics; // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Bluetooth::Rfcomm::
    RfcommDeviceService;                                           // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Bluetooth::Rfcomm::RfcommServiceId; // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Enumeration::DeviceInformation;     // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Enumeration::
    IDeviceInformationStatics; // NOLINT(misc-include-cleaner)
using winrt::Windows::Foundation::AsyncStatus;

constexpr winrt::guid MetaRfcommService{
    0x8B0D2687, 0x42A4, 0x44CB, {0x94, 0x36, 0xFB, 0xA3, 0xB9, 0xB9, 0x6D, 0xE2}};

template <typename Operation>
bool awaitOperation(const Operation& operation, std::chrono::milliseconds timeout) {
    if (operation.wait_for(timeout) == AsyncStatus::Started) {
        operation.Cancel();
        return false;
    }
    return operation.Status() == AsyncStatus::Completed;
}

struct Resolved {
    std::uint64_t address = 0;
    std::uint32_t channel = 0;
};

std::optional<Resolved> resolveMetaService(RfcommConnect& status) {
    const auto id_factory =
        winrt::get_activation_factory<RfcommServiceId, IRfcommServiceIdStatics>();
    const auto service_factory =
        winrt::get_activation_factory<RfcommDeviceService, IRfcommDeviceServiceStatics>();
    const auto information_factory =
        winrt::get_activation_factory<DeviceInformation, IDeviceInformationStatics>();
    if (!id_factory || !service_factory || !information_factory) {
        status = RfcommConnect::ConnectFailed;
        return std::nullopt;
    }
    const auto service_id = id_factory.FromUuid(MetaRfcommService);
    const auto selector = service_factory.GetDeviceSelector(service_id);
    const auto lookup = information_factory.FindAllAsync(selector);
    if (!awaitOperation(lookup, std::chrono::seconds(5))) {
        status = RfcommConnect::ConnectFailed;
        return std::nullopt;
    }
    const auto services = lookup.GetResults();
    if (!services || services.Size() == 0) {
        status = RfcommConnect::NoMetaService;
        return std::nullopt;
    }
    if (services.Size() > 1) {
        status = RfcommConnect::AmbiguousService;
        return std::nullopt;
    }
    const auto opening = service_factory.FromIdAsync(services.GetAt(0).Id());
    if (!awaitOperation(opening, std::chrono::seconds(5))) {
        status = RfcommConnect::ConnectFailed;
        return std::nullopt;
    }
    const auto service = opening.GetResults();
    if (!service) {
        status = RfcommConnect::ConnectFailed;
        return std::nullopt;
    }
    const auto device = service.Device();
    if (!device || !device.DeviceInformation().Pairing().IsPaired()) {
        status = RfcommConnect::NoMetaService;
        return std::nullopt;
    }
    Resolved resolved;
    resolved.address = device.BluetoothAddress();
    const auto name = winrt::to_string(service.ConnectionServiceName());
    std::uint32_t channel = 0;
    for (const char digit : name) {
        if (digit < '0' || digit > '9') {
            break;
        }
        channel = (channel * 10U) + static_cast<std::uint32_t>(digit - '0');
    }
    resolved.channel = channel;
    service.Close();
    status = RfcommConnect::Ok;
    return resolved;
}

std::unique_ptr<Transport> connectSocket(const Resolved& resolved, RfcommOpen& result) {
    WinsockLibrary library;
    if (library.error() != 0) {
        result.status = RfcommConnect::NoRadio;
        result.detail = static_cast<std::uint32_t>(library.error());
        return nullptr;
    }
    const auto socket =
        WSASocketW(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM, nullptr, 0, WSA_FLAG_NO_HANDLE_INHERIT);
    if (socket == INVALID_SOCKET) {
        result.status = RfcommConnect::ConnectFailed;
        result.detail = static_cast<std::uint32_t>(WSAGetLastError());
        return nullptr;
    }
    SOCKADDR_BTH remote{};
    remote.addressFamily = AF_BTH;
    remote.btAddr = resolved.address;
    if (resolved.channel >= 1 && resolved.channel <= 30) {
        remote.port = resolved.channel;
    } else {
        remote.serviceClassId = GUID{.Data1 = 0x8B0D2687,
                                     .Data2 = 0x42A4,
                                     .Data3 = 0x44CB,
                                     .Data4 = {0x94, 0x36, 0xFB, 0xA3, 0xB9, 0xB9, 0x6D, 0xE2}};
    }
    const auto* const endpoint = reinterpret_cast<const sockaddr*>(
        &remote); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    if (connect(socket, endpoint, sizeof(remote)) == SOCKET_ERROR) {
        result.status = RfcommConnect::ConnectFailed;
        result.detail = static_cast<std::uint32_t>(WSAGetLastError());
        closesocket(socket);
        return nullptr;
    }
    result.status = RfcommConnect::Ok;
    return std::make_unique<SduSocket>(std::move(library), socket);
}
} // namespace

RfcommOpen openBondedMetaRfcomm() {
    RfcommOpen result;
    std::optional<Resolved> resolved;
    try {
        const Apartment apartment;
        resolved = resolveMetaService(result.status);
    } catch (const winrt::hresult_error& error) {
        result.status = RfcommConnect::ConnectFailed;
        result.detail = static_cast<std::uint32_t>(error.code().value);
        return result;
    }
    if (!resolved) {
        return result;
    }
    result.transport = connectSocket(*resolved, result);
    return result;
}
} // namespace mgl
