// SPDX-License-Identifier: AGPL-3.0-only
// The glasses' Wi-Fi link (svc10) and media streaming control (svc51350) messages. Media streaming
// is ported in behaviour from Visor core/media StreamProto.kt at the revision in
// THIRD_PARTY_NOTICES.md; the glasses-hosted access point follows the Meta AI app's com.oculus.atc
// protocol classes.
#ifndef MGL_STREAM_HPP
#define MGL_STREAM_HPP

#include "mgl/bytes.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace mgl::stream {
// svc10 ATC LinkManager2 message types, from the Meta app (X/EnumC41442Mo5.java).
inline constexpr std::uint16_t ServiceAtc = 10;
inline constexpr std::uint16_t MtCreateSoftApNetwork = 0x1000;
inline constexpr std::uint16_t MtSoftApNetworkInfo = 0x1001;
inline constexpr std::uint16_t MtDestroySoftApNetwork = 0x1002;
inline constexpr std::uint16_t MtDestroySoftApNetworkResult = 0x1003;

// Band enum FKv (0 unspecified, 1 2.4 GHz, 2 5 GHz) and width enum EnumC214916e (0 unspecified,
// 1 20 MHz, 2 40 MHz, 3 80 MHz). The camera stream is proven only on 2.4 GHz at 20 MHz.
inline constexpr std::uint32_t Band24GHz = 1;
inline constexpr std::uint32_t Bw20MHz = 1;

// svc28 app manager.
inline constexpr std::uint16_t ServiceStatus = 28;
inline constexpr std::uint16_t MtStartApp = 2000;
inline constexpr std::uint16_t MtStartAppResponse = 2001;
inline constexpr std::uint16_t MtInit = 1000; // device-status subscribe
inline constexpr std::uint16_t AppLivestream = 3;

// svc100 peer build info (Visor Canary.kt). An empty app-info announce is answered with the
// glasses' device info; Visor sends it every 2 s as the Wi-Fi session keepalive.
inline constexpr std::uint16_t ServicePeerBuildInfo = 100;
inline constexpr std::uint16_t MtPeerAppInfo = 1;
inline constexpr std::uint16_t MtPeerDeviceInfo = 3;

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
inline constexpr std::uint16_t MtStreamingLifecycle = 19;
inline constexpr std::uint16_t MtSystemHealth = 21;
inline constexpr std::uint16_t MtSupportedConfiguration = 22;
inline constexpr std::uint16_t MtSessionSettings = 27;

inline constexpr std::uint8_t ActionStart = 1;
inline constexpr std::uint8_t ActionStop = 2;

// CreateSoftApNetwork (0x1000): asks the glasses to host their own access point on the given band,
// width and channel, with the given WPA2 SSID and passphrase. The PC then joins it as a client.
// Fields: 3 band (FKv), 4 channel, 6 country code, 7 UUID (v4, 16 bytes), 8 credentials
// {1 ssid, 2 password}, 9 bandwidth (EnumC214916e).
[[nodiscard]] Bytes buildCreateSoftApNetwork(const std::string& ssid, const std::string& passphrase,
                                             std::uint32_t band, std::uint32_t bandwidth,
                                             std::uint32_t channel, const std::string& country_code,
                                             const std::array<std::uint8_t, 16>& uuid);
// DestroySoftApNetwork (0x1002) has an empty body; the glasses answer
// DestroySoftApNetworkResult (0x1003) with a status in field 1.

// SoftApNetworkInfo (0x1001): the glasses' access point once it is up.
struct SoftApInfo {
    std::uint32_t status = 0; // G1Q: 1 = SUCCESS
    std::string address;      // the glasses' address on their own network
    std::uint32_t port = 0;   // the media port the glasses listen on
    std::string ssid;         // echoed credentials
    std::string passphrase;
    std::uint32_t channel = 0;
    [[nodiscard]] bool ok() const noexcept { return status == 1 && !address.empty() && port > 0; }
};
[[nodiscard]] std::optional<SoftApInfo> parseSoftApNetworkInfo(ByteView payload);

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

// ControlStreamStopped (type 13): field 2 is the firmware's stop reason. nullopt if malformed.
[[nodiscard]] std::optional<std::uint64_t> parseStreamStopped(ByteView payload);
// The firmware's name for a stop reason, as Visor StreamProto.kt lists them ("thermal threshold",
// "doff", "weak signal", ...), or "unknown" past the end of the list.
[[nodiscard]] std::string_view stopReasonText(std::uint64_t reason);

// SystemHealthEvent (type 21): status, type (2 battery, 3 thermal) and a zigzag-coded level.
struct HealthEvent {
    std::uint64_t status = 0;
    std::uint64_t type = 0;
    std::int64_t level = 0;
};
[[nodiscard]] std::optional<HealthEvent> parseHealthEvent(ByteView payload);

// CodecConfigurationUpdateRequest (type 1): field 2 codec (1 audio, 2 video), field 3 bytes.
struct CodecConfig {
    std::uint32_t codec = 0;
    Bytes data; // Annex-B parameter sets for video
};
[[nodiscard]] std::optional<CodecConfig> parseCodecConfig(ByteView payload);
} // namespace mgl::stream
#endif
