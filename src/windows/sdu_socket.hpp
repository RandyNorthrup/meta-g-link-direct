// SPDX-License-Identifier: AGPL-3.0-only
// Shared Winsock whole-SDU transport: owns a connected socket and cuts the byte stream into SDUs
// with inspectFramePrefix for the current link phase. RFCOMM and TCP transports both use it.
#ifndef MGL_WINDOWS_SDU_SOCKET_HPP
#define MGL_WINDOWS_SDU_SOCKET_HPP

#include "mgl/bytes.hpp"
#include "mgl/sdu.hpp"
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

// Owns a connected socket and buffers partial reads into whole SDUs.
class SduSocket final : public Transport {
  public:
    SduSocket(WinsockLibrary library, SOCKET socket);
    SduSocket(const SduSocket&) = delete;
    SduSocket& operator=(const SduSocket&) = delete;
    SduSocket(SduSocket&&) = delete;
    SduSocket& operator=(SduSocket&&) = delete;
    ~SduSocket() override;

    [[nodiscard]] bool send(ByteView frame) override;
    [[nodiscard]] SduResult receive(LinkPhase phase, std::chrono::milliseconds timeout) override;
    void close() override;

  private:
    WinsockLibrary library_;
    SOCKET socket_;
    Bytes buffer_;

    [[nodiscard]] SduResult takeFrame(LinkPhase phase);
    [[nodiscard]] TransportStatus fill(std::chrono::milliseconds timeout);
};
} // namespace mgl
#endif
