// SPDX-License-Identifier: AGPL-3.0-only
// The part of a transport every byte stream shares: buffering partial reads and cutting them into
// whole SDUs with inspectFramePrefix for the current link phase. Platform transports add sending
// and reading from their kind of socket.
#ifndef MGL_BUFFERED_TRANSPORT_HPP
#define MGL_BUFFERED_TRANSPORT_HPP

#include "mgl/bytes.hpp"
#include "mgl/sdu.hpp"
#include "mgl/transport.hpp"

#include <chrono>

namespace mgl {
class BufferedTransport : public Transport {
  public:
    [[nodiscard]] SduResult receive(LinkPhase phase, std::chrono::milliseconds timeout) final;

  protected:
    // Waits up to timeout for bytes and appends what arrived with keep(). Ok when some did,
    // TimedOut when none did, Closed when the link ended.
    [[nodiscard]] virtual TransportStatus fill(std::chrono::milliseconds timeout) = 0;
    void keep(ByteView received);

  private:
    [[nodiscard]] SduResult takeFrame(LinkPhase phase);

    Bytes buffer_;
};
} // namespace mgl
#endif
