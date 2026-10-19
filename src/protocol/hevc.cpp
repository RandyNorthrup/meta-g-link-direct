// SPDX-License-Identifier: AGPL-3.0-only
// Port of Visor core/media Hevc.kt (annexBUnits, hevcNalType, parseHevcSps) at the revision in
// THIRD_PARTY_NOTICES.md. Scope limited to frame/keyframe detection and the SPS picture size.
#include "mgl/hevc.hpp"

#include "mgl/bytes.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <vector>

namespace mgl::hevc {
namespace {
constexpr int FirstKeyframeNal = 16;
constexpr int LastKeyframeNal = 21;
constexpr int SpsNal = 33;
constexpr unsigned NalTypeMask = 0x3F;
constexpr std::size_t NalHeaderSize = 2;
constexpr std::array<std::uint8_t, 3> StartCode{0, 0, 1};

int nalType(std::uint8_t first_byte) {
    return static_cast<int>((static_cast<unsigned>(first_byte) >> 1U) & NalTypeMask);
}

// The NAL units of an Annex-B buffer (payloads, start codes removed).
std::vector<ByteView> units(ByteView data) {
    std::vector<std::size_t> starts;
    std::vector<std::size_t> ends;
    std::size_t index = 0;
    while (index + StartCode.size() <= data.size()) {
        if (std::ranges::equal(data.subspan(index, StartCode.size()), StartCode)) {
            if (!starts.empty()) {
                // A zero byte before 00 00 01 belongs to a four-byte start code.
                const bool four_byte_code = index > 0 && data.subspan(index - 1).front() == 0;
                ends.push_back(four_byte_code ? index - 1 : index);
            }
            starts.push_back(index + StartCode.size());
            index += StartCode.size();
        } else {
            ++index;
        }
    }
    std::vector<ByteView> found;
    if (starts.empty()) {
        return found;
    }
    ends.push_back(data.size());
    for (std::size_t i = 0; i < starts.size(); ++i) {
        const auto start = starts.at(i);
        const auto end = ends.at(i);
        if (start + NalHeaderSize <= end) {
            found.push_back(data.subspan(start, end - start));
        }
    }
    return found;
}

// Removes HEVC emulation-prevention bytes (00 00 03 -> 00 00) from a NAL payload.
Bytes unescape(ByteView unit) {
    Bytes out;
    out.reserve(unit.size());
    int zeros = 0;
    for (const auto byte : unit) {
        if (zeros >= 2 && byte == 3) {
            zeros = 0;
            continue;
        }
        out.push_back(byte);
        zeros = byte == 0 ? zeros + 1 : 0;
    }
    return out;
}

// Exp-Golomb / fixed-bit reader over a NAL payload starting after the two-byte NAL header.
class BitReader {
  public:
    BitReader(ByteView data, std::size_t start_byte) : data_(data), pos_(start_byte * 8) {}

    std::uint32_t bits(unsigned count) {
        std::uint32_t value = 0;
        for (unsigned i = 0; i < count; ++i) {
            const auto byte_index = pos_ >> 3U;
            std::uint32_t bit = 0;
            if (byte_index < data_.size()) {
                const auto byte = static_cast<std::uint32_t>(data_.subspan(byte_index).front());
                bit = (byte >> (7U - (pos_ & 7U))) & 1U;
            }
            value = (value << 1U) | bit;
            ++pos_;
        }
        return value;
    }

    // Exp-Golomb ue(v). A code with 32 or more leading zeros cannot fit 32 bits: it is malformed
    // (or runs past the end, which reads as zeros) and yields MalformedCode, which no caller
    // accepts as a picture size.
    std::uint32_t ue() {
        constexpr unsigned MaxLeadingZeros = 31;
        unsigned zeros = 0;
        while (bits(1) == 0) {
            if (zeros == MaxLeadingZeros) {
                return MalformedCode;
            }
            ++zeros;
        }
        return ((1U << zeros) - 1U) + bits(zeros);
    }

    static constexpr std::uint32_t MalformedCode = std::numeric_limits<std::uint32_t>::max();

    void skip(unsigned count) { pos_ += count; }

  private:
    ByteView data_;
    std::size_t pos_;
};

// The sub-layer part of profile_tier_level, after the 12-byte general profile, tier and level.
void skipSubLayers(BitReader& reader, std::uint32_t sub_layers) {
    std::vector<bool> present(sub_layers, false);
    std::vector<bool> level_present(sub_layers, false);
    for (std::uint32_t i = 0; i < sub_layers; ++i) {
        present.at(i) = reader.bits(1) == 1;
        level_present.at(i) = reader.bits(1) == 1;
    }
    if (sub_layers > 0) {
        for (std::uint32_t i = sub_layers; i < 8; ++i) {
            reader.bits(2);
        }
    }
    for (std::uint32_t i = 0; i < sub_layers; ++i) {
        if (present.at(i)) {
            reader.skip(88);
        }
        if (level_present.at(i)) {
            reader.skip(8);
        }
    }
}

// The SPS fields from sps_seq_parameter_set_id through the conformance window.
std::optional<Dimensions> pictureSize(BitReader& reader) {
    reader.ue(); // sps_seq_parameter_set_id
    const auto chroma = reader.ue();
    if (chroma == 3) {
        reader.bits(1);
    }
    std::int64_t width = reader.ue();
    std::int64_t height = reader.ue();
    if (reader.bits(1) == 1) { // conformance window
        const std::int64_t sub_w = (chroma == 1 || chroma == 2) ? 2 : 1;
        const std::int64_t sub_h = (chroma == 1) ? 2 : 1;
        const std::int64_t left = reader.ue();
        const std::int64_t right = reader.ue();
        const std::int64_t top = reader.ue();
        const std::int64_t bottom = reader.ue();
        width -= sub_w * (left + right);
        height -= sub_h * (top + bottom);
    }
    // HEVC's largest level (6.2) tops out at 8192 x 4320; anything beyond 16384 is malformed.
    constexpr std::int64_t MaxPictureSide = 16384;
    if (width <= 0 || height <= 0 || width > MaxPictureSide || height > MaxPictureSide) {
        return std::nullopt;
    }
    return Dimensions{.width = static_cast<std::uint32_t>(width),
                      .height = static_cast<std::uint32_t>(height)};
}
} // namespace

std::vector<int> nalTypes(ByteView annex_b) {
    const auto found = units(annex_b);
    std::vector<int> types;
    types.reserve(found.size());
    std::ranges::transform(found, std::back_inserter(types),
                           [](ByteView unit) { return nalType(unit.front()); });
    return types;
}

bool hasKeyframe(ByteView annex_b) {
    return std::ranges::any_of(nalTypes(annex_b), [](int type) {
        return type >= FirstKeyframeNal && type <= LastKeyframeNal;
    });
}

std::optional<Dimensions> spsDimensions(ByteView annex_b) {
    for (const auto nal : units(annex_b)) {
        if (nalType(nal.front()) != SpsNal) {
            continue;
        }
        const auto unit = unescape(nal);
        BitReader reader(ByteView(unit.data(), unit.size()), NalHeaderSize);
        reader.bits(4); // sps_video_parameter_set_id
        const auto sub_layers = reader.bits(3);
        reader.bits(1);  // temporal_id_nesting
        reader.skip(96); // general_profile_tier_level, 12 bytes
        skipSubLayers(reader, sub_layers);
        return pictureSize(reader);
    }
    return std::nullopt;
}
} // namespace mgl::hevc
