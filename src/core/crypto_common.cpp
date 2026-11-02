// SPDX-License-Identifier: AGPL-3.0-only
// The parts of mgl/crypto.hpp that are the same over any provider.
#include "mgl/bytes.hpp"
#include "mgl/crypto.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>

namespace mgl::crypto {
namespace {
// P-256 group order n and n / 2, big-endian.
constexpr std::array<std::uint8_t, ScalarSize> GroupOrder{
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84, 0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51,
};
constexpr std::array<std::uint8_t, ScalarSize> HalfOrder{
    0x7F, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xDE, 0x73, 0x7D, 0x56, 0xD3, 0x8B, 0xCF, 0x42, 0x79, 0xDC, 0xE5, 0x61, 0x7E, 0x31, 0x92, 0xA8,
};
} // namespace

Digest hmacSha256(ByteView key, std::initializer_list<ByteView> parts) {
    return HmacSha256(key).compute(parts);
}

std::string randomText(std::string_view alphabet, std::size_t length) {
    constexpr std::size_t ByteValues = 256;
    if (alphabet.empty() || alphabet.size() > ByteValues) {
        throw CryptoError("randomText needs an alphabet of 1 to 256 symbols");
    }
    // Bytes at or above the largest multiple of the alphabet size are rejected, so every symbol is
    // equally likely; modulo alone would favour the first symbols.
    const std::size_t accepted = ByteValues - (ByteValues % alphabet.size());
    std::string out;
    out.reserve(length);
    std::array<std::uint8_t, 64> pool{};
    while (out.size() < length) {
        randomBytes(pool);
        for (const std::uint8_t byte : pool) {
            if (byte < accepted && out.size() < length) {
                out.push_back(alphabet.at(byte % alphabet.size()));
            }
        }
    }
    return out;
}

bool constantTimeEqual(ByteView left, ByteView right) noexcept {
    if (left.size() != right.size()) {
        return false;
    }
    std::uint8_t difference = 0;
    auto right_byte = right.begin();
    for (const auto left_byte : left) {
        difference |= static_cast<std::uint8_t>(left_byte ^ *right_byte);
        ++right_byte;
    }
    return difference == 0;
}

Signature lowS(const Signature& signature) noexcept {
    const auto s = std::span(signature).subspan<ScalarSize>();
    if (!std::ranges::lexicographical_compare(HalfOrder, s)) {
        return signature;
    }
    Signature out = signature;
    unsigned borrow = 0;
    for (std::size_t index = ScalarSize; index-- > 0;) {
        const auto minuend = static_cast<unsigned>(GroupOrder.at(index));
        const auto subtrahend = static_cast<unsigned>(signature.at(ScalarSize + index)) + borrow;
        borrow = minuend < subtrahend ? 1U : 0U;
        constexpr unsigned ByteRange = 0x100;
        out.at(ScalarSize + index) =
            static_cast<std::uint8_t>(minuend + (borrow * ByteRange) - subtrahend);
    }
    return out;
}
} // namespace mgl::crypto
