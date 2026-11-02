// SPDX-License-Identifier: AGPL-3.0-only
// The probe's Windows radio diagnostics; see windows_diagnostics.hpp.
#include "windows_diagnostics.hpp"

#include "mgl/capabilities.hpp"
#include "mgl/discovery.hpp"
#include "mgl/rfcomm.hpp"
#include "private_input.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <sstream>
#include <string_view>

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

namespace mgl::probe {
bool isWindowsDiagnostic(std::string_view mode) {
    return mode.empty() || mode == "--discover" || mode == "--direct-classic-uuid" ||
           mode == "--direct-classic-channel7" || classicMode(mode).has_value();
}

int runWindowsDiagnostic(std::string_view mode) {
    if (mode == "--direct-classic-uuid" || mode == "--direct-classic-channel7") {
        return runDirectProbe(mode == "--direct-classic-channel7"
                                  ? mgl::DirectRfcommRoute::DocumentedChannel7
                                  : mgl::DirectRfcommRoute::ServiceUuid);
    }
    return runProbe(mode);
}
} // namespace mgl::probe
