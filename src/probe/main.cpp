// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/bytes.hpp"
#include "mgl/capabilities.hpp"
#include "mgl/crypto.hpp"
#include "mgl/discovery.hpp"
#include "mgl/hevc.hpp"
#include "mgl/identity.hpp"
#include "mgl/media_session.hpp"
#include "mgl/provision.hpp"
#include "mgl/rfcomm.hpp"
#include "mgl/rfcomm_transport.hpp"
#include "mgl/session.hpp"
#include "mgl/setup.hpp"
#include "mgl/stream.hpp"
#include "private_input.hpp"

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

// Deletes this PC's sealed glasses identity and imported tokens. The glasses themselves are not
// contacted: they keep this PC's key as owner until they are set up somewhere else.
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
// line. For scripts; people use the setup window (meta-g-link-setup.exe).
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
// owner key, EndLinkSetup. Reaching trust_accepted/ready proves the PC owns the glasses.
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
    session.setDemand({.video = true,
                       .audio = false,
                       .keep_link = false,
                       .video_config = mgl::stream::VideoConfig{},
                       .wifi_adapter = {}});
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
                   "[--auth-url|--provision|--import-tokens|--link|--stream|"
                   "--forget|--handshake|--discover|--discover-classic|--connect-classic|"
                   "--discover-classic-live|--connect-classic-live|--pair-classic|"
                   "--direct-classic-uuid|--direct-classic-channel7|--help]\n"
                   "Default: local radio/association counts only.\n"
                   "Setup and camera:\n"
                   "People set up the glasses with the setup window, meta-g-link-setup.exe.\n"
                   "--auth-url: print the Meta login address used by --provision.\n"
                   "--provision: read the login code on stdin and make this PC the glasses' "
                   "owner. The Meta AI phone app loses the glasses until they are added there "
                   "again.\n"
                   "--import-tokens: seal Meta app tokens from stdin; they override the built-in "
                   "ones.\n"
                   "--link: authenticated Bluetooth session with the stored owner key; reports "
                   "the stage reached.\n"
                   "--stream: the OBS source's camera path, for sixty seconds of video; status "
                   "on stderr, summary on stdout.\n"
                   "--forget: delete this PC's stored glasses identity and imported tokens.\n"
                   "Only --auth-url and --provision contact Meta. --stream asks the "
                   "glasses to "
                   "host their own Wi-Fi network and joins this PC's Wi-Fi adapter to it while it "
                   "runs; the adapter is free for its usual networks afterwards.\n"
                   "Diagnostics:\n"
                   "--handshake: open the bonded Meta RFCOMM service and run the AirShield "
                   "handshake only (no owner key, no trust, no ownership change); reports the "
                   "stage reached.\n"
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
