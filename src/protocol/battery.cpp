// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/battery.hpp"

#include "mgl/stream.hpp"

#include <format>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

namespace mgl::battery {
Reading readingOf(const stream::BatteryState& state) noexcept {
    return Reading{.percent = state.percent, .charging = state.charging()};
}

std::string overlayText(const std::optional<Reading>& reading) {
    if (!reading) {
        return {};
    }
    return std::format("{}%{}", reading->percent, reading->charging ? " charging" : "");
}

void Publisher::setListener(Listener listener) {
    const std::scoped_lock lock(mutex_);
    listener_ = std::move(listener);
}

void Publisher::refresh(const Source& source) {
    const std::scoped_lock lock(mutex_);
    const std::optional<Reading> current = source();
    if (current == published_) {
        return;
    }
    published_ = current;
    if (listener_) {
        listener_(current);
    }
}
} // namespace mgl::battery
