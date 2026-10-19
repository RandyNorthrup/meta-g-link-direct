// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/rfcomm.hpp"
#include "meta_service.hpp"
#include "mgl/sdu.hpp"

#include <windows.h> // IWYU pragma: keep — SDK architecture prerequisites; WIN32_LEAN_AND_MEAN excludes Winsock 1.

#include <guiddef.h>
#include <minwindef.h>
#include <winerror.h>
#include <winsock2.h>
#include <ws2bth.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <utility>

namespace mgl {
namespace {
class WinsockSession final {
  public:
    explicit WinsockSession(DirectRfcommResult& result)
        : result_(result), startup_error_(startWinsock()) {
        result_.socket_error = static_cast<std::uint32_t>(startup_error_);
    }
    ~WinsockSession() {
        if (startup_error_ == 0 && WSACleanup() == SOCKET_ERROR) {
            result_.cleanup_error = static_cast<std::uint32_t>(WSAGetLastError());
        }
    }
    WinsockSession(const WinsockSession&) = delete;
    WinsockSession& operator=(const WinsockSession&) = delete;
    WinsockSession(WinsockSession&&) = delete;
    WinsockSession& operator=(WinsockSession&&) = delete;

  private:
    DirectRfcommResult& result_;
    int startup_error_;
    static int startWinsock() {
        WSADATA data{};
        return WSAStartup(MAKEWORD(2, 2), &data);
    }
};

class Socket final {
  public:
    explicit Socket(DirectRfcommResult& result)
        : result_(result), value_(WSASocketW(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM, nullptr, 0,
                                             WSA_FLAG_NO_HANDLE_INHERIT)) {
        if (std::cmp_equal(value_, INVALID_SOCKET)) {
            result_.socket_error = static_cast<std::uint32_t>(WSAGetLastError());
        }
    }
    ~Socket() {
        if (std::cmp_not_equal(value_, INVALID_SOCKET) && closesocket(value_) == SOCKET_ERROR) {
            result_.cleanup_error = static_cast<std::uint32_t>(WSAGetLastError());
        }
    }
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&&) = delete;
    Socket& operator=(Socket&&) = delete;
    [[nodiscard]] SOCKET get() const noexcept { return value_; }

