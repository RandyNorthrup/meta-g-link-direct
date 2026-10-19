// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/airshield.hpp"
#include "mgl/capabilities.hpp"
#include "mgl/crypto.hpp"
#include "mgl/discovery.hpp"
#include "mgl/hevc.hpp"
#include "mgl/identity.hpp"
#include "mgl/provision.hpp"
#include "mgl/rfcomm.hpp"
#include "mgl/rfcomm_transport.hpp"
#include "mgl/session.hpp"
#include "mgl/stream.hpp"
#include "mgl/tcp_transport.hpp"
#include "mgl/wifi_group.hpp"
#include "private_input.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {
int runDirectProbe(mgl::DirectRfcommRoute route) {
    const auto input = mgl::readPrivateAddress();
    mgl::DirectRfcommResult result{};
    if (input.value && input.error == 0) {
        result = mgl::probeDirectRfcomm(*input.value, route);
    }
    std::ostringstream output;
    output << R"({"address_input_valid":)" << (input.value ? "true" : "false")
           << R"(,"input_error":)" << input.error << R"(,"socket_error":)" << result.socket_error
           << R"(,"cleanup_error":)" << result.cleanup_error << R"(,"connection_attempted":)"
           << (result.connection_attempted ? "true" : "false") << R"(,"connected":)"
           << (result.connected ? "true" : "false") << R"(,"timed_out":)"
           << (result.timed_out ? "true" : "false") << R"(,"received_bytes":)"
           << result.received_bytes << R"(,"declared_frame_bytes":)" << result.declared_frame_bytes
           << R"(,"frame_received":)" << (result.frame_received ? "true" : "false")
           << R"(,"reference_hello_prefix":)" << (result.reference_hello_prefix ? "true" : "false")
           << R"(,"documented_channel7":)"
           << (route == mgl::DirectRfcommRoute::DocumentedChannel7 ? "true" : "false")
           << R"(,"glasses_authenticated":false})" << '\n';
    const auto json = output.str();
    std::cout << json;
    constexpr int DiagnosticIoFailed = 4;
    constexpr int InvalidInput = 5;
    constexpr int ProbeFailed = 3;
    if (!std::cout || !mgl::writeRequestedReport(json)) {
        return DiagnosticIoFailed;
    }
    if (input.error != 0) {
        return ProbeFailed;
    }
    if (!input.value) {
        return InvalidInput;
    }
    return result.socket_error == 0 && result.cleanup_error == 0 && result.connected &&
                   result.frame_received
               ? EXIT_SUCCESS
               : ProbeFailed;
}

const char* stageWord(mgl::LinkStage stage) {
    switch (stage) {
    case mgl::LinkStage::NotStarted:
        return "not_started";
    case mgl::LinkStage::Connected:
        return "connected";
    case mgl::LinkStage::HandshakeSent:
        return "handshake_sent";
    case mgl::LinkStage::KeysDerived:
        return "keys_derived";
    case mgl::LinkStage::TrustAccepted:
        return "trust_accepted";
    case mgl::LinkStage::Ready:
        return "ready";
    }
    return "unknown";
}

const char* errorWord(mgl::LinkError error) {
    switch (error) {
    case mgl::LinkError::None:
        return "none";
    case mgl::LinkError::TransportFailed:
        return "transport_failed";
    case mgl::LinkError::HandshakeIncomplete:
        return "handshake_incomplete";
    case mgl::LinkError::BadPeerKey:
        return "bad_peer_key";
    case mgl::LinkError::FrameRejected:
        return "frame_rejected";
    case mgl::LinkError::TrustRejected:
        return "trust_rejected";
    case mgl::LinkError::EndLinkSetupMissing:
        return "end_link_setup_missing";
    }
    return "unknown";
}

const char* connectWord(mgl::RfcommConnect status) {
    switch (status) {
    case mgl::RfcommConnect::Ok:
        return "ok";
    case mgl::RfcommConnect::NoRadio:
        return "no_radio";
    case mgl::RfcommConnect::NoMetaService:
        return "no_meta_service";
    case mgl::RfcommConnect::AmbiguousService:
        return "ambiguous_service";
    case mgl::RfcommConnect::ConnectFailed:
        return "connect_failed";
    }
    return "unknown";
}

