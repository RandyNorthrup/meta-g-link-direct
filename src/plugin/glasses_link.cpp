// SPDX-License-Identifier: AGPL-3.0-only
// The glasses link core behind glasses_link.h: one MediaSession for the process, driven by what
// the registered OBS sources show, with each medium decoded once and handed to every live source
// that shows it.
//
// OBS calls in from its video and UI threads, so nothing here waits on the radio: the session's
// worker does the radio work, a replaced session finishes its teardown on a background thread, and
// module unload waits for that. Media reach a source only while it is registered and live; removal
// takes the same lock as the hand-off, so no frame races into a source OBS is about to free.
#include "glasses_link.h"

#include "aac_decoder.hpp"
#include "hevc_decoder.hpp"

#include "mgl/bytes.hpp"
#include "mgl/identity.hpp"
#include "mgl/media_session.hpp"
#include "mgl/stream.hpp"
#include "mgl/wifi_station.hpp"
#include "mgl/win_text.hpp"

#include <windows.h> // IWYU pragma: keep — SDK umbrella prerequisites.

#include <errhandlingapi.h>
#include <fileapi.h>
#include <handleapi.h>
#include <minwindef.h>
#include <processthreadsapi.h>
#include <synchapi.h>
#include <winbase.h>
#include <winnt.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

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
using FrameFn = void (*)(void* context, const MglVideoFrame* frame);
using AudioFn = void (*)(void* context, const MglAudioBlock* audio);

// Replaced sessions finish their teardown (stop the stream, leave the glasses' Wi-Fi network) on
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

// The state the OBS properties show. A session reports only while it is the link's current one, so
// a session that is still tearing down cannot overwrite a newer state.
class StatusCell {
  public:
    std::uint64_t beginSession() {
        const std::scoped_lock lock(mutex_);
        ++generation_;
        state_ = mgl::MediaState::Idle;
        battery_.reset();
        return generation_;
    }
    void report(std::uint64_t session, mgl::MediaState state) {
        const std::scoped_lock lock(mutex_);
        if (session == generation_) {
            state_ = state;
            // A battery report is only as current as the link that carried it.
            if (state == mgl::MediaState::Idle || state == mgl::MediaState::Failed ||
                state == mgl::MediaState::Stopped || state == mgl::MediaState::Reconnecting) {
                battery_.reset();
            }
        }
    }
    void reportBattery(std::uint64_t session, const mgl::stream::BatteryState& reported) {
        const std::scoped_lock lock(mutex_);
        if (session == generation_) {
            battery_ = reported;
        }
    }
    [[nodiscard]] mgl::MediaState current() const {
        const std::scoped_lock lock(mutex_);
        return state_;
    }
    [[nodiscard]] std::optional<mgl::stream::BatteryState> battery() const {
        const std::scoped_lock lock(mutex_);
        return battery_;
    }

  private:
    mutable std::mutex mutex_;
    std::uint64_t generation_ = 0;
    mgl::MediaState state_ = mgl::MediaState::Idle;
    std::optional<mgl::stream::BatteryState> battery_;
};

// The decoders of one session, used only on that session's worker thread.
struct Decoders {
    mgl::plugin::HevcDecoder video;
    bool video_open = false;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    mgl::plugin::AacDecoder audio;
    std::uint64_t audio_blocks = 0;
    bool audio_layout_reported = false;
};

const char* stateKey(mgl::MediaState state) {
    switch (state) {
    case mgl::MediaState::Idle:
        return "Status.Idle";
    case mgl::MediaState::Waiting:
        return "Status.Waiting";
    case mgl::MediaState::Connecting:
        return "Status.Connecting";
    case mgl::MediaState::Ready:
        return "Status.Ready";
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

struct HandleCloser {
    void operator()(HANDLE handle) const noexcept {
        if (handle != nullptr && handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
        }
    }
};
using UniqueHandle = std::unique_ptr<void, HandleCloser>;

// The installed setup window, %LOCALAPPDATA%\MetaGLinkDirect\bin\meta-g-link-setup.exe, when
// present.
std::optional<std::wstring> setupToolPath() {
    const auto directory = mgl::identity::storageDirectory();
    if (!directory) {
        return std::nullopt;
    }
    std::wstring path = mgl::widenUtf8(*directory) + L"\\bin\\meta-g-link-setup.exe";
    if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        return std::nullopt;
    }
    return path;
}
} // namespace

