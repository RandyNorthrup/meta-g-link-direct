// SPDX-License-Identifier: AGPL-3.0-only
// DataX message layer. Behaviour follows Visor core/protocol Header.kt and core/session
// DataXConnection.kt / ChannelRegistry.kt at the revision in THIRD_PARTY_NOTICES.md.
#ifndef MGL_DATAX_HPP
#define MGL_DATAX_HPP

#include "mgl/bytes.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace mgl::datax {
inline constexpr std::size_t FixedHeaderSize = 4;
inline constexpr std::size_t TagSize = 4;
inline constexpr std::size_t MaximumPayload = 16383;
inline constexpr std::uint16_t OriginatorBit = 0x8000;
inline constexpr std::uint16_t ChannelMask = 0x7FFF;
inline constexpr std::uint16_t StatusSuccess = 0x1000;
inline constexpr std::uint16_t StatusServiceNotFound = 0xC001;

struct Header {
    // Counts the extension tags as well as the data, so the wire size is 4 + payload_length.
    std::uint16_t payload_length = 0;
    std::uint16_t channel = 0;
    std::uint16_t service = 0;
    std::uint16_t type = 0;
    std::uint16_t error = 0;
    bool has_service = false;
    bool has_type = false;
    bool has_error = false;
    bool closed = false;
    std::size_t header_size = FixedHeaderSize;

    [[nodiscard]] std::size_t wireSize() const noexcept { return FixedHeaderSize + payload_length; }
    [[nodiscard]] std::size_t dataSize() const noexcept { return wireSize() - header_size; }
};

// nullopt when fewer than four bytes are present, or when the extension tags run past the bytes
// given or past the declared payload length.
[[nodiscard]] std::optional<Header> parseHeader(ByteView data);

struct Tags {
    std::optional<std::uint16_t> service;
    std::optional<std::uint16_t> type;
    std::optional<std::uint16_t> error;
};

// nullopt when the tags and payload exceed the 14-bit length field.
[[nodiscard]] std::optional<Bytes> buildMessage(std::uint16_t channel, const Tags& tags,
                                                ByteView payload);

// The reply channel for a channel id: the originator bit cleared.
[[nodiscard]] constexpr std::uint16_t mirror(std::uint16_t channel) noexcept {
    return static_cast<std::uint16_t>(channel & ChannelMask);
}

// A decrypted frame without its modified PKCS#7 tail (N bytes of N + 0xC0), when one matches.
[[nodiscard]] ByteView withoutPad(ByteView plaintext) noexcept;

struct Message {
    Header header;
    Bytes payload;
};

// Splits decrypted AirShield frames into DataX messages. One message may continue into the next
// frame; that fragment is always finished before any other message starts.
class Decoder {
  public:
    // Appends complete messages to out. False when framing is malformed; the link must close.
    [[nodiscard]] bool decode(ByteView plaintext, std::vector<Message>& out);

  private:
    struct Fragment {
        Header header;
        Bytes data;
        std::size_t needed;
    };
    std::optional<Fragment> fragment_;
    std::unordered_map<std::uint16_t, std::uint16_t> last_type_;

    void emit(Header header, ByteView payload, std::vector<Message>& out);
};

// Which service each channel carries. Keys are the low 15 bits; the originator bit says whose.
class ChannelRegistry {
  public:
    [[nodiscard]] std::optional<std::uint16_t> openLocal(std::uint16_t service);
    void bindInbound(std::uint16_t channel, std::uint16_t service);
    [[nodiscard]] std::uint16_t serviceForInbound(std::uint16_t channel) const;
    void closeInbound(std::uint16_t channel);

  private:
    // 0x8001 is the link-setup channel, so local service channels start at 2.
    static constexpr std::uint16_t FirstLocalChannel = 2;
    std::uint16_t next_ = FirstLocalChannel;
    std::unordered_map<std::uint16_t, std::uint16_t> local_;
    std::unordered_map<std::uint16_t, std::uint16_t> peer_;
};
} // namespace mgl::datax
#endif
