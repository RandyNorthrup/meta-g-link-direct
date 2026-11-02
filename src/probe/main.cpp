// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/bytes.hpp"
#include "mgl/crypto.hpp"
#include "mgl/hevc.hpp"
#include "mgl/identity.hpp"
#include "mgl/media_session.hpp"
#include "mgl/provision.hpp"
#include "mgl/rfcomm_transport.hpp"
#include "mgl/session.hpp"
#include "mgl/setup.hpp"
#include "mgl/stream.hpp"

#ifdef _WIN32
#include "windows_diagnostics.hpp"
#endif

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace {
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
    const auto opened = mgl::openBondedMetaRfcomm();
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

// Deletes this computer's sealed glasses identity and imported tokens. The glasses themselves are
// not contacted: they keep this computer's key as owner until they are set up somewhere else.
int runForget() {
    constexpr int ProbeFailed = 3;
    const bool forgotten = mgl::identity::forgetOwnerIdentity();
    std::cout << R"({"forgotten":)" << (forgotten ? "true" : "false") << "}\n";
    return (forgotten && std::cout) ? EXIT_SUCCESS : ProbeFailed;
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

// The owner key sealed by --provision, or nullopt after reporting why it is unavailable.
std::optional<mgl::crypto::OwnerKey> loadStoredOwner() {
    const auto identity = mgl::identity::loadOwnerIdentity();
    if (!identity) {
        std::cerr << "No stored owner identity. Run --provision first.\n";
        return std::nullopt;
    }
    auto owner = mgl::crypto::OwnerKey::importPrivate(
        mgl::ByteView(identity->owner_private.data(), identity->owner_private.size()));
    if (!owner) {
        std::cerr << "Stored owner key is unreadable.\n";
    }
    return owner;
}

std::string trimmedLine(std::string line) {
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
        line.pop_back();
    }
    const auto first = line.find_first_not_of(' ');
    return first == std::string::npos ? std::string() : line.substr(first);
}

// Reads the login auth code on stdin and runs the change of owner, reporting each phase as a JSON
// line. For scripts; people use the setup window (meta-g-link-setup).
int runProvision() {
    constexpr int ProbeFailed = 3;
    constexpr int InvalidInput = 5;
    std::string line;
    std::getline(std::cin, line);
    const std::string code = trimmedLine(line);
    if (code.empty()) {
        std::cerr << "No auth code on stdin.\n";
        return InvalidInput;
    }
    const auto outcome = mgl::setup::changeOwner(code);
    using mgl::setup::Phase;
    if (outcome.phase == Phase::Auth) {
        std::cout << R"({"phase":"auth","ok":false,"error":")" << jsonEscape(outcome.auth_error)
                  << "\"}\n";
    } else if (outcome.phase == Phase::Connect) {
        std::cout << R"({"phase":"connect","ok":false,"rfcomm_connect":")"
                  << connectWord(outcome.rfcomm) << "\"}\n";
    } else if (outcome.phase == Phase::Handshake) {
        std::cout << R"({"phase":"handshake","ok":false,"stage":")" << stageWord(outcome.handshake)
                  << "\"}\n";
    } else {
        const auto& result = outcome.change_owner;
        std::cout << R"({"phase":"change_owner","ok":)" << (result.ok ? "true" : "false")
                  << R"(,"stage":")" << provisionStageWord(result.stage) << R"(","glasses_status":)"
                  << result.glasses_status << R"(,"serial":")" << jsonEscape(result.serial)
                  << R"(","error":")" << jsonEscape(result.error) << "\"}\n";
    }
    if (!outcome.ok) {
        std::cerr << outcome.problem << '\n';
        return ProbeFailed;
    }
    return std::cout ? EXIT_SUCCESS : ProbeFailed;
}
// Opens a full authenticated link using the stored owner identity: handshake, trust with the
// owner key, EndLinkSetup. Reaching trust_accepted/ready proves this computer owns the glasses.
int runLink() {
    constexpr int ProbeFailed = 3;
    const auto owner = loadStoredOwner();
    if (!owner) {
        return ProbeFailed;
    }
    const auto opened = mgl::setup::openGlassesRfcomm();
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

// What the bounded receive window saw.
struct StreamStats {
    std::uint64_t frames = 0;
    std::uint64_t keyframes = 0;
    std::uint64_t video_bytes = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

// Takes the picture size from an SPS in the buffer until one has been recorded.
void recordDimensions(StreamStats& stats, mgl::ByteView annex) {
    if (stats.width != 0) {
        return;
    }
    if (const auto dims = mgl::hevc::spsDimensions(annex)) {
        stats.width = dims->width;
        stats.height = dims->height;
    }
}

// The full live media path, through the same MediaSession the OBS source runs: authenticated
// Bluetooth link, Wi-Fi handover, TCP session, camera start, then sixty seconds of real HEVC
// frames. Connection attempts, retries and teardown are the production ones; their log lines go
// to stderr and the JSON summary to stdout.
int runStream() {
    constexpr int ProbeFailed = 3;
    constexpr auto FirstFrameBudget = std::chrono::seconds(120);
    constexpr auto VideoWindow = std::chrono::seconds(60);
    if (!loadStoredOwner()) {
        return ProbeFailed;
    }
    std::mutex mutex;
    std::condition_variable changed;
    StreamStats stats;
    auto state = mgl::MediaState::Idle;

    mgl::MediaCallbacks callbacks;
    callbacks.on_frame = [&](std::uint64_t /*pts*/, mgl::ByteView annex, bool keyframe) {
        const std::scoped_lock lock(mutex);
        ++stats.frames;
        stats.video_bytes += annex.size();
        if (keyframe) {
            ++stats.keyframes;
        }
        recordDimensions(stats, annex);
        changed.notify_all();
    };
    callbacks.on_parameters = [&](mgl::ByteView parameters) {
        const std::scoped_lock lock(mutex);
        recordDimensions(stats, parameters);
    };
    callbacks.on_log = [](const std::string& message) { std::cerr << message << '\n'; };
    callbacks.on_state = [&](mgl::MediaState next) {
        const std::scoped_lock lock(mutex);
        state = next;
        changed.notify_all();
    };

    mgl::MediaSession session(std::move(callbacks));
    const mgl::stream::VideoConfig default_profile{};
    session.setDemand({
        .video = true,
        .audio = false,
        .keep_link = false,
        .video_config = default_profile,
        .wifi_adapter = {},
    });
    session.start();
    {
        std::unique_lock lock(mutex);
        changed.wait_for(lock, FirstFrameBudget,
                         [&] { return stats.frames > 0 || state == mgl::MediaState::Failed; });
    }
    bool streamed = false;
    {
        const std::scoped_lock lock(mutex);
        streamed = stats.frames > 0;
    }
    if (streamed) {
        std::this_thread::sleep_for(VideoWindow);
    }
    session.stop();

    const std::scoped_lock lock(mutex);
    std::cout << R"({"phase":"stream","frames":)" << stats.frames << R"(,"keyframes":)"
              << stats.keyframes << R"(,"video_bytes":)" << stats.video_bytes << R"(,"width":)"
              << stats.width << R"(,"height":)" << stats.height << R"(,"ok":)"
              << (stats.frames > 0 ? "true" : "false") << "}\n";
    return (stats.frames > 0 && std::cout) ? EXIT_SUCCESS : ProbeFailed;
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--help") {
            std::cout
                << "Usage: meta-g-link-probe "
                   "[--auth-url|--provision|--import-tokens|--link|--stream|--forget|--handshake"
#ifdef _WIN32
                   "|--discover|--discover-classic|--connect-classic|"
                   "--discover-classic-live|--connect-classic-live|--pair-classic|"
                   "--direct-classic-uuid|--direct-classic-channel7"
#endif
                   "|--help]\n"
#ifdef _WIN32
                   "Default: local radio/association counts only.\n"
#endif
                   "Setup and camera:\n"
                   "People set up the glasses with the setup window, meta-g-link-setup.\n"
                   "--auth-url: print the Meta login address used by --provision.\n"
                   "--provision: read the login code on stdin and make this computer the glasses' "
                   "owner. The Meta AI phone app loses the glasses until they are added there "
                   "again.\n"
                   "--import-tokens: seal Meta app tokens from stdin; they override the built-in "
                   "ones.\n"
                   "--link: authenticated Bluetooth session with the stored owner key; reports "
                   "the stage reached.\n"
                   "--stream: the OBS source's camera path, for sixty seconds of video; status "
                   "on stderr, summary on stdout.\n"
                   "--forget: delete this computer's stored glasses identity and imported tokens.\n"
                   "Only --auth-url and --provision contact Meta. --stream asks the "
                   "glasses to "
                   "host their own Wi-Fi network and joins this computer's Wi-Fi adapter to it "
                   "while it runs; the adapter is free for its usual networks afterwards.\n"
                   "Diagnostics:\n"
                   "--handshake: open the bonded Meta RFCOMM service and run the AirShield "
                   "handshake only (no owner key, no trust, no ownership change); reports the "
                   "stage reached.\n"
#ifdef _WIN32
                   "--discover: active BLE scan for 10 seconds; aggregate report counts only.\n"
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
                   "Only --pair-classic requests Windows pairing. The diagnostics make no Meta "
                   "login, record no identifiers and change no network settings.\n"
#endif
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
        if (mode == "--stream") {
            return runStream();
        }
        if (mode == "--forget") {
            return runForget();
        }
#ifdef _WIN32
        if (argc <= 2 && mgl::probe::isWindowsDiagnostic(mode)) {
            return mgl::probe::runWindowsDiagnostic(mode);
        }
#endif
        std::cerr << "Invalid arguments; use --help.\n";
        constexpr int InvalidArguments = 5;
        return InvalidArguments;
    } catch (const std::exception& error) {
        constexpr int DiagnosticWriteFailed = 4;
        if (std::fputs(error.what(), stderr) == EOF || std::fputc('\n', stderr) == EOF) {
            return DiagnosticWriteFailed;
        }
        return EXIT_FAILURE;
    }
}