// Proves the AirShield handshake and key schedule against the bonded glasses, with no owner key
// and so no trust exchange. Reaching keys_derived confirms the crypto/framing port; reaching ready
// confirms EndLinkSetup. It changes no ownership and sends no application frames.
int runHandshakeProbe() {
    constexpr int ProbeFailed = 3;
    constexpr int NoRegisteredMetaService = 6;
    auto opened = mgl::openBondedMetaRfcomm();
    mgl::LinkOutcome outcome;
    if (opened.transport) {
        mgl::GlassesSession session(*opened.transport, nullptr);
        outcome = session.open();
    }
    std::ostringstream output;
    output << R"({"rfcomm_connect":")" << connectWord(opened.status) << R"(","connect_detail":)"
           << opened.detail << R"(,"stage":")" << stageWord(outcome.stage) << R"(","link_error":")"
           << errorWord(outcome.error) << R"(","reached_keys":)"
           << (outcome.stage >= mgl::LinkStage::KeysDerived ? "true" : "false")
           << R"(,"reached_ready":)" << (outcome.stage == mgl::LinkStage::Ready ? "true" : "false")
           << R"(,"glasses_authenticated":false})" << '\n';
    const auto json = output.str();
    std::cout << json;
    if (!std::cout) {
        return ProbeFailed;
    }
    if (opened.status == mgl::RfcommConnect::NoMetaService) {
        return NoRegisteredMetaService;
    }
    if (opened.status != mgl::RfcommConnect::Ok) {
        return ProbeFailed;
    }
    return outcome.stage >= mgl::LinkStage::KeysDerived ? EXIT_SUCCESS : ProbeFailed;
}

mgl::ByteView viewOf(const mgl::Bytes& bytes) { return {bytes.data(), bytes.size()}; }

bool envFlag(const char* name) {
    char* value = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(&value, &size, name) != 0 || value == nullptr) {
        return false;
    }
    std::free(value);
    return true;
}

const char* provisionStageWord(mgl::provision::ProvisionStage stage) {
    switch (stage) {
    case mgl::provision::ProvisionStage::NotStarted:
        return "not_started";
    case mgl::provision::ProvisionStage::Identity:
        return "identity";
    case mgl::provision::ProvisionStage::SkipChallenge:
        return "skip_challenge";
    case mgl::provision::ProvisionStage::PairRequest:
        return "pair_request";
    case mgl::provision::ProvisionStage::StartChangeOwner:
        return "start_change_owner";
    case mgl::provision::ProvisionStage::Pair:
        return "pair";
    case mgl::provision::ProvisionStage::FinishChangeOwner:
        return "finish_change_owner";
    case mgl::provision::ProvisionStage::Done:
        return "done";
    }
    return "unknown";
}

std::string jsonEscape(std::string_view text) {
    std::string out;
    for (const char ch : text) {
        if (ch == '"' || ch == '\\') {
            out.push_back('\\');
            out.push_back(ch);
        } else if (static_cast<unsigned char>(ch) >= 0x20) {
            out.push_back(ch);
        }
    }
    return out;
}

// Prints the Meta login URL to open in a browser. The redirect it ends at carries the auth code.
int runAuthUrl() {
    const auto request = mgl::provision::authorizeRequest();
    std::cout << request.url << '\n';
    return std::cout ? EXIT_SUCCESS : EXIT_FAILURE;
}

// Reads the extracted Meta token text from stdin and seals it locally. No token is printed back.
int runImportTokens() {
    std::ostringstream buffer;
    buffer << std::cin.rdbuf();
    const auto text = buffer.str();
    if (text.empty()) {
        std::cerr << "No token text on stdin.\n";
        return 5;
    }
    if (!mgl::identity::saveTokenBlob(text)) {
        std::cerr << "Could not seal the token store.\n";
        return 3;
    }
    std::cout << R"({"tokens_stored":true})" << '\n';
    return std::cout ? EXIT_SUCCESS : EXIT_FAILURE;
}

// Splits the sealed token blob into the HW token and the AR app tokens to try. An explicit
// "AR|<id>|<secret>" line is used as-is; a bare "<id>|<secret>" line is an AR candidate tried with
// the "AR|" prefix. No token is ever embedded in this repo; the store supplies them.
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
        if (line.rfind("HW|", 0) == 0) {
            hw_token = line;
        } else if (line.rfind("AR|", 0) == 0) {
            ar_tokens.push_back(line);
        } else {
            ar_tokens.push_back("AR|" + line);
        }
    }
}

