// SPDX-License-Identifier: AGPL-3.0-only
// Port of Visor core/media Wifi.kt and StreamProto.kt at the revision in THIRD_PARTY_NOTICES.md.
#include "mgl/stream.hpp"

#include "mgl/bytes.hpp"
#include "mgl/protobuf.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace mgl::stream {
namespace {
std::string textOf(ByteView bytes) {
    return std::string(reinterpret_cast<const char*>(bytes.data()),
                       bytes.size()); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}
ByteView strView(std::string_view text) {
    return {reinterpret_cast<const std::uint8_t*>(text.data()),
            text.size()}; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}
} // namespace

Bytes buildJoinAccessPoint(const std::string& ssid, const std::string& passphrase,
                           const std::array<std::uint8_t, 16>& uuid, std::uint8_t bandwidth) {
    Bytes out;
    proto::appendStringField(out, 1, ssid);
    proto::appendStringField(out, 2, passphrase);
    proto::appendBytesField(out, 3, {uuid.data(), uuid.size()});
    proto::appendVarintField(out, 4, bandwidth);
    return out;
}

std::optional<JoinResult> parseJoinResult(ByteView payload) {
    const auto message = proto::Message::parse(payload);
    if (!message) {
        return std::nullopt;
    }
    JoinResult result;
    result.status = static_cast<std::uint32_t>(message->varintOr(1, 0));
    result.port = static_cast<std::uint32_t>(message->varintOr(3, 0));
    // field 2 repeated NetworkAddress, each with the address text in field 3.
    for (const auto entry : message->repeatedBytes(2)) {
        const auto address = proto::Message::parse(entry);
        if (address) {
            if (const auto text = address->bytes(3)) {
                const auto value = textOf(*text);
                if (!value.empty()) {
                    result.address = value;
                    break;
                }
            }
        }
    }
    return result;
}

Bytes buildStartApp(std::uint16_t app_id) {
    Bytes out;
    proto::appendVarintField(out, 1, app_id);
    return out;
}

Bytes buildSubscribe() {
    // CompanionDeviceInit: field 1 = subscription (DEVICE_STATE = 1) as length-delimited varint,
    // field 2 = an interaction key string.
    Bytes subscription;
    proto::appendVarint(subscription, 1);
    Bytes out;
    proto::appendBytesField(out, 1, subscription);
    proto::appendStringField(out, 2, "meta-g-link-direct");
    return out;
}

Bytes buildSessionSettings(std::uint16_t app_type, bool bypass_don) {
    Bytes timers;
    proto::appendVarintField(timers, 1, 30);  // doff timer seconds
    proto::appendVarintField(timers, 2, 600); // inactivity timer seconds
    Bytes overrides;
    proto::appendVarintField(overrides, 1, bypass_don ? 1 : 0);
    proto::appendVarintField(overrides, 2, 0); // media batching interval ms
    proto::appendVarintField(overrides, 3, 1); // allow frame capture
    proto::appendVarintField(overrides, 4, 0); // allow OCR streaming
    Bytes out;
    proto::appendBytesField(out, 3, timers);
    proto::appendVarintField(out, 4, app_type);
    proto::appendBytesField(out, 5, overrides);
    return out;
}

Bytes buildConfigureVideo(std::uint32_t nonce, const VideoConfig& config) {
    Bytes video;
    proto::appendStringField(video, 1, "video/hevc");
    proto::appendVarintField(video, 2, config.width);
    proto::appendVarintField(video, 3, config.height);
    proto::appendVarintField(video, 4, config.bitrate);
    proto::appendVarintField(video, 5, config.frame_rate);
    proto::appendVarintField(video, 6, config.iframe_interval);
    proto::appendVarintField(video, 7, 1);                 // eisMode
    proto::appendVarintField(video, 8, config.frame_rate); // capture frame rate
    Bytes out;
    proto::appendVarintField(out, 1, nonce);
    proto::appendBytesField(out, 2, video);
    proto::appendVarintField(out, 3, 1); // enabled
    return out;
}

Bytes buildConfigureAudio(std::uint32_t nonce, bool enabled) {
    Bytes audio;
    proto::appendStringField(audio, 1, "audio/mp4a-latm");
    proto::appendVarintField(audio, 2, 48'000);
    proto::appendVarintField(audio, 3, 96'000);
    proto::appendVarintField(audio, 4, 2);
    proto::appendVarintField(audio, 5, 0); // AAC-LC
    Bytes out;
    proto::appendVarintField(out, 1, nonce);
    proto::appendBytesField(out, 2, audio);
    proto::appendVarintField(out, 3, enabled ? 1 : 0);
    return out;
}

Bytes buildControlStream(std::uint32_t nonce, std::uint8_t action) {
    Bytes out;
    proto::appendVarintField(out, 1, nonce);
    proto::appendVarintField(out, 2, action);
    return out;
}

std::optional<StreamAnswer> parseStreamAnswer(ByteView payload) {
    const auto message = proto::Message::parse(payload);
    if (!message) {
        return std::nullopt;
    }
    StreamAnswer answer;
    answer.nonce = static_cast<std::uint32_t>(message->varintOr(1, 0));
    if (const auto error = message->bytes(2)) {
        const auto inner = proto::Message::parse(*error);
        if (inner) {
            answer.error = static_cast<std::uint32_t>(inner->varintOr(1, 0));
        }
    }
    return answer;
}

std::optional<CodecConfig> parseCodecConfig(ByteView payload) {
    const auto message = proto::Message::parse(payload);
    if (!message) {
        return std::nullopt;
    }
    CodecConfig config;
    config.codec = static_cast<std::uint32_t>(message->varintOr(2, 0));
    if (const auto data = message->bytes(3)) {
        config.data.assign(data->begin(), data->end());
    }
    return config;
}
} // namespace mgl::stream
