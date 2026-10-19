// SPDX-License-Identifier: AGPL-3.0-only
// Port of Visor core/media Hevc.kt (annexBUnits, hevcNalType, parseHevcSps) at the revision in
// THIRD_PARTY_NOTICES.md. Scope limited to frame/keyframe detection and the SPS picture size.
#include "mgl/hevc.hpp"

#include "mgl/bytes.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace mgl::hevc {
namespace {
constexpr int FirstKeyframeNal = 16;
constexpr int LastKeyframeNal = 21;
constexpr int VpsNal = 32;
constexpr int SpsNal = 33;

int nalType(std::uint8_t first_byte) { return (first_byte >> 1) & 0x3F; }

// The NAL units of an Annex-B buffer (payloads, start codes removed).
std::vector<std::pair<std::size_t, std::size_t>> units(ByteView data) {
    std::vector<std::size_t> starts;
    std::vector<std::size_t> ends;
    std::size_t index = 0;
    while (index + 2 < data.size()) {
        if (data[index] == 0 && data[index + 1] == 0 && data[index + 2] == 1) {
            if (!starts.empty()) {
                ends.push_back(index > 0 && data[index - 1] == 0 ? index - 1 : index);
            }
            starts.push_back(index + 3);
            index += 3;
        } else {
            ++index;
        }
    }
    std::vector<std::pair<std::size_t, std::size_t>> ranges;
    if (starts.empty()) {
        return ranges;
    }
    ends.push_back(data.size());
    for (std::size_t i = 0; i < starts.size(); ++i) {
        if (starts[i] + 2 <= ends[i]) {
            ranges.emplace_back(starts[i], ends[i]);
        }
    }
    return ranges;
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
                bit = (data_[byte_index] >> (7U - (pos_ & 7U))) & 1U;
            }
            value = (value << 1U) | bit;
            ++pos_;
        }
        return value;
    }

    std::uint32_t ue() {
        unsigned zeros = 0;
        while (bits(1) == 0 && zeros < 32) {
            ++zeros;
        }
        return ((1U << zeros) - 1U) + bits(zeros);
    }

    void skip(unsigned count) { pos_ += count; }

  private:
    ByteView data_;
    std::size_t pos_;
};
} // namespace

std::vector<int> nalTypes(ByteView annex_b) {
    std::vector<int> types;
    for (const auto& [start, end] : units(annex_b)) {
        types.push_back(nalType(annex_b[start]));
    }
    return types;
}

bool hasKeyframe(ByteView annex_b) {
    for (const int type : nalTypes(annex_b)) {
        if (type >= FirstKeyframeNal && type <= LastKeyframeNal) {
            return true;
        }
    }
    return false;
}

bool hasParameterSets(ByteView annex_b) {
    for (const int type : nalTypes(annex_b)) {
        if (type >= VpsNal && type <= SpsNal + 1) {
            return true;
        }
    }
    return false;
}

std::optional<Dimensions> spsDimensions(ByteView annex_b) {
    for (const auto& [start, end] : units(annex_b)) {
        if (nalType(annex_b[start]) != SpsNal) {
            continue;
        }
        const auto unit = unescape(annex_b.subspan(start, end - start));
        BitReader reader(ByteView(unit.data(), unit.size()), 2); // skip the two-byte NAL header
        reader.bits(4);                                          // sps_video_parameter_set_id
        const auto sub_layers = reader.bits(3);
        reader.bits(1);  // temporal_id_nesting
        reader.skip(96); // general_profile_tier_level, 12 bytes
        std::vector<bool> present(sub_layers, false);
        std::vector<bool> level_present(sub_layers, false);
        for (std::uint32_t i = 0; i < sub_layers; ++i) {
            present[i] = reader.bits(1) == 1;
            level_present[i] = reader.bits(1) == 1;
        }
        if (sub_layers > 0) {
            for (std::uint32_t i = sub_layers; i < 8; ++i) {
                reader.bits(2);
            }
        }
        for (std::uint32_t i = 0; i < sub_layers; ++i) {
            if (present[i]) {
                reader.skip(88);
            }
            if (level_present[i]) {
                reader.skip(8);
            }
        }
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
        if (width <= 0 || height <= 0) {
            return std::nullopt;
        }
        return Dimensions{.width = static_cast<std::uint32_t>(width),
                          .height = static_cast<std::uint32_t>(height)};
    }
    return std::nullopt;
}
} // namespace mgl::hevc