// The one-time Meta-authenticated change of owner. Reads the login auth code on stdin.
int runProvision() {
    constexpr int ProbeFailed = 3;
    constexpr int InvalidInput = 5;
    const auto blob = mgl::identity::loadTokenBlob();
    if (!blob) {
        std::cerr << "No token store. Run --import-tokens first.\n";
        return ProbeFailed;
    }
    std::string hw_token;
    std::vector<std::string> ar_tokens;
    parseTokenBlob(*blob, hw_token, ar_tokens);
    if (hw_token.empty()) {
        std::cerr << "Token store has no HW token.\n";
        return ProbeFailed;
    }
    if (ar_tokens.empty()) {
        std::cerr << "Token store has no AR app token (expected an \"AR|<id>|<secret>\" line).\n";
        return ProbeFailed;
    }
    std::string code;
    std::getline(std::cin, code);
    while (!code.empty() && (code.back() == '\r' || code.back() == '\n' || code.back() == ' ')) {
        code.pop_back();
    }
    if (code.empty()) {
        std::cerr << "No auth code on stdin.\n";
        return InvalidInput;
    }

    const auto auth = mgl::provision::authenticate(code, ar_tokens);
    if (!auth.error.empty()) {
        std::cout << R"({"phase":"auth","ok":false,"error":")" << jsonEscape(auth.error) << "\"}\n";
        return ProbeFailed;
    }

    // The glasses allow one RFCOMM session at a time and need a moment between them, so a connect
    // right after a previous attempt can fail transiently; retry a few times.
    mgl::RfcommOpen opened;
    for (int attempt = 0; attempt < 6; ++attempt) {
        opened = mgl::openBondedMetaRfcomm();
        if (opened.transport) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    if (!opened.transport) {
        std::cout << R"({"phase":"connect","ok":false,"rfcomm_connect":")"
                  << connectWord(opened.status) << "\"}\n";
        return ProbeFailed;
    }
    mgl::GlassesSession session(*opened.transport, nullptr);
    const auto handshake = session.openHandshake();
    if (handshake.stage != mgl::LinkStage::KeysDerived) {
        std::cout << R"({"phase":"handshake","ok":false,"stage":")" << stageWord(handshake.stage)
                  << "\"}\n";
        return ProbeFailed;
    }

    const auto owner = mgl::crypto::OwnerKey::generate();
    const auto result = mgl::provision::runChangeOwner(session, owner, hw_token, auth.token);
    std::cout << R"({"phase":"change_owner","ok":)" << (result.ok ? "true" : "false")
              << R"(,"stage":")" << provisionStageWord(result.stage) << R"(","glasses_status":)"
              << result.glasses_status << R"(,"serial":")" << jsonEscape(result.serial)
              << R"(","error":")" << jsonEscape(result.error) << "\"}\n";
    if (!result.ok) {
        return ProbeFailed;
    }
    mgl::identity::OwnerIdentity identity;
    identity.owner_private = owner.exportPrivate();
    identity.serial = result.serial;
    identity.user_id = result.user_id;
    if (!mgl::identity::saveOwnerIdentity(identity)) {
        std::cerr << "Owner changed but the identity could not be sealed locally.\n";
        return ProbeFailed;
    }
    return std::cout ? EXIT_SUCCESS : ProbeFailed;
}

const char* wifiWord(mgl::WifiGroupStatus status) {
    switch (status) {
    case mgl::WifiGroupStatus::Ok:
        return "ok";
    case mgl::WifiGroupStatus::NoAdapterSupport:
        return "no_adapter_support";
    case mgl::WifiGroupStatus::StartFailed:
        return "start_failed";
    case mgl::WifiGroupStatus::Aborted:
        return "aborted";
    }
    return "unknown";
}

// Brings up the Windows Wi-Fi Direct group owner the glasses would join, to prove the adapter can
// host it. Holds the group briefly, then stops. No glasses interaction.
int runWifi() {
    mgl::WifiGroup group;
    const auto owner = mgl::WifiGroupOwner::start(group);
    std::cout << R"({"wifi_group":")" << wifiWord(group.status) << R"(","detail":)" << group.detail
              << R"(,"ssid":")" << jsonEscape(group.ssid) << R"(","passphrase_len":)"
              << group.passphrase.size() << R"(,"up":)" << (owner ? "true" : "false") << "}\n";
    if (owner) {
        constexpr int HoldSeconds = 40;
        std::this_thread::sleep_for(std::chrono::seconds(HoldSeconds));
    }
    constexpr int ProbeFailed = 3;
    return (owner && std::cout) ? EXIT_SUCCESS : ProbeFailed;
}

// Opens a full authenticated link using the stored owner identity: handshake, trust with the
// owner key, EndLinkSetup. Reaching trust_accepted/ready proves the PC owns the glasses.
int runLink() {
    constexpr int ProbeFailed = 3;
    const auto identity = mgl::identity::loadOwnerIdentity();
    if (!identity) {
        std::cerr << "No stored owner identity. Run --provision first.\n";
        return ProbeFailed;
    }
    const auto owner = mgl::crypto::OwnerKey::importPrivate(
        mgl::ByteView(identity->owner_private.data(), identity->owner_private.size()));
    if (!owner) {
        std::cerr << "Stored owner key is unreadable.\n";
        return ProbeFailed;
    }
    mgl::RfcommOpen opened;
    for (int attempt = 0; attempt < 6; ++attempt) {
        opened = mgl::openBondedMetaRfcomm();
        if (opened.transport) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    if (!opened.transport) {
        std::cout << R"({"rfcomm_connect":")" << connectWord(opened.status) << R"(","ok":false})"
                  << '\n';
        return ProbeFailed;
    }
    mgl::GlassesSession session(*opened.transport, &*owner);
    const auto outcome = session.open();
    std::cout << R"({"stage":")" << stageWord(outcome.stage) << R"(","link_error":")"
              << errorWord(outcome.error) << R"(","trust_tx_ok":)"
              << (outcome.trust_tx_ok ? "true" : "false") << R"(,"trust_rx_ok":)"
              << (outcome.trust_rx_ok ? "true" : "false") << R"(,"trust_status":)"
              << outcome.trust_status << R"(,"ready":)"
              << (outcome.stage == mgl::LinkStage::Ready ? "true" : "false") << "}\n";
    if (!std::cout) {
        return ProbeFailed;
    }
    return outcome.stage == mgl::LinkStage::Ready ? EXIT_SUCCESS : ProbeFailed;
}

