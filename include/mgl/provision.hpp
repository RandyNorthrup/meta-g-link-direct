// SPDX-License-Identifier: AGPL-3.0-only
// The Meta-authenticated change-owner setup. Ported in behaviour from Visor core/identity token/
// and provision/ at the revision in THIRD_PARTY_NOTICES.md. Meta is contacted only here, during
// setup; after it the streaming path makes no Meta call.
#ifndef MGL_PROVISION_HPP
#define MGL_PROVISION_HPP

#include "mgl/crypto.hpp"
#include "mgl/session.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mgl::provision {
// The Meta AI app's client tokens, "<kind>|<app id>|<client secret>". They identify the Meta AI
// app, not a person: every copy of that app carries them, and they are already public (for
// example github.com/callbacked/kinesis, Sources/Kinesis/MetaAuth.swift). They are only defaults:
// a token store imported with --import-tokens takes precedence, so a rotation by Meta needs no new
// build. User tokens, owner keys and passwords are never embedded.
inline constexpr std::string_view DefaultHwAppToken =
    "HW|1312539125771114|98588f106d5d542adbf590619ca071fe"; // gitleaks:allow
inline constexpr std::string_view DefaultArAppToken =
    "AR|306760944872162|a919421a55a8ea18080ab2f10f57f1be"; // gitleaks:allow

// Where the Meta login sends the browser when it finishes, with the auth code and the state in the
// query. A window hosting the login recognises the end of it by this address.
inline constexpr std::string_view LoginRedirectUri =
    "https://accounts.meta.com/native/oidc/redirect/metaai";

// The browser login half.
struct Authorize {
    std::string url;   // the accounts.meta.com page to open
    std::string state; // the CSRF state carried in the redirect
};
[[nodiscard]] Authorize authorizeRequest();

// Exchange the redirect's auth code for the AR-universe user token (ARAe). nullopt with a reason
// on failure.
struct AuthResult {
    std::string token; // the ARAe, used as user_access_token on the hardware calls
    std::string error;
};

// code -> FRLA (token-exchange) -> ARAe (ar.graph.meta.com/login). The AR login needs the Meta AR
// app token as its OAuth bearer; it is not embedded in this repo but supplied by the caller from
// the local token store. Each candidate is tried until one is accepted, so a store may hold more
// than one. nullopt/error if none works or the list is empty.
[[nodiscard]] AuthResult authenticate(const std::string& auth_code,
                                      const std::vector<std::string>& ar_app_tokens);

enum class ProvisionStage : std::uint8_t {
    NotStarted,
    Identity,
    SkipChallenge,
    PairRequest,
    StartChangeOwner,
    Pair,
    FinishChangeOwner,
    Done
};

struct ProvisionResult {
    ProvisionStage stage = ProvisionStage::NotStarted;
    bool ok = false;
    std::string serial;
    std::string user_id;
    std::string error;
    std::uint16_t glasses_status = 0; // last svc36 status code, if the glasses rejected a step
};

// Runs the svc36 change-owner exchange over a session whose handshake already reached KeysDerived.
// owner is the P-256 key this computer will install as the new owner. hw_token is the Meta HW app
// token; user_token is the ARAe from authenticate(). On success the owner key now owns the glasses.
[[nodiscard]] ProvisionResult runChangeOwner(GlassesSession& session, const crypto::OwnerKey& owner,
                                             const std::string& hw_token,
                                             const std::string& user_token);
} // namespace mgl::provision
#endif
