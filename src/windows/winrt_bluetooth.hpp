// SPDX-License-Identifier: AGPL-3.0-only
// C++/WinRT names shared by the Classic diagnostics and the RFCOMM transport. LLVM cannot map
// these SDK projection declarations back to the included public headers, so each is declared once
// here with that include-cleaner exception and used by name elsewhere.
#ifndef MGL_WINDOWS_WINRT_BLUETOOTH_HPP
#define MGL_WINDOWS_WINRT_BLUETOOTH_HPP

#include "meta_service.hpp"

#include <winrt/Windows.Devices.Bluetooth.Rfcomm.h> // IWYU pragma: keep
#include <winrt/Windows.Devices.Enumeration.h>      // IWYU pragma: keep
#include <winrt/Windows.Foundation.h>               // IWYU pragma: keep
#include <winrt/base.h>

namespace mgl::winrt_bluetooth {
using winrt::Windows::Devices::Bluetooth::Rfcomm::
    IRfcommDeviceServiceStatics; // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Bluetooth::Rfcomm::
    IRfcommServiceIdStatics; // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Bluetooth::Rfcomm::
    RfcommDeviceService;                                           // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Bluetooth::Rfcomm::RfcommServiceId; // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Enumeration::DeviceInformation;     // NOLINT(misc-include-cleaner)
using winrt::Windows::Devices::Enumeration::
    IDeviceInformationStatics;                 // NOLINT(misc-include-cleaner)
using winrt::Windows::Foundation::AsyncStatus; // NOLINT(misc-include-cleaner)

// The Meta RFCOMM service class in the form the WinRT APIs take.
inline constexpr winrt::guid MetaRfcommService{MetaRfcommServiceClass};
} // namespace mgl::winrt_bluetooth
#endif