// Full path up to the Wi-Fi handover: authenticated BT link, host a Wi-Fi group, tell the glasses
// to join it, and report the TCP endpoint they return. Proves Windows-as-group-owner interop.
int runWifiJoin() {
    constexpr int ProbeFailed = 3;
    const auto identity = mgl::identity::loadOwnerIdentity();
    if (!identity) {
        std::cerr << "No stored owner identity. Run --provision first.\n";
        return ProbeFailed;
    }
    const auto owner = mgl::crypto::OwnerKey::importPrivate(
        mgl::ByteView(identity->owner_private.data(), identity->owner_private.size()));
    if (!owner) {
        std::cerr << "Stored owner key is unreadable.\n";
        return ProbeFailed;
    }
    mgl::WifiGroup group;
    const auto group_owner = mgl::WifiGroupOwner::start(group);
    if (!group_owner) {
        std::cout << R"({"phase":"wifi_group","wifi_group":")" << wifiWord(group.status)
                  << R"(","detail":)" << group.detail << "}\n";
        return ProbeFailed;
    }
    mgl::RfcommOpen opened;
    for (int attempt = 0; attempt < 6; ++attempt) {
        opened = mgl::openBondedMetaRfcomm();
        if (opened.transport) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    if (!opened.transport) {
        std::cout << R"({"phase":"connect","rfcomm_connect":")" << connectWord(opened.status)
                  << "\"}\n";
        return ProbeFailed;
    }
    mgl::GlassesSession session(*opened.transport, &*owner);
    const auto link = session.open();
    if (link.stage != mgl::LinkStage::Ready) {
        std::cout << R"({"phase":"link","stage":")" << stageWord(link.stage) << "\"}\n";
        return ProbeFailed;
    }
    // The glasses answer INTERNAL_ERROR (status 2) for 15-45 s after a Wi-Fi firmware restart, so
    // retry the join several times, a fresh v4 UUID each time (Visor uses eight tries).
    mgl::stream::JoinResult result;
    bool got = false;
    for (int attempt = 0; attempt < 8 && !result.ok(); ++attempt) {
        if (attempt > 0) {
            std::this_thread::sleep_for(std::chrono::seconds(4));
        }
        const auto uuid = mgl::airshield::randomUuid();
        const auto join = mgl::stream::buildJoinAccessPoint(group.ssid, group.passphrase, uuid,
                                                            mgl::stream::Bandwidth20MHz);
        if (!session.sendOnService(mgl::stream::ServiceAtc, mgl::stream::MtJoinAccessPoint,
                                   {join.data(), join.size()})) {
            std::cout << R"({"phase":"join_send","ok":false})" << "\n";
            return ProbeFailed;
        }
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(12);
        while (std::chrono::steady_clock::now() < deadline) {
            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::steady_clock::now());
            auto message = session.receiveRaw(remaining);
            if (!message) {
                break;
            }
            if (message->service == mgl::stream::ServiceAtc &&
                message->type == mgl::stream::MtJoinAccessPointResult) {
                if (const auto parsed = mgl::stream::parseJoinResult(
                        {message->payload.data(), message->payload.size()})) {
                    result = *parsed;
                    got = true;
                    break;
                }
            }
        }
    }
    std::cout << R"({"phase":"join","ssid":")" << jsonEscape(group.ssid) << R"(","answered":)"
              << (got ? "true" : "false") << R"(,"status":)" << result.status
              << R"(,"has_address":)" << (result.address.empty() ? "false" : "true")
              << R"(,"port":)" << result.port << R"(,"ok":)" << (result.ok() ? "true" : "false")
              << "}\n";
    return (got && result.ok() && std::cout) ? EXIT_SUCCESS : ProbeFailed;
}

