// SPDX-License-Identifier: AGPL-3.0-only
// Joins the glasses' network through NetworkManager over D-Bus. The connection is volatile (it
// never reaches disk, so the random credentials are not stored) and bound to this program's bus
// name, so NetworkManager takes the adapter back to its usual network even if the program dies.
#include "gio_handles.hpp"
#include "interface_address.hpp"
#include "mgl/wifi_station.hpp"

#include <gio/gio.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace mgl {
namespace {
using namespace std::chrono_literals;
using linux_bus::ConnectionPtr;
using linux_bus::Failure;
using linux_bus::VariantPtr;

constexpr const char* NetworkManagerName = "org.freedesktop.NetworkManager";
constexpr const char* NetworkManagerPath = "/org/freedesktop/NetworkManager";
constexpr const char* PropertiesInterface = "org.freedesktop.DBus.Properties";
constexpr const char* DeviceInterface = "org.freedesktop.NetworkManager.Device";
constexpr const char* ActiveInterface = "org.freedesktop.NetworkManager.Connection.Active";
constexpr std::uint32_t DeviceTypeWifi = 2;
constexpr std::uint32_t ActiveStateActivated = 2;
constexpr std::uint32_t ActiveStateDeactivating = 3;
constexpr std::uint32_t ActiveStateDeactivated = 4;
constexpr int BusTimeoutMs = 10000;
// How long association plus DHCP may take before the connect counts as failed.
constexpr auto ConnectBudget = 25s;
constexpr auto ConnectPollInterval = 100ms;

struct Device {
    std::string path;
    std::string interface_name;
    std::string driver;
};

std::vector<Device> wifiDevices(GDBusConnection* bus) {
    Failure failure;
    const VariantPtr reply(g_dbus_connection_call_sync(
        bus, NetworkManagerName, NetworkManagerPath, "org.freedesktop.NetworkManager", "GetDevices",
        nullptr, G_VARIANT_TYPE("(ao)"), G_DBUS_CALL_FLAGS_NONE, BusTimeoutMs, nullptr,
        failure.out()));
    std::vector<Device> devices;
    if (!reply) {
        return devices;
    }
    const VariantPtr paths(g_variant_get_child_value(reply.get(), 0));
    GVariantIter iter;
    g_variant_iter_init(&iter, paths.get());
    const char* path = nullptr;
    while (g_variant_iter_next(&iter, "&o", &path)) {
        Failure property_failure;
        const VariantPtr all(g_dbus_connection_call_sync(
            bus, NetworkManagerName, path, PropertiesInterface, "GetAll",
            g_variant_new("(s)", DeviceInterface), G_VARIANT_TYPE("(a{sv})"),
            G_DBUS_CALL_FLAGS_NONE, BusTimeoutMs, nullptr, property_failure.out()));
        if (!all) {
            continue;
        }
        const VariantPtr values(g_variant_get_child_value(all.get(), 0));
        std::uint32_t kind = 0;
        const char* name = nullptr;
        const char* driver = "";
        if (g_variant_lookup(values.get(), "DeviceType", "u", &kind) && kind == DeviceTypeWifi &&
            g_variant_lookup(values.get(), "Interface", "&s", &name)) {
            (void)g_variant_lookup(values.get(), "Driver", "&s", &driver);
            devices.push_back({.path = path, .interface_name = name, .driver = driver});
        }
    }
    return devices;
}

// One connection's settings: a hidden-style WPA2-PSK client that actively probes for the network,
// so a network that has only just started is found without waiting for a scan, and that never
// becomes the default route, so another adapter stays the way out to the internet.
GVariant* connectionSettings(const std::string& ssid, const std::string& passphrase) {
    GVariantBuilder settings;
    g_variant_builder_init(&settings, G_VARIANT_TYPE("a{sa{sv}}"));

    g_variant_builder_open(&settings, G_VARIANT_TYPE("{sa{sv}}"));
    g_variant_builder_add(&settings, "s", "connection");
    g_variant_builder_open(&settings, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&settings, "{sv}", "id", g_variant_new_string(ssid.c_str()));
    g_variant_builder_add(&settings, "{sv}", "type", g_variant_new_string("802-11-wireless"));
    g_variant_builder_add(&settings, "{sv}", "autoconnect", g_variant_new_boolean(FALSE));
    g_variant_builder_close(&settings);
    g_variant_builder_close(&settings);

    g_variant_builder_open(&settings, G_VARIANT_TYPE("{sa{sv}}"));
    g_variant_builder_add(&settings, "s", "802-11-wireless");
    g_variant_builder_open(&settings, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(
        &settings, "{sv}", "ssid",
        g_variant_new_fixed_array(G_VARIANT_TYPE_BYTE, ssid.data(), ssid.size(), sizeof(char)));
    g_variant_builder_add(&settings, "{sv}", "mode", g_variant_new_string("infrastructure"));
    g_variant_builder_add(&settings, "{sv}", "band", g_variant_new_string("bg"));
    g_variant_builder_add(&settings, "{sv}", "hidden", g_variant_new_boolean(TRUE));
    g_variant_builder_close(&settings);
    g_variant_builder_close(&settings);

    g_variant_builder_open(&settings, G_VARIANT_TYPE("{sa{sv}}"));
    g_variant_builder_add(&settings, "s", "802-11-wireless-security");
    g_variant_builder_open(&settings, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&settings, "{sv}", "key-mgmt", g_variant_new_string("wpa-psk"));
    g_variant_builder_add(&settings, "{sv}", "psk", g_variant_new_string(passphrase.c_str()));
    g_variant_builder_close(&settings);
    g_variant_builder_close(&settings);

    g_variant_builder_open(&settings, G_VARIANT_TYPE("{sa{sv}}"));
    g_variant_builder_add(&settings, "s", "ipv4");
    g_variant_builder_open(&settings, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&settings, "{sv}", "method", g_variant_new_string("auto"));
    g_variant_builder_add(&settings, "{sv}", "never-default", g_variant_new_boolean(TRUE));
    g_variant_builder_close(&settings);
    g_variant_builder_close(&settings);

    g_variant_builder_open(&settings, G_VARIANT_TYPE("{sa{sv}}"));
    g_variant_builder_add(&settings, "s", "ipv6");
    g_variant_builder_open(&settings, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&settings, "{sv}", "method", g_variant_new_string("ignore"));
    g_variant_builder_close(&settings);
    g_variant_builder_close(&settings);

    return g_variant_builder_end(&settings);
}

// The active connection's state; nullopt when it is gone.
std::optional<std::uint32_t> activeState(GDBusConnection* bus, const std::string& active) {
    Failure failure;
    const VariantPtr reply(g_dbus_connection_call_sync(
        bus, NetworkManagerName, active.c_str(), PropertiesInterface, "Get",
        g_variant_new("(ss)", ActiveInterface, "State"), G_VARIANT_TYPE("(v)"),
        G_DBUS_CALL_FLAGS_NONE, BusTimeoutMs, nullptr, failure.out()));
    if (!reply) {
        return std::nullopt;
    }
    const VariantPtr boxed(g_variant_get_child_value(reply.get(), 0));
    const VariantPtr state(g_variant_get_variant(boxed.get()));
    return g_variant_get_uint32(state.get());
}

void deactivate(GDBusConnection* bus, const std::string& active) {
    Failure failure;
    const VariantPtr reply(g_dbus_connection_call_sync(
        bus, NetworkManagerName, NetworkManagerPath, "org.freedesktop.NetworkManager",
        "DeactivateConnection", g_variant_new("(o)", active.c_str()), nullptr,
        G_DBUS_CALL_FLAGS_NONE, BusTimeoutMs, nullptr, failure.out()));
}

class StationImpl final : public WifiStationConnection {
  public:
    StationImpl(ConnectionPtr bus, std::string active)
        : bus_(std::move(bus)), active_(std::move(active)) {}
    // Deactivating removes the volatile connection; NetworkManager then returns the adapter to its
    // usual network.
    ~StationImpl() override { deactivate(bus_.get(), active_); }
    StationImpl(const StationImpl&) = delete;
    StationImpl& operator=(const StationImpl&) = delete;
    StationImpl(StationImpl&&) = delete;
    StationImpl& operator=(StationImpl&&) = delete;

  private:
    ConnectionPtr bus_;
    std::string active_;
};
} // namespace

std::vector<WifiAdapter> listWifiAdapters() {
    Failure failure;
    const auto bus = linux_bus::systemBus(failure);
    if (!bus) {
        return {};
    }
    std::vector<WifiAdapter> adapters;
    for (const auto& device : wifiDevices(bus.get())) {
        adapters.push_back({
            .id = device.interface_name,
            .description = device.driver.empty()
                               ? device.interface_name
                               : device.interface_name + " (" + device.driver + ")",
        });
    }
    return adapters;
}

std::unique_ptr<WifiStationConnection> connectWifiStation(const std::string& ssid,
                                                          const std::string& passphrase,
                                                          const std::string& adapter_id,
                                                          const std::string& peer_address,
                                                          WifiStation& out) {
    constexpr std::size_t MaxSsidLength = 32;
    if (ssid.empty() || ssid.size() > MaxSsidLength) {
        out.status = WifiStationStatus::ProfileFailed;
        return nullptr;
    }
    if (!validIpv4(peer_address)) {
        out.status = WifiStationStatus::BadPeer;
        return nullptr;
    }
    Failure bus_failure;
    auto bus = linux_bus::systemBus(bus_failure);
    if (!bus) {
        out.status = WifiStationStatus::NoAdapter;
        out.detail_text = bus_failure.message();
        return nullptr;
    }
    const auto devices = wifiDevices(bus.get());
    if (devices.empty()) {
        out.status = WifiStationStatus::NoAdapter;
        return nullptr;
    }
    const auto chosen =
        adapter_id.empty() ? devices.begin() : std::ranges::find_if(devices, [&](const Device& d) {
            return d.interface_name == adapter_id;
        });
    if (chosen == devices.end()) {
        out.status = WifiStationStatus::AdapterMissing;
        return nullptr;
    }

    GVariantBuilder options;
    g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&options, "{sv}", "persist", g_variant_new_string("volatile"));
    g_variant_builder_add(&options, "{sv}", "bind-activation", g_variant_new_string("dbus-client"));
    Failure failure;
    const VariantPtr reply(g_dbus_connection_call_sync(
        bus.get(), NetworkManagerName, NetworkManagerPath, "org.freedesktop.NetworkManager",
        "AddAndActivateConnection2",
        g_variant_new("(@a{sa{sv}}oo@a{sv})", connectionSettings(ssid, passphrase),
                      chosen->path.c_str(), "/", g_variant_builder_end(&options)),
        G_VARIANT_TYPE("(ooa{sv})"), G_DBUS_CALL_FLAGS_NONE, BusTimeoutMs, nullptr, failure.out()));
    if (!reply) {
        out.status = WifiStationStatus::ProfileFailed;
        out.detail = static_cast<std::uint32_t>(failure.code());
        out.detail_text = failure.message();
        return nullptr;
    }
    const char* active_path = nullptr;
    g_variant_get_child(reply.get(), 1, "&o", &active_path);
    std::string active(active_path);

    // Wait for association, then for DHCP from the glasses.
    const auto requested = std::chrono::steady_clock::now();
    const auto deadline = requested + ConnectBudget;
    const auto since_request = [&] {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - requested);
    };
    bool associated = false;
    bool addressed = false;
    bool gone = false;
    while (!addressed && !gone && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(ConnectPollInterval);
        const auto state = activeState(bus.get(), active);
        gone = !state || *state == ActiveStateDeactivating || *state == ActiveStateDeactivated;
        if (!associated && state && *state == ActiveStateActivated) {
            associated = true;
            out.associated_after = since_request();
        }
        addressed = associated && interfaceOnSubnetOf(chosen->interface_name, peer_address);
    }
    out.addressed_after = since_request();
    if (!addressed) {
        if (!gone) {
            deactivate(bus.get(), active);
        }
        out.status = associated ? WifiStationStatus::NoAddress : WifiStationStatus::ConnectFailed;
        return nullptr;
    }
    out.status = WifiStationStatus::Ok;
    return std::make_unique<StationImpl>(std::move(bus), std::move(active));
}
} // namespace mgl
