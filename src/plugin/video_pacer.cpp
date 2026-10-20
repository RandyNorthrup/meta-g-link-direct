// SPDX-License-Identifier: AGPL-3.0-only
#include "video_pacer.hpp"

#include "hevc_decoder.hpp"

#include "mgl/bytes.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <mutex>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>

namespace mgl::plugin {
namespace {
// A picture is decoded this long before it is due, so decoding does not make it late. OBS holds a
// picture handed over early until its timestamp.
constexpr auto DecodeLead = std::chrono::milliseconds(15);
// More queued pictures than this (3 s of video) means decoding cannot keep up.
constexpr std::size_t MaxQueuedPictures = 90;
// How often the pacer reports what it saw while video flows.
constexpr auto ReportEvery = std::chrono::seconds(60);
constexpr std::int64_t MicrosPerMilli = 1000;
} // namespace

VideoPacer::VideoPacer(PictureSink picture, LogSink log)
    : picture_(std::move(picture)), log_(std::move(log)) {}

VideoPacer::~VideoPacer() {
    {
        const std::scoped_lock lock(mutex_);
        stopping_ = true;
    }
    changed_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}

bool VideoPacer::open() {
    if (!decoder_.open()) {
        return false;
    }
    window_.began = Clock::now();
    try {
        thread_ = std::thread([this] { run(); });
    } catch (const std::system_error& error) {
        log_("The video pacer thread could not start. The error was:");
        log_(error.what());
        return false;
    }
    return true;
}

void VideoPacer::parameters(ByteView annex_b) {
    push({.parameters = true,
          .data = Bytes(annex_b.begin(), annex_b.end()),
          .pts_us = 0,
          .keyframe = false,
          .present_at = {}});
}

void VideoPacer::unit(std::uint64_t pts_us, ByteView annex_b, bool keyframe,
                      Clock::time_point present_at, Timing timing) {
    bool overflowed = false;
    {
        const std::scoped_lock lock(mutex_);
        timing_ = timing;
        ++window_.received;
        if (skip_to_keyframe_ && !keyframe) {
            ++window_.skipped;
            return;
        }
        skip_to_keyframe_ = false;
        const auto queued = static_cast<std::size_t>(
            std::ranges::count_if(queue_, [](const Entry& entry) { return !entry.parameters; }));
        if (queued >= MaxQueuedPictures) {
            // Decoding has fallen behind: drop the queued pictures (parameter sets stay, they
            // apply to what follows) and carry on from a picture that decodes on its own.
            overflowed = true;
            window_.skipped += queued;
            std::erase_if(queue_, [](const Entry& entry) { return !entry.parameters; });
            if (!keyframe) {
                skip_to_keyframe_ = true;
                ++window_.skipped;
            }
        }
        if (!skip_to_keyframe_) {
            queue_.push_back({.parameters = false,
                              .data = Bytes(annex_b.begin(), annex_b.end()),
                              .pts_us = pts_us,
                              .keyframe = keyframe,
                              .present_at = present_at});
        }
    }
    changed_.notify_one();
    if (overflowed) {
        log_("Video decoding fell behind; skipped ahead to the next keyframe.");
    }
}

void VideoPacer::push(Entry entry) {
    {
        const std::scoped_lock lock(mutex_);
        queue_.push_back(std::move(entry));
    }
    changed_.notify_one();
}

void VideoPacer::run() noexcept {
    for (;;) {
        Entry entry;
        {
            std::unique_lock lock(mutex_);
            for (;;) {
                if (stopping_) {
                    return;
                }
                if (queue_.empty()) {
                    changed_.wait(lock);
                    continue;
                }
                const Entry& front = queue_.front();
                if (front.parameters) {
                    break;
                }
                const auto start = front.present_at - DecodeLead;
                if (Clock::now() >= start) {
                    break;
                }
                changed_.wait_until(lock, start);
            }
            entry = std::move(queue_.front());
            queue_.pop_front();
        }
        try {
            present(entry);
            report(Clock::now());
        } catch (const std::exception& error) {
            // Nothing may escape the thread. One picture is lost; the next one carries on. The
            // sink takes views, so reporting allocates nothing here.
            log_("A video picture could not be shown; the next one carries on. The error was:");
            log_(error.what());
        }
    }
}

void VideoPacer::present(Entry& entry) {
    const ByteView data{entry.data.data(), entry.data.size()};
    if (entry.parameters) {
        // A new stream configuration: its report window starts with its first picture.
        decoder_.setParameters(data);
        const std::scoped_lock lock(mutex_);
        window_ = {.began = Clock::now(), .late_at_start = timing_.late_units};
        return;
    }
    std::uint64_t shown = 0;
    const bool decoded =
        decoder_.decode(entry.pts_us, data, entry.keyframe, [&](const DecodedFrame& picture) {
            picture_(picture, entry.present_at);
            ++shown;
        });
    const std::scoped_lock lock(mutex_);
    window_.shown += shown;
    if (!decoded) {
        ++window_.undecodable;
    }
}

void VideoPacer::report(Clock::time_point now) {
    Window window;
    Timing timing;
    {
        const std::scoped_lock lock(mutex_);
        if (now - window_.began < ReportEvery) {
            return;
        }
        window = window_;
        timing = timing_;
        window_ = {.began = now, .late_at_start = timing_.late_units};
    }
    const double seconds = std::chrono::duration<double>(now - window.began).count();
    log_(std::format("Video over the last {:.0f} s: {:.1f} fps from the glasses, {:.1f} fps "
                     "shown, presentation delay {} ms, {} late, {} undecodable, {} skipped.",
                     seconds, static_cast<double>(window.received) / seconds,
                     static_cast<double>(window.shown) / seconds, timing.delay_us / MicrosPerMilli,
                     timing.late_units - window.late_at_start, window.undecodable, window.skipped));
}
} // namespace mgl::plugin
