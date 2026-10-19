// SPDX-License-Identifier: AGPL-3.0-only
// Port of Visor core/protocol Header.kt and core/session DataXConnection.kt splitting at
// e7077b4051b1e3e605c667aa2a010fbe41f346a0. See THIRD_PARTY_NOTICES.md.
#include "mgl/datax.hpp"

#include "mgl/bytes.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace mgl::datax {
namespace {
constexpr std::uint8_t ExtensionBit = 0x80;
constexpr std::uint8_t LengthHighMask = 0x3F;
constexpr unsigned ByteShift = 8;
constexpr std::uint8_t ContinuationBit = 0x80;
constexpr std::uint8_t TagIdMask = 0x7F;
constexpr std::uint8_t TagService = 1;
constexpr std::uint8_t TagType = 2;
constexpr std::uint8_t TagError = 3;
constexpr std::uint8_t TagTypeClosed = 4;
constexpr std::uint8_t PadOffset = 0xC0;
constexpr std::size_t MaximumPad = 16;
constexpr std::size_t Big16Size = 2;

std::uint16_t readBig16(ByteView data, std::size_t offset) noexcept {
    const auto field = data.subspan(offset, Big16Size);
    return static_cast<std::uint16_t>((static_cast<unsigned>(field.front()) << ByteShift) |
                                      field.back());
}

// The 14-bit payload length carried by the first two header bytes.
std::size_t payloadLength(ByteView data) noexcept {
    const auto field = data.first(Big16Size);
    return (static_cast<std::size_t>(field.front() & LengthHighMask) << ByteShift) | field.back();
}

void appendTag(Bytes& out, std::uint8_t id, std::uint16_t value, bool more) {
    out.push_back(static_cast<std::uint8_t>((more ? ContinuationBit : 0U) | id));
    out.push_back(0);
    out.push_back(static_cast<std::uint8_t>(value >> ByteShift));
    out.push_back(static_cast<std::uint8_t>(value));
}

void applyTag(Header& header, std::uint8_t id, std::uint16_t value) noexcept {
    switch (id) {
    case TagService:
        header.service = value;
        header.has_service = true;
        header.has_type = true;
        header.closed = value == 0;
        break;
    case TagType:
        header.type = value;
        header.has_type = true;
        break;
    case TagError:
        header.error = value;
        header.has_error = true;
        break;
    case TagTypeClosed:
        header.type = value;
        header.closed = true;
        break;
    default:
        // QoS and unknown tags carry nothing the client acts on.
        break;
    }
}
} // namespace

std::optional<Header> parseHeader(ByteView data) {
    if (data.size() < FixedHeaderSize) {
        return std::nullopt;
    }
    Header header;
    header.payload_length = static_cast<std::uint16_t>(payloadLength(data));
    header.channel = readBig16(data, 2);
    if ((data.front() & ExtensionBit) == 0) {
        return header;
    }
    const auto available = std::min(data.size(), header.wireSize());
    for (;;) {
        if (header.header_size + TagSize > available) {
            return std::nullopt;
        }
        const auto entry = data.subspan(header.header_size, TagSize);
        const auto tag = entry.front();
        applyTag(header, static_cast<std::uint8_t>(tag & TagIdMask), readBig16(entry, 2));
        header.header_size += TagSize;
        if ((tag & ContinuationBit) == 0) {
            return header;
        }
    }
}

std::optional<Bytes> buildMessage(std::uint16_t channel, const Tags& tags, ByteView payload) {
    struct Entry {
        std::uint8_t id;
        std::uint16_t value;
    };
    std::vector<Entry> entries;
    if (tags.service) {
        entries.push_back({.id = TagService, .value = *tags.service});
    }
    if (tags.type) {
        entries.push_back({.id = TagType, .value = *tags.type});
    }
    if (tags.error) {
        entries.push_back({.id = TagError, .value = *tags.error});
    }
    const auto payload_length = (entries.size() * TagSize) + payload.size();
    if (payload_length > MaximumPayload) {
        return std::nullopt;
    }
    Bytes out;
    out.reserve(FixedHeaderSize + payload_length);
    const auto extension = entries.empty() ? 0U : ExtensionBit;
    out.push_back(static_cast<std::uint8_t>(extension | (payload_length >> ByteShift)));
    out.push_back(static_cast<std::uint8_t>(payload_length));
    out.push_back(static_cast<std::uint8_t>(channel >> ByteShift));
    out.push_back(static_cast<std::uint8_t>(channel));
    for (std::size_t index = 0; index < entries.size(); ++index) {
        const auto& entry = entries.at(index);
        appendTag(out, entry.id, entry.value, index + 1 < entries.size());
    }
    append(out, payload);
    return out;
}

