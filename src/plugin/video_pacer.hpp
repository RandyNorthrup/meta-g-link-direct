// SPDX-License-Identifier: AGPL-3.0-only
// Decodes the glasses' video on a thread of its own and hands each picture on just before its
// presentation time (mgl/playout.hpp). The glasses' radio releases frames in bursts; paced this way
// they reach OBS at their capture cadence instead of in clumps that OBS would thin out.
#ifndef MGL_PLUGIN_VIDEO_PACER_HPP
#define MGL_PLUGIN_VIDEO_PACER_HPP

#include "hevc_decoder.hpp"

#include "mgl/bytes.hpp"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string_view>
#include <thread>

namespace mgl::plugin {
class VideoPacer {
  public:
    using Clock = std::chrono::steady_clock;
    // Runs on the pacer thread for each picture, shortly before present_at.
    using PictureSink = std::function<void(const DecodedFrame&, Clock::time_point present_at)>;
    // Must not throw: the pacer also reports through it while handling an error.
    using LogSink = std::function<void(std::string_view)>;

    // How the presentation delay stood when a unit was scheduled, for the periodic report.
    struct Timing {
        std::int64_t delay_us = 0;
        std::uint64_t late_units = 0;
    };

    VideoPacer(PictureSink picture, LogSink log);
    // Stops the thread; pictures still queued are not shown.
    ~VideoPacer();
    VideoPacer(const VideoPacer&) = delete;
    VideoPacer& operator=(const VideoPacer&) = delete;
    VideoPacer(VideoPacer&&) = delete;
    VideoPacer& operator=(VideoPacer&&) = delete;

    // Opens the decoder and starts the thread. False if the decoder is unavailable; nothing is
    // shown then.
    [[nodiscard]] bool open();
    // Called from the media worker, in arrival order: parameter sets apply to the units after
    // them, and a unit is decoded shortly before present_at.
    void parameters(ByteView annex_b);
    void unit(std::uint64_t pts_us, ByteView annex_b, bool keyframe, Clock::time_point present_at,
              Timing timing);

  private:
    struct Entry {
        bool parameters = false;
        Bytes data;
        std::uint64_t pts_us = 0;
        bool keyframe = false;
        Clock::time_point present_at;
    };
    // What the last report window saw.
    struct Window {
        Clock::time_point began;
        std::uint64_t received = 0;
        std::uint64_t shown = 0;
        std::uint64_t undecodable = 0;
        std::uint64_t skipped = 0;
        std::uint64_t late_at_start = 0;
    };

    void push(Entry entry);
    void run() noexcept;
    void present(Entry& entry);
    void report(Clock::time_point now);

    PictureSink picture_;
    LogSink log_;
    HevcDecoder decoder_; // used on the pacer thread once it runs
    std::mutex mutex_;
    std::condition_variable changed_;
    std::deque<Entry> queue_;
    bool stopping_ = false;
    bool skip_to_keyframe_ = false; // the queue overflowed; wait for a picture that decodes alone
    Timing timing_;                 // the latest, under mutex_
    Window window_;                 // pacer thread, except received and skipped under mutex_
    std::thread thread_;            // last: stops before the members it uses go away
};
} // namespace mgl::plugin
#endif
