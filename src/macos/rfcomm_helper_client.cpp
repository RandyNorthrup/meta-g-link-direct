// SPDX-License-Identifier: AGPL-3.0-only
// The RFCOMM transport on macOS: starts Meta G Link Helper and speaks to the glasses through its
// Unix socket (see rfcomm_helper.mm for why the Bluetooth connection lives there).
#include "fd_transport.hpp"
#include "helper_client.hpp"
#include "helper_protocol.hpp"
#include "mgl/rfcomm_transport.hpp"
#include "mgl/transport.hpp"

#include <cerrno>
#include <cstdint>
#include <memory>

namespace mgl {
RfcommOpen openBondedMetaRfcomm() {
    RfcommOpen result;
    int error = 0;
    helper::Descriptor peer = helper::startHelper(helper::BluetoothRole, error);
    if (!peer.valid()) {
        result.status = error == ENOENT ? RfcommConnect::NoRadio : RfcommConnect::ConnectFailed;
        result.detail = static_cast<std::uint32_t>(error);
        return result;
    }
    const auto answer = helper::readAnswer(peer.get());
    if (!answer) {
        result.status = RfcommConnect::ConnectFailed;
        result.detail = static_cast<std::uint32_t>(ETIMEDOUT);
        return result;
    }
    result.detail = static_cast<std::uint32_t>(answer->detail);
    if (answer->status == helper::StatusOk) {
        result.status = RfcommConnect::Ok;
        result.transport = std::make_unique<FdTransport>(peer.release());
    } else if (answer->status == helper::StatusNoMetaService) {
        result.status = RfcommConnect::NoMetaService;
    } else if (answer->status == helper::StatusAmbiguous) {
        result.status = RfcommConnect::AmbiguousService;
    } else if (answer->status == helper::StatusNoPermission) {
        result.status = RfcommConnect::NoRadio; // Bluetooth is not allowed for the helper
    } else {
        result.status = RfcommConnect::ConnectFailed;
    }
    return result;
}
} // namespace mgl
