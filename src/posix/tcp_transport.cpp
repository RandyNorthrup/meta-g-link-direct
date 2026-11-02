// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/tcp_transport.hpp"

#include "fd_transport.hpp"

#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

namespace mgl {
namespace {
// A connect may not wait out the system's full SYN retry time: an attempt made before the route to
// the glasses is ready then fails quickly and the caller retries.
constexpr auto ConnectTimeout = std::chrono::milliseconds(3000);
} // namespace

TcpOpen openTcp(const std::string& address, std::uint16_t port) {
    TcpOpen result;
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_NUMERICHOST;
    const auto service = std::to_string(port);
    addrinfo* resolved = nullptr;
    if (const int failed = getaddrinfo(address.c_str(), service.c_str(), &hints, &resolved);
        failed != 0 || resolved == nullptr) {
        result.detail = static_cast<std::uint32_t>(failed);
        return result;
    }
    const std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> owned(resolved, &freeaddrinfo);
    const int descriptor =
        socket(resolved->ai_family, resolved->ai_socktype, resolved->ai_protocol);
    if (descriptor < 0) {
        result.detail = static_cast<std::uint32_t>(errno);
        return result;
    }
    if (const int error =
            connectWithin(descriptor, resolved->ai_addr, resolved->ai_addrlen, ConnectTimeout);
        error != 0) {
        result.detail = static_cast<std::uint32_t>(error);
        close(descriptor);
        return result;
    }
    // Low latency for the media stream: disable Nagle.
    const int on = 1;
    (void)setsockopt(descriptor, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
    result.ok = true;
    result.transport = std::make_unique<FdTransport>(descriptor);
    return result;
}
} // namespace mgl
