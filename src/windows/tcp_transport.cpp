// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/tcp_transport.hpp"

#include "sdu_socket.hpp"

#include <windows.h> // IWYU pragma: keep — SDK umbrella prerequisites.

#include <minwindef.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace mgl {
TcpOpen openTcp(const std::string& address, std::uint16_t port) {
    TcpOpen result;
    WinsockLibrary library;
    if (library.error() != 0) {
        result.detail = static_cast<std::uint32_t>(library.error());
        return result;
    }
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    const auto service = std::to_string(port);
    addrinfo* resolved = nullptr;
    if (getaddrinfo(address.c_str(), service.c_str(), &hints, &resolved) != 0 ||
        resolved == nullptr) {
        result.detail = static_cast<std::uint32_t>(WSAGetLastError());
        return result;
    }
    const auto socket = WSASocketW(resolved->ai_family, resolved->ai_socktype,
                                   resolved->ai_protocol, nullptr, 0, WSA_FLAG_NO_HANDLE_INHERIT);
    if (std::cmp_equal(socket, INVALID_SOCKET)) {
        result.detail = static_cast<std::uint32_t>(WSAGetLastError());
        freeaddrinfo(resolved);
        return result;
    }
    const auto status = connect(socket, resolved->ai_addr, static_cast<int>(resolved->ai_addrlen));
    freeaddrinfo(resolved);
    if (status == SOCKET_ERROR) {
        result.detail = static_cast<std::uint32_t>(WSAGetLastError());
        closesocket(socket);
        return result;
    }
    // Low latency for the media stream: disable Nagle.
    const DWORD on = 1;
    // Winsock's C ABI type-erases the option value as char.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto* const option = reinterpret_cast<const char*>(&on);
    setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, option, sizeof(on));
    result.ok = true;
    result.transport = std::make_unique<SduSocket>(std::move(library), socket);
    return result;
}
} // namespace mgl
