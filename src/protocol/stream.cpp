// SPDX-License-Identifier: AGPL-3.0-only
// Media streaming messages ported from Visor core/media StreamProto.kt at the revision in
// THIRD_PARTY_NOTICES.md. The glasses-hosted access point messages (svc10 CreateSoftApNetwork and
// SoftApNetworkInfo) follow the Meta AI app's protocol classes, com.oculus.atc.
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
std::string textOf(ByteView bytes) { return {bytes.begin(), bytes.end()}; }
} // namespace

Bytes buildCreateSoftApNetwork(const std::string& ssid, const std::string& passphrase,
                               std::uint32_t band, std::uint32_t bandwidth, std::uint32_t channel,
                               const std::string& country_code,
                               const std::array<std::uint8_t, 16>& uuid) {
    Bytes credentials;
    proto::appendStringField(credentials, 1, ssid);
    proto::appendStringField(credentials, 2, passphrase);
    Bytes out;
    proto::appendVarintField(out, 3, band);
    proto::appendVarintField(out, 4, channel);
    proto::appendStringField(out, 6, country_code);
    proto::appendBytesField(out, 7, {uuid.data(), uuid.size()});
    proto::appendBytesField(out, 8, {credentials.data(), credentials.size()});
    proto::appendVarintField(out, 9, bandwidth);
    return out;
}

std::optional<SoftApInfo> parseSoftApNetworkInfo(ByteView payload) {
    const auto message = proto::Message::parse(payload);
    if (!message) {
        return std::nullopt;
    }
    SoftApInfo info;
    info.status = static_cast<std::uint32_t>(message->varintOr(1, 0));
    info.port = static_cast<std::uint32_t>(message->varintOr(4, 0));
    info.channel = static_cast<std::uint32_t>(message->varintOr(5, 0));
    // field 2 repeated NetworkAddress, each with the address text in field 3.
    for (const auto entry : message->repeatedBytes(2)) {
        if (const auto address = proto::Message::parse(entry)) {
            if (const auto text = address->bytes(3)) {
                if (const auto value = textOf(*text); !value.empty()) {
                    info.address = value;
                    break;
                }
            }
        }
    }
    // field 3 credentials {1 ssid, 2 password}, echoed back.
    if (const auto creds = message->bytes(3)) {
        if (const auto parsed = proto::Message::parse(*creds)) {
            if (const auto ssid = parsed->bytes(1)) {
                info.ssid = textOf(*ssid);
            }
            if (const auto pass = parsed->bytes(2)) {
                info.passphrase = textOf(*pass);
            }
        }
    }
    return info;
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

std::optional<std::uint64_t> parseStreamStopped(ByteView payload) {
    const auto message = proto::Message::parse(payload);
    if (!message) {
        return std::nullopt;
    }
    return message->varintOr(2, 0);
}

std::string_view stopReasonText(std::uint64_t reason) {
    // The firmware's ControlStreamStopped reasons, in enum order (Visor StreamProto.kt).
    constexpr std::array<std::string_view, 24> Reasons{"unknown",
                                                       "user initiated",
                                                       "system preempt",
                                                       "system shutdown",
                                                       "battery low",
                                                       "thermal threshold",
                                                       "inactivity",
                                                       "camera failure",
                                                       "doff",
                                                       "temples closed",
                                                       "audio failure",
                                                       "mobile stream error",
                                                       "peak power interrupt",
                                                       "user pause",
                                                       "tamper detected",
                                                       "hardware toggle",
                                                       "shutdown for battery",
                                                       "frame initiated",
                                                       "client preempt",
                                                       "version not supported",
                                                       "client initiated",
                                                       "companion requested stop",
                                                       "weak signal",
                                                       "service destroyed"};
    return reason < Reasons.size() ? Reasons.at(reason) : Reasons.front();
}

std::optional<HealthEvent> parseHealthEvent(ByteView payload) {
    const auto message = proto::Message::parse(payload);
    if (!message) {
        return std::nullopt;
    }
    // Field 3 is a zigzag-coded sint64.
    const auto raw = message->varintOr(3, 0);
    const auto magnitude = static_cast<std::int64_t>(raw >> 1U);
    return HealthEvent{.status = message->varintOr(1, 0),
                       .type = message->varintOr(2, 0),
                       .level = (raw & 1U) != 0 ? -magnitude - 1 : magnitude};
}
} // namespace mgl::stream