ByteView withoutPad(ByteView plaintext) noexcept {
    if (plaintext.empty()) {
        return plaintext;
    }
    const auto last = plaintext.back();
    const auto pad = static_cast<std::size_t>(static_cast<std::uint8_t>(last - PadOffset));
    if (pad == 0 || pad > MaximumPad || pad > plaintext.size()) {
        return plaintext;
    }
    const auto tail = plaintext.last(pad);
    if (!std::ranges::all_of(tail, [last](std::uint8_t byte) { return byte == last; })) {
        return plaintext;
    }
    return plaintext.first(plaintext.size() - pad);
}

void Decoder::emit(Header header, ByteView payload, std::vector<Message>& out) {
    // Only an opening frame names its type; later ones on the channel reuse the last type.
    if (header.type != 0) {
        last_type_[header.channel] = header.type;
    } else if (!header.has_error) {
        if (const auto cached = last_type_.find(header.channel); cached != last_type_.end()) {
            header.type = cached->second;
        }
    }
    out.push_back({.header = header, .payload = Bytes(payload.begin(), payload.end())});
}

bool Decoder::decode(ByteView plaintext, std::vector<Message>& out) {
    std::size_t position = 0;
    if (fragment_) {
        const auto body = withoutPad(plaintext);
        const auto take = std::min(fragment_->needed - fragment_->data.size(), body.size());
        append(fragment_->data, body.first(take));
        position = take;
        if (fragment_->data.size() < fragment_->needed) {
            return true;
        }
        auto finished = std::move(*fragment_);
        fragment_.reset();
        emit(finished.header, finished.data, out);
    }
    while (plaintext.size() - position >= FixedHeaderSize) {
        const auto rest = plaintext.subspan(position);
        const auto wire_size = FixedHeaderSize + payloadLength(rest);
        if (rest.size() >= wire_size) {
            const auto header = parseHeader(rest.first(wire_size));
            if (!header) {
                return false;
            }
            emit(*header, rest.subspan(header->header_size, header->dataSize()), out);
            position += wire_size;
            continue;
        }
        // Either the padded tail of the frame or a message that continues in the next frame.
        const auto body = withoutPad(plaintext);
        if (body.size() <= position || body.size() - position < FixedHeaderSize) {
            return true;
        }
        const auto head = body.subspan(position);
        const auto header = parseHeader(head);
        if (!header || header->header_size > head.size()) {
            return false;
        }
        fragment_ =
            Fragment{.header = *header,
                     .data = Bytes(head.begin() + static_cast<std::ptrdiff_t>(header->header_size),
                                   head.end()),
                     .needed = header->dataSize()};
        return fragment_->data.size() < fragment_->needed;
    }
    return true;
}

std::optional<std::uint16_t> ChannelRegistry::openLocal(std::uint16_t service) {
    if (next_ > ChannelMask) {
        return std::nullopt;
    }
    const auto channel = next_++;
    local_[channel] = service;
    return static_cast<std::uint16_t>(OriginatorBit | channel);
}

void ChannelRegistry::bindInbound(std::uint16_t channel, std::uint16_t service) {
    auto& map = (channel & OriginatorBit) != 0 ? peer_ : local_;
    map[mirror(channel)] = service;
}

std::uint16_t ChannelRegistry::serviceForInbound(std::uint16_t channel) const {
    const auto& map = (channel & OriginatorBit) != 0 ? peer_ : local_;
    const auto found = map.find(mirror(channel));
    return found == map.end() ? 0 : found->second;
}

void ChannelRegistry::closeInbound(std::uint16_t channel) {
    auto& map = (channel & OriginatorBit) != 0 ? peer_ : local_;
    map.erase(mirror(channel));
}
} // namespace mgl::datax
