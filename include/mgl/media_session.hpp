// SPDX-License-Identifier: AGPL-3.0-only
// The full live-camera pipeline as a reusable, self-driving session: it owns a worker thread that
// authenticates over Bluetooth, asks the glasses to host their own 2.4 GHz access point, joins it
// from this PC, opens the TCP media session, starts the camera and delivers reassembled HEVC access
// units. The OBS source and the diagnostic CLI both drive it; it performs no decoding itself.
#ifndef MGL_MEDIA_SESSION_HPP
#define MGL_MEDIA_SESSION_HPP

#include "mgl/bytes.hpp"
#include "mgl/stream.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

namespace mgl {
namespace crypto {
class OwnerKey;
} // namespace crypto
class GlassesSession;

// Streaming means camera frames are arriving, not merely that the stream was requested. Waiting
// means another session in this process holds the glasses.
enum class MediaState : std::uint8_t {
    Idle,
    Waiting,
    Connecting,
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
    // A human-readable status line, redacted of identifiers. Optional.
    std::function<void(const std::string&)> on_log;
    // A state transition. Optional.
    std::function<void(MediaState)> on_state;
};

class MediaSession {
  public:
    explicit MediaSession(MediaCallbacks callbacks, const stream::VideoConfig& config = {});
    ~MediaSession();
    MediaSession(const MediaSession&) = delete;
    MediaSession& operator=(const MediaSession&) = delete;
    MediaSession(MediaSession&&) = delete;
    MediaSession& operator=(MediaSession&&) = delete;

    // Launches the worker thread. Returns immediately. Only one session per process drives the
    // glasses at a time; a later one waits (state Waiting) until the earlier one has finished.
    void start();
    // Asks the worker to finish and returns at once. The worker still tears the link down (stops
    // the stream, leaves the glasses' Wi-Fi network) before it exits; stop() or the destructor
    // waits for that.
    void requestStop() noexcept;
    // Signals the worker to stop and joins it. Safe to call more than once.
    void stop();

  private:
    // How one connection attempt ended. Fatal stops retrying (no usable identity); Failed is a
    // transient failure before any video; Dropped means video flowed and then stopped.
    enum class SessionEnd : std::uint8_t { Fatal, Failed, Dropped };
    // The Wi-Fi TCP transport and the media session over it, torn down session first.
    struct MediaLink;
    // The Bluetooth session that asked the glasses to host their access point, and where to reach
    // it. The glasses leave Bluetooth once this PC joins their network.
    struct ControlLink;

    MediaCallbacks callbacks_;
    stream::VideoConfig config_;
    std::atomic<MediaState> state_{MediaState::Idle};
    std::atomic<bool> stop_{false};
    std::thread worker_;

    void run();
    // The reconnect loop, once this session holds the glasses.
    void runConnected();
    // runSession inside the worker's exception boundary.
    [[nodiscard]] SessionEnd attemptSession();
    // One connection attempt through to a stream drop.
    [[nodiscard]] SessionEnd runSession();
    // Authenticates over Bluetooth and asks the glasses to host their own 2.4 GHz access point.
    [[nodiscard]] std::unique_ptr<ControlLink> openControlLink(const crypto::OwnerKey& owner);
    // After a failure before the media link opens, closes the glasses' access point over
    // Bluetooth so the retry finds the glasses' Wi-Fi free.
    void abandonAccessPoint(ControlLink& control);
    // Opens the AirShield media session over the glasses' access point, retrying while the
    // glasses' server comes up.
    [[nodiscard]] std::unique_ptr<MediaLink> openMediaLink(const crypto::OwnerKey& owner,
                                                           const stream::SoftApInfo& access_point);
    // Sends the stream setup in order, each step acknowledged. nonce advances past the requests.
    [[nodiscard]] bool startCamera(GlassesSession& media, std::uint32_t& nonce);
    // Sends one setup request; false (logged) if the link refused it.
    [[nodiscard]] bool sendStep(GlassesSession& media, std::string_view name, std::uint16_t service,
                                std::uint16_t type, const Bytes& payload);
    // Sends one setup request and waits for its acknowledgement. A missing acknowledgement is
    // logged and tolerated; a dropped link or an answer carrying an error ends the attempt.
    [[nodiscard]] bool awaitStep(GlassesSession& media, std::string_view name,
                                 std::uint16_t service, std::uint16_t type, const Bytes& payload,
                                 std::uint16_t response, std::chrono::seconds wait);
    // Delivers video until it stops, then stops the stream and leaves the access point.
    [[nodiscard]] SessionEnd streamVideo(GlassesSession& media, std::uint32_t nonce);
    void setState(MediaState next);
    void log(const std::string& message);
    // Sleeps in short steps; false if stop was requested before the time passed.
    bool interruptibleSleep(std::chrono::milliseconds total);
};
} // namespace mgl
#endif