// One OBS source registered with the link. media, live and live_order are written under both of
// the link's locks and read by the worker under the output lock.
struct MglSource {
    FrameFn on_frame = nullptr;
    AudioFn on_audio = nullptr;
    void* context = nullptr;
    bool video = false; // shows the glasses' video while live
    bool audio = false; // shows the glasses' microphones while live
    std::size_t quality = 0;
    bool live = false;
    std::uint64_t live_order = 0; // smaller went live earlier
};

namespace {
class Link {
  public:
    Link() = default;
    Link(const Link&) = delete;
    Link& operator=(const Link&) = delete;
    Link(Link&&) = delete;
    Link& operator=(Link&&) = delete;
    // Unload normally stops the watcher first; this only covers a process ending without it.
    ~Link() {
        if (shutdown_event_) {
            SetEvent(shutdown_event_.get());
        }
        if (setup_watcher_.joinable()) {
            setup_watcher_.join();
        }
    }

    void init(LogFn on_log) {
        on_log_.store(on_log);
        const std::scoped_lock lock(mutex_);
        if (!shutdown_event_) {
            shutdown_event_.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
            if (!shutdown_event_) {
                log(std::format("Could not create the setup watch event (error {}); the Set up "
                                "button will be unavailable.",
                                GetLastError()));
            }
        }
    }

    void log(const std::string& message) const {
        if (const LogFn on_log = on_log_.load(); on_log != nullptr) {
            on_log(message.c_str());
        }
    }

    MglSource* add(std::unique_ptr<MglSource> source) {
        const std::scoped_lock lock(mutex_);
        MglSource* const raw = source.get();
        {
            const std::scoped_lock output(output_mutex_);
            sources_.push_back(std::move(source));
        }
        refreshLocked();
        return raw;
    }

    void remove(const MglSource* source) {
        const std::scoped_lock lock(mutex_);
        {
            const std::scoped_lock output(output_mutex_);
            std::erase_if(sources_, [source](const auto& entry) { return entry.get() == source; });
        }
        refreshLocked();
    }

    // Changes a source's registration under both locks, then lets the link follow.
    template <typename Change> void change(MglSource& source, Change apply) {
        const std::scoped_lock lock(mutex_);
        {
            const std::scoped_lock output(output_mutex_);
            apply(source);
        }
        refreshLocked();
    }

    void setLive(MglSource& source, bool live) {
        change(source, [this, live](MglSource& target) {
            if (live && !target.live) {
                target.live_order = ++live_counter_;
            }
            target.live = live;
            if (live) {
                // A source going live is a user action, so a session that ended on a missing or
                // refused identity gets a fresh try.
                retry_ended_ = true;
            }
        });
    }

    void setWifiAdapter(std::string id) {
        const std::scoped_lock lock(mutex_);
        wifi_adapter_ = std::move(id);
        refreshLocked();
    }

    void setKeepReady(bool keep_ready) {
        const std::scoped_lock lock(mutex_);
        keep_ready_ = keep_ready;
        refreshLocked();
    }

    [[nodiscard]] const char* statusKey() const {
        {
            const std::scoped_lock lock(mutex_);
            if (held_) {
                return "Status.Setup";
            }
        }
        return stateKey(status_.current());
    }

    [[nodiscard]] std::optional<mgl::stream::BatteryState> battery() const {
        return status_.battery();
    }

    const char* startSetup() {
        const std::scoped_lock lock(mutex_);
        if (setup_running_) {
            return "Setup.Running";
        }
        if (shut_down_ || !shutdown_event_) {
            return "Setup.Failed";
        }
        const auto tool = setupToolPath();
        if (!tool) {
            log("The setup tool is not installed. Run Install.cmd from the release zip, then try "
                "again.");
            return "Setup.ToolMissing";
        }
        if (setup_watcher_.joinable()) {
            setup_watcher_.join(); // the previous setup's watcher has already finished
        }
        // Release the glasses first: the setup needs its own Bluetooth session with them.
        held_ = true;
        refreshLocked();
        std::wstring command = L"\"" + *tool + L"\"";
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION process{};
        if (CreateProcessW(tool->c_str(), command.data(), nullptr, nullptr, FALSE, 0, nullptr,
                           nullptr, &startup, &process) == FALSE) {
            const DWORD error = GetLastError();
            held_ = false;
            refreshLocked();
            log(std::format("Windows could not start the setup tool (error {}).", error));
            return "Setup.Failed";
        }
        CloseHandle(process.hThread);
        UniqueHandle watched(process.hProcess);
        try {
            setup_watcher_ = std::thread(
                [this, handle = std::move(watched)]() mutable { watchSetup(std::move(handle)); });
        } catch (const std::system_error& error) {
            held_ = false;
            refreshLocked();
            log(std::string("The setup window opened, but the glasses link could not watch it, "
                            "so it carries on now: ") +
                error.what());
            return "Setup.Started";
        }
        setup_running_ = true;
        log("The glasses setup window is open. The glasses link waits until it closes.");
        return "Setup.Started";
    }

