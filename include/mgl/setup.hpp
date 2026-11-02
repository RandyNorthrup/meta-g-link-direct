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
#include <functional>
#include <optional>
#include <string>
#include <thread>

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

// What the address a finished login ended on holds.
struct LoginAddress {
    std::optional<std::string> code; // the auth code, when the address belongs to this login
    std::string problem;             // for people, when it holds no usable code
};

// Reads the address a login ended on. An address carrying another login's state is refused.
[[nodiscard]] LoginAddress readLoginAddress(const std::string& address,
                                            const std::string& expected_state);

// The glasses' Meta RFCOMM service, retried a few times: the glasses accept one session at a time
// and need a moment between them.
[[nodiscard]] RfcommOpen openGlassesRfcomm();

// Runs the whole change of owner and seals the new identity. Blocking; it can take a minute. An
// internal error comes back as a problem in the outcome, so a worker thread can run it as it is.
[[nodiscard]] Outcome changeOwner(const std::string& auth_code);

// One changeOwner run on its own thread, for a window that must keep answering meanwhile. The
// destructor waits for a run still going: a change of owner must not be cut off half way.
class OwnerChange {
  public:
    OwnerChange() = default;
    OwnerChange(const OwnerChange&) = delete;
    OwnerChange& operator=(const OwnerChange&) = delete;
    OwnerChange(OwnerChange&&) = delete;
    OwnerChange& operator=(OwnerChange&&) = delete;
    ~OwnerChange();

    // Starts a run, after any earlier one has ended. finished is called on the run's thread as it
    // ends, to tell the window; the window then calls finish on its own thread.
    void start(std::string auth_code, std::function<void()> finished);
    // Waits for the run to end and returns its outcome.
    [[nodiscard]] Outcome finish();

  private:
    std::thread thread_;
    Outcome outcome_; // written by the run, read after it is joined
};
} // namespace mgl::setup
#endif