namespace stream_probe {
// Sends the Wi-Fi join on the Bluetooth session and returns the glasses' TCP endpoint, retrying
// through the INTERNAL_ERROR window.
std::optional<mgl::stream::JoinResult> joinWifi(mgl::GlassesSession& session,
                                                const mgl::WifiGroup& group) {
    for (int attempt = 0; attempt < 8; ++attempt) {
        if (attempt > 0) {
            std::this_thread::sleep_for(std::chrono::seconds(4));
        }
        const auto uuid = mgl::airshield::randomUuid();
        const auto join = mgl::stream::buildJoinAccessPoint(group.ssid, group.passphrase, uuid,
                                                            mgl::stream::Bandwidth20MHz);
        if (!session.sendOnService(mgl::stream::ServiceAtc, mgl::stream::MtJoinAccessPoint,
                                   {join.data(), join.size()})) {
            return std::nullopt;
        }
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(12);
        while (std::chrono::steady_clock::now() < deadline) {
            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::steady_clock::now());
            auto message = session.receiveRaw(remaining);
            if (!message) {
                break;
            }
            if (message->service == mgl::stream::ServiceAtc &&
                message->type == mgl::stream::MtJoinAccessPointResult) {
                if (const auto parsed = mgl::stream::parseJoinResult(
                        {message->payload.data(), message->payload.size()})) {
                    if (parsed->ok()) {
                        return parsed;
                    }
                    break; // non-success status; retry
                }
            }
        }
    }
    return std::nullopt;
}
} // namespace stream_probe

