// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/capabilities.hpp"

#include <windows.h> // IWYU pragma: keep — Windows API headers require the umbrella prerequisites.

#include <bluetoothapis.h>
#include <errhandlingapi.h>
#include <handleapi.h>
#include <minwindef.h>
#include <winerror.h>
#include <winnt.h>
#include <wlanapi.h>

#include <memory>

namespace mgl {
namespace {
struct WlanCloser {
    void operator()(void* handle) const noexcept { WlanCloseHandle(handle, nullptr); }
};
} // namespace

WirelessCapabilities inspectWirelessCapabilities() noexcept {
    WirelessCapabilities result{};
    constexpr DWORD WlanClientVersion = 2;
    DWORD negotiated_version = 0;
    HANDLE raw_wlan = nullptr;
    result.wifi_error = WlanOpenHandle(WlanClientVersion, nullptr, &negotiated_version, &raw_wlan);
    if (result.wifi_error == ERROR_SUCCESS) {
        const std::unique_ptr<void, WlanCloser> wlan(raw_wlan);
        WLAN_INTERFACE_INFO_LIST* raw_list = nullptr;
        result.wifi_error = WlanEnumInterfaces(wlan.get(), nullptr, &raw_list);
        if (result.wifi_error == ERROR_SUCCESS) {
            const std::unique_ptr<WLAN_INTERFACE_INFO_LIST, decltype(&WlanFreeMemory)> list(
                raw_list, &WlanFreeMemory);
            result.wifi_interfaces = list->dwNumberOfItems;
        }
    }

    BLUETOOTH_FIND_RADIO_PARAMS parameters{};
    parameters.dwSize = sizeof(parameters);
    HANDLE raw_radio = nullptr;
    auto* const raw_find = BluetoothFindFirstRadio(&parameters, &raw_radio);
    if (raw_find != nullptr) {
        const std::unique_ptr<void, decltype(&BluetoothFindRadioClose)> find(
            raw_find, &BluetoothFindRadioClose);
        const std::unique_ptr<void, decltype(&CloseHandle)> radio(raw_radio, &CloseHandle);
        result.bluetooth_radio = true;
    } else {
        const auto error = GetLastError();
        result.bluetooth_error = error == ERROR_NO_MORE_ITEMS ? ERROR_SUCCESS : error;
    }
    return result;
}
} // namespace mgl
