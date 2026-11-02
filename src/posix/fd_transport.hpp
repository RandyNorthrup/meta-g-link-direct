// SPDX-License-Identifier: AGPL-3.0-only
// The POSIX whole-SDU transport: owns a connected socket; BufferedTransport cuts its byte stream
// into SDUs. RFCOMM (on Linux) and TCP both use it.
#ifndef MGL_POSIX_FD_TRANSPORT_HPP
#define MGL_POSIX_FD_TRANSPORT_HPP

#include "mgl/buffered_transport.hpp"
#include "mgl/bytes.hpp"
#include "mgl/transport.hpp"

#include <chrono>

namespace mgl {
class FdTransport final : public BufferedTransport {
  public:
    explicit FdTransport(int descriptor);
    FdTransport(const FdTransport&) = delete;
    FdTransport& operator=(const FdTransport&) = delete;
    FdTransport(FdTransport&&) = delete;
    FdTransport& operator=(FdTransport&&) = delete;
    ~FdTransport() override;

    [[nodiscard]] bool send(ByteView frame) override;
    void close() override;

  protected:
    [[nodiscard]] TransportStatus fill(std::chrono::milliseconds timeout) override;

  private:
    int descriptor_;
};

// Connects a non-blocking socket within timeout and returns it to blocking mode. 0 on success,
// otherwise the errno.
[[nodiscard]] int connectWithin(int descriptor, const void* address, unsigned address_size,
                                std::chrono::milliseconds timeout);
} // namespace mgl
#endif
