// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/buffered_transport.hpp"

#include "mgl/bytes.hpp"
#include "mgl/sdu.hpp"
#include "mgl/transport.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

namespace mgl {
void BufferedTransport::keep(ByteView received) {
    buffer_.insert(buffer_.end(), received.begin(), received.end());
}

SduResult BufferedTransport::takeFrame(LinkPhase phase) {
    std::array<std::uint8_t, 10> head{};
    const auto head_len = std::min(buffer_.size(), head.size());
    std::copy_n(buffer_.begin(), head_len, head.begin());
    const auto prefix = inspectFramePrefix(std::span(head).first(head_len), phase);
    if (prefix.status == PrefixStatus::Invalid) {
        return {.status = TransportStatus::Unframeable, .frame = {}};
    }
    if (prefix.status == PrefixStatus::NeedMore || prefix.frame_size > MaximumSduSize ||
        buffer_.size() < prefix.frame_size) {
        if (prefix.status == PrefixStatus::Ready && prefix.frame_size > MaximumSduSize) {
            return {.status = TransportStatus::Unframeable, .frame = {}};
        }
        return {.status = TransportStatus::Ok, .frame = {}}; // need more bytes
    }
    Bytes frame(buffer_.begin(), buffer_.begin() + static_cast<std::ptrdiff_t>(prefix.frame_size));
    buffer_.erase(buffer_.begin(),
                  buffer_.begin() + static_cast<std::ptrdiff_t>(prefix.frame_size));
    return {.status = TransportStatus::Ok, .frame = std::move(frame)};
}

SduResult BufferedTransport::receive(LinkPhase phase, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    for (;;) {
        auto framed = takeFrame(phase);
        if (framed.status != TransportStatus::Ok || !framed.frame.empty()) {
            return framed;
        }
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        if (remaining <= std::chrono::milliseconds::zero()) {
            return {.status = TransportStatus::TimedOut, .frame = {}};
        }
        const auto filled = fill(remaining);
        if (filled != TransportStatus::Ok) {
            return {.status = filled, .frame = {}};
        }
    }
}
} // namespace mgl
