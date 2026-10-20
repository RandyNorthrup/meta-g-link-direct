// SPDX-License-Identifier: AGPL-3.0-only
// Presentation times for media that arrive in bursts.
//
// The glasses stamp every video and audio unit with its capture time, in microseconds on one clock
// for both media, but their radio releases the units in bursts: on Gen 2, four or five video frames
// within a few milliseconds, then 110-160 ms of nothing, whatever the bitrate (measured 2026-10-20,
// reports/FRAME_RATE_VERIFICATION.md). A player that shows each unit as it arrives therefore shows
// clumps. PlayoutClock maps each capture time onto the receiver's clock plus a delay that covers
// the gaps, so a player can present the units at their capture cadence. The Meta AI app paces its
// own player the same way.
//
// All times are microseconds. Capture times come from the glasses' clock; arrival and presentation
// times from one monotonic receiver clock.
#ifndef MGL_PLAYOUT_HPP
#define MGL_PLAYOUT_HPP

#include <cstddef>
#include <cstdint>
#include <deque>

namespace mgl::playout {
struct Settings {
    // The delay before any lateness has been seen: about one burst gap, so the first seconds are
    // already smooth.
    std::int64_t initial_delay_us = 150'000;
    // The delay never falls below this, so ordinary jitter does not drop frames...
    std::int64_t min_delay_us = 20'000;
    // ...and never rises above this: a unit later than that is presented on arrival instead of
    // holding everything behind it.
    std::int64_t max_delay_us = 500'000;
    // Added to the worst lateness seen, so the next burst of the same size is on time.
    std::int64_t margin_us = 15'000;
    // How far back the worst lateness and the earliest arrival are remembered, in buckets.
    std::int64_t bucket_us = 1'000'000;
    std::size_t window_buckets = 10;
    // The delay shrinks by at most elapsed time divided by this: 100 plays at most 1% fast while it
    // catches up, which is not visible or audible.
    std::int64_t shrink_divisor = 100;
    // A capture time this far from what the arrivals predict restarts the clock (a new stream with
    // a new time base), keeping the current delay.
    std::int64_t discontinuity_us = 3'000'000;
};

class PlayoutClock {
  public:
    PlayoutClock() = default;
    explicit PlayoutClock(const Settings& settings) : settings_(settings) {}

    // Records one unit and returns the receiver time at which to present it: its capture time plus
    // the current offset and delay, never earlier than its arrival.
    [[nodiscard]] std::int64_t presentAt(std::uint64_t capture_us, std::int64_t arrival_us);

    // The current delay over the earliest arrival in the window.
    [[nodiscard]] std::int64_t delayUs() const noexcept { return base_ - earliest(); }
    // Units that came later than the delay then covered. Each raised the delay, and the picture
    // or sound held until it was due.
    [[nodiscard]] std::uint64_t lateUnits() const noexcept { return late_; }
    // Times the capture clock jumped and the clock restarted.
    [[nodiscard]] std::uint64_t restarts() const noexcept { return restarts_; }

  private:
    // arrival - capture, the smallest and largest seen in one bucket of arrival time.
    struct Bucket {
        std::int64_t start_us = 0;
        std::int64_t earliest = 0;
        std::int64_t latest = 0;
    };

    void start(std::int64_t offset, std::int64_t arrival_us, std::int64_t delay);
    void record(std::int64_t offset, std::int64_t arrival_us);
    [[nodiscard]] std::int64_t earliest() const noexcept;
    [[nodiscard]] std::int64_t latest() const noexcept;

    Settings settings_;
    std::deque<Bucket> buckets_;
    std::int64_t base_ = 0; // presentation = capture + base_
    std::int64_t last_arrival_us_ = 0;
    std::uint64_t late_ = 0;
    std::uint64_t restarts_ = 0;
    bool started_ = false;
};
} // namespace mgl::playout
#endif
