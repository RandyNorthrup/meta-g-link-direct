// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/playout.hpp"

#include <algorithm>
#include <cstdint>

namespace mgl::playout {
std::int64_t PlayoutClock::presentAt(std::uint64_t capture_us, std::int64_t arrival_us) {
    const auto capture = static_cast<std::int64_t>(capture_us);
    const std::int64_t offset = arrival_us - capture;
    // Late: the delay so far did not cover this unit, so its presentation holds the picture.
    const bool late = started_ && capture + base_ < arrival_us;
    if (!started_) {
        start(offset, arrival_us, settings_.initial_delay_us);
    } else if (offset < earliest() - settings_.discontinuity_us ||
               offset > latest() + settings_.discontinuity_us) {
        const std::int64_t delay = delayUs();
        buckets_.clear();
        ++restarts_;
        start(offset, arrival_us, delay);
    } else {
        record(offset, arrival_us);
    }

    // The base follows the worst lateness in the window: up at once, so the next burst of that
    // size is on time; down slowly, so catching up is not visible.
    const std::int64_t lowest = earliest() + settings_.min_delay_us;
    const std::int64_t highest = earliest() + settings_.max_delay_us;
    const std::int64_t target = std::clamp(latest() + settings_.margin_us, lowest, highest);
    if (target > base_) {
        base_ = target;
    } else {
        const std::int64_t elapsed = std::max<std::int64_t>(arrival_us - last_arrival_us_, 0);
        base_ = std::max(target, base_ - (elapsed / settings_.shrink_divisor));
    }
    last_arrival_us_ = std::max(last_arrival_us_, arrival_us);
    if (late) {
        ++late_;
    }
    // Past the largest delay a unit is presented on arrival rather than holding everything behind
    // it.
    return std::max(capture + base_, arrival_us);
}

void PlayoutClock::start(std::int64_t offset, std::int64_t arrival_us, std::int64_t delay) {
    started_ = true;
    buckets_.push_back({.start_us = arrival_us, .earliest = offset, .latest = offset});
    base_ = offset + std::clamp(delay, settings_.min_delay_us, settings_.max_delay_us);
    last_arrival_us_ = arrival_us;
}

void PlayoutClock::record(std::int64_t offset, std::int64_t arrival_us) {
    if (arrival_us - buckets_.back().start_us >= settings_.bucket_us) {
        buckets_.push_back({.start_us = arrival_us, .earliest = offset, .latest = offset});
        while (buckets_.size() > settings_.window_buckets) {
            buckets_.pop_front();
        }
        return;
    }
    Bucket& current = buckets_.back();
    current.earliest = std::min(current.earliest, offset);
    current.latest = std::max(current.latest, offset);
}

std::int64_t PlayoutClock::earliest() const noexcept {
    return buckets_.empty() ? 0 : std::ranges::min(buckets_, {}, &Bucket::earliest).earliest;
}

std::int64_t PlayoutClock::latest() const noexcept {
    return buckets_.empty() ? 0 : std::ranges::max(buckets_, {}, &Bucket::latest).latest;
}
} // namespace mgl::playout
