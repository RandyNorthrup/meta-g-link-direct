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
constexpr std::uint8_t AirshieldDataBits = 0x40;
constexpr std::uint8_t TypeMask = 0xC0;
constexpr std::uint8_t LengthHighMask = 0x3F;
constexpr unsigned LengthHighShift = 8;
} // namespace

FramePrefix inspectFramePrefix(std::span<const std::uint8_t> head) noexcept {
    if (head.size() < MinimumPrefix) {
        return {.status = PrefixStatus::NeedMore, .frame_size = 0};
    }
    const auto first = head.front();
    if ((first & DataxBit) != 0) {
        const auto payload_size =
            (static_cast<std::size_t>(first & LengthHighMask) << LengthHighShift) |
            static_cast<std::size_t>(head.subspan<1>().front());
        return {.status = PrefixStatus::Ready, .frame_size = DataxHeader + payload_size};
    }
    if ((first & TypeMask) == AirshieldDataBits) {
        if (head.size() < AirshieldHeader) {
            return {.status = PrefixStatus::NeedMore, .frame_size = 0};
        }
        const auto block_count =
            static_cast<std::size_t>(head.subspan<AirshieldHeader - 1>().front()) + 1;
        return {.status = PrefixStatus::Ready,
                .frame_size = AirshieldHeader + (AesBlock * block_count)};
    }
    return {.status = PrefixStatus::Invalid, .frame_size = 0};
}
} // namespace mgl