// The full live media path: authenticated BT link, Wi-Fi handover, TCP session, start the camera
// stream, and receive real HEVC frames. Reports frame/keyframe counts and the decoded picture size.
int runStream() {
    constexpr int ProbeFailed = 3;
    const auto identity = mgl::identity::loadOwnerIdentity();
    if (!identity) {
        std::cerr << "No stored owner identity. Run --provision first.\n";
        return ProbeFailed;
    }
    const auto owner = mgl::crypto::OwnerKey::importPrivate(
        mgl::ByteView(identity->owner_private.data(), identity->owner_private.size()));
    if (!owner) {
        std::cerr << "Stored owner key is unreadable.\n";
        return ProbeFailed;
    }
    mgl::WifiGroup group;
    const auto group_owner = mgl::WifiGroupOwner::start(group);
    if (!group_owner) {
        std::cout << R"({"phase":"wifi_group","wifi_group":")" << wifiWord(group.status) << "\"}\n";
        return ProbeFailed;
    }

    mgl::stream::JoinResult join;
    {
        mgl::RfcommOpen bt;
        for (int attempt = 0; attempt < 6 && !bt.transport; ++attempt) {
            bt = mgl::openBondedMetaRfcomm();
            if (!bt.transport) {
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
        }
        if (!bt.transport) {
            std::cout << R"({"phase":"bt_connect","ok":false})" << "\n";
            return ProbeFailed;
        }
        mgl::GlassesSession bt_session(*bt.transport, &*owner);
        if (bt_session.open().stage != mgl::LinkStage::Ready) {
            std::cout << R"({"phase":"bt_link","ok":false})" << "\n";
            return ProbeFailed;
        }
        const auto result = stream_probe::joinWifi(bt_session, group);
        if (!result) {
            std::cout << R"({"phase":"wifi_join","ok":false})" << "\n";
            return ProbeFailed;
        }
        join = *result;
    } // the Bluetooth session is released; the glasses are on Wi-Fi now

    auto tcp = mgl::openTcp(join.address, static_cast<std::uint16_t>(join.port));
    if (!tcp.transport) {
        std::cout << R"({"phase":"tcp_connect","ok":false,"detail":)" << tcp.detail << "}\n";
        return ProbeFailed;
    }
    mgl::GlassesSession media(*tcp.transport, &*owner);
    const auto link = media.open();
    if (link.stage != mgl::LinkStage::Ready) {
        std::cout << R"({"phase":"tcp_link","stage":")" << stageWord(link.stage) << "\"}\n";
        return ProbeFailed;
    }

    const bool debug = envFlag("MGL_STREAM_DEBUG");
    // Reads messages until one of the given service/type arrives or the timeout passes, tracing
    // everything when debugging. Returns the matching payload.
    const auto awaitMedia = [&](std::uint16_t service, std::uint16_t type,
                                std::chrono::seconds timeout) -> std::optional<mgl::Bytes> {
        const auto until = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < until) {
            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                until - std::chrono::steady_clock::now());
            auto message = media.receiveRaw(remaining);
            if (!message) {
                return std::nullopt;
            }
            if (debug) {
                std::cerr << "[rx] svc=" << message->service << " type=" << message->type
                          << " err=" << (message->has_error ? message->error : 0)
                          << " len=" << message->payload.size() << "\n";
            }
            if (message->service == service && message->type == type) {
                return std::move(message->payload);
            }
        }
        return std::nullopt;
    };

    std::uint32_t nonce = 1;
    (void)media.sendOnService(mgl::stream::ServiceStatus, mgl::stream::MtInit,
                              viewOf(mgl::stream::buildSubscribe()));
    (void)media.sendOnService(mgl::stream::ServiceStatus, mgl::stream::MtStartApp,
                              viewOf(mgl::stream::buildStartApp(mgl::stream::AppLivestream)));
    const auto start_app = awaitMedia(mgl::stream::ServiceStatus, mgl::stream::MtStartAppResponse,
                                      std::chrono::seconds(7));
    if (debug) {
        std::cerr << "[start-app] " << (start_app ? "answered" : "no answer") << "\n";
    }
    (void)media.sendOnService(mgl::stream::ServiceMedia, mgl::stream::MtSessionSettings,
                              viewOf(mgl::stream::buildSessionSettings(3, /*bypass_don=*/true)));
    const mgl::stream::VideoConfig config{};
    (void)media.sendOnService(mgl::stream::ServiceMedia, mgl::stream::MtConfigureVideo,
                              viewOf(mgl::stream::buildConfigureVideo(nonce++, config)));
    (void)awaitMedia(mgl::stream::ServiceMedia, mgl::stream::MtConfigureVideoResponse,
                     std::chrono::seconds(5));
    (void)media.sendOnService(mgl::stream::ServiceMedia, mgl::stream::MtConfigureAudio,
                              viewOf(mgl::stream::buildConfigureAudio(nonce++, /*enabled=*/false)));
    (void)awaitMedia(mgl::stream::ServiceMedia, mgl::stream::MtConfigureAudioResponse,
                     std::chrono::seconds(5));
    (void)media.sendOnService(
        mgl::stream::ServiceMedia, mgl::stream::MtControlStream,
        viewOf(mgl::stream::buildControlStream(nonce++, mgl::stream::ActionStart)));
    (void)awaitMedia(mgl::stream::ServiceMedia, mgl::stream::MtControlStreamResponse,
                     std::chrono::seconds(5));

    // Receive and reassemble frames for a bounded window.
    mgl::Bytes frame;
    std::uint64_t frames = 0;
    std::uint64_t keyframes = 0;
    std::uint64_t video_bytes = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t video_error = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
    while (std::chrono::steady_clock::now() < deadline) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        auto message = media.receiveRaw(remaining);
        if (!message) {
            break;
        }
        if (message->service != mgl::stream::ServiceMedia) {
            continue;
        }
        const mgl::ByteView payload(message->payload.data(), message->payload.size());
        switch (message->type) {
        case mgl::stream::MtVideoChunk:
            mgl::append(frame, payload);
            break;
        case mgl::stream::MtVideoChunkEnd: {
            mgl::append(frame, payload);
            if (frame.size() > 8) {
                const mgl::ByteView annex(frame.data() + 8, frame.size() - 8);
                ++frames;
                video_bytes += annex.size();
                if (mgl::hevc::hasKeyframe(annex)) {
                    ++keyframes;
                }
                if (width == 0) {
                    if (const auto dims = mgl::hevc::spsDimensions(annex)) {
                        width = dims->width;
                        height = dims->height;
                    }
                }
            }
            frame.clear();
            break;
        }
        case mgl::stream::MtCodecConfig:
            if (const auto codec = mgl::stream::parseCodecConfig(payload)) {
                if (width == 0) {
                    if (const auto dims =
                            mgl::hevc::spsDimensions({codec->data.data(), codec->data.size()})) {
                        width = dims->width;
                        height = dims->height;
                    }
                }
            }
            break;
        case mgl::stream::MtConfigureVideoResponse:
        case mgl::stream::MtControlStreamResponse:
            if (const auto answer = mgl::stream::parseStreamAnswer(payload)) {
                if (answer->error != 0 && video_error == 0) {
                    video_error = answer->error;
                }
            }
            break;
        default:
            break;
        }
    }
    (void)media.sendOnService(
        mgl::stream::ServiceMedia, mgl::stream::MtControlStream,
        viewOf(mgl::stream::buildControlStream(nonce++, mgl::stream::ActionStop)));
    // Leave the access point so the glasses drop off cleanly for the next session.
    (void)media.sendOnService(mgl::stream::ServiceAtc, mgl::stream::MtLeaveAccessPoint, {});
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    std::cout << R"({"phase":"stream","frames":)" << frames << R"(,"keyframes":)" << keyframes
              << R"(,"video_bytes":)" << video_bytes << R"(,"width":)" << width << R"(,"height":)"
              << height << R"(,"stream_error":)" << video_error << R"(,"ok":)"
              << (frames > 0 ? "true" : "false") << "}\n";
    return (frames > 0 && std::cout) ? EXIT_SUCCESS : ProbeFailed;
}

