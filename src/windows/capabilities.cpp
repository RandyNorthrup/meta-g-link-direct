// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/capabilities.hpp"

#include <windows.h> // IWYU pragma: keep — Windows API headers require the umbrella prerequisites.

#include <bluetoothapis.h>
#include <bthdef.h>
#include <errhandlingapi.h>
#include <handleapi.h>
#include <ioapiset.h>
#include <minwindef.h>
#include <winerror.h>
#include <winioctl.h>
#include <winnt.h>
#include <wlanapi.h>

#include <bthioctl.h> // IWYU pragma: keep — requires CTL_CODE from winioctl.h above.

#include <memory>
#include <span>

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
            // WlanEnumInterfaces allocates the SDK's variable-length trailing array.
            for (const auto& interface :
                 std::span(&list->InterfaceInfo[0], list->dwNumberOfItems)) {
                if (interface.isState == wlan_interface_state_connected) {
                    ++result.wifi_connected_interfaces;
                }
            }
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
        BTH_LOCAL_RADIO_INFO information{};
        DWORD returned = 0;
        if (DeviceIoControl(radio.get(), IOCTL_BTH_GET_LOCAL_INFO, nullptr, 0, &information,
                            sizeof(information), &returned, nullptr) == FALSE) {
            result.bluetooth_features_error = GetLastError();
        } else if (returned != sizeof(information)) {
            result.bluetooth_features_error = ERROR_INVALID_DATA;
        } else {
            result.bluetooth_features_valid = true;
            result.bluetooth_le_supported =
                LMP_LE_SUPPORTED(information.radioInfo.lmpSupportedFeatures) != 0;
            result.bluetooth_classic_supported =
                LMP_BR_EDR_NOT_SUPPORTED(information.radioInfo.lmpSupportedFeatures) == 0;
            result.bluetooth_hci_version = information.hciVersion;
            result.bluetooth_lmp_version = information.radioInfo.lmpVersion;
        }
    } else {
        const auto error = GetLastError();
        result.bluetooth_error = error == ERROR_NO_MORE_ITEMS ? ERROR_SUCCESS : error;
    }
    return result;
}
} // namespace mgl
