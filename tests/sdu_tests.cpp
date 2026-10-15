// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/sdu.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>

namespace {
void expect(std::span<const std::uint8_t> prefix, mgl::PrefixStatus status, std::size_t length,
            const char* name) {
    const auto actual = mgl::inspectFramePrefix(prefix);
    if (actual.status != status || actual.frame_size != length) {
        throw std::runtime_error(std::string(name) + ": unexpected prefix status or frame length");
    }
}

void testKnownVectors() {
    using enum mgl::PrefixStatus;
    expect({}, NeedMore, 0, "Empty prefix needs more input");
    constexpr std::array<std::uint8_t, 1> OneByte{0x80};
    expect(OneByte, NeedMore, 0, "One byte needs more input");
    constexpr std::array<std::uint8_t, 2> DataxEmpty{0x80, 0x00};
    expect(DataxEmpty, Ready, 4, "DataX empty has 4 bytes");
    constexpr std::array<std::uint8_t, 2> DataxPayload{0x81, 0x02};
    expect(DataxPayload, Ready, 262, "DataX length is big-endian and excludes fixed header");
    constexpr std::array<std::uint8_t, 2> DataxMaximum{0xFF, 0xFF};
    expect(DataxMaximum, Ready, 16387, "DataX length strips both flag bits");
    constexpr std::array<std::uint8_t, 2> ShortAirshield{0x40, 0x00};
    expect(ShortAirshield, NeedMore, 0, "AirShield prefix needs its block count");
    constexpr std::array<std::uint8_t, 10> AirshieldMinimum{0x40};
    expect(AirshieldMinimum, Ready, 26, "AirShield zero count still carries one AES block");
    constexpr std::array<std::uint8_t, 10> AirshieldMaximum{0x7F, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF};
    expect(AirshieldMaximum, Ready, 4106, "AirShield maximum carries 256 AES blocks");
    constexpr std::array<std::uint8_t, 2> InvalidType{0x3F, 0xFF};
    expect(InvalidType, Invalid, 0, "Unknown types are invalid, not incomplete");
}

void testTruncationAndTypes() {
    constexpr std::array<std::uint8_t, 10> Airshield{0x40};
    for (std::size_t length = 0; length < Airshield.size(); ++length) {
        expect(std::span(Airshield).first(length), mgl::PrefixStatus::NeedMore, 0,
               "Every truncated AirShield prefix needs more input");
    }
    constexpr unsigned InvalidTypeCount = 64;
    for (unsigned first = 0; first < InvalidTypeCount; ++first) {
        const std::array<std::uint8_t, 2> prefix{static_cast<std::uint8_t>(first), 0};
        expect(prefix, mgl::PrefixStatus::Invalid, 0, "Every unrecognized type must be rejected");
    }
}

} // namespace

int main() {
    try {
        testKnownVectors();
        testTruncationAndTypes();
        std::cout << "83 independent framing/truncation/type cases passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        constexpr int DiagnosticWriteFailed = 4;
        if (std::fputs(error.what(), stderr) == EOF || std::fputc('\n', stderr) == EOF) {
            return DiagnosticWriteFailed;
        }
        return EXIT_FAILURE;
    }
}
