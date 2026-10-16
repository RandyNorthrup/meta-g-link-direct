// SPDX-License-Identifier: AGPL-3.0-only
#ifndef MGL_RFCOMM_HPP
#define MGL_RFCOMM_HPP

#include <cstdint>

namespace mgl {
enum class DirectRfcommRoute : std::uint8_t { ServiceUuid, DocumentedChannel7 };
struct DirectRfcommResult {
    std::uint32_t socket_error;
    std::uint32_t cleanup_error;
    std::uint32_t received_bytes;
    std::uint32_t declared_frame_bytes;
    bool connection_attempted;
    bool connected;
    bool timed_out;
    bool frame_received;
    bool reference_hello_prefix;
};

// Explicit single trustworthy target, no OS bond prerequisite and no application writes.
// Nonblocking connect/receive deadlines. One bounded frame, no raw output/storage.
[[nodiscard]] DirectRfcommResult probeDirectRfcomm(std::uint64_t private_address,
                                                   DirectRfcommRoute route);
} // namespace mgl
#endif
