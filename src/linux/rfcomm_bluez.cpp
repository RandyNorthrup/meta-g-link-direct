// SPDX-License-Identifier: AGPL-3.0-only
// The RFCOMM transport on Linux: BlueZ names the paired device that offers the Meta service, its
// SDP record gives the channel, and an AF_BLUETOOTH socket carries the SDUs. The service UUID is
// from the pinned Visor BluetoothRfcommTransport.kt (see THIRD_PARTY_NOTICES.md).
#include "fd_transport.hpp"
#include "gio_handles.hpp"
#include "mgl/rfcomm_transport.hpp"
#include "mgl/transport.hpp"

#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include <gio/gio.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mgl {
namespace {
constexpr std::string_view MetaServiceText = "8b0d2687-42a4-44cb-9436-fba3b9b96de2";
constexpr std::array<std::uint8_t, 16> MetaServiceBytes{
    0x8B, 0x0D, 0x26, 0x87, 0x42, 0xA4, 0x44, 0xCB, 0x94, 0x36, 0xFB, 0xA3, 0xB9, 0xB9, 0x6D, 0xE2,
};
constexpr int BusTimeoutMs = 5000;
constexpr auto ConnectTimeout = std::chrono::seconds(10);

struct PairedService {
    std::string address;
    std::string name;
};

bool offersMetaService(GVariant* uuids) {
    GVariantIter iter;
    g_variant_iter_init(&iter, uuids);
    const char* uuid = nullptr;
    while (g_variant_iter_next(&iter, "&s", &uuid)) {
        if (std::string_view(uuid) == MetaServiceText) {
            return true;
        }
    }
    return false;
}

// Every paired device BlueZ knows that lists the Meta service.
std::vector<PairedService> pairedMetaServices(GDBusConnection* bus, std::uint32_t& detail) {
    linux_bus::Failure failure;
    const linux_bus::VariantPtr reply(
        g_dbus_connection_call_sync(bus, "org.bluez", "/", "org.freedesktop.DBus.ObjectManager",
                                    "GetManagedObjects", nullptr, G_VARIANT_TYPE("(a{oa{sa{sv}}})"),
                                    G_DBUS_CALL_FLAGS_NONE, BusTimeoutMs, nullptr, failure.out()));
    std::vector<PairedService> found;
    if (!reply) {
        detail = static_cast<std::uint32_t>(failure.code());
        return found;
    }
    const linux_bus::VariantPtr objects(g_variant_get_child_value(reply.get(), 0));
    GVariantIter object_iter;
    g_variant_iter_init(&object_iter, objects.get());
    const char* path = nullptr;
    GVariant* interfaces_raw = nullptr;
    while (g_variant_iter_next(&object_iter, "{&o@a{sa{sv}}}", &path, &interfaces_raw)) {
        const linux_bus::VariantPtr interfaces(interfaces_raw);
        const linux_bus::VariantPtr device(
            g_variant_lookup_value(interfaces.get(), "org.bluez.Device1", G_VARIANT_TYPE("a{sv}")));
        if (!device) {
            continue;
        }
        gboolean paired = FALSE;
        const char* address = nullptr;
        const char* name = "";
        if (!g_variant_lookup(device.get(), "Paired", "b", &paired) || paired == FALSE ||
            !g_variant_lookup(device.get(), "Address", "&s", &address)) {
            continue;
        }
        (void)g_variant_lookup(device.get(), "Name", "&s", &name);
        const linux_bus::VariantPtr uuids(
            g_variant_lookup_value(device.get(), "UUIDs", G_VARIANT_TYPE("as")));
        if (uuids && offersMetaService(uuids.get())) {
            found.push_back({.address = address, .name = name});
        }
    }
    return found;
}

// The Meta service's RFCOMM channel from the device's own SDP record.
std::optional<int> serviceChannel(const bdaddr_t& target) {
    bdaddr_t any{};
    sdp_session_t* const session = sdp_connect(&any, &target, SDP_RETRY_IF_BUSY);
    if (session == nullptr) {
        return std::nullopt;
    }
    uuid_t service{};
    sdp_uuid128_create(&service, MetaServiceBytes.data());
    sdp_list_t* const search = sdp_list_append(nullptr, &service);
    std::uint32_t range = 0x0000FFFF;
    sdp_list_t* const attributes = sdp_list_append(nullptr, &range);
    sdp_list_t* response = nullptr;
    std::optional<int> channel;
    if (sdp_service_search_attr_req(session, search, SDP_ATTR_REQ_RANGE, attributes, &response) ==
        0) {
        for (const sdp_list_t* entry = response; entry != nullptr && !channel;
             entry = entry->next) {
            auto* const record = static_cast<sdp_record_t*>(entry->data);
            sdp_list_t* protocols = nullptr;
            if (sdp_get_access_protos(record, &protocols) == 0) {
                if (const int found = sdp_get_proto_port(protocols, RFCOMM_UUID); found > 0) {
                    channel = found;
                }
                sdp_list_foreach(
                    protocols,
                    [](void* list, void* /*unused*/) {
                        sdp_list_free(static_cast<sdp_list_t*>(list), nullptr);
                    },
                    nullptr);
                sdp_list_free(protocols, nullptr);
            }
        }
    }
    sdp_list_free(response, reinterpret_cast<sdp_free_func_t>(sdp_record_free));
    sdp_list_free(search, nullptr);
    sdp_list_free(attributes, nullptr);
    sdp_close(session);
    return channel;
}

std::unique_ptr<Transport> connectChannel(const bdaddr_t& target, int channel, RfcommOpen& result) {
    const int descriptor = socket(AF_BLUETOOTH, SOCK_STREAM | SOCK_CLOEXEC, BTPROTO_RFCOMM);
    if (descriptor < 0) {
        // A sandbox without Bluetooth access lands here, as does a kernel without RFCOMM.
        result.status = RfcommConnect::NoRadio;
        result.detail = static_cast<std::uint32_t>(errno);
        return nullptr;
    }
    // The glasses require the bonded, encrypted link Windows and Android also ask for.
    const int security = RFCOMM_LM_AUTH | RFCOMM_LM_ENCRYPT;
    (void)setsockopt(descriptor, SOL_RFCOMM, RFCOMM_LM, &security, sizeof(security));
    sockaddr_rc remote{};
    remote.rc_family = AF_BLUETOOTH;
    remote.rc_bdaddr = target;
    remote.rc_channel = static_cast<std::uint8_t>(channel);
    if (const int error = connectWithin(descriptor, &remote, sizeof(remote), ConnectTimeout);
        error != 0) {
        result.status = RfcommConnect::ConnectFailed;
        result.detail = static_cast<std::uint32_t>(error);
        close(descriptor);
        return nullptr;
    }
    result.status = RfcommConnect::Ok;
    return std::make_unique<FdTransport>(descriptor);
}
} // namespace

RfcommOpen openBondedMetaRfcomm() {
    RfcommOpen result;
    linux_bus::Failure failure;
    const auto bus = linux_bus::systemBus(failure);
    if (!bus) {
        result.status = RfcommConnect::NoRadio;
        result.detail = static_cast<std::uint32_t>(failure.code());
        return result;
    }
    const auto services = pairedMetaServices(bus.get(), result.detail);
    if (services.empty()) {
        result.status = RfcommConnect::NoMetaService;
        return result;
    }
    if (services.size() > 1) {
        result.status = RfcommConnect::AmbiguousService;
        return result;
    }
    bdaddr_t target{};
    if (str2ba(services.front().address.c_str(), &target) != 0) {
        result.status = RfcommConnect::ConnectFailed;
        return result;
    }
    const auto channel = serviceChannel(target);
    if (!channel) {
        result.status = RfcommConnect::ConnectFailed;
        result.detail = static_cast<std::uint32_t>(errno);
        return result;
    }
    result.transport = connectChannel(target, *channel, result);
    return result;
}
} // namespace mgl
