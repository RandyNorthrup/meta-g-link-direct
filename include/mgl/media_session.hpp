// SPDX-License-Identifier: AGPL-3.0-only
// The live glasses link as a reusable, self-driving session: it owns a worker thread that
// authenticates over Bluetooth, asks the glasses to host their own 2.4 GHz access point, joins it
// from this PC, opens the TCP media session and then follows a demand: it streams the wanted media,
// changes the stream in place when the demand changes (the glasses accept a stop, a new
// configuration and a start within one session), keeps the link up with the camera off when asked
// to, and disconnects when nothing is wanted. It delivers reassembled HEVC and AAC access units and
// performs no decoding itself. The OBS plugin and the diagnostic CLI both drive it.
#ifndef MGL_MEDIA_SESSION_HPP
#define MGL_MEDIA_SESSION_HPP

#include "mgl/bytes.hpp"
#include "mgl/stream.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

namespace mgl {
namespace crypto {
class OwnerKey;
} // namespace crypto
class GlassesSession;

// Ready means the link is up with nothing streaming. Streaming means media is arriving, not merely
// that it was requested. Waiting means another session in this process holds the glasses.
enum class MediaState : std::uint8_t {
    Idle,
    Waiting,
    Connecting,
    Ready,
    Streaming,
    Reconnecting,
    Failed,
    Stopped
};

struct MediaCallbacks {
    // One reassembled HEVC access unit: presentation time (microseconds, glasses clock) and the
    // Annex-B bytes. Called on the worker thread.
    std::function<void(std::uint64_t pts_us, ByteView annex_b, bool keyframe)> on_frame;
    // The HEVC parameter sets (VPS/SPS/PPS, Annex-B) when the glasses send or update them.
    std::function<void(ByteView parameter_sets)> on_parameters;
    // When audio starts: the AAC AudioSpecificConfig, then one raw AAC access unit at a time
    // (presentation time in microseconds, glasses clock). Called on the worker thread. Optional.
    std::function<void(ByteView audio_specific_config)> on_audio_config;
    std::function<void(std::uint64_t pts_us, ByteView aac)> on_audio;
    // A human-readable status line, redacted of identifiers. Optional.
    std::function<void(const std::string&)> on_log;
    // A state transition. Optional.
    std::function<void(MediaState)> on_state;
    // The glasses' battery whenever it changes while the media link is up. Optional.
    std::function<void(const stream::BatteryState&)> on_battery;
};

// What the glasses are wanted for right now. The session connects while wantsLink() holds, streams
// the wanted media, and applies a changed choice to the running stream.
struct MediaDemand {
    bool video = false;     // camera frames
    bool audio = false;     // the glasses' microphones, 48 kHz stereo AAC
    bool keep_link = false; // stay connected with the camera off while nothing streams
    stream::VideoConfig video_config;
    // The WifiAdapter::id that joins the glasses' network; empty uses the first adapter Windows
    // lists. Changing it reconnects.
    std::string wifi_adapter;

    bool operator==(const MediaDemand&) const = default;
    [[nodiscard]] bool wantsStream() const noexcept { return video || audio; }
    [[nodiscard]] bool wantsLink() const noexcept { return wantsStream() || keep_link; }
};

class MediaSession {
  public:
    explicit MediaSession(MediaCallbacks callbacks);
    ~MediaSession();
    MediaSession(const MediaSession&) = delete;
    MediaSession& operator=(const MediaSession&) = delete;
    MediaSession(MediaSession&&) = delete;
    MediaSession& operator=(MediaSession&&) = delete;

    // Replaces the demand. Safe from any thread; the worker applies it within a fraction of a
    // second.
    void setDemand(const MediaDemand& demand);
    // Launches the worker thread. Returns immediately. Only one session per process drives the
    // glasses at a time; a later one waits (state Waiting) until the earlier one has finished.
    void start();
    // Asks the worker to finish and returns at once. The worker still tears the link down (stops
    // the stream, leaves the glasses' Wi-Fi network) before it exits; stop() or the destructor
    // waits for that.
    void requestStop() noexcept;
    // Signals the worker to stop and joins it. Safe to call more than once.
    void stop();
    // True once the worker has ended on its own: no usable identity, or the glasses refused this
    // PC's owner key. A new session is needed after the setup runs again.
    [[nodiscard]] bool ended() const noexcept { return ended_.load(); }

