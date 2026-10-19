// SPDX-License-Identifier: AGPL-3.0-only
// Native Wi-Fi station connect to the glasses' hosted access point. See mgl/wifi_station.hpp.
#include "mgl/wifi_station.hpp"

#include <windows.h> // IWYU pragma: keep — SDK umbrella prerequisites.

// The IP Helper address-table declarations exist only once Winsock's IP definitions are in.
#include <winsock2.h> // IWYU pragma: keep
#include <ws2ipdef.h> // IWYU pragma: keep

#include <combaseapi.h>
#include <guiddef.h>
#include <ifdef.h>
#include <iphlpapi.h> // IWYU pragma: keep — the documented entry header for netioapi.h.
#include <minwindef.h>
#include <netioapi.h>
#include <nldef.h>
#include <stringapiset.h>
#include <winerror.h>
#include <winnls.h>
#include <winnt.h>
#include <wlanapi.h>
#include <wlantypes.h>
#include <ws2def.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace mgl {
namespace {
using namespace std::chrono_literals;

// How long association plus DHCP may take before the connect counts as failed.
constexpr auto ConnectBudget = 25s;
constexpr auto ConnectPollInterval = 500ms;
// The Native Wi-Fi client API version for Windows Vista and later.
constexpr DWORD WlanClientVersion = 2;

std::wstring widen(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int needed =
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(needed), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), wide.data(),
                        needed);
    return wide;
}

std::string narrow(std::wstring_view text) {
    if (text.empty()) {
        return {};
    }
    const int needed = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                                           nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<std::size_t>(needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), utf8.data(), needed,
                        nullptr, nullptr);
    return utf8;
}

// The interface GUID as "{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}", the form WifiAdapter::id uses.
std::string guidText(const GUID& guid) {
    constexpr int GuidTextLength = 39; // braces, 32 digits, 4 dashes and the terminator
    std::array<wchar_t, GuidTextLength> text{};
    const int written = StringFromGUID2(guid, text.data(), GuidTextLength);
    return written > 0
               ? narrow(std::wstring_view(text.data(), static_cast<std::size_t>(written - 1)))
               : std::string{};
}

// A manual-connect WPA2-PSK profile for one network. The SSID and passphrase are our own random
// credentials drawn from an alphanumeric alphabet, so they need no XML escaping.
std::wstring profileXml(const std::string& ssid, const std::string& passphrase) {
    const std::wstring name = widen(ssid);
    return L"<?xml version=\"1.0\"?>\n"
           L"<WLANProfile xmlns=\"http://www.microsoft.com/networking/WLAN/profile/v1\">\n"
           L"  <name>" +
           name +
           L"</name>\n"
           L"  <SSIDConfig><SSID><name>" +
           name +
           L"</name></SSID></SSIDConfig>\n"
           L"  <connectionType>ESS</connectionType>\n"
           L"  <connectionMode>manual</connectionMode>\n"
           L"  <MSM><security>\n"
           L"    <authEncryption><authentication>WPA2PSK</authentication>"
           L"<encryption>AES</encryption><useOneX>false</useOneX></authEncryption>\n"
           L"    <sharedKey><keyType>passPhrase</keyType><protected>false</protected>"
           L"<keyMaterial>" +
           widen(passphrase) +
           L"</keyMaterial></sharedKey>\n"
           L"  </security></MSM>\n"
           L"</WLANProfile>\n";
}

// Owns a Native Wi-Fi client handle.
class WlanClient {
  public:
    explicit WlanClient(HANDLE handle) : handle_(handle) {}
    ~WlanClient() {
        if (handle_ != nullptr) {
            WlanCloseHandle(handle_, nullptr);
        }
    }
    WlanClient(const WlanClient&) = delete;
    WlanClient& operator=(const WlanClient&) = delete;
    WlanClient(WlanClient&&) = delete;
    WlanClient& operator=(WlanClient&&) = delete;

    [[nodiscard]] HANDLE get() const noexcept { return handle_; }
    // Hands the handle to a longer-lived owner.
    [[nodiscard]] HANDLE release() noexcept { return std::exchange(handle_, nullptr); }