    bool forget() {
        const std::scoped_lock lock(mutex_);
        if (session_) {
            retirement().retire(std::exchange(session_, nullptr)); // release the glasses first
        }
        const bool gone = mgl::identity::forgetOwnerIdentity();
        log(gone ? "Forgot the glasses: this PC's stored identity was deleted. Run the setup to "
                   "use them again."
                 : "Could not delete all of this PC's stored glasses identity.");
        refreshLocked();
        return gone;
    }

    bool shutdown(std::chrono::milliseconds limit) {
        {
            const std::scoped_lock lock(mutex_);
            shut_down_ = true;
            if (session_) {
                retirement().retire(std::exchange(session_, nullptr));
            }
            if (shutdown_event_) {
                SetEvent(shutdown_event_.get());
            }
        }
        // Outside the lock: the watcher takes it when the setup window closes.
        if (setup_watcher_.joinable()) {
            setup_watcher_.join();
        }
        return retirement().waitIdle(limit);
    }

  private:
    [[nodiscard]] mgl::MediaDemand demandLocked() const {
        mgl::MediaDemand demand;
        demand.wifi_adapter = wifi_adapter_;
        if (held_ || shut_down_) {
            return demand;
        }
        const MglSource* lead = nullptr;
        for (const auto& source : sources_) {
            if (!source->live) {
                continue;
            }
            if (source->video) {
                demand.video = true;
                if (lead == nullptr || source->live_order < lead->live_order) {
                    lead = source.get();
                }
            }
            if (source->audio) {
                demand.audio = true;
            }
        }
        demand.video_config = QualityPresets.at(lead != nullptr ? lead->quality : 0).config;
        demand.keep_link = keep_ready_ && !sources_.empty();
        return demand;
    }

    // mutex_ is held. Hands the demand to the session, creating one when the link is wanted.
    void refreshLocked() {
        const auto demand = demandLocked();
        if (session_ && session_->ended() && retry_ended_ && demand.wantsLink()) {
            retirement().retire(std::exchange(session_, nullptr));
        }
        retry_ended_ = false;
        if (session_) {
            session_->setDemand(demand);
            return;
        }
        if (!demand.wantsLink() || shut_down_) {
            return;
        }
        session_ = makeSession();
        session_->setDemand(demand);
        session_->start();
    }

    std::unique_ptr<mgl::MediaSession> makeSession() {
        auto decoders = std::make_shared<Decoders>();
        decoders->video_open = decoders->video.open();
        if (!decoders->video_open) {
            log("The HEVC decoder could not be opened; the glasses' video cannot be shown.");
        }
        const std::uint64_t generation = status_.beginSession();
        mgl::MediaCallbacks callbacks;
        callbacks.on_parameters = [decoders](mgl::ByteView parameters) {
            decoders->video.setParameters(parameters);
        };
        callbacks.on_frame = [this, decoders](std::uint64_t pts, mgl::ByteView annex,
                                              bool keyframe) {
            if (decoders->video_open) {
                decoders->video.decode(
                    pts, annex, keyframe,
                    [this, &target = *decoders](const mgl::plugin::DecodedFrame& decoded) {
                        deliverPicture(target, decoded);
                    });
            }
        };
        callbacks.on_audio_config = [this, decoders](mgl::ByteView config) {
            if (!decoders->audio.configure(config)) {
                log("The AAC decoder could not open the glasses' audio format; glasses audio is "
                    "unavailable.");
            }
        };
        callbacks.on_audio = [this, decoders](std::uint64_t pts, mgl::ByteView aac) {
            decoders->audio.decode(
                pts, aac, [this, &target = *decoders](const mgl::plugin::DecodedAudio& decoded) {
                    deliverSound(target, decoded);
                });
        };
        callbacks.on_log = [this](const std::string& message) { log(message); };
        callbacks.on_state = [this, generation](mgl::MediaState state) {
            status_.report(generation, state);
        };
        callbacks.on_battery = [this, generation](const mgl::stream::BatteryState& battery) {
            status_.reportBattery(generation, battery);
        };
        return std::make_unique<mgl::MediaSession>(std::move(callbacks));
    }