  private:
    DirectRfcommResult& result_;
    SOCKET value_;
};

bool inspectSocketError(SOCKET socket, DirectRfcommResult& result) {
    std::array<char, sizeof(int)> error_buffer{};
    int length = static_cast<int>(error_buffer.size());
    if (getsockopt(socket, SOL_SOCKET, SO_ERROR, error_buffer.data(), &length) == SOCKET_ERROR) {
        result.socket_error = static_cast<std::uint32_t>(WSAGetLastError());
        return false;
    }
    int error = 0;
    if (std::cmp_not_equal(length, sizeof(error))) {
        result.socket_error = WSAEINVAL;
        return false;
    }
    std::memcpy(&error, error_buffer.data(), sizeof(error));
    result.socket_error = static_cast<std::uint32_t>(error);
    return error == 0;
}

fd_set singleSocketSet(SOCKET socket) {
    fd_set sockets{};
    FD_SET(socket, &sockets);
    return sockets;
}

bool waitSocket(SOCKET socket, bool writing, std::chrono::steady_clock::time_point deadline,
                DirectRfcommResult& result) {
    const auto remaining = std::chrono::duration_cast<std::chrono::microseconds>(
        deadline - std::chrono::steady_clock::now());
    if (remaining <= std::chrono::microseconds::zero()) {
        result.timed_out = true;
        result.socket_error = WSAETIMEDOUT;
        return false;
    }
    auto ready = singleSocketSet(socket);
    auto failed = singleSocketSet(socket);
    constexpr auto MicrosPerSecond = 1000000;
    const timeval timeout{.tv_sec = static_cast<std::int32_t>(remaining.count() / MicrosPerSecond),
                          .tv_usec =
                              static_cast<std::int32_t>(remaining.count() % MicrosPerSecond)};
    const int selected =
        select(0, writing ? nullptr : &ready, writing ? &ready : nullptr, &failed, &timeout);
    if (selected == 0) {
        result.timed_out = true;
        result.socket_error = WSAETIMEDOUT;
        return false;
    }
    if (selected == SOCKET_ERROR) {
        result.socket_error = static_cast<std::uint32_t>(WSAGetLastError());
        return false;
    }
    return inspectSocketError(socket, result);
}

void receiveHello(SOCKET socket, DirectRfcommResult& result) {
    constexpr std::size_t MaximumFrame = 4096;
    constexpr std::size_t MaximumPrefix = 10;
    constexpr auto ReceiveTimeout = std::chrono::seconds(5);
    const auto deadline = std::chrono::steady_clock::now() + ReceiveTimeout;
    std::array<char, MaximumFrame> frame{};
    std::size_t target = 2;
    std::size_t received = 0;
    while (received < target) {
        if (!waitSocket(socket, false, deadline, result)) {
            return;
        }
        const int count =
            recv(socket, frame.data() + received, static_cast<int>(target - received), 0);
        if (count == 0) {
            result.socket_error = WSAECONNRESET;
            return;
        }
        if (count == SOCKET_ERROR) {
            const auto error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                continue;
            }
            result.socket_error = static_cast<std::uint32_t>(error);
            return;
        }
        received += static_cast<std::size_t>(count);
        result.received_bytes = static_cast<std::uint32_t>(received);
        std::array<std::uint8_t, MaximumPrefix> prefix{};
        const auto length = std::min(received, prefix.size());
        for (std::size_t index = 0; index < length; ++index) {
            prefix.at(index) = static_cast<std::uint8_t>(frame.at(index));
        }
        const auto inspected = inspectFramePrefix(std::span(prefix).first(length));
        if (inspected.status == PrefixStatus::Invalid) {
            result.socket_error = WSAEINVAL;
            return;
        }
        if (inspected.status == PrefixStatus::NeedMore) {
            target = MaximumPrefix;
            continue;
        }
        if (inspected.frame_size > MaximumFrame) {
            result.socket_error = WSAEMSGSIZE;
            return;
        }
        target = inspected.frame_size;
        result.declared_frame_bytes = static_cast<std::uint32_t>(target);
    }
    result.frame_received = true;
    // Public Gen 2 channel-7 observation; this recognizes a candidate, not trusted identity.
    constexpr std::array<std::uint8_t, 12> Reference{0x80, 0x70, 0x80, 0x01, 0x81, 0x00,
                                                     0x00, 0x05, 0x02, 0x00, 0x00, 0x01};
    result.reference_hello_prefix = received >= Reference.size();
    for (std::size_t index = 0; index < std::min(received, Reference.size()); ++index) {
        result.reference_hello_prefix =
            result.reference_hello_prefix &&
            static_cast<std::uint8_t>(frame.at(index)) == Reference.at(index);
    }
}

void runDirect(std::uint64_t address, DirectRfcommRoute route, DirectRfcommResult& result) {
    const WinsockSession session(result);
    if (result.socket_error != 0) {
        return;
    }
    const Socket socket(result);
    if (result.socket_error != 0) {
        return;
    }
    u_long nonblocking = 1;
    if (ioctlsocket(socket.get(), FIONBIO, &nonblocking) == SOCKET_ERROR) {
        result.socket_error = static_cast<std::uint32_t>(WSAGetLastError());
        return;
    }
    SOCKADDR_BTH remote{};
    remote.addressFamily = AF_BTH;
    remote.btAddr = address;
    if (route == DirectRfcommRoute::DocumentedChannel7) {
        constexpr std::uint32_t DocumentedControlChannel = 7;
        remote.port = DocumentedControlChannel;
    } else {
        remote.serviceClassId = MetaRfcommServiceClass;
    }
    constexpr auto ConnectTimeout = std::chrono::seconds(5);
    const auto deadline = std::chrono::steady_clock::now() + ConnectTimeout;
    result.connection_attempted = true;
    // Winsock's public C ABI type-erases its documented SOCKADDR_BTH structure.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto* endpoint = reinterpret_cast<const sockaddr*>(&remote);
    const int status = connect(socket.get(), endpoint, sizeof(remote));
    if (status == SOCKET_ERROR) {
        const auto error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK && error != WSAEINPROGRESS) {
            result.socket_error = static_cast<std::uint32_t>(error);
            return;
        }
        if (!waitSocket(socket.get(), true, deadline, result)) {
            return;
        }
    }
    result.connected = true;
    receiveHello(socket.get(), result);
}
} // namespace

DirectRfcommResult probeDirectRfcomm(std::uint64_t private_address, DirectRfcommRoute route) {
    DirectRfcommResult result{};
    runDirect(private_address, route, result);
    return result;
}
} // namespace mgl