void writeClassicReport(const mgl::ClassicDiscovery& classic) {
    std::cout << ",\"classic_devices\":" << classic.classic_devices
              << ",\"classic_paired_devices\":" << classic.classic_paired_devices
              << ",\"classic_error\":" << classic.classic_error
              << ",\"meta_rfcomm_services\":" << classic.meta_rfcomm_services
              << ",\"rfcomm_hresult\":" << classic.rfcomm_hresult
              << ",\"inquiry_completed\":" << (classic.inquiry_completed ? "true" : "false")
              << ",\"rfcomm_timed_out\":" << (classic.rfcomm_timed_out ? "true" : "false")
              << ",\"rfcomm_ambiguous\":" << (classic.rfcomm_ambiguous ? "true" : "false")
              << ",\"rfcomm_connection_attempted\":"
              << (classic.rfcomm_connection_attempted ? "true" : "false")
              << ",\"rfcomm_connected\":" << (classic.rfcomm_connected ? "true" : "false");
    std::cout << ",\"uncached_peer_queries\":" << classic.uncached_peer_queries
              << ",\"uncached_peers_resolved\":" << classic.uncached_peers_resolved
              << ",\"uncached_service_queries\":" << classic.uncached_service_queries
              << ",\"uncached_query_failures\":" << classic.uncached_query_failures
              << ",\"uncached_bluetooth_error\":" << classic.uncached_bluetooth_error
              << ",\"uncached_requested\":" << (classic.uncached_requested ? "true" : "false")
              << ",\"uncached_complete\":" << (classic.uncached_complete ? "true" : "false")
              << ",\"uncached_limit_reached\":"
              << (classic.uncached_limit_reached ? "true" : "false")
              << ",\"pairing_error\":" << classic.pairing_error << ",\"pairing_dialog_requested\":"
              << (classic.pairing_dialog_requested ? "true" : "false")
              << ",\"windows_bonded\":" << (classic.windows_bonded ? "true" : "false");
}

std::optional<mgl::ClassicMode> classicMode(std::string_view mode) {
    if (mode == "--pair-classic") {
        return mgl::ClassicMode::PairAndConnect;
    }
    if (mode == "--discover-classic") {
        return mgl::ClassicMode::RegisteredDiscovery;
    }
    if (mode == "--connect-classic") {
        return mgl::ClassicMode::RegisteredConnect;
    }
    if (mode == "--discover-classic-live") {
        return mgl::ClassicMode::LiveDiscovery;
    }
    if (mode == "--connect-classic-live") {
        return mgl::ClassicMode::LiveConnect;
    }
    return std::nullopt;
}

int classicExit(const mgl::ClassicDiscovery& classic, bool connect, int baseline_exit) {
    constexpr int ProbeFailed = 3;
    constexpr int NoRegisteredMetaService = 6;
    constexpr int AmbiguousMetaServices = 7;
    if (classic.classic_error != 0 || classic.rfcomm_hresult != 0 || classic.pairing_error != 0 ||
        (classic.pairing_dialog_requested && !classic.windows_bonded) ||
        (classic.uncached_requested && !classic.uncached_complete &&
         classic.meta_rfcomm_services == 0) ||
        (!connect && !classic.inquiry_completed) ||
        (connect && classic.rfcomm_connection_attempted && !classic.rfcomm_connected)) {
        return ProbeFailed;
    }
    if (classic.rfcomm_ambiguous) {
        return AmbiguousMetaServices;
    }
    return baseline_exit == EXIT_SUCCESS && classic.meta_rfcomm_services == 0
               ? NoRegisteredMetaService
               : baseline_exit;
}