    // Worker thread: hands one decoded picture to every live source that shows video.
    void deliverPicture(Decoders& decoders, const mgl::plugin::DecodedFrame& decoded) {
        if (decoded.width != decoders.width || decoded.height != decoders.height) {
            decoders.width = decoded.width;
            decoders.height = decoded.height;
            log(std::format("receiving video {}x{}", decoded.width, decoded.height));
        }
        const MglVideoFrame frame{.plane_y = std::get<0>(decoded.plane),
                                  .plane_u = std::get<1>(decoded.plane),
                                  .plane_v = std::get<2>(decoded.plane),
                                  .stride_y = std::get<0>(decoded.linesize),
                                  .stride_u = std::get<1>(decoded.linesize),
                                  .stride_v = std::get<2>(decoded.linesize),
                                  .width = decoded.width,
                                  .height = decoded.height,
                                  .timestamp_ns = decoded.pts_us * NanosPerMicro,
                                  .full_range = decoded.full_range};
        const std::scoped_lock lock(output_mutex_);
        for (const auto& source : sources_) {
            if (source->live && source->video && source->on_frame != nullptr) {
                source->on_frame(source->context, &frame);
            }
        }
    }

    // Worker thread: hands one block of decoded audio to every live source that shows audio.
    // Mono and stereo are handed on; the glasses are asked for stereo.
    void deliverSound(Decoders& decoders, const mgl::plugin::DecodedAudio& decoded) {
        constexpr std::uint32_t MaxDeliveredChannels = 2;
        if (decoded.channels > MaxDeliveredChannels) {
            if (!decoders.audio_layout_reported) {
                decoders.audio_layout_reported = true;
                log(std::format("The glasses sent {}-channel audio; only mono and stereo are "
                                "supported.",
                                decoded.channels));
            }
            return;
        }
        if (decoders.audio_blocks++ == 0) {
            log(std::format("receiving audio {} Hz, {} channel(s)", decoded.sample_rate,
                            decoded.channels));
        }
        MglAudioBlock sound{};
        std::ranges::copy(decoded.plane, std::begin(sound.planes));
        sound.channels = decoded.channels;
        sound.frames = decoded.frames;
        sound.sample_rate = decoded.sample_rate;
        sound.timestamp_ns = decoded.pts_us * NanosPerMicro;
        const std::scoped_lock lock(output_mutex_);
        for (const auto& source : sources_) {
            if (source->live && source->audio && source->on_audio != nullptr) {
                source->on_audio(source->context, &sound);
            }
        }
    }

    // The setup watcher thread: waits for the setup window to close, then lets the link reconnect
    // with whatever identity the setup left. Unload ends the wait and leaves the window open.
    void watchSetup(UniqueHandle process) noexcept {
        try {
            const std::array<HANDLE, 2> waits{process.get(), shutdown_event_.get()};
            const DWORD woke = WaitForMultipleObjects(static_cast<DWORD>(waits.size()),
                                                      waits.data(), FALSE, INFINITE);
            const std::scoped_lock lock(mutex_);
            setup_running_ = false;
            if (woke != WAIT_OBJECT_0) {
                return; // unload, or the wait failed; the window stays open on its own
            }
            DWORD code = 0;
            const bool known = GetExitCodeProcess(process.get(), &code) != FALSE;
            log(known && code == 0
                    ? "Glasses setup finished; reconnecting with the new identity."
                    : std::format("The glasses setup closed without finishing (exit {}).", code));
            held_ = false;
            if (session_) {
                retirement().retire(
                    std::exchange(session_, nullptr)); // the identity may have changed
            }
            refreshLocked();
        } catch (...) {
            // Nothing may escape the thread, and building a message could throw again.
            logFixed("Watching the glasses setup failed; restart OBS if the glasses link stays "
                     "paused.");
        }
    }

    // Logs a fixed message without allocating, for paths that must not throw.
    void logFixed(const char* message) const noexcept {
        if (const LogFn on_log = on_log_.load(); on_log != nullptr) {
            on_log(message);
        }
    }

