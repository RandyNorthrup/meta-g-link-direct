// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/address.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace mgl {
namespace {
std::optional<unsigned> hexDigit(char value) noexcept {
    if (value >= '0' && value <= '9') {
        return static_cast<unsigned>(value - '0');
    }
    if (value >= 'A' && value <= 'F') {
        return static_cast<unsigned>(value - 'A') + 10;
    }
    if (value >= 'a' && value <= 'f') {
        return static_cast<unsigned>(value - 'a') + 10;
    }
    return std::nullopt;
}
} // namespace

std::optional<std::uint64_t> parseBluetoothAddress(std::string_view input) noexcept {
    constexpr std::size_t CompactLength = 12;
    constexpr std::size_t SeparatedLength = 17;
    constexpr std::size_t Octets = 6;
    if (input.size() != CompactLength && input.size() != SeparatedLength) {
        return std::nullopt;
    }
    const bool separated = input.size() == SeparatedLength;
    const std::span bytes(input.data(), input.size());
    const char separator = separated ? bytes.subspan<2, 1>().front() : '\0';
    if (separated && separator != ':' && separator != '-') {
        return std::nullopt;
    }
    std::uint64_t address = 0;
    for (std::size_t octet = 0; octet < Octets; ++octet) {
        const auto index = octet * (separated ? 3 : 2);
        const auto octet_bytes = bytes.subspan(index, 2);
        const auto high = hexDigit(octet_bytes.front());
        const auto low = hexDigit(octet_bytes.back());
        if (!high || !low) {
            return std::nullopt;
        }
        if (separated && octet + 1 < Octets && bytes.subspan(index + 2, 1).front() != separator) {
            return std::nullopt;
        }
        address = (address << 8U) | (*high << 4U) | *low;
    }
    constexpr std::uint64_t BroadcastAddress = 0xFFFFFFFFFFFF;
    return address == 0 || address == BroadcastAddress ? std::nullopt : std::optional{address};
}
} // namespace mgl
