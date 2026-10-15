// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/capabilities.hpp"

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>

namespace {
int runProbe() {
    const auto capabilities = mgl::inspectWirelessCapabilities();
    std::cout << "{\"wifi_interfaces\":" << capabilities.wifi_interfaces
              << ",\"bluetooth_radio\":" << (capabilities.bluetooth_radio ? "true" : "false")
              << ",\"wifi_error\":" << capabilities.wifi_error
              << ",\"bluetooth_error\":" << capabilities.bluetooth_error
              << ",\"glasses_connection_tested\":false}\n";
    constexpr int HardwareUnavailable = 2;
    constexpr int ProbeFailed = 3;
    if (capabilities.wifi_error != 0 || capabilities.bluetooth_error != 0) {
        return ProbeFailed;
    }
    return capabilities.wifi_interfaces > 0 && capabilities.bluetooth_radio ? 0
                                                                            : HardwareUnavailable;
}

} // namespace

int main() {
    try {
        return runProbe();
    } catch (const std::exception& error) {
        constexpr int DiagnosticWriteFailed = 4;
        if (std::fputs(error.what(), stderr) == EOF || std::fputc('\n', stderr) == EOF) {
            return DiagnosticWriteFailed;
        }
        return EXIT_FAILURE;
    }
}
