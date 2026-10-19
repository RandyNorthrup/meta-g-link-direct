// SPDX-License-Identifier: AGPL-3.0-only
// The full live-camera pipeline as a reusable, self-driving session: it owns a worker thread that
// brings up the Wi-Fi group, authenticates over Bluetooth, hands the glasses to Wi-Fi, opens the
// TCP media session, starts the camera and delivers reassembled HEVC access units. The OBS source
// and the diagnostic CLI both drive it; it performs no decoding itself.
#ifndef MGL_MEDIA_SESSION_HPP
#define MGL_MEDIA_SESSION_HPP

#include "mgl/bytes.hpp"
#include "mgl/stream.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace mgl {
enum class MediaState : std::uint8_t { Idle, Connecting, Streaming, Reconnecting, Failed, Stopped };

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
    explicit MediaSession(MediaCallbacks callbacks, stream::VideoConfig config = {});
    ~MediaSession();
    MediaSession(const MediaSession&) = delete;
    MediaSession& operator=(const MediaSession&) = delete;
    MediaSession(MediaSession&&) = delete;
    MediaSession& operator=(MediaSession&&) = delete;

    // Launches the worker thread. Returns immediately.
    void start();
    // Signals the worker to stop and joins it. Safe to call more than once.
    void stop();
    [[nodiscard]] MediaState state() const noexcept { return state_.load(); }

  private:
    MediaCallbacks callbacks_;
    stream::VideoConfig config_;
    std::atomic<MediaState> state_{MediaState::Idle};
    std::atomic<bool> stop_{false};
    std::thread worker_;

    void run();
    // One connection attempt through to a stream drop. Returns true only on a fatal error that
    // should stop retrying; a transient failure or a dropped stream returns false so run() retries.
    [[nodiscard]] bool runSession();
    void setState(MediaState state);
    void log(const std::string& message);
    // Sleeps in short steps, returning early if stop() is requested.
    void interruptibleSleep(std::chrono::milliseconds total);
};
} // namespace mgl
#endif
