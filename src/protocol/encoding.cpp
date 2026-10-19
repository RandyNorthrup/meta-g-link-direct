// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/encoding.hpp"

#include "mgl/bytes.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mgl::encoding {
namespace {
constexpr std::string_view Base64Alphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
constexpr char Pad = '=';
constexpr unsigned ByteBits = 8;
constexpr unsigned SextetBits = 6;

std::uint8_t decodeSymbol(char symbol) {
    if (symbol >= 'A' && symbol <= 'Z') {
        return static_cast<std::uint8_t>(symbol - 'A');
    }
    if (symbol >= 'a' && symbol <= 'z') {
        return static_cast<std::uint8_t>(symbol - 'a' + 26);
    }
    if (symbol >= '0' && symbol <= '9') {
        return static_cast<std::uint8_t>(symbol - '0' + 52);
    }
    if (symbol == '+') {
        return 62;
    }
    return 63; // '/'
}

bool isBase64(char symbol) {
    return (symbol >= 'A' && symbol <= 'Z') || (symbol >= 'a' && symbol <= 'z') ||
           (symbol >= '0' && symbol <= '9') || symbol == '+' || symbol == '/';
}

// CRC-32 (IEEE 802.3), for the gzip trailer.
std::uint32_t crc32(ByteView data) {
    static constexpr std::uint32_t Polynomial = 0xEDB88320;
    std::uint32_t crc = 0xFFFFFFFF;
    for (const auto byte : data) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask = (crc & 1U) != 0 ? 0xFFFFFFFFU : 0U;
            crc = (crc >> 1U) ^ (Polynomial & mask);
        }
    }
    return ~crc;
}
} // namespace

std::string base64Encode(ByteView data) {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    std::size_t index = 0;
    while (index + 3 <= data.size()) {
        const std::uint32_t triple = (static_cast<std::uint32_t>(data[index]) << 16) |
                                     (static_cast<std::uint32_t>(data[index + 1]) << 8) |
                                     data[index + 2];
        out.push_back(Base64Alphabet[(triple >> 18) & 0x3F]);
        out.push_back(Base64Alphabet[(triple >> 12) & 0x3F]);
        out.push_back(Base64Alphabet[(triple >> 6) & 0x3F]);
        out.push_back(Base64Alphabet[triple & 0x3F]);
        index += 3;
    }
    const auto remaining = data.size() - index;
    if (remaining == 1) {
        const std::uint32_t triple = static_cast<std::uint32_t>(data[index]) << 16;
        out.push_back(Base64Alphabet[(triple >> 18) & 0x3F]);
        out.push_back(Base64Alphabet[(triple >> 12) & 0x3F]);
        out.push_back(Pad);
        out.push_back(Pad);
    } else if (remaining == 2) {
        const std::uint32_t triple = (static_cast<std::uint32_t>(data[index]) << 16) |
                                     (static_cast<std::uint32_t>(data[index + 1]) << 8);
        out.push_back(Base64Alphabet[(triple >> 18) & 0x3F]);
        out.push_back(Base64Alphabet[(triple >> 12) & 0x3F]);
        out.push_back(Base64Alphabet[(triple >> 6) & 0x3F]);
        out.push_back(Pad);
    }
    return out;
}

Bytes base64Decode(std::string_view text) {
    Bytes out;
    std::uint32_t accumulator = 0;
    unsigned bits = 0;
    for (const char symbol : text) {
        if (!isBase64(symbol)) {
            continue; // Python's b64decode without validate drops stray characters.
        }
        accumulator = (accumulator << SextetBits) | decodeSymbol(symbol);
        bits += SextetBits;
        if (bits >= ByteBits) {
            bits -= ByteBits;
            out.push_back(static_cast<std::uint8_t>((accumulator >> bits) & 0xFF));
        }
    }
    return out;
}

std::string quotePlus(std::string_view value) {
    static constexpr std::string_view Hex = "0123456789ABCDEF";
    std::string out;
    out.reserve(value.size());
    for (const unsigned char ch : value) {
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') ||
            ch == '_' || ch == '.' || ch == '-' || ch == '~') {
            out.push_back(static_cast<char>(ch));
        } else if (ch == ' ') {
            out.push_back('+');
        } else {
            out.push_back('%');
            out.push_back(Hex[ch >> 4]);
            out.push_back(Hex[ch & 0x0F]);
        }
    }
    return out;
}

std::string formUrlEncode(const std::vector<std::pair<std::string, std::string>>& fields) {
    std::string out;
    bool first = true;
    for (const auto& [key, value] : fields) {
        if (!first) {
            out.push_back('&');
        }
        first = false;
        out += quotePlus(key);
        out.push_back('=');
        out += quotePlus(value);
    }
    return out;
}

Bytes gzip(ByteView data) {
    Bytes out;
    // Fixed 10-byte gzip header: magic, DEFLATE, no flags, no mtime, no extra, unknown OS.
    const std::array<std::uint8_t, 10> header{0x1F, 0x8B, 0x08, 0x00, 0x00,
                                              0x00, 0x00, 0x00, 0x00, 0xFF};
    append(out, {header.data(), header.size()});
    // Stored DEFLATE blocks: each carries up to 65535 bytes as [BFINAL|BTYPE=00][LEN][~LEN][data].
    constexpr std::size_t MaxBlock = 0xFFFF;
    std::size_t offset = 0;
    if (data.empty()) {
        out.push_back(0x01); // a final empty stored block
        out.push_back(0x00);
        out.push_back(0x00);
        out.push_back(0xFF);
        out.push_back(0xFF);
    }
    while (offset < data.size()) {
        const auto chunk = std::min(MaxBlock, data.size() - offset);
        const bool final_block = offset + chunk >= data.size();
        out.push_back(final_block ? 0x01 : 0x00);
        out.push_back(static_cast<std::uint8_t>(chunk & 0xFF));
        out.push_back(static_cast<std::uint8_t>((chunk >> 8) & 0xFF));
        const auto inverted = static_cast<std::uint16_t>(~chunk);
        out.push_back(static_cast<std::uint8_t>(inverted & 0xFF));
        out.push_back(static_cast<std::uint8_t>((inverted >> 8) & 0xFF));
        append(out, data.subspan(offset, chunk));
        offset += chunk;
    }
    const auto crc = crc32(data);
    const auto size = static_cast<std::uint32_t>(data.size());
    for (const auto value : {crc, size}) {
        for (unsigned shift = 0; shift < 32; shift += ByteBits) {
            out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFF));
        }
    }
    return out;
}
} // namespace mgl::encoding
