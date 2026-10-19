// SPDX-License-Identifier: AGPL-3.0-only
// The glasses' Wi-Fi join (svc10) and media streaming control (svc51350) messages. Ported in
// behaviour from Visor core/media Wifi.kt and StreamProto.kt at the revision in
// THIRD_PARTY_NOTICES.md.
#ifndef MGL_STREAM_HPP
#define MGL_STREAM_HPP

#include "mgl/bytes.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace mgl::stream {
// svc10 ATC LinkManager2.
inline constexpr std::uint16_t ServiceAtc = 10;
inline constexpr std::uint16_t MtJoinAccessPoint = 0x1010;
inline constexpr std::uint16_t MtJoinAccessPointResult = 0x1011;
inline constexpr std::uint16_t MtLeaveAccessPoint = 0x1012;

// svc28 app manager.
inline constexpr std::uint16_t ServiceStatus = 28;
inline constexpr std::uint16_t MtStartApp = 2000;
inline constexpr std::uint16_t MtStartAppResponse = 2001;
inline constexpr std::uint16_t MtInit = 1000; // device-status subscribe
inline constexpr std::uint16_t AppLivestream = 3;

// svc51350 media streaming control.
inline constexpr std::uint16_t ServiceMedia = 51350;
inline constexpr std::uint16_t MtCodecConfig = 1;
inline constexpr std::uint16_t MtConfigureAudio = 3;
inline constexpr std::uint16_t MtConfigureAudioResponse = 4;
inline constexpr std::uint16_t MtConfigureVideo = 5;
inline constexpr std::uint16_t MtConfigureVideoResponse = 6;
inline constexpr std::uint16_t MtControlStream = 7;
inline constexpr std::uint16_t MtControlStreamResponse = 8;
inline constexpr std::uint16_t MtAudioChunk = 9;
inline constexpr std::uint16_t MtVideoChunk = 10;
inline constexpr std::uint16_t MtAudioChunkEnd = 11;
inline constexpr std::uint16_t MtVideoChunkEnd = 12;
inline constexpr std::uint16_t MtControlStreamStopped = 13;
inline constexpr std::uint16_t MtSystemHealth = 21;
inline constexpr std::uint16_t MtSessionSettings = 27;

inline constexpr std::uint8_t ActionStart = 1;
inline constexpr std::uint8_t ActionStop = 2;

// Bandwidth enum for the join; 1 = 20 MHz, the only width the glasses' Wi-Fi firmware holds stably.
inline constexpr std::uint8_t Bandwidth20MHz = 1;

// JoinAccessPoint: ssid, passphrase, a v4 UUID (16 bytes), and bandwidth.
[[nodiscard]] Bytes buildJoinAccessPoint(const std::string& ssid, const std::string& passphrase,
                                         const std::array<std::uint8_t, 16>& uuid,
                                         std::uint8_t bandwidth);

struct JoinResult {
    std::uint32_t status = 0; // 1 = SUCCESS
    std::string address;
    std::uint32_t port = 0;
    [[nodiscard]] bool ok() const noexcept { return status == 1 && !address.empty() && port > 0; }
};
[[nodiscard]] std::optional<JoinResult> parseJoinResult(ByteView payload);

// svc28 StartApp field 1 = app id.
[[nodiscard]] Bytes buildStartApp(std::uint16_t app_id);
// CompanionDeviceInit (type 1000) subscribing to DEVICE_STATE.
[[nodiscard]] Bytes buildSubscribe();

struct VideoConfig {
    std::uint32_t width = 720;
    std::uint32_t height = 1280;
    std::uint32_t bitrate = 4'000'000;
    std::uint32_t frame_rate = 30;
    std::uint32_t iframe_interval = 1;
};

// SessionSettings (type 27): timers, application type, override settings.
[[nodiscard]] Bytes buildSessionSettings(std::uint16_t app_type, bool bypass_don);
// ConfigureVideoStreamRequest (type 5): nonce, VideoConfig, enabled.
[[nodiscard]] Bytes buildConfigureVideo(std::uint32_t nonce, const VideoConfig& config);
// ConfigureAudioStreamRequest (type 3): nonce, AudioConfig, enabled flag.
[[nodiscard]] Bytes buildConfigureAudio(std::uint32_t nonce, bool enabled);
// ControlStreamRequest (type 7): nonce, action.
[[nodiscard]] Bytes buildControlStream(std::uint32_t nonce, std::uint8_t action);

struct StreamAnswer {
    std::uint32_t nonce = 0;
    std::uint32_t error = 0; // 0 = success
};
[[nodiscard]] std::optional<StreamAnswer> parseStreamAnswer(ByteView payload);

// CodecConfigurationUpdateRequest (type 1): field 2 codec (1 audio, 2 video), field 3 bytes.
struct CodecConfig {
    std::uint32_t codec = 0;
    Bytes data; // Annex-B parameter sets for video
};
[[nodiscard]] std::optional<CodecConfig> parseCodecConfig(ByteView payload);
} // namespace mgl::stream
#endif
