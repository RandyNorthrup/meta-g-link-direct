// SPDX-License-Identifier: AGPL-3.0-only
// The one-time setup that makes this PC the glasses' owner, from the auth code of a Meta login:
// Meta authentication, the Bluetooth handshake, the svc36 change of owner, and sealing the new
// identity locally. The setup window and the setup tool's --provision both run it.
#ifndef MGL_SETUP_HPP
#define MGL_SETUP_HPP

#include "mgl/provision.hpp"
#include "mgl/rfcomm_transport.hpp"
#include "mgl/session.hpp"

#include <cstdint>
#include <string>

namespace mgl::setup {
// How far a setup run got.
enum class Phase : std::uint8_t { Auth, Connect, Handshake, ChangeOwner, SaveIdentity, Done };

struct Outcome {
    Phase phase = Phase::Auth;
    bool ok = false;
    std::string auth_error;                      // Meta's reason, when stopped at Auth
    RfcommConnect rfcomm = RfcommConnect::Ok;    // when stopped at Connect
    LinkStage handshake = LinkStage::NotStarted; // when stopped at Handshake
    provision::ProvisionResult change_owner;     // from ChangeOwner on
    std::string problem; // for people: where it stopped and what to do; empty on success
};

// The glasses' Meta RFCOMM service, retried a few times: the glasses accept one session at a time
// and need a moment between them.
[[nodiscard]] RfcommOpen openGlassesRfcomm();

// Runs the whole change of owner and seals the new identity. Blocking; it can take a minute.
[[nodiscard]] Outcome changeOwner(const std::string& auth_code);
} // namespace mgl::setup
#endif
