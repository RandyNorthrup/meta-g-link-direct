// SPDX-License-Identifier: AGPL-3.0-only
#include "sdu_socket.hpp"

#include "mgl/bytes.hpp"
#include "mgl/sdu.hpp"
#include "mgl/transport.hpp"

#include <winsock2.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>
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
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
}

bool SduSocket::send(ByteView frame) {
    std::size_t sent = 0;
    while (sent < frame.size()) {
        constexpr std::size_t MaxChunk = 0x4000;
        const auto chunk = static_cast<int>(std::min(frame.size() - sent, MaxChunk));
        const auto wrote =
            ::send(socket_,
                   reinterpret_cast<const char*>(
                       frame.data() + sent), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                   chunk, 0);
        if (wrote == SOCKET_ERROR || wrote <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(wrote);
    }
    return true;
}

SduResult SduSocket::takeFrame(LinkPhase phase) {
    std::array<std::uint8_t, 10> head{};
    const auto head_len = std::min(buffer_.size(), head.size());
    std::copy_n(buffer_.begin(), head_len, head.begin());
    const auto prefix = inspectFramePrefix(std::span(head).first(head_len), phase);
    if (prefix.status == PrefixStatus::Invalid) {
        return {.status = TransportStatus::Unframeable, .frame = {}};
    }
    if (prefix.status == PrefixStatus::NeedMore || prefix.frame_size > MaximumSduSize ||
        buffer_.size() < prefix.frame_size) {
        if (prefix.status == PrefixStatus::Ready && prefix.frame_size > MaximumSduSize) {
            return {.status = TransportStatus::Unframeable, .frame = {}};
        }
        return {.status = TransportStatus::Ok, .frame = {}}; // need more bytes
    }
    Bytes frame(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(prefix.frame_size));
    buffer_.erase(buffer_.begin(),
                  buffer_.begin() + static_cast<std::ptrdiff_t>(prefix.frame_size));
    return {.status = TransportStatus::Ok, .frame = std::move(frame)};
}

TransportStatus SduSocket::fill(std::chrono::milliseconds timeout) {
    fd_set readable;
    FD_ZERO(&readable);
    FD_SET(socket_, &readable);
    constexpr long MicrosPerMilli = 1000;
    constexpr long MicrosPerSecond = 1000000;
    const auto micros = timeout.count() * MicrosPerMilli;
    timeval tv{.tv_sec = static_cast<long>(micros / MicrosPerSecond),
               .tv_usec = static_cast<long>(micros % MicrosPerSecond)};
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
    const auto* const first = reinterpret_cast<const std::uint8_t*>(
        chunk.data()); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    buffer_.insert(buffer_.end(), first, first + read);
    return TransportStatus::Ok;
}

SduResult SduSocket::receive(LinkPhase phase, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    for (;;) {
        auto framed = takeFrame(phase);
        if (framed.status != TransportStatus::Ok || !framed.frame.empty()) {
            return framed;
        }
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        if (remaining <= std::chrono::milliseconds::zero()) {
            return {.status = TransportStatus::TimedOut, .frame = {}};
        }
        const auto filled = fill(remaining);
        if (filled != TransportStatus::Ok) {
            return {.status = filled, .frame = {}};
        }
    }
}
} // namespace mgl
