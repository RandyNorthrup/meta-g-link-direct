// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/tcp_transport.hpp"

#include "sdu_socket.hpp"

#include <windows.h> // IWYU pragma: keep — SDK umbrella prerequisites.

#include <minwindef.h>
#include <winerror.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace mgl {
namespace {
// A connect may not wait out Windows' full SYN retry time (about 21 s): an attempt made before the
// route to the glasses is ready then fails quickly and the caller retries.
constexpr INT ConnectTimeoutMs = 3000;
constexpr unsigned PollFailure = static_cast<unsigned>(POLLERR) | static_cast<unsigned>(POLLHUP);

// The error a failed non-blocking connect left on socket; a refusal when Winsock reports none.
int pendingConnectError(SOCKET socket) {
    int reported = 0;
    int size = sizeof(reported);
    // Winsock's C ABI type-erases the option value as char.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    auto* const value = reinterpret_cast<char*>(&reported);
    if (getsockopt(socket, SOL_SOCKET, SO_ERROR, value, &size) == SOCKET_ERROR) {
        return WSAGetLastError();
    }
    return reported != 0 ? reported : WSAECONNREFUSED;
}

// Waits up to ConnectTimeoutMs for a connect in progress. 0 on success, otherwise the error.
int awaitConnect(SOCKET socket) {
    WSAPOLLFD pending{.fd = socket, .events = POLLWRNORM, .revents = 0};
    const int ready = WSAPoll(&pending, 1, ConnectTimeoutMs);
    if (ready == 0) {
        return WSAETIMEDOUT;
    }
    if (ready == SOCKET_ERROR) {
        return WSAGetLastError();
    }
    if ((static_cast<unsigned>(pending.revents) & PollFailure) != 0) {
        return pendingConnectError(socket);
    }
    return 0;
}

// Connects socket within ConnectTimeoutMs. 0 on success, otherwise the Winsock error.
int boundedConnect(SOCKET socket, const sockaddr* target, int target_size) {
    u_long non_blocking = 1;
    if (ioctlsocket(socket, FIONBIO, &non_blocking) == SOCKET_ERROR) {
        return WSAGetLastError();
    }
    int error = 0;
    if (connect(socket, target, target_size) == SOCKET_ERROR) {
        error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK) {
            error = awaitConnect(socket);
        }
    }
    // The transport reads and writes with blocking calls and its own timeouts.
    u_long blocking = 0;
    if (error == 0 && ioctlsocket(socket, FIONBIO, &blocking) == SOCKET_ERROR) {
        error = WSAGetLastError();
    }
    return error;
}
} // namespace

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
    const int error =
        boundedConnect(socket, resolved->ai_addr, static_cast<int>(resolved->ai_addrlen));
    freeaddrinfo(resolved);
    if (error != 0) {
        result.detail = static_cast<std::uint32_t>(error);
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
