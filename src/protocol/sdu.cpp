// SPDX-License-Identifier: AGPL-3.0-only
// Port of core/transport/.../Sdu.kt from macedonga/visor, commit
// e7077b4051b1e3e605c667aa2a010fbe41f346a0. See THIRD_PARTY_NOTICES.md.
#include "mgl/sdu.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace mgl {
namespace {
constexpr std::size_t DataxHeader = 4;
constexpr std::size_t AirshieldHeader = 10;
constexpr std::size_t AesBlock = 16;
constexpr std::size_t MinimumPrefix = 2;
constexpr std::uint8_t DataxBit = 0x80;
constexpr std::uint8_t LengthHighMask = 0x3F;
constexpr unsigned LengthHighShift = 8;
constexpr std::size_t CommandSize = 2;
constexpr std::uint8_t LongCommandBit = 0x80;
constexpr std::size_t LongCommandHeader = 4;
constexpr std::size_t MaximumAdvertisement = 1024;

FramePrefix airshieldData(std::span<const std::uint8_t> head) noexcept {
    if (head.size() < AirshieldHeader) {
        return {.status = PrefixStatus::NeedMore, .frame_size = 0};
    }
    const auto block_count = static_cast<std::size_t>(head[AirshieldHeader - 1]) + 1;
    return {.status = PrefixStatus::Ready,
            .frame_size = AirshieldHeader + (AesBlock * block_count)};
}

// COMMAND: [0x80 | stream][code]; the ADVERTISEMENT long form sets bit 7 of the code and adds a
// big-endian 16-bit length of at most 1024 payload bytes.
FramePrefix airshieldCommand(std::span<const std::uint8_t> head) noexcept {
    if ((head[1] & LongCommandBit) == 0) {
        return {.status = PrefixStatus::Ready, .frame_size = CommandSize};
    }
    if (head.size() < LongCommandHeader) {
        return {.status = PrefixStatus::NeedMore, .frame_size = 0};
    }
    const auto length = (static_cast<std::size_t>(head[2]) << LengthHighShift) | head[3];
    if (length > MaximumAdvertisement) {
        return {.status = PrefixStatus::Invalid, .frame_size = 0};
    }
    return {.status = PrefixStatus::Ready, .frame_size = LongCommandHeader + length};
}
} // namespace

FramePrefix inspectFramePrefix(std::span<const std::uint8_t> head) noexcept {
    return inspectFramePrefix(head, LinkPhase::Plaintext);
}

FramePrefix inspectFramePrefix(std::span<const std::uint8_t> head, LinkPhase phase) noexcept {
    if (head.size() < MinimumPrefix) {
        return {.status = PrefixStatus::NeedMore, .frame_size = 0};
    }
    const auto first = head.front();
    if ((first & AirshieldTypeMask) == AirshieldData) {
        return airshieldData(head);
    }
    if (phase == LinkPhase::Encrypted) {
        if ((first & AirshieldTypeMask) == AirshieldCommand) {
            return airshieldCommand(head);
        }
        return {.status = PrefixStatus::Invalid, .frame_size = 0};
    }
    if ((first & DataxBit) != 0) {
        const auto payload_size =
            (static_cast<std::size_t>(first & LengthHighMask) << LengthHighShift) |
            static_cast<std::size_t>(head[1]);
        return {.status = PrefixStatus::Ready, .frame_size = DataxHeader + payload_size};
    }
    return {.status = PrefixStatus::Invalid, .frame_size = 0};
}
} // namespace mgl
