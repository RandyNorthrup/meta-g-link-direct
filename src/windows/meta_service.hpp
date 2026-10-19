// SPDX-License-Identifier: AGPL-3.0-only
// The Meta glasses' RFCOMM service class, from the pinned Visor BluetoothRfcommTransport.kt (see
// THIRD_PARTY_NOTICES.md). The one definition for every Winsock and WinRT caller.
#ifndef MGL_WINDOWS_META_SERVICE_HPP
#define MGL_WINDOWS_META_SERVICE_HPP

#include <windows.h> // IWYU pragma: keep — SDK umbrella prerequisites.

#include <guiddef.h>

namespace mgl {
inline constexpr GUID MetaRfcommServiceClass{
    .Data1 = 0x8B0D2687,
    .Data2 = 0x42A4,
    .Data3 = 0x44CB,
    .Data4 = {0x94, 0x36, 0xFB, 0xA3, 0xB9, 0xB9, 0x6D, 0xE2}};
} // namespace mgl
#endif
