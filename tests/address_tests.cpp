// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/address.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string_view>

namespace {
int runAddressTests() {
    constexpr std::array Valid{"12:34:56:78:9A:BC", "12-34-56-78-9a-bc", "123456789abc"};
    for (const auto* input : Valid) {
        const auto actual = mgl::parseBluetoothAddress(input);
        if (!actual || *actual != 0x123456789ABC) {
            std::cerr << "Address octet order or supported format incorrect\n";
            return EXIT_FAILURE;
        }
    }
    constexpr std::array Invalid{"",
                                 "123456789ab",
                                 "123456789abcd",
                                 "123456789abz",
                                 "12:34:56:78:9A",
                                 "12:34:56:78:9A:BC:DE",
                                 "12:34-56:78:9A:BC",
                                 "12.34.56.78.9A.BC",
                                 " 123456789ABC",
                                 "123456789ABC ",
                                 "00:00:00:00:00:00",
                                 "FF:FF:FF:FF:FF:FF",
                                 "00-00-00-00-00-00",
                                 "ffffffffffff",
                                 "12:34:56:78:9A:BG"};
    if (std::ranges::any_of(Invalid, [](std::string_view input) {
            return mgl::parseBluetoothAddress(input).has_value();
        })) {
        std::cerr << "Malformed/reserved Bluetooth address accepted\n";
        return EXIT_FAILURE;
    }
    const auto low = mgl::parseBluetoothAddress("000000000001");
    const auto high = mgl::parseBluetoothAddress("fffffffffffe");
    if (!low || *low != 1 || !high || *high != 0xFFFFFFFFFFFE) {
        std::cerr << "Valid address boundary rejected\n";
        return EXIT_FAILURE;
    }
    std::cout << "20 independent private-address format/order/boundary cases passed\n";
    return EXIT_SUCCESS;
}
} // namespace

int main() {
    try {
        return runAddressTests();
    } catch (const std::exception&) {
        if (std::fputs("Address test execution failed\n", stderr) == EOF) {
            return 4;
        }
        return EXIT_FAILURE;
    }
}
