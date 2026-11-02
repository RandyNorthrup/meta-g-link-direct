// SPDX-License-Identifier: AGPL-3.0-only
#include "interface_address.hpp"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

namespace mgl {
namespace {
std::uint32_t ipv4Of(const sockaddr* address) {
    sockaddr_in view{};
    std::memcpy(&view, address, sizeof(view));
    return view.sin_addr.s_addr;
}
} // namespace

bool validIpv4(const std::string& peer) {
    in_addr parsed{};
    return inet_pton(AF_INET, peer.c_str(), &parsed) == 1;
}

bool interfaceOnSubnetOf(const std::string& interface_name, const std::string& peer) {
    in_addr target{};
    if (inet_pton(AF_INET, peer.c_str(), &target) != 1) {
        return false;
    }
    ifaddrs* raw = nullptr;
    if (getifaddrs(&raw) != 0) {
        return false;
    }
    const std::unique_ptr<ifaddrs, decltype(&freeifaddrs)> owned(raw, &freeifaddrs);
    for (const ifaddrs* entry = raw; entry != nullptr; entry = entry->ifa_next) {
        if (entry->ifa_addr == nullptr || entry->ifa_netmask == nullptr ||
            entry->ifa_addr->sa_family != AF_INET || interface_name != entry->ifa_name) {
            continue;
        }
        const std::uint32_t mask = ipv4Of(entry->ifa_netmask);
        if (mask != 0 && (ipv4Of(entry->ifa_addr) & mask) == (target.s_addr & mask)) {
            return true;
        }
    }
    return false;
}
} // namespace mgl
