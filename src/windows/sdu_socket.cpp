// SPDX-License-Identifier: AGPL-3.0-only
#include "sdu_socket.hpp"

#include "mgl/bytes.hpp"
#include "mgl/transport.hpp"

#include <windows.h> // IWYU pragma: keep — SDK umbrella prerequisites.

#include <minwindef.h>
#include <winsock2.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace mgl {
WinsockLibrary::WinsockLibrary() {
    WSADATA data{};
    startup_error_ = WSAStartup(MAKEWORD(2, 2), &data);
}

WinsockLibrary::WinsockLibrary(WinsockLibrary&& other) noexcept
    : startup_error_(other.startup_error_), owns_(other.owns_) {
    other.owns_ = false;
}

WinsockLibrary::~WinsockLibrary() {
    if (startup_error_ == 0 && owns_) {
        WSACleanup();
    }
}

SduSocket::SduSocket(WinsockLibrary library, SOCKET socket)
    : library_(std::move(library)), socket_(socket) {}

SduSocket::~SduSocket() { SduSocket::close(); }

void SduSocket::close() {
    if (std::cmp_not_equal(socket_, INVALID_SOCKET)) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
}

bool SduSocket::send(ByteView frame) {
    // Winsock's C ABI takes the byte buffer as char.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto* const bytes = reinterpret_cast<const char*>(frame.data());
    std::size_t sent = 0;
    while (sent < frame.size()) {
        constexpr std::size_t MaxChunk = 0x4000;
        const auto chunk = static_cast<int>(std::min(frame.size() - sent, MaxChunk));
        const auto wrote = ::send(socket_, bytes + sent, chunk, 0);
        if (wrote == SOCKET_ERROR || wrote <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(wrote);
    }
    return true;
}

TransportStatus SduSocket::fill(std::chrono::milliseconds timeout) {
    fd_set readable;
    FD_ZERO(&readable);
    FD_SET(socket_, &readable);
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(timeout);
    const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(timeout - seconds);
    const timeval tv{
        .tv_sec = static_cast<decltype(timeval::tv_sec)>(seconds.count()),
        .tv_usec = static_cast<decltype(timeval::tv_usec)>(micros.count()),
    };
    const auto ready = select(0, &readable, nullptr, nullptr, &tv);
    if (ready == 0) {
        return TransportStatus::TimedOut;
    }
    if (ready == SOCKET_ERROR) {
        return TransportStatus::Closed;
    }
    std::array<char, 4096> chunk{};
    const auto read = recv(socket_, chunk.data(), static_cast<int>(chunk.size()), 0);
    if (read == 0 || read == SOCKET_ERROR) {
        return TransportStatus::Closed;
    }
    // Winsock's C ABI returns the byte buffer as char.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto* const first = reinterpret_cast<const std::uint8_t*>(chunk.data());
    keep(ByteView(first, static_cast<std::size_t>(read)));
    return TransportStatus::Ok;
}
} // namespace mgl
