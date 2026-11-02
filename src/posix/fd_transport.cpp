// SPDX-License-Identifier: AGPL-3.0-only
#include "fd_transport.hpp"

#include "mgl/bytes.hpp"
#include "mgl/transport.hpp"

#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace mgl {
namespace {
constexpr int NoDescriptor = -1;

// A vanished peer is reported by the return value; it must not raise SIGPIPE in the host process.
#ifdef MSG_NOSIGNAL
constexpr int SendFlags = MSG_NOSIGNAL;
#else
constexpr int SendFlags = 0;
#endif

int setBlocking(int descriptor, bool blocking) {
    const int flags = fcntl(descriptor, F_GETFL, 0);
    if (flags < 0) {
        return errno;
    }
    const int updated = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return fcntl(descriptor, F_SETFL, updated) < 0 ? errno : 0;
}
} // namespace

FdTransport::FdTransport(int descriptor) : descriptor_(descriptor) {
#ifdef SO_NOSIGPIPE
    const int on = 1;
    (void)setsockopt(descriptor_, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
#endif
}

FdTransport::~FdTransport() { FdTransport::close(); }

void FdTransport::close() {
    if (descriptor_ != NoDescriptor) {
        ::close(descriptor_);
        descriptor_ = NoDescriptor;
    }
}

bool FdTransport::send(ByteView frame) {
    std::size_t sent = 0;
    while (sent < frame.size()) {
        const auto wrote = ::send(descriptor_, frame.data() + sent, frame.size() - sent, SendFlags);
        if (wrote < 0 && errno == EINTR) {
            continue;
        }
        if (wrote <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(wrote);
    }
    return true;
}

TransportStatus FdTransport::fill(std::chrono::milliseconds timeout) {
    pollfd readable{.fd = descriptor_, .events = POLLIN, .revents = 0};
    const int ready = poll(&readable, 1, static_cast<int>(timeout.count()));
    if (ready == 0) {
        return TransportStatus::TimedOut;
    }
    if (ready < 0) {
        return errno == EINTR ? TransportStatus::TimedOut : TransportStatus::Closed;
    }
    std::array<std::uint8_t, 4096> chunk{};
    const auto read = recv(descriptor_, chunk.data(), chunk.size(), 0);
    if (read < 0 && errno == EINTR) {
        return TransportStatus::TimedOut;
    }
    if (read <= 0) {
        return TransportStatus::Closed;
    }
    keep(ByteView(chunk.data(), static_cast<std::size_t>(read)));
    return TransportStatus::Ok;
}

int connectWithin(int descriptor, const void* address, unsigned address_size,
                  std::chrono::milliseconds timeout) {
    if (const int error = setBlocking(descriptor, false); error != 0) {
        return error;
    }
    // The socket C ABI type-erases its documented address structures.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    if (connect(descriptor, reinterpret_cast<const sockaddr*>(address), address_size) != 0) {
        if (errno != EINPROGRESS) {
            return errno;
        }
        pollfd pending{.fd = descriptor, .events = POLLOUT, .revents = 0};
        const int ready = poll(&pending, 1, static_cast<int>(timeout.count()));
        if (ready == 0) {
            return ETIMEDOUT;
        }
        if (ready < 0) {
            return errno;
        }
        int reported = 0;
        socklen_t size = sizeof(reported);
        if (getsockopt(descriptor, SOL_SOCKET, SO_ERROR, &reported, &size) != 0) {
            return errno;
        }
        if (reported != 0) {
            return reported;
        }
    }
    return setBlocking(descriptor, true);
}
} // namespace mgl
