// SPDX-License-Identifier: AGPL-3.0-only
// The camera core behind camera.h: it runs the direct glasses media pipeline on its own worker,
// decodes the HEVC frames and hands each picture to the OBS glue with its timestamp.
//
// OBS calls start and stop on its video thread, so neither may wait on the radio. A stopped
// capture is detached from OBS at once and finishes its teardown on a background thread; the
// worker's callbacks share a per-run Pipeline, so nothing they touch is freed underneath them.
#include "camera.h"

#include "aac_decoder.hpp"
#include "hevc_decoder.hpp"

#include "mgl/bytes.hpp"
#include "mgl/media_session.hpp"
#include "mgl/stream.hpp"
#include "mgl/wifi_station.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <new>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>

namespace {
constexpr std::uint64_t NanosPerMicro = 1000;

// Selectable quality profiles, best first. 720x1280 is the confirmed target; lower profiles ease a
// struggling link. Labels come from the locale file.
struct QualityPreset {
    const char* key = nullptr;
    mgl::stream::VideoConfig config;
};
constexpr std::array<QualityPreset, 4> QualityPresets{{
    {.key = "q720",
     .config = {.width = 720,
                .height = 1280,
                .bitrate = 4'000'000,
                .frame_rate = 30,
                .iframe_interval = 1}},
    {.key = "q1080",
     .config = {.width = 1080,
                .height = 1920,
                .bitrate = 4'000'000,
                .frame_rate = 30,
                .iframe_interval = 1}},
    {.key = "q504",
     .config = {.width = 504,
                .height = 896,
                .bitrate = 1'500'000,
                .frame_rate = 30,
                .iframe_interval = 1}},
    {.key = "q360",
     .config =
         {.width = 360, .height = 640, .bitrate = 600'000, .frame_rate = 24, .iframe_interval = 1}},
}};

using LogFn = void (*)(const char* message);
using FrameFn = void (*)(void* context, const MglCameraFrame* frame);
using AudioFn = void (*)(void* context, const MglCameraAudio* audio);

void logLine(LogFn on_log, const std::string& message) {
    if (on_log != nullptr) {
        on_log(message.c_str());
    }
}

// The status the OBS properties show. One cell per camera, created with it and never replaced, so
// the UI thread can read it while the video thread starts and stops captures. A capture run
// reports only while it is the camera's current run, so a run that is still tearing down cannot
// overwrite a newer status.
class StatusCell {
  public:
    std::uint64_t beginRun() {
        const std::scoped_lock lock(mutex_);
        ++generation_;
        state_ = mgl::MediaState::Connecting;
        return generation_;
    }
    void endRun(mgl::MediaState final_state) {
        const std::scoped_lock lock(mutex_);
        ++generation_;
        state_ = final_state;
    }
    void report(std::uint64_t run, mgl::MediaState state) {
        const std::scoped_lock lock(mutex_);
        if (run == generation_) {
            state_ = state;
        }
    }
    [[nodiscard]] mgl::MediaState current() const {
        const std::scoped_lock lock(mutex_);
        return state_;
    }

  private:
    mutable std::mutex mutex_;
    std::uint64_t generation_ = 0;
    mgl::MediaState state_ = mgl::MediaState::Idle;
};

// Everything one capture run touches from its worker thread. The worker's callbacks share it, so
// it outlives a camera that is stopped or destroyed while a frame is in flight.
struct Pipeline {
    FrameFn on_frame = nullptr;
    AudioFn on_audio = nullptr;
    LogFn on_log = nullptr;
    void* context = nullptr;
    std::shared_ptr<StatusCell> status;
    std::uint64_t run = 0;
    mgl::plugin::HevcDecoder decoder;      // worker thread only
    std::uint64_t decoded = 0;             // worker thread only
    mgl::plugin::AacDecoder audio_decoder; // worker thread only
    std::uint64_t decoded_audio = 0;       // worker thread only
    bool audio_layout_reported = false;    // worker thread only
    std::mutex output_mutex;
    bool attached = true; // guarded by output_mutex; false once OBS must not receive frames
};

// Stopped sessions finish their teardown (stop the stream, leave the glasses' Wi-Fi network) on
// their own threads. Module unload waits for them.
class Retirement {
  public:
    void retire(std::unique_ptr<mgl::MediaSession> session) {
        session->requestStop();
        {
            const std::scoped_lock lock(mutex_);
            ++pending_;
        }
        try {
            std::thread([this, owned = std::move(session)]() mutable {
                owned.reset(); // joins the worker once its teardown is done
                finish();
            }).detach();
        } catch (const std::system_error&) {
            // No thread could be created. The lambda, and with it the session, was destroyed
            // during the throw, which already tore the link down on this thread.
            finish();
        }
    }

    bool waitIdle(std::chrono::milliseconds limit) {
        std::unique_lock lock(mutex_);
        return done_.wait_for(lock, limit, [this] { return pending_ == 0; });
    }

  private:
    void finish() {
        {
            const std::scoped_lock lock(mutex_);
            --pending_;
        }
        done_.notify_all();
    }

    std::mutex mutex_;
    std::condition_variable done_;
    std::size_t pending_ = 0;
};

Retirement& retirement() {
    static Retirement instance;
    return instance;
}
} // namespace

struct MglCamera {
    FrameFn on_frame = nullptr;
    AudioFn on_audio = nullptr;
    LogFn on_log = nullptr;
    void* context = nullptr;
    std::atomic<std::size_t> quality{0}; // index into QualityPresets
    std::atomic<bool> audio{false};      // glasses microphones on the next start
    // Set from OBS's settings thread, read on start; empty means the first adapter.
    std::mutex wifi_adapter_mutex;
    std::string wifi_adapter;
    std::shared_ptr<StatusCell> status = std::make_shared<StatusCell>();
    // Owned by OBS's video thread (activate, deactivate) and destroy.
    std::shared_ptr<Pipeline> pipeline;
    std::unique_ptr<mgl::MediaSession> session;
};

namespace {
const char* stateKey(mgl::MediaState state) {
    switch (state) {
    case mgl::MediaState::Idle:
        return "Status.Idle";
    case mgl::MediaState::Waiting:
        return "Status.Waiting";
    case mgl::MediaState::Connecting:
        return "Status.Connecting";
    case mgl::MediaState::Streaming:
        return "Status.Streaming";
    case mgl::MediaState::Reconnecting:
        return "Status.Reconnecting";
    case mgl::MediaState::Failed:
        return "Status.Failed";
    case mgl::MediaState::Stopped:
        return "Status.Stopped";
    }
    return "Status.Idle";
}

// Runs on the worker thread for each decoded picture.
void deliverPicture(Pipeline& pipeline, const mgl::plugin::DecodedFrame& decoded) {
    if (pipeline.decoded == 0) {
        logLine(pipeline.on_log, "receiving video " + std::to_string(decoded.width) + "x" +
                                     std::to_string(decoded.height));
    }
    ++pipeline.decoded;
    const MglCameraFrame frame{.plane_y = std::get<0>(decoded.plane),
                               .plane_u = std::get<1>(decoded.plane),
                               .plane_v = std::get<2>(decoded.plane),
                               .stride_y = std::get<0>(decoded.linesize),
                               .stride_u = std::get<1>(decoded.linesize),
                               .stride_v = std::get<2>(decoded.linesize),
                               .width = decoded.width,
                               .height = decoded.height,
                               .timestamp_ns = decoded.pts_us * NanosPerMicro,
                               .full_range = decoded.full_range};
    // Holding the lock across the hand-off means a stop that has detached the pipeline never
    // races a frame into a source OBS is about to free.
    const std::scoped_lock lock(pipeline.output_mutex);
    if (pipeline.attached) {
        pipeline.on_frame(pipeline.context, &frame);
    }
}

// Runs on the worker thread for each block of decoded audio. Mono and stereo are handed on; the
// glasses are asked for stereo.
void deliverSound(Pipeline& pipeline, const mgl::plugin::DecodedAudio& decoded) {
    constexpr std::uint32_t MaxDeliveredChannels = 2;
    if (decoded.channels > MaxDeliveredChannels) {
        if (!pipeline.audio_layout_reported) {
            pipeline.audio_layout_reported = true;
            logLine(pipeline.on_log, "The glasses sent " + std::to_string(decoded.channels) +
                                         "-channel audio; only mono and stereo are supported.");
        }
        return;
    }
    if (pipeline.decoded_audio == 0) {
        logLine(pipeline.on_log, "receiving audio " + std::to_string(decoded.sample_rate) +
                                     " Hz, " + std::to_string(decoded.channels) + " channel(s)");
    }
    ++pipeline.decoded_audio;
    MglCameraAudio sound{};
    std::ranges::copy(decoded.plane, std::begin(sound.planes));
    sound.channels = decoded.channels;
    sound.frames = decoded.frames;
    sound.sample_rate = decoded.sample_rate;
    sound.timestamp_ns = decoded.pts_us * NanosPerMicro;
    const std::scoped_lock lock(pipeline.output_mutex);
    if (pipeline.attached && pipeline.on_audio != nullptr) {
        pipeline.on_audio(pipeline.context, &sound);
    }
}

void stopCapture(MglCamera& camera, mgl::MediaState final_state) {
    if (camera.pipeline) {
        const std::scoped_lock lock(camera.pipeline->output_mutex);
        camera.pipeline->attached = false;
    }
    camera.pipeline.reset();
    if (camera.session) {
        retirement().retire(std::move(camera.session));
    }
    camera.status->endRun(final_state);
}

void startCapture(MglCamera& camera) {
    if (camera.session) {
        return; // already running
    }
    auto pipeline = std::make_shared<Pipeline>();
    pipeline->on_frame = camera.on_frame;
    pipeline->on_audio = camera.on_audio;
    pipeline->on_log = camera.on_log;
    pipeline->context = camera.context;
    pipeline->status = camera.status;
    pipeline->run = camera.status->beginRun();
    if (!pipeline->decoder.open()) {
        logLine(camera.on_log, "The HEVC decoder could not be opened; the camera cannot start.");
        camera.status->report(pipeline->run, mgl::MediaState::Failed);
        return;
    }
    mgl::MediaCallbacks callbacks;
    callbacks.on_parameters = [pipeline](mgl::ByteView params) {
        pipeline->decoder.setParameters(params);
    };
    callbacks.on_frame = [pipeline](std::uint64_t pts, mgl::ByteView annex, bool keyframe) {
        pipeline->decoder.decode(pts, annex, keyframe,
                                 [&target = *pipeline](const mgl::plugin::DecodedFrame& decoded) {
                                     deliverPicture(target, decoded);
                                 });
    };
    callbacks.on_audio_config = [pipeline](mgl::ByteView config) {
        if (!pipeline->audio_decoder.configure(config)) {
            logLine(pipeline->on_log, "The AAC decoder could not open the glasses' audio format; "
                                      "the camera continues without audio.");
        }
    };
    callbacks.on_audio = [pipeline](std::uint64_t pts, mgl::ByteView aac) {
        pipeline->audio_decoder.decode(
            pts, aac, [&target = *pipeline](const mgl::plugin::DecodedAudio& decoded) {
                deliverSound(target, decoded);
            });
    };
    callbacks.on_log = [on_log = camera.on_log](const std::string& message) {
        logLine(on_log, message);
    };
    callbacks.on_state = [pipeline](mgl::MediaState state) {
        pipeline->status->report(pipeline->run, state);
    };
    mgl::MediaOptions options{.video = QualityPresets.at(camera.quality.load()).config,
                              .audio = camera.audio.load(),
                              .wifi_adapter = {}};
    {
        const std::scoped_lock lock(camera.wifi_adapter_mutex);
        options.wifi_adapter = camera.wifi_adapter;
    }
    auto session = std::make_unique<mgl::MediaSession>(std::move(callbacks), std::move(options));
    session->start();
    camera.pipeline = std::move(pipeline);
    camera.session = std::move(session);
}

// No C++ exception may unwind into OBS's C callbacks: report it, detach the capture and mark the
// camera failed.
template <typename Action> void guarded(MglCamera& camera, std::string_view what, Action action) {
    try {
        action();
    } catch (const std::exception& error) {
        logLine(camera.on_log, std::string(what) + " failed: " + error.what());
        try {
            stopCapture(camera, mgl::MediaState::Failed);
        } catch (const std::exception& cleanup) {
            logLine(camera.on_log, std::string("camera cleanup failed: ") + cleanup.what());
        }
    }
}
} // namespace

MglCamera* mglCameraCreate(void (*on_frame)(void* context, const MglCameraFrame* frame),
                           void (*on_audio)(void* context, const MglCameraAudio* audio),
                           void (*on_log)(const char* message), void* context) {
    if (on_frame == nullptr) {
        return nullptr;
    }
    try {
        auto camera = std::make_unique<MglCamera>();
        camera->on_frame = on_frame;
        camera->on_audio = on_audio;
        camera->on_log = on_log;
        camera->context = context;
        // The caller owns the camera from here until mglCameraDestroy.
        return camera.release();
    } catch (const std::bad_alloc&) {
        return nullptr;
    }
}

void mglCameraDestroy(MglCamera* camera) {
    // Takes back the camera the caller has owned since mglCameraCreate.
    const std::unique_ptr<MglCamera> owned(camera);
    if (owned == nullptr) {
        return;
    }
    guarded(*owned, "camera stop", [&owned] { stopCapture(*owned, mgl::MediaState::Stopped); });
}

void mglCameraStart(MglCamera* camera) {
    if (camera == nullptr) {
        return;
    }
    guarded(*camera, "camera start", [camera] { startCapture(*camera); });
}

void mglCameraStop(MglCamera* camera) {
    if (camera == nullptr) {
        return;
    }
    guarded(*camera, "camera stop", [camera] { stopCapture(*camera, mgl::MediaState::Stopped); });
}

bool mglCameraShutdown(std::uint32_t timeout_ms) {
    try {
        return retirement().waitIdle(std::chrono::milliseconds(timeout_ms));
    } catch (const std::exception&) {
        return false;
    }
}

const char* mglCameraQualityKey(std::size_t index) {
    return index < QualityPresets.size() ? QualityPresets.at(index).key : nullptr;
}

void mglCameraSetQuality(MglCamera* camera, const char* key) {
    if (camera == nullptr || key == nullptr) {
        return;
    }
    const std::string_view wanted(key);
    for (std::size_t index = 0; index < QualityPresets.size(); ++index) {
        if (wanted == QualityPresets.at(index).key) {
            camera->quality.store(index);
            return;
        }
    }
}

void mglCameraSetAudio(MglCamera* camera, bool enabled) {
    if (camera != nullptr) {
        camera->audio.store(enabled);
    }
}

const char* mglCameraStatusKey(const MglCamera* camera) {
    return stateKey(camera != nullptr ? camera->status->current() : mgl::MediaState::Idle);
}

namespace {
// UTF-8 continuation bytes look like 10xxxxxx.
constexpr unsigned char Utf8ContinuationMask = 0xC0;
constexpr unsigned char Utf8ContinuationBits = 0x80;

// Copies text into a fixed C field, NUL-terminated, cutting at a UTF-8 character boundary.
void copyField(std::span<char> field, std::string_view text) {
    std::size_t length = std::min(text.size(), field.size() - 1);
    if (length < text.size()) {
        // The byte at length starts the part that does not fit; back off while it would split a
        // multi-byte character.
        while (length > 0 && (static_cast<unsigned char>(text.at(length)) & Utf8ContinuationMask) ==
                                 Utf8ContinuationBits) {
            --length;
        }
    }
    *std::ranges::copy(text.substr(0, length), field.begin()).out = '\0';
}
} // namespace

bool mglWifiAdapterList(MglWifiAdapter* adapters, std::size_t capacity, std::size_t* count) {
    if (count == nullptr) {
        return false;
    }
    *count = 0;
    if (adapters == nullptr) {
        return false;
    }
    try {
        const auto found = mgl::listWifiAdapters();
        const std::span<MglWifiAdapter> out(adapters, capacity);
        auto slot = out.begin();
        for (const auto& adapter : found) {
            if (slot == out.end()) {
                break;
            }
            copyField(slot->id, adapter.id);
            copyField(slot->name, adapter.description);
            ++slot;
        }
        *count = static_cast<std::size_t>(slot - out.begin());
        return true;
    } catch (const std::exception&) {
        return false; // the caller logs the failure
    }
}

void mglCameraSetWifiAdapter(MglCamera* camera, const char* id) {
    if (camera == nullptr) {
        return;
    }
    try {
        const std::scoped_lock lock(camera->wifi_adapter_mutex);
        camera->wifi_adapter = id != nullptr ? id : "";
    } catch (const std::exception& error) {
        logLine(camera->on_log,
                std::string("Could not store the Wi-Fi adapter choice: ") + error.what());
    }
}
