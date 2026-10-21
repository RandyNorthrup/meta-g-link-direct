// SPDX-License-Identifier: AGPL-3.0-only
// The glasses' battery as the plugin shows it outside the source properties: in a text file that
// OBS text sources and other tools read, and to obs-websocket clients such as Stream Deck plugins.
//
// The glasses report their battery every 10-20 seconds while a link is up (measured 2026-10-20,
// reports/ACCESSIBILITY_AND_QUALITY_VERIFICATION.md). A report is only as current as the link that
// carried it, so without a link the battery is unknown rather than its last value.
#ifndef MGL_BATTERY_HPP
#define MGL_BATTERY_HPP

#include "mgl/stream.hpp"

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>

namespace mgl::battery {
// What is shown of a battery report. Voltage and temperature change with every report and are left
// out, so a reading changes only when what is shown changes.
struct Reading {
    std::uint32_t percent = 0;
    bool charging = false;
    bool operator==(const Reading&) const = default;
};

[[nodiscard]] Reading readingOf(const stream::BatteryState& state) noexcept;

// The text of the battery file: "87%", "87% charging", or empty while the battery is unknown, so an
// overlay shows nothing rather than a stale level.
[[nodiscard]] std::string overlayText(const std::optional<Reading>& reading);

// Hands each change of the shown reading to one listener, in order, and never the same reading
// twice in a row. It starts at unknown. Callers may report from any thread: refresh reads the
// current reading itself, under the publisher's lock, so a slower caller cannot publish an older
// value after a newer one.
class Publisher {
  public:
    using Listener = std::function<void(const std::optional<Reading>&)>;
    using Source = std::function<std::optional<Reading>()>;

    // Replaces the listener. Returns once a call to the previous listener in progress has finished,
    // so after setListener({}) the previous listener is never called again.
    void setListener(Listener listener);
    // Reads the current reading from source and passes it to the listener if it differs from the
    // last one published. The listener must not call back into this publisher.
    void refresh(const Source& source);

  private:
    std::mutex mutex_;
    Listener listener_;
    std::optional<Reading> published_;
};
} // namespace mgl::battery
#endif
