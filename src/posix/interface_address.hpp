// SPDX-License-Identifier: AGPL-3.0-only
// Whether a network interface holds an address next to the glasses.
#ifndef MGL_POSIX_INTERFACE_ADDRESS_HPP
#define MGL_POSIX_INTERFACE_ADDRESS_HPP

#include <string>

namespace mgl {
// True once the named interface holds an IPv4 address on the same subnet as peer, the glasses'
// dotted-quad address. An address still left from the adapter's previous network does not count.
[[nodiscard]] bool interfaceOnSubnetOf(const std::string& interface_name, const std::string& peer);

// True when peer is a dotted-quad IPv4 address.
[[nodiscard]] bool validIpv4(const std::string& peer);
} // namespace mgl
#endif