  private:
    HANDLE handle_;
};

// Frees memory the Native Wi-Fi API allocated.
struct WlanFree {
    void operator()(void* memory) const noexcept { WlanFreeMemory(memory); }
};

// Frees an IP Helper table.
struct MibFree {
    void operator()(void* table) const noexcept { FreeMibTable(table); }
};

using InterfaceList = std::unique_ptr<WLAN_INTERFACE_INFO_LIST, WlanFree>;

// The WLAN interfaces Windows lists; null when the WLAN service cannot enumerate them.
InterfaceList enumerateInterfaces(HANDLE client) {
    WLAN_INTERFACE_INFO_LIST* raw = nullptr;
    if (WlanEnumInterfaces(client, nullptr, &raw) != ERROR_SUCCESS || raw == nullptr) {
        return nullptr;
    }
    return InterfaceList(raw);
}

std::span<const WLAN_INTERFACE_INFO> interfacesOf(const WLAN_INTERFACE_INFO_LIST& list) {
    return {&list.InterfaceInfo[0], list.dwNumberOfItems};
}

bool interfaceConnected(HANDLE client, const GUID& guid) {
    DWORD size = 0;
    void* data = nullptr;
    WLAN_OPCODE_VALUE_TYPE kind = wlan_opcode_value_type_query_only;
    if (WlanQueryInterface(client, &guid, wlan_intf_opcode_interface_state, nullptr, &size, &data,
                           &kind) != ERROR_SUCCESS ||
        data == nullptr || size < sizeof(WLAN_INTERFACE_STATE)) {
        return false;
    }
    const std::unique_ptr<void, WlanFree> owned(data);
    WLAN_INTERFACE_STATE state = wlan_interface_state_not_ready;
    std::copy_n(static_cast<const std::byte*>(data), sizeof(state),
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) — WLAN C ABI value.
                reinterpret_cast<std::byte*>(&state));
    return state == wlan_interface_state_connected;
}

// True once the interface holds an IPv4 address assigned by DHCP and ready for use. The glasses
// run the DHCP server on their network; a self-assigned link-local address does not count.
bool hasDhcpAddress(const GUID& guid) {
    NET_LUID luid{};
    if (ConvertInterfaceGuidToLuid(&guid, &luid) != NO_ERROR) {
        return false;
    }
    MIB_UNICASTIPADDRESS_TABLE* raw = nullptr;
    if (GetUnicastIpAddressTable(AF_INET, &raw) != NO_ERROR || raw == nullptr) {
        return false;
    }
    const std::unique_ptr<MIB_UNICASTIPADDRESS_TABLE, MibFree> table(raw);
    const std::span rows(&table->Table[0], table->NumEntries);
    return std::ranges::any_of(rows, [&](const MIB_UNICASTIPADDRESS_ROW& row) {
        return row.InterfaceLuid.Value == luid.Value && row.PrefixOrigin == IpPrefixOriginDhcp &&
               row.DadState == IpDadStatePreferred;
    });
}

class StationImpl final : public WifiStationConnection {
  public:
    StationImpl(HANDLE client, const GUID& guid, std::wstring profile)
        : client_(client), guid_(guid), profile_(std::move(profile)) {}
    // Deleting the profile disconnects the adapter from the glasses' network; Windows then
    // returns it to its usual networks.
    ~StationImpl() override { WlanDeleteProfile(client_.get(), &guid_, profile_.c_str(), nullptr); }
    StationImpl(const StationImpl&) = delete;
    StationImpl& operator=(const StationImpl&) = delete;
    StationImpl(StationImpl&&) = delete;
    StationImpl& operator=(StationImpl&&) = delete;

  private:
    WlanClient client_;
    GUID guid_;
    std::wstring profile_;
};
} // namespace

