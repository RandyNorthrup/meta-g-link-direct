// SPDX-License-Identifier: AGPL-3.0-only
// Minimal protobuf wire encoding for the glasses' service messages. Ported in behaviour from
// Visor core/protocol Wire.kt at the revision in THIRD_PARTY_NOTICES.md; parsing here is strict.
#ifndef MGL_PROTOBUF_HPP
#define MGL_PROTOBUF_HPP

#include "mgl/bytes.hpp"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace mgl::proto {
void appendVarint(Bytes& out, std::uint64_t value);
void appendVarintField(Bytes& out, std::uint32_t field, std::uint64_t value);
void appendBytesField(Bytes& out, std::uint32_t field, ByteView value);
void appendStringField(Bytes& out, std::uint32_t field, std::string_view value);

// A flat message parsed without copying. Returned views point into the parsed buffer, which the
// caller must keep alive. Fixed32/fixed64 fields are skipped; groups and truncation are invalid.
class Message {
  public:
    [[nodiscard]] static std::optional<Message> parse(ByteView data);
    [[nodiscard]] std::optional<std::uint64_t> varint(std::uint32_t field) const;
    [[nodiscard]] std::optional<ByteView> bytes(std::uint32_t field) const;
    [[nodiscard]] std::vector<ByteView> repeatedBytes(std::uint32_t field) const;
    [[nodiscard]] std::uint64_t varintOr(std::uint32_t field, std::uint64_t fallback) const {
        return varint(field).value_or(fallback);
    }

  private:
    struct Field {
        std::uint32_t number = 0;
        bool length_delimited = false;
        std::uint64_t varint = 0;
        ByteView bytes;
    };
    std::vector<Field> fields_;
};
} // namespace mgl::proto
#endif
