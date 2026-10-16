// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/capabilities.hpp"
#include "mgl/discovery.hpp"

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string_view>

namespace {
int runProbe(bool discover) {
    const auto capabilities = mgl::inspectWirelessCapabilities();
    std::cout << "{\"wifi_interfaces\":" << capabilities.wifi_interfaces
              << ",\"wifi_connected_interfaces\":" << capabilities.wifi_connected_interfaces
              << ",\"bluetooth_radio\":" << (capabilities.bluetooth_radio ? "true" : "false")
              << ",\"wifi_error\":" << capabilities.wifi_error
              << ",\"bluetooth_error\":" << capabilities.bluetooth_error;
    constexpr int HardwareUnavailable = 2;
    constexpr int ProbeFailed = 3;
    int exit_code = capabilities.wifi_interfaces > 0 && capabilities.bluetooth_radio
                        ? EXIT_SUCCESS
                        : HardwareUnavailable;
    if (capabilities.wifi_error != 0 || capabilities.bluetooth_error != 0) {
        exit_code = ProbeFailed;
    }
    if (discover) {
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
    std::cout << ",\"glasses_connection_tested\":false}\n";
    return std::cout ? exit_code : ProbeFailed;
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--help") {
            std::cout
                << "Usage: meta-g-link-probe [--discover|--help]\n"
                   "Default: local radio/association counts only.\n"
                   "--discover: passive BLE scan for 10 seconds; aggregate report counts only.\n"
                   "Meta marker is a candidate, not verified model or authenticated glasses.\n"
                   "No pairing, login, device identifiers, or network changes.\n"
                   "Exit: 0 completed; 2 hardware missing; 3 API/I/O failure; 5 invalid arguments; "
                   "6 scan completed without Meta candidate.\n";
            return std::cout ? EXIT_SUCCESS : EXIT_FAILURE;
        }
        const bool discover = argc == 2 && std::string_view(argv[1]) == "--discover";
        if (argc != 1 && !discover) {
            std::cerr << "Invalid arguments; use --help.\n";
            constexpr int InvalidArguments = 5;
            return InvalidArguments;
        }
        return runProbe(discover);
    } catch (const std::exception& error) {
        constexpr int DiagnosticWriteFailed = 4;
        if (std::fputs(error.what(), stderr) == EOF || std::fputc('\n', stderr) == EOF) {
            return DiagnosticWriteFailed;
        }
        return EXIT_FAILURE;
    }
}
