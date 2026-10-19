// SPDX-License-Identifier: AGPL-3.0-only
// A synchronous Bluetooth RFCOMM transport to the bonded glasses. Resolves the Meta service on
// the paired device, connects an authenticated/encrypted socket, and delivers whole SDUs.
#ifndef MGL_RFCOMM_TRANSPORT_HPP
#define MGL_RFCOMM_TRANSPORT_HPP

#include "mgl/transport.hpp"

#include <cstdint>
#include <memory>

namespace mgl {
enum class RfcommConnect : std::uint8_t {
    Ok,
    NoRadio,
    NoMetaService,    // no Windows-registered Meta RFCOMM service on a paired device
    AmbiguousService, // more than one paired Meta service; refuse rather than guess
    ConnectFailed     // the socket did not open
};

struct RfcommOpen {
    RfcommConnect status = RfcommConnect::ConnectFailed;
    std::uint32_t detail = 0; // Winsock/HRESULT error when status is ConnectFailed/NoRadio
    std::unique_ptr<Transport> transport;
};

// Opens the single bonded Meta RFCOMM service. The peer address stays inside the implementation.
[[nodiscard]] RfcommOpen openBondedMetaRfcomm();
} // namespace mgl
#endif
