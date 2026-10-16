// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/capabilities.hpp"
#include "mgl/discovery.hpp"

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string_view>

namespace {
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
}

int classicExit(const mgl::ClassicDiscovery& classic, bool connect, int baseline_exit) {
    constexpr int ProbeFailed = 3;
    constexpr int NoRegisteredMetaService = 6;
    constexpr int AmbiguousMetaServices = 7;
    if (classic.classic_error != 0 || classic.rfcomm_hresult != 0 ||
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
    if (mode == "--discover-classic" || mode == "--connect-classic") {
        const bool connect = mode == "--connect-classic";
        const auto classic = mgl::inspectClassicRfcomm(connect);
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
                   "[--discover|--discover-classic|--connect-classic|--help]\n"
                   "Default: local radio/association counts only.\n"
                   "--discover: passive BLE scan for 10 seconds; aggregate report counts only.\n"
                   "--discover-classic: Classic inquiry (5.12s) and registered Meta service "
                   "lookup.\n"
                   "--connect-classic: require one paired Meta service; open/close socket, no "
                   "frames.\n"
                   "Each service lookup/open/connect has a five-second deadline. No silent "
                   "fallback.\n"
                   "Meta marker is a candidate, not verified model or authenticated glasses.\n"
                   "No pairing, login, device identifiers, or network changes.\n"
                   "Exit: 0 completed; 2 hardware missing; 3 API/I/O failure; 5 invalid arguments; "
                   "6 no Meta candidate/service; 7 ambiguous Meta services.\n";
            return std::cout ? EXIT_SUCCESS : EXIT_FAILURE;
        }
        const auto mode = argc == 2 ? std::string_view(argv[1]) : std::string_view{};
        const bool valid_mode =
            mode == "--discover" || mode == "--discover-classic" || mode == "--connect-classic";
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
