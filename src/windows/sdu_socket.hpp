// SPDX-License-Identifier: AGPL-3.0-only
// The Winsock whole-SDU transport: owns a connected socket; BufferedTransport cuts its byte stream
// into SDUs. RFCOMM and TCP transports both use it.
#ifndef MGL_WINDOWS_SDU_SOCKET_HPP
#define MGL_WINDOWS_SDU_SOCKET_HPP

#include "mgl/buffered_transport.hpp"
#include "mgl/bytes.hpp"
#include "mgl/transport.hpp"

#include <winsock2.h>

#include <chrono>

namespace mgl {
// RAII for one process-wide Winsock initialisation reference, movable so a transport can own it.
class WinsockLibrary {
  public:
    WinsockLibrary();
    WinsockLibrary(WinsockLibrary&& other) noexcept;
    WinsockLibrary& operator=(WinsockLibrary&&) = delete;
    WinsockLibrary(const WinsockLibrary&) = delete;
    WinsockLibrary& operator=(const WinsockLibrary&) = delete;
    ~WinsockLibrary();
    [[nodiscard]] int error() const noexcept { return startup_error_; }

  private:
    int startup_error_;
    bool owns_ = true;
};

// Owns a connected socket.
class SduSocket final : public BufferedTransport {
  public:
    SduSocket(WinsockLibrary library, SOCKET socket);
    SduSocket(const SduSocket&) = delete;
    SduSocket& operator=(const SduSocket&) = delete;
    SduSocket(SduSocket&&) = delete;
    SduSocket& operator=(SduSocket&&) = delete;
    ~SduSocket() override;

    [[nodiscard]] bool send(ByteView frame) override;
    void close() override;

  protected:
    [[nodiscard]] TransportStatus fill(std::chrono::milliseconds timeout) override;

  private:
    WinsockLibrary library_;
    SOCKET socket_;
};
} // namespace mgl
#endif
