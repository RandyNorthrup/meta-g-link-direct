// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/setup.hpp"

#include "mgl/crypto.hpp"
#include "mgl/identity.hpp"
#include "mgl/provision.hpp"
#include "mgl/rfcomm_transport.hpp"
#include "mgl/session.hpp"

#include <algorithm>
#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace mgl::setup {
namespace {
// Splits the sealed token blob into the HW token and the AR app tokens to try. An explicit
// "AR|<id>|<secret>" line is used as-is; a bare "<id>|<secret>" line is an AR candidate tried with
// the "AR|" prefix. Imported tokens override the built-in Meta AI app defaults.
void parseTokenBlob(const std::string& blob, std::string& hw_token,
                    std::vector<std::string>& ar_tokens) {
    std::istringstream lines(blob);
    std::string line;
    while (std::getline(lines, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        if (line.starts_with("HW|")) {
            hw_token = line;
        } else if (line.starts_with("AR|")) {
            ar_tokens.push_back(line);
        } else {
            ar_tokens.push_back("AR|" + line);
        }
    }
}

Outcome stopped(Outcome outcome, std::string problem) {
    outcome.problem = std::move(problem);
    return outcome;
}
} // namespace

RfcommOpen openGlassesRfcomm() {
    constexpr int Attempts = 6;
    constexpr auto Pause = std::chrono::seconds(3);
    RfcommOpen opened;
    for (int attempt = 0; attempt < Attempts; ++attempt) {
        opened = openBondedMetaRfcomm();
        if (opened.transport) {
            break;
        }
        std::this_thread::sleep_for(Pause);
    }
    return opened;
}

Outcome changeOwner(const std::string& auth_code) {
    Outcome outcome;
    // Imported tokens come first; the built-in Meta AI app tokens fill whatever the store lacks,
    // so setup works without --import-tokens.
    std::string hw_token;
    std::vector<std::string> ar_tokens;
    if (const auto blob = identity::loadTokenBlob()) {
        parseTokenBlob(*blob, hw_token, ar_tokens);
    }
    if (hw_token.empty()) {
        hw_token = provision::DefaultHwAppToken;
    }
    if (std::ranges::find(ar_tokens, provision::DefaultArAppToken) == ar_tokens.end()) {
        ar_tokens.emplace_back(provision::DefaultArAppToken);
    }

    outcome.phase = Phase::Auth;
    const auto auth = provision::authenticate(auth_code, ar_tokens);
    if (!auth.error.empty()) {
        outcome.auth_error = auth.error;
        return stopped(std::move(outcome), "Meta did not accept the login (" + auth.error +
                                               "). Try again with a fresh login.");
    }

    outcome.phase = Phase::Connect;
    const auto opened = openGlassesRfcomm();
    if (!opened.transport) {
        outcome.rfcomm = opened.status;
        return stopped(std::move(outcome),
                       "This PC could not open a Bluetooth connection to the glasses. Check that "
                       "they are paired with this PC, charged and out of the case, and that your "
                       "phone's Bluetooth is off.");
    }

    outcome.phase = Phase::Handshake;
    GlassesSession session(*opened.transport, nullptr);
    outcome.handshake = session.openHandshake().stage;
    if (outcome.handshake != LinkStage::KeysDerived) {
        return stopped(std::move(outcome),
                       "The glasses did not finish the encrypted Bluetooth handshake. Put them in "
                       "the case for a few seconds, take them out and try again.");
    }

    outcome.phase = Phase::ChangeOwner;
    const auto owner = crypto::OwnerKey::generate();
    outcome.change_owner = provision::runChangeOwner(session, owner, hw_token, auth.token);
    if (!outcome.change_owner.ok) {
        std::string problem = "The change of owner stopped: " + outcome.change_owner.error + ".";
        return stopped(std::move(outcome), std::move(problem));
    }

    outcome.phase = Phase::SaveIdentity;
    identity::OwnerIdentity sealed;
    sealed.owner_private = owner.exportPrivate();
    sealed.serial = outcome.change_owner.serial;
    sealed.user_id = outcome.change_owner.user_id;
    if (!identity::saveOwnerIdentity(sealed)) {
        return stopped(std::move(outcome), "The glasses now belong to this PC, but the identity "
                                           "could not be saved on this PC. Run the setup again.");
    }
    outcome.phase = Phase::Done;
    outcome.ok = true;
    return outcome;
}
} // namespace mgl::setup