int runProbe(std::string_view mode) {
    const auto capabilities = mgl::inspectWirelessCapabilities();
    std::cout << "{\"wifi_interfaces\":" << capabilities.wifi_interfaces
              << ",\"wifi_connected_interfaces\":" << capabilities.wifi_connected_interfaces
              << ",\"bluetooth_radio\":" << (capabilities.bluetooth_radio ? "true" : "false")
              << ",\"wifi_error\":" << capabilities.wifi_error
              << ",\"bluetooth_error\":" << capabilities.bluetooth_error
              << ",\"bluetooth_features_error\":" << capabilities.bluetooth_features_error
              << ",\"bluetooth_features_valid\":"
              << (capabilities.bluetooth_features_valid ? "true" : "false")
              << ",\"bluetooth_le_supported\":"
              << (capabilities.bluetooth_le_supported ? "true" : "false")
              << ",\"bluetooth_classic_supported\":"
              << (capabilities.bluetooth_classic_supported ? "true" : "false")
              << ",\"bluetooth_hci_version\":"
              << static_cast<unsigned>(capabilities.bluetooth_hci_version)
              << ",\"bluetooth_lmp_version\":"
              << static_cast<unsigned>(capabilities.bluetooth_lmp_version);
    constexpr int HardwareUnavailable = 2;
    constexpr int ProbeFailed = 3;
    int exit_code = capabilities.wifi_interfaces > 0 && capabilities.bluetooth_radio
                        ? EXIT_SUCCESS
                        : HardwareUnavailable;
    if (capabilities.wifi_error != 0 || capabilities.bluetooth_error != 0) {
        exit_code = ProbeFailed;
    }
    if (mode == "--discover") {
        const auto discovery = mgl::inspectBluetoothAdvertisements();
        std::cout << R"(,"scan_duration_seconds":10,"advertisement_reports":)"
                  << discovery.advertisement_reports
                  << ",\"meta_advertisement_reports\":" << discovery.meta_advertisement_reports
                  << ",\"discovery_hresult\":" << discovery.hresult
                  << ",\"discovery_bluetooth_error\":" << discovery.bluetooth_error
                  << ",\"scan_completed\":" << (discovery.scan_completed ? "true" : "false");
        constexpr int NoAdvertisingCandidate = 6;
        if (discovery.hresult != 0 || discovery.bluetooth_error != 0 || !discovery.scan_completed) {
            exit_code = ProbeFailed;
        } else if (exit_code == EXIT_SUCCESS && discovery.meta_advertisement_reports == 0) {
            exit_code = NoAdvertisingCandidate;
        }
    }
    if (const auto classic_mode = classicMode(mode)) {
        const bool connect = *classic_mode == mgl::ClassicMode::PairAndConnect ||
                             *classic_mode == mgl::ClassicMode::RegisteredConnect ||
                             *classic_mode == mgl::ClassicMode::LiveConnect;
        const auto classic = mgl::inspectClassicRfcomm(*classic_mode);
        writeClassicReport(classic);
        exit_code = classicExit(classic, connect, exit_code);
    }
    std::cout << ",\"glasses_connection_tested\":false}\n";
    return std::cout ? exit_code : ProbeFailed;
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--help") {
            std::cout
                << "Usage: meta-g-link-probe "
                   "[--discover|--discover-classic|--connect-classic|--discover-classic-live|--"
                   "connect-classic-live|--pair-classic|--direct-classic-uuid|--direct-classic-"
                   "channel7|--handshake|--help]\n"
                   "Default: local radio/association counts only.\n"
                   "--handshake: open the bonded Meta RFCOMM service and run the AirShield "
                   "handshake only (no owner key, no trust, no ownership change); reports the "
                   "stage reached.\n"
                   "--discover: passive BLE scan for 10 seconds; aggregate report counts only.\n"
                   "--discover-classic: Classic inquiry (5.12s) and registered Meta service "
                   "lookup.\n"
                   "--connect-classic: require one paired Meta service; open/close socket, no "
                   "frames.\n"
                   "Each service lookup/open/connect has a five-second deadline. No silent "
                   "fallback.\n"
                   "--discover-classic-live: directly query inquiry peers with uncached Meta SDP.\n"
                   "--connect-classic-live: uncached lookup then one paired Meta socket check.\n"
                   "Live lookup: 20.48-second inquiry, eight peers, twenty-second query budget.\n"
                   "--pair-classic: Windows picker pairs your selection, then uncached socket "
                   "check.\n"
                   "Pairing waits for your selection/confirmation or Cancel; never runs "
                   "implicitly.\n"
                   "--direct-classic-uuid / --direct-classic-channel7: private Bluetooth address "
                   "on stdin;\n"
                   "one unpaired transport/hello probe, no application writes or route fallback.\n"
                   "Meta marker is a candidate, not verified model or authenticated glasses.\n"
                   "Only --pair-classic requests Windows pairing. No Meta login, identifiers or "
                   "network changes.\n"
                   "Exit: 0 completed; 2 hardware missing; 3 API/probe failure; 4 report I/O; "
                   "5 invalid arguments/address; "
                   "6 no Meta candidate/service; 7 ambiguous Meta services.\n";
            return std::cout ? EXIT_SUCCESS : EXIT_FAILURE;
        }
        const auto mode = argc == 2 ? std::string_view(argv[1]) : std::string_view{};
        if (mode == "--handshake") {
            return runHandshakeProbe();
        }
        if (mode == "--auth-url") {
            return runAuthUrl();
        }
        if (mode == "--import-tokens") {
            return runImportTokens();
        }
        if (mode == "--provision") {
            return runProvision();
        }
        if (mode == "--link") {
            return runLink();
        }
        if (mode == "--wifi") {
            return runWifi();
        }
        if (mode == "--wifi-join") {
            return runWifiJoin();
        }
        if (mode == "--stream") {
            return runStream();
        }
        if (mode == "--direct-classic-uuid" || mode == "--direct-classic-channel7") {
            return runDirectProbe(mode == "--direct-classic-channel7"
                                      ? mgl::DirectRfcommRoute::DocumentedChannel7
                                      : mgl::DirectRfcommRoute::ServiceUuid);
        }
        const bool valid_mode = mode == "--discover" || classicMode(mode).has_value();
        if (argc != 1 && !valid_mode) {
            std::cerr << "Invalid arguments; use --help.\n";
            constexpr int InvalidArguments = 5;
            return InvalidArguments;
        }
        return runProbe(mode);
    } catch (const std::exception& error) {
        constexpr int DiagnosticWriteFailed = 4;
        if (std::fputs(error.what(), stderr) == EOF || std::fputc('\n', stderr) == EOF) {
            return DiagnosticWriteFailed;
        }
        return EXIT_FAILURE;
    }
}