std::vector<WifiAdapter> listWifiAdapters() {
    HANDLE handle = nullptr;
    DWORD negotiated = 0;
    if (WlanOpenHandle(WlanClientVersion, nullptr, &negotiated, &handle) != ERROR_SUCCESS) {
        return {};
    }
    const WlanClient client(handle);
    const auto list = enumerateInterfaces(client.get());
    if (!list) {
        return {};
    }
    std::vector<WifiAdapter> adapters;
    for (const auto& info : interfacesOf(*list)) {
        // The description is a fixed-size, NUL-terminated array.
        const auto& raw = info.strInterfaceDescription;
        const auto* const end = std::ranges::find(raw, L'\0');
        adapters.push_back({.id = guidText(info.InterfaceGuid),
                            .description = narrow(std::wstring_view(std::begin(raw), end))});
    }
    return adapters;
}

std::unique_ptr<WifiStationConnection> connectWifiStation(const std::string& ssid,
                                                          const std::string& passphrase,
                                                          const std::string& adapter_id,
                                                          WifiStation& out) {
    if (ssid.empty() || ssid.size() > DOT11_SSID_MAX_LENGTH) {
        out.status = WifiStationStatus::ProfileFailed;
        return nullptr;
    }
    HANDLE handle = nullptr;
    DWORD negotiated = 0;
    if (const DWORD opened = WlanOpenHandle(WlanClientVersion, nullptr, &negotiated, &handle);
        opened != ERROR_SUCCESS) {
        out.status = WifiStationStatus::NoAdapter;
        out.detail = opened;
        return nullptr;
    }
    WlanClient client(handle);

    const auto list = enumerateInterfaces(client.get());
    const auto interfaces = list ? interfacesOf(*list) : std::span<const WLAN_INTERFACE_INFO>{};
    if (interfaces.empty()) {
        out.status = WifiStationStatus::NoAdapter;
        return nullptr;
    }
    const auto chosen =
        adapter_id.empty() ? interfaces.begin()
                           : std::ranges::find_if(interfaces, [&](const WLAN_INTERFACE_INFO& info) {
                                 return guidText(info.InterfaceGuid) == adapter_id;
                             });
    if (chosen == interfaces.end()) {
        out.status = WifiStationStatus::AdapterMissing;
        return nullptr;
    }
    const GUID guid = chosen->InterfaceGuid;

    const std::wstring xml = profileXml(ssid, passphrase);
    DWORD reason = 0;
    if (const DWORD set =
            WlanSetProfile(client.get(), &guid, 0, xml.c_str(), nullptr, TRUE, nullptr, &reason);
        set != ERROR_SUCCESS) {
        out.status = WifiStationStatus::ProfileFailed;
        out.detail = set;
        return nullptr;
    }

    const std::wstring profile_name = widen(ssid);
    DOT11_SSID dot11{};
    dot11.uSSIDLength = static_cast<ULONG>(ssid.size());
    std::copy_n(ssid.begin(), ssid.size(), std::begin(dot11.ucSSID));
    WLAN_CONNECTION_PARAMETERS params{.wlanConnectionMode = wlan_connection_mode_profile,
                                      .strProfile = profile_name.c_str(),
                                      .pDot11Ssid = &dot11,
                                      .pDesiredBssidList = nullptr,
                                      .dot11BssType = dot11_BSS_type_infrastructure,
                                      .dwFlags = 0};
    if (const DWORD connect = WlanConnect(client.get(), &guid, &params, nullptr);
        connect != ERROR_SUCCESS) {
        WlanDeleteProfile(client.get(), &guid, profile_name.c_str(), nullptr);
        out.status = WifiStationStatus::ConnectFailed;
        out.detail = connect;
        return nullptr;
    }

    // Wait for association, then for DHCP from the glasses.
    const auto deadline = std::chrono::steady_clock::now() + ConnectBudget;
    bool associated = false;
    bool addressed = false;
    while (!addressed && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(ConnectPollInterval);
        associated = associated || interfaceConnected(client.get(), guid);
        addressed = associated && hasDhcpAddress(guid);
    }
    if (!addressed) {
        WlanDeleteProfile(client.get(), &guid, profile_name.c_str(), nullptr);
        out.status = associated ? WifiStationStatus::NoAddress : WifiStationStatus::ConnectFailed;
        return nullptr;
    }
    out.status = WifiStationStatus::Ok;
    return std::make_unique<StationImpl>(client.release(), guid, profile_name);
}
} // namespace mgl
