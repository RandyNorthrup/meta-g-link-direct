// SPDX-License-Identifier: AGPL-3.0-only
// A byte-stream link to the glasses, cut into whole SDUs. The interface is portable; the
// implementations are platform transports (Bluetooth RFCOMM now, Wi-Fi TCP later).
#ifndef MGL_TRANSPORT_HPP
#define MGL_TRANSPORT_HPP

#include "mgl/bytes.hpp"
#include "mgl/sdu.hpp"

#include <chrono>
#include <optional>

namespace mgl {
enum class TransportStatus : std::uint8_t {
    Ok,
    Closed,     // the peer or the stack closed the link
    TimedOut,   // the deadline passed before a whole SDU arrived
    Unframeable // a first byte that neither framing can describe
};

struct SduResult {
    TransportStatus status = TransportStatus::Closed;
    Bytes frame;
};

// A whole-message byte pipe. Implementations read and cut SDUs using inspectFramePrefix for the
// current link phase, and never deliver a partial or over-long frame.
class Transport {
  public:
    Transport() = default;
    Transport(const Transport&) = delete;
    Transport& operator=(const Transport&) = delete;
    Transport(Transport&&) = delete;
    Transport& operator=(Transport&&) = delete;
    virtual ~Transport() = default;

    [[nodiscard]] virtual bool send(ByteView frame) = 0;
    [[nodiscard]] virtual SduResult receive(LinkPhase phase, std::chrono::milliseconds timeout) = 0;
    virtual void close() = 0;
};
} // namespace mgl
#endif