  private:
    // How one connection attempt ended. Fatal stops retrying (no usable identity); Failed is a
    // transient failure before any media; Dropped means media flowed and then stopped; Released
    // means the demand no longer needs this link (nothing wanted, or another Wi-Fi adapter).
    enum class SessionEnd : std::uint8_t { Fatal, Failed, Dropped, Released };
    // The media the running stream carries; nothing configured yet is the empty setup.
    struct StreamSetup {
        std::optional<stream::VideoConfig> video;
        bool audio = false;
        bool operator==(const StreamSetup&) const = default;
        [[nodiscard]] bool streams() const noexcept { return video.has_value() || audio; }
    };
    // How one acknowledged stream request ended: answered (or tolerably unanswered), refused by the
    // glasses with an error code, or failed because the link dropped or stop was requested.
    enum class Step : std::uint8_t { Done, Refused, Failed };
    // What has been sent to the glasses in this media session, so a change sends only what differs.
    struct Configured {
        std::optional<stream::VideoConfig> video; // the last enabled profile
        bool video_enabled = false;
        std::optional<bool> audio;
        bool audio_refused = false; // the glasses refused audio; keep video without it
    };
    // The Wi-Fi TCP transport and the media session over it, torn down session first.
    struct MediaLink;
    // The Bluetooth session that asked the glasses to host their access point, and where to reach
    // it. The glasses leave Bluetooth once this PC joins their network.
    struct ControlLink;

    MediaCallbacks callbacks_;
    std::atomic<MediaState> state_{MediaState::Idle};
    std::atomic<bool> stop_{false};
    std::atomic<bool> ended_{false};
    mutable std::mutex demand_mutex_;
    std::condition_variable demand_changed_;
    MediaDemand demand_; // guarded by demand_mutex_
    std::thread worker_;
    // When the current connection attempt began; the step log lines report time since then.
    std::chrono::steady_clock::time_point attempt_started_;
    // The stream request counter of the current media session; worker thread only.
    std::uint32_t nonce_ = 1;

    void run();
    // The reconnect loop, once this session holds the glasses.
    void runConnected();
    // Blocks until the demand wants the link or stop is requested; false on stop.
    [[nodiscard]] bool awaitDemand();
    [[nodiscard]] MediaDemand currentDemand() const;
    // runSession inside the worker's exception boundary.
    [[nodiscard]] SessionEnd attemptSession();
    // One connection attempt: connect, then serve the demand until the link ends or is released.
    [[nodiscard]] SessionEnd runSession();
    // Authenticates over Bluetooth and asks the glasses to host their own 2.4 GHz access point.
    // On failure returns null and sets failure: Fatal when the glasses refuse this PC's owner key,
    // Failed otherwise.
    [[nodiscard]] std::unique_ptr<ControlLink> openControlLink(const crypto::OwnerKey& owner,
                                                               SessionEnd& failure);
    // After a failure before the media link opens, closes the glasses' access point over
    // Bluetooth so the retry finds the glasses' Wi-Fi free.
    void abandonAccessPoint(ControlLink& control);
    // Opens the AirShield media session over the glasses' access point, retrying while the
    // glasses' server comes up.
    [[nodiscard]] std::unique_ptr<MediaLink> openMediaLink(const crypto::OwnerKey& owner,
                                                           const stream::SoftApInfo& access_point);
    // Starts the glasses' livestream app; once per media session, before any stream.
    [[nodiscard]] bool prepareMedia(GlassesSession& media);
    // Follows the demand over an open media session until it ends or no longer fits the demand.
    [[nodiscard]] SessionEnd serve(GlassesSession& media, const std::string& wifi_adapter);
    // Why the open link no longer fits the demand (stop, nothing wanted, another adapter), or
    // nullptr while it still does.
    [[nodiscard]] const char* releaseReason(const MediaDemand& demand,
                                            const std::string& wifi_adapter) const;
    // Moves the stream from running to wanted: stop, send what changed, start. Returns the setup
    // now running, which lacks audio when the glasses refused it, or nullopt when the link failed.
    [[nodiscard]] std::optional<StreamSetup> applySetup(GlassesSession& media,
                                                        const StreamSetup& running,
                                                        const StreamSetup& wanted,
                                                        Configured& configured);
    [[nodiscard]] static StreamSetup setupFor(const MediaDemand& demand,
                                              const Configured& configured);
    // Sends one setup request; false (logged) if the link refused it.
    [[nodiscard]] bool sendStep(GlassesSession& media, std::string_view name, std::uint16_t service,
                                std::uint16_t type, const Bytes& payload);
    // Sends one setup request and waits for its acknowledgement. A missing acknowledgement is
    // logged and tolerated.
    [[nodiscard]] Step awaitStep(GlassesSession& media, std::string_view name,
                                 std::uint16_t service, std::uint16_t type, const Bytes& payload,
                                 std::uint16_t response, std::chrono::seconds wait);
    // Stops a running stream before the link goes, so the glasses end it cleanly. Best effort.
    void stopBeforeLeaving(GlassesSession& media, const StreamSetup& running);
    // The link is up with nothing streaming; restarted says media flowed before in this session.
    void announceReady(bool restarted);
    // The wanted medium arrived after a start; a restart reports the time since change_began.
    void announceFlow(bool video, bool restarted,
                      std::chrono::steady_clock::time_point change_began);
    void setState(MediaState next);
    void log(const std::string& message);
    // The time since the current connection attempt began, such as "2.3 s".
    [[nodiscard]] std::string elapsed() const;
    // Sleeps in short steps; false if stop was requested before the time passed.
    bool interruptibleSleep(std::chrono::milliseconds total);
};
} // namespace mgl
#endif
