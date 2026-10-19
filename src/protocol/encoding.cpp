// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/encoding.hpp"

#include "mgl/bytes.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
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
constexpr std::uint32_t ByteMask = 0xFFU;
constexpr std::uint32_t SextetMask = 0x3FU;
constexpr unsigned NibbleBits = 4;
constexpr unsigned NibbleMask = 0x0FU;
// Base64 maps each group of three bytes (24 bits) to four symbols.
constexpr std::size_t GroupBytes = 3;
constexpr std::size_t GroupSymbols = 4;
constexpr unsigned GroupBits = 24;
constexpr unsigned StoredLengthBytes = 2;
constexpr unsigned TrailerFieldBytes = 4;

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

// Appends the four symbols for one to three bytes; a short final group carries one symbol more
// than its byte count and is padded with '='.
void encodeGroup(ByteView group, std::string& out) {
    std::uint32_t bits = 0;
    unsigned shift = GroupBits;
    for (const auto byte : group) {
        shift -= ByteBits;
        bits |= static_cast<std::uint32_t>(byte) << shift;
    }
    shift = GroupBits;
    for (std::size_t symbol = 0; symbol < GroupSymbols; ++symbol) {
        shift -= SextetBits;
        out.push_back(symbol <= group.size() ? Base64Alphabet.at((bits >> shift) & SextetMask)
                                             : Pad);
    }
}

} // namespace

std::string base64Encode(ByteView data) {
    std::string out;
    out.reserve(((data.size() + GroupBytes - 1) / GroupBytes) * GroupSymbols);
    for (auto rest = data; !rest.empty();) {
        const auto group = rest.first(std::min(GroupBytes, rest.size()));
        encodeGroup(group, out);
        rest = rest.subspan(group.size());
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
            out.push_back(static_cast<std::uint8_t>((accumulator >> bits) & ByteMask));
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
            out.push_back(Hex.at(static_cast<unsigned>(ch) >> NibbleBits));
            out.push_back(Hex.at(static_cast<unsigned>(ch) & NibbleMask));
        }
    }
    return out;
}

namespace {
std::optional<unsigned> hexValue(char digit) {
    constexpr unsigned DecimalDigits = 10;
    if (digit >= '0' && digit <= '9') {
        return static_cast<unsigned>(digit - '0');
    }
    if (digit >= 'a' && digit <= 'f') {
        return static_cast<unsigned>(digit - 'a') + DecimalDigits;
    }
    if (digit >= 'A' && digit <= 'F') {
        return static_cast<unsigned>(digit - 'A') + DecimalDigits;
    }
    return std::nullopt;
}

// Percent-decodes a query value; '+' stays '+', as the browser's address bar shows it.
std::optional<std::string> percentDecode(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t index = 0; index < text.size(); ++index) {
        if (text.at(index) != '%') {
            out.push_back(text.at(index));
            continue;
        }
        if (text.size() - index < 3) {
            return std::nullopt;
        }
        const auto high = hexValue(text.at(index + 1));
        const auto low = hexValue(text.at(index + 2));
        if (!high || !low) {
            return std::nullopt;
        }
        out.push_back(static_cast<char>((*high << NibbleBits) | *low));
        index += 2;
    }
    return out;
}

std::string_view trimmed(std::string_view text) {
    constexpr std::string_view Space = " \t\r\n";
    const auto first = text.find_first_not_of(Space);
    if (first == std::string_view::npos) {
        return {};
    }
    return text.substr(first, text.find_last_not_of(Space) - first + 1);
}
} // namespace

std::optional<std::string> queryParameter(std::string_view address, std::string_view name) {
    std::string_view query = trimmed(address);
    if (const auto fragment = query.find('#'); fragment != std::string_view::npos) {
        query = query.substr(0, fragment);
    }
    if (const auto start = query.find('?'); start != std::string_view::npos) {
        query = query.substr(start + 1);
    }
    while (!query.empty()) {
        const auto end = query.find('&');
        const auto pair = query.substr(0, end);
        const auto equals = pair.find('=');
        if (equals != std::string_view::npos && pair.substr(0, equals) == name) {
            auto value = percentDecode(pair.substr(equals + 1));
            if (!value || value->empty()) {
                return std::nullopt;
            }
            return value;
        }
        if (end == std::string_view::npos) {
            break;
        }
        query.remove_prefix(end + 1);
    }
    return std::nullopt;
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
        const auto length = static_cast<std::uint32_t>(chunk);
        appendLittleEndian(out, length, StoredLengthBytes);
        appendLittleEndian(out, ~length, StoredLengthBytes); // NLEN, the one's complement of LEN
        append(out, data.subspan(offset, chunk));
        offset += chunk;
    }
    // Trailer: CRC-32, then the input size modulo 2^32.
    appendLittleEndian(out, crc32(data), TrailerFieldBytes);
    appendLittleEndian(out, static_cast<std::uint32_t>(data.size()), TrailerFieldBytes);
    return out;
}
} // namespace mgl::encoding