    mutable std::mutex mutex_;
    std::mutex output_mutex_; // held while media are handed to sources
    std::vector<std::unique_ptr<MglSource>> sources_;
    std::string wifi_adapter_;
    bool keep_ready_ = false;
    bool held_ = false;        // a setup window is open; the glasses are left to it
    bool retry_ended_ = false; // a user action asks for a fresh session after a fatal end
    bool shut_down_ = false;
    bool setup_running_ = false;
    std::uint64_t live_counter_ = 0;
    std::unique_ptr<mgl::MediaSession> session_;
    StatusCell status_;
    std::atomic<LogFn> on_log_{nullptr};
    UniqueHandle shutdown_event_;
    std::thread setup_watcher_;
};

Link& link() {
    static Link instance;
    return instance;
}

// No C++ exception may unwind into OBS's C callbacks: report it and carry on.
template <typename Action> void guarded(std::string_view what, Action action) {
    try {
        action();
    } catch (const std::exception& error) {
        link().log(std::string(what) + " failed: " + error.what());
    }
}

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

void mglLinkInit(void (*on_log)(const char* message)) {
    guarded("glasses link start", [on_log] { link().init(on_log); });
}

bool mglLinkShutdown(std::uint32_t timeout_ms) {
    try {
        return link().shutdown(std::chrono::milliseconds(timeout_ms));
    } catch (const std::exception& error) {
        link().log(std::string("glasses link shutdown failed: ") + error.what());
        return false;
    }
}

MglSource* mglSourceCreate(void (*on_frame)(void* context, const MglVideoFrame* frame),
                           void (*on_audio)(void* context, const MglAudioBlock* audio),
                           void* context) {
    try {
        auto source = std::make_unique<MglSource>();
        source->on_frame = on_frame;
        source->on_audio = on_audio;
        source->context = context;
        // The link owns the source until mglSourceDestroy.
        return link().add(std::move(source));
    } catch (const std::exception& error) {
        link().log(std::string("glasses source setup failed: ") + error.what());
        return nullptr;
    }
}

void mglSourceDestroy(MglSource* source) {
    if (source != nullptr) {
        guarded("glasses source removal", [source] { link().remove(source); });
    }
}

void mglSourceSetLive(MglSource* source, bool live) {
    if (source != nullptr) {
        guarded("glasses source start or stop", [source, live] { link().setLive(*source, live); });
    }
}

void mglSourceSetMedia(MglSource* source, bool video, bool audio) {
    if (source != nullptr) {
        guarded("glasses source media choice", [source, video, audio] {
            link().change(*source, [video, audio](MglSource& target) {
                target.video = video;
                target.audio = audio;
            });
        });
    }
}

const char* mglQualityKey(std::size_t index) {
    return index < QualityPresets.size() ? QualityPresets.at(index).key : nullptr;
}

void mglSourceSetQuality(MglSource* source, const char* key) {
    if (source == nullptr || key == nullptr) {
        return;
    }
    const std::string_view wanted(key);
    const auto preset = std::ranges::find_if(
        QualityPresets, [wanted](const QualityPreset& entry) { return wanted == entry.key; });
    if (preset == QualityPresets.end()) {
        return;
    }
    const auto index = static_cast<std::size_t>(std::distance(QualityPresets.begin(), preset));
    guarded("glasses quality choice", [source, index] {
        link().change(*source, [index](MglSource& target) { target.quality = index; });
    });
}

void mglLinkSetWifiAdapter(const char* id) {
    guarded("Wi-Fi adapter choice",
            [id] { link().setWifiAdapter(id != nullptr ? std::string(id) : std::string()); });
}

void mglLinkSetKeepReady(bool keep_ready) {
    guarded("keep-ready choice", [keep_ready] { link().setKeepReady(keep_ready); });
}

const char* mglLinkStatusKey() {
    try {
        return link().statusKey();
    } catch (const std::exception&) {
        return "Status.Idle"; // a status line must not fail; the next refresh retries
    }
}

int mglLinkBattery(bool* charging) {
    if (charging != nullptr) {
        *charging = false;
    }
    try {
        const auto battery = link().battery();
        if (!battery) {
            return -1;
        }
        if (charging != nullptr) {
            *charging = battery->charging();
        }
        return static_cast<int>(battery->percent);
    } catch (const std::exception&) {
        return -1; // a status line must not fail; the next refresh retries
    }
}

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

const char* mglSetupStart() {
    try {
        return link().startSetup();
    } catch (const std::exception& error) {
        link().log(std::string("starting the glasses setup failed: ") + error.what());
        return "Setup.Failed";
    }
}

bool mglForgetGlasses() {
    try {
        return link().forget();
    } catch (const std::exception& error) {
        link().log(std::string("forgetting the glasses failed: ") + error.what());
        return false;
    }
}
