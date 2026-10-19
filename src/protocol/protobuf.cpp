// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/protobuf.hpp"

#include "mgl/bytes.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace mgl::proto {
namespace {
constexpr std::uint8_t ContinuationBit = 0x80;
constexpr std::uint8_t PayloadBits = 0x7F;
constexpr unsigned BitsPerByte = 7;
constexpr std::size_t MaximumVarintBytes = 10;
constexpr unsigned WireTypeBits = 3;
constexpr std::uint64_t WireTypeMask = 0x07;
constexpr std::uint64_t WireVarint = 0;
constexpr std::uint64_t WireFixed64 = 1;
constexpr std::uint64_t WireLengthDelimited = 2;
constexpr std::uint64_t WireFixed32 = 5;
constexpr std::size_t Fixed64Size = 8;
constexpr std::size_t Fixed32Size = 4;
constexpr std::uint64_t MaximumFieldNumber = (1U << 29U) - 1U;

struct Cursor {
    ByteView data;
    std::size_t offset = 0;

    std::optional<std::uint64_t> readVarint() {
        std::uint64_t value = 0;
        for (std::size_t index = 0; index < MaximumVarintBytes; ++index) {
            if (offset >= data.size()) {
                return std::nullopt;
            }
            const auto byte = data.subspan(offset).front();
            ++offset;
            const auto bits = static_cast<std::uint64_t>(byte & PayloadBits);
            // The tenth byte may contribute only the top bit of a 64-bit value.
            if (index == MaximumVarintBytes - 1 && bits > 1) {
                return std::nullopt;
            }
            value |= bits << (BitsPerByte * index);
            if ((byte & ContinuationBit) == 0) {
                return value;
            }
        }
        return std::nullopt;
    }

    bool skip(std::size_t count) {
        if (data.size() - offset < count) {
            return false;
        }
        offset += count;
        return true;
    }
};

// The key and length that precede a length-delimited field's bytes.
// A field key: the field number above the three wire-type bits.
std::uint64_t fieldKey(std::uint32_t field, std::uint64_t wire_type) {
    return (static_cast<std::uint64_t>(field) << WireTypeBits) | wire_type;
}

void appendLengthDelimitedPrefix(Bytes& out, std::uint32_t field, std::size_t size) {
    appendVarint(out, fieldKey(field, WireLengthDelimited));
    appendVarint(out, size);
}
} // namespace

void appendVarint(Bytes& out, std::uint64_t value) {
    while (value > PayloadBits) {
        out.push_back(static_cast<std::uint8_t>((value & PayloadBits) | ContinuationBit));
        value >>= BitsPerByte;
    }
    out.push_back(static_cast<std::uint8_t>(value));
}

void appendVarintField(Bytes& out, std::uint32_t field, std::uint64_t value) {
    appendVarint(out, fieldKey(field, WireVarint));
    appendVarint(out, value);
}

void appendBytesField(Bytes& out, std::uint32_t field, ByteView value) {
    appendLengthDelimitedPrefix(out, field, value.size());
    append(out, value);
}

void appendStringField(Bytes& out, std::uint32_t field, std::string_view value) {
    appendLengthDelimitedPrefix(out, field, value.size());
    out.insert(out.end(), value.begin(), value.end());
}

std::optional<Message> Message::parse(ByteView data) {
    Message message;
    Cursor cursor{.data = data};
    while (cursor.offset < data.size()) {
        const auto key = cursor.readVarint();
        if (!key) {
            return std::nullopt;
        }
        const auto number = *key >> WireTypeBits;
        if (number == 0 || number > MaximumFieldNumber) {
            return std::nullopt;
        }
        Field field{.number = static_cast<std::uint32_t>(number),
                    .length_delimited = false,
                    .varint = 0,
                    .bytes = {}};
        switch (*key & WireTypeMask) {
        case WireVarint: {
            const auto value = cursor.readVarint();
            if (!value) {
                return std::nullopt;
            }
            field.varint = *value;
            message.fields_.push_back(field);
            break;
        }
        case WireLengthDelimited: {
            const auto length = cursor.readVarint();
            if (!length || *length > data.size() - cursor.offset) {
                return std::nullopt;
            }
            field.length_delimited = true;
            field.bytes = data.subspan(cursor.offset, static_cast<std::size_t>(*length));
            cursor.offset += static_cast<std::size_t>(*length);
            message.fields_.push_back(field);
            break;
        }
        case WireFixed64:
            if (!cursor.skip(Fixed64Size)) {
                return std::nullopt;
            }
            break;
        case WireFixed32:
            if (!cursor.skip(Fixed32Size)) {
                return std::nullopt;
            }
            break;
        default:
            return std::nullopt;
        }
    }
    return message;
}

std::optional<std::uint64_t> Message::varint(std::uint32_t field) const {
    const auto found = std::ranges::find_if(fields_, [field](const auto& entry) {
        return entry.number == field && !entry.length_delimited;
    });
    return found != fields_.end() ? std::optional(found->varint) : std::nullopt;
}

std::optional<ByteView> Message::bytes(std::uint32_t field) const {
    const auto found = std::ranges::find_if(fields_, [field](const auto& entry) {
        return entry.number == field && entry.length_delimited;
    });
    return found != fields_.end() ? std::optional(found->bytes) : std::nullopt;
}

std::vector<ByteView> Message::repeatedBytes(std::uint32_t field) const {
    std::vector<ByteView> values;
    for (const auto& entry : fields_) {
        if (entry.number == field && entry.length_delimited) {
            values.push_back(entry.bytes);
        }
    }
    return values;
}
} // namespace mgl::proto
