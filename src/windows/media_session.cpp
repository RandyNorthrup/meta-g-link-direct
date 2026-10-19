// SPDX-License-Identifier: AGPL-3.0-only
// Worker-thread media pipeline. The stream setup order mirrors Visor's LiveStream bring-up at the
// revision in THIRD_PARTY_NOTICES.md. The Wi-Fi link differs from Visor: the glasses host their
// own access point (svc10 CreateSoftApNetwork, from the Meta app) and the PC joins it as a client.
// Every path where the glasses joined an access point stopped sending video within a minute;
// the glasses-hosted 2.4 GHz / 20 MHz network streamed full resolution without a drop.
#include "mgl/media_session.hpp"

#include "mgl/airshield.hpp"
#include "mgl/bytes.hpp"
#include "mgl/crypto.hpp"
#include "mgl/hevc.hpp"
#include "mgl/identity.hpp"
#include "mgl/rfcomm_transport.hpp"
#include "mgl/session.hpp"
#include "mgl/stream.hpp"
#include "mgl/tcp_transport.hpp"
#include "mgl/transport.hpp"
#include "mgl/wifi_station.hpp"
#include "wifi_credentials.hpp"

#include <windows.h> // IWYU pragma: keep — SDK umbrella prerequisites.

#include <winnls.h>

#include <winrt/base.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

namespace mgl {
namespace {
using namespace std::chrono_literals;

ByteView viewOf(const Bytes& bytes) { return {bytes.data(), bytes.size()}; }

void logLine(const MediaCallbacks& callbacks, const std::string& text) {
    if (callbacks.on_log) {
        callbacks.on_log(text);
    }
}

// How often waits check for a stop request.
constexpr auto StopPollInterval = 200ms;

// Sleeps in short steps; false if stop was set before the time passed.
bool sleepUnlessStopped(std::chrono::milliseconds total, const std::atomic<bool>& stop) {
    const auto until = std::chrono::steady_clock::now() + total;
    while (std::chrono::steady_clock::now() < until) {
        if (stop.load()) {
            return false;
        }
        std::this_thread::sleep_for(StopPollInterval);
    }
    return !stop.load();
}

// The glasses hold one session at a time and host one network for one PC, so only one media
// session in the process drives the radio. Later sessions wait their turn instead of fighting over
// the link, and a session that is still tearing down keeps the lease until its teardown is done.
class RadioLease {
  public:
    bool tryAcquire(const void* holder) {
        const std::scoped_lock lock(mutex_);
        if (holder_ != nullptr) {
            return false;
        }
        holder_ = holder;
        return true;
    }

    // Waits until the radio is free or stop is set; true once acquired.
    bool acquire(const void* holder, const std::atomic<bool>& stop) {
        std::unique_lock lock(mutex_);
        while (holder_ != nullptr) {
            if (stop.load()) {
                return false;
            }
            (void)freed_.wait_for(lock, StopPollInterval);
        }
        holder_ = holder;
        return true;
    }

    void release(const void* holder) {
        {
            const std::scoped_lock lock(mutex_);
            if (holder_ == holder) {
                holder_ = nullptr;
            }
        }
        freed_.notify_all();
    }

  private:
    std::mutex mutex_;
    std::condition_variable freed_;
    const void* holder_ = nullptr;
};

RadioLease& radioLease() {
    static RadioLease lease;
    return lease;
}

// Gives the lease back when the worker leaves run(), on every path.
class LeaseHold {
  public:
    explicit LeaseHold(const void* holder) : holder_(holder) {}
    ~LeaseHold() { radioLease().release(holder_); }
    LeaseHold(const LeaseHold&) = delete;
    LeaseHold& operator=(const LeaseHold&) = delete;
    LeaseHold(LeaseHold&&) = delete;
    LeaseHold& operator=(LeaseHold&&) = delete;

  private:
    const void* holder_;
};

// The glasses' access point: 2.4 GHz at 20 MHz on channel 6, which every regulatory domain allows.
inline constexpr std::uint32_t AccessPointChannel = 6;
// The glasses take a few seconds to start the access point and report it.
inline constexpr auto AccessPointAnswerWait = 30s;

// The user's region as an ISO 3166 code for the access point's regulatory settings. Falls back to
// "US" when Windows has no region; channel 6 at 20 MHz is legal in every domain either way.
std::string regionCode() {
    constexpr int RegionCodeLength = 3; // two letters and the terminator
    std::array<wchar_t, RegionCodeLength> name{};
    if (GetUserDefaultGeoName(name.data(), RegionCodeLength) == RegionCodeLength) {
        std::string code;
        for (const wchar_t letter : std::wstring_view(name.data(), RegionCodeLength - 1)) {
            if (letter < L'A' || letter > L'Z') {
                return "US";
            }
            code.push_back(static_cast<char>(letter));
        }
        return code;
    }
    return "US";
}

// Waits for SoftApNetworkInfo after a CreateSoftApNetwork. nullopt on timeout, link loss or stop.
std::optional<stream::SoftApInfo> awaitAccessPoint(GlassesSession& session,
                                                   const std::atomic<bool>& stop) {
    const auto deadline = std::chrono::steady_clock::now() + AccessPointAnswerWait;
    while (std::chrono::steady_clock::now() < deadline && !stop.load()) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        auto message = session.receiveRaw(std::min<std::chrono::milliseconds>(remaining, 500ms));
        if (!message) {
            if (session.linkLost()) {
                return std::nullopt;
            }
            continue;
        }
        if (message->service == stream::ServiceAtc &&
            message->type == stream::MtSoftApNetworkInfo) {
            return stream::parseSoftApNetworkInfo(viewOf(message->payload));
        }
    }
    return std::nullopt;
}

// Asks the glasses to host their own access point with fresh credentials and returns where to
// reach them. A failed start (for example INTERNAL_ERROR while the glasses' Wi-Fi is still busy
// with a previous network) is retried a few times.
std::optional<stream::SoftApInfo> hostAccessPoint(GlassesSession& session,
                                                  const std::atomic<bool>& stop,
                                                  const MediaCallbacks& callbacks) {
    constexpr int HostAttempts = 4;
    const std::string region = regionCode();
    for (int attempt = 0; attempt < HostAttempts; ++attempt) {
        if (attempt > 0 && !sleepUnlessStopped(4s, stop)) {
            return std::nullopt;
        }
        const auto credentials = randomWifiCredentials();
        const auto request = stream::buildCreateSoftApNetwork(
            credentials.ssid, credentials.passphrase, stream::Band24GHz, stream::Bw20MHz,
            AccessPointChannel, region, airshield::randomUuid());
        if (!session.sendOnService(stream::ServiceAtc, stream::MtCreateSoftApNetwork,
                                   viewOf(request))) {
            return std::nullopt;
        }
        auto answer = awaitAccessPoint(session, stop);
        if (answer && answer->ok()) {
            // The glasses echo the credentials; keep ours if the echo is empty.
            if (answer->ssid.empty()) {
                answer->ssid = credentials.ssid;
            }
            if (answer->passphrase.empty()) {
                answer->passphrase = credentials.passphrase;
            }
            return answer;
        }
        logLine(callbacks,
                answer ? std::format("The glasses could not start their Wi-Fi network (status {}, "
                                     "try {}).",
                                     answer->status, attempt + 1)
                       : std::format("The glasses did not report their Wi-Fi network (try {}).",
                                     attempt + 1));
        if (session.linkLost() || stop.load()) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::string describeStationFailure(const WifiStation& station) {
    std::string text = "This PC could not join the glasses' Wi-Fi network: ";
    switch (station.status) {
    case WifiStationStatus::NoAdapter:
        text += "no Wi-Fi adapter is available";
        break;
    case WifiStationStatus::AdapterMissing:
        text += "the Wi-Fi adapter chosen in the source properties is not available";
        break;
    case WifiStationStatus::ProfileFailed:
        text += "Windows refused the network profile";
        break;
    case WifiStationStatus::ConnectFailed:
        text += "the adapter did not connect in time";
        break;
    case WifiStationStatus::NoAddress:
        text += "no network address was assigned";
        break;
    case WifiStationStatus::BadPeer:
        text += "the glasses reported an address that is not IPv4";
        break;
    case WifiStationStatus::Ok:
        text += "unexpected success";
        break;
    }
    if (station.detail != 0) {
        text += std::format(" (error {})", station.detail);
    }
    return text + ".";
}

enum class Ack : std::uint8_t { Received, Missing, LinkLost };

struct AckResult {
    Ack ack = Ack::Missing;
    Bytes payload;
};

// Each stream setup step must be acknowledged before the next, or the glasses drop the setup.
// Waits for the matching response until the timeout passes, the link drops or stop is set.
AckResult awaitMedia(GlassesSession& media, std::uint16_t service, std::uint16_t type,
                     std::chrono::seconds timeout, const std::atomic<bool>& stop) {
    const auto until = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < until && !stop.load()) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            until - std::chrono::steady_clock::now());
        auto message = media.receiveRaw(remaining);
        if (!message) {
            break;
        }
        if (message->service == service && message->type == type) {
            return {.ack = Ack::Received, .payload = std::move(message->payload)};
        }
    }
    return {.ack = media.linkLost() ? Ack::LinkLost : Ack::Missing, .payload = {}};
}

// Hands one reassembled video access unit to the frame callback. True if it was delivered.
bool deliverFrame(ByteView frame, const MediaCallbacks& callbacks) {
    const auto unit = stream::splitTimedUnit(frame);
    if (!unit || !callbacks.on_frame) {
        return false;
    }
    callbacks.on_frame(unit->pts_us, unit->payload, hevc::hasKeyframe(unit->payload));
    return true;
}

// Hands one reassembled AAC access unit to the audio callback.
void deliverAudio(ByteView unit, const MediaCallbacks& callbacks) {
    if (const auto timed = stream::splitTimedUnit(unit); timed && callbacks.on_audio) {
        callbacks.on_audio(timed->pts_us, timed->payload);
    }
}

// Routes a codec configuration. Field 2 names the codec; failing that, video parameter sets are
// Annex-B NAL units and an AAC AudioSpecificConfig is not (as Visor's LiveStream.kt decides).
void deliverCodecConfig(const stream::CodecConfig& codec, const MediaCallbacks& callbacks) {
    if (codec.data.empty()) {
        return;
    }
    const ByteView data = viewOf(codec.data);
    const bool video = codec.codec == stream::CodecVideo ||
                       (codec.codec != stream::CodecAudio && !hevc::nalTypes(data).empty());
    if (video) {
        if (callbacks.on_parameters) {
            callbacks.on_parameters(data);
        }
    } else if (callbacks.on_audio_config) {
        callbacks.on_audio_config(data);
    }
}

// Media chunks being reassembled into whole access units.
struct Reassembly {
    Bytes video;
    Bytes audio;
};

enum class MediaEvent : std::uint8_t { Nothing, Frame, AudioUnit, StreamStopped };

// The glasses' own reports on the stream: why they stopped it, and battery or heat changes. Other
// types are noted once each, by type and size only.
MediaEvent handleStreamReport(const RawMessage& message, const MediaCallbacks& callbacks,
                              std::set<std::uint16_t>& noted) {
    const ByteView payload(message.payload.data(), message.payload.size());
    if (message.type == stream::MtControlStreamStopped) {
        const auto reason = stream::parseStreamStopped(payload).value_or(0);
        logLine(callbacks, std::format("The glasses stopped the stream: {} (reason {}).",
                                       stream::stopReasonText(reason), reason));
        return MediaEvent::StreamStopped;
    }
    if (message.type == stream::MtSystemHealth) {
        if (const auto health = stream::parseHealthEvent(payload)) {
            logLine(callbacks, std::format("Glasses health: type {}, status {}, level {}.",
                                           health->type, health->status, health->level));
        }
        return MediaEvent::Nothing;
    }
    if (noted.insert(message.type).second) {
        logLine(callbacks, std::format("Media message type {} ({} bytes).", message.type,
                                       message.payload.size()));
    }
    return MediaEvent::Nothing;
}

// Feeds one media-service message into access-unit reassembly, or reports it.
MediaEvent handleMediaMessage(const RawMessage& message, Reassembly& units,
                              const MediaCallbacks& callbacks, std::set<std::uint16_t>& noted) {
    const ByteView payload(message.payload.data(), message.payload.size());
    switch (message.type) {
    case stream::MtVideoChunk:
        append(units.video, payload);
        return MediaEvent::Nothing;
    case stream::MtVideoChunkEnd: {
        append(units.video, payload);
        const bool delivered = deliverFrame(viewOf(units.video), callbacks);
        units.video.clear();
        return delivered ? MediaEvent::Frame : MediaEvent::Nothing;
    }
    case stream::MtCodecConfig:
        if (const auto codec = stream::parseCodecConfig(payload)) {
            deliverCodecConfig(*codec, callbacks);
        }
        return MediaEvent::Nothing;
    case stream::MtAudioChunk:
        append(units.audio, payload);
        return MediaEvent::Nothing;
    case stream::MtAudioChunkEnd:
        append(units.audio, payload);
        deliverAudio(viewOf(units.audio), callbacks);
        units.audio.clear();
        return MediaEvent::AudioUnit;
    default:
        return handleStreamReport(message, callbacks, noted);
    }
}

// Notes, once each, a message from a service other than media and the keepalive, by service,
// type and size only.
void noteOtherService(const RawMessage& message, const MediaCallbacks& callbacks,
                      std::set<std::uint32_t>& noted) {
    constexpr unsigned TypeBits = 16;
    const auto key = (static_cast<std::uint32_t>(message.service) << TypeBits) | message.type;
    if (noted.insert(key).second) {
        logLine(callbacks, std::format("Wi-Fi message service {} type {} ({} bytes).",
                                       message.service, message.type, message.payload.size()));
    }
}

// The Wi-Fi session keepalive, as Visor's LiveStream.kt keepAlive: the phone is otherwise silent
// once the stream runs, and the glasses drop a silent session after about half a minute. Every
// 2 s an empty svc100 app-info announce goes out; the glasses answer with their device info.
class Keepalive {
  public:
    // Sends the announce when one is due. False if the send failed.
    bool tick(GlassesSession& media) {
        const auto now = std::chrono::steady_clock::now();
        if (now < next_) {
            return true;
        }
        next_ = now + Interval;
        return media.sendOnService(stream::ServicePeerBuildInfo, stream::MtPeerAppInfo, {});
    }

    // True, and recorded, when the message is the glasses' answer.
    bool observe(const RawMessage& message) {
        if (message.service != stream::ServicePeerBuildInfo ||
            message.type != stream::MtPeerDeviceInfo) {
            return false;
        }
        answered_at_ = std::chrono::steady_clock::now();
        return true;
    }

    // True once answers that were arriving have stopped for the silence window. A session that
    // never answered may simply not host svc100, so that alone does not count.
    [[nodiscard]] bool silent() const {
        return answered_at_ && std::chrono::steady_clock::now() - *answered_at_ > Silence;
    }

  private:
    static constexpr auto Interval = 2s;
    static constexpr auto Silence = 8s;
    std::chrono::steady_clock::time_point next_; // the epoch: the first announce goes at once
    std::optional<std::chrono::steady_clock::time_point> answered_at_;
};

// The receive side of one media session: reassembly, the keepalive, and whether the wanted medium
// is arriving. Video paces a stream that carries it; an audio-only stream is paced by its audio.
class StreamWatch {
  public:
    // After the stream changed: nothing has arrived for the new setup yet.
    void restart() {
        flowing_ = false;
        last_media_at_ = std::chrono::steady_clock::now();
    }
    [[nodiscard]] bool flowing() const noexcept { return flowing_; }
    [[nodiscard]] bool streamed() const noexcept { return streamed_; }

    enum class Pump : std::uint8_t { Continue, FirstMedia, LinkLost, Silent, Stalled, StreamEnded };

    // Sends the keepalive when due, then reads and handles one message, waiting a short slice so a
    // changed demand is applied quickly. video and audio say what the running stream carries.
    Pump pump(GlassesSession& media, bool video, bool audio, const MediaCallbacks& callbacks) {
        constexpr auto ReceiveSlice = 200ms;
        if (!keepalive_.tick(media)) {
            return Pump::LinkLost;
        }
        auto message = media.receiveRaw(ReceiveSlice);
        if (media.linkLost()) {
            return Pump::LinkLost;
        }
        if (keepalive_.silent()) {
            return Pump::Silent;
        }
        if (stalled(video || audio)) {
            return Pump::Stalled;
        }
        if (!message || keepalive_.observe(*message)) {
            return Pump::Continue; // a quiet gap or a keepalive answer
        }
        if (message->service == stream::ServiceStatus && reportBattery(*message, callbacks)) {
            return Pump::Continue;
        }
        if (message->service != stream::ServiceMedia) {
            noteOtherService(*message, callbacks, noted_other_);
            return Pump::Continue;
        }
        const auto event = handleMediaMessage(*message, units_, callbacks, noted_media_);
        if (event == MediaEvent::StreamStopped && (video || audio)) {
            return Pump::StreamEnded;
        }
        return record(event, video, audio) ? Pump::FirstMedia : Pump::Continue;
    }

  private:
    // A device-status push that carries the battery: logged and reported when the percentage or
    // the charging state changes. False for any other message.
    bool reportBattery(const RawMessage& message, const MediaCallbacks& callbacks) {
        if (message.type < stream::MtDeviceStateFirst || message.type > stream::MtDeviceStateLast) {
            return false;
        }
        const auto battery = stream::parseBatteryState(viewOf(message.payload));
        if (!battery) {
            return false;
        }
        if (!battery_ || battery_->percent != battery->percent ||
            battery_->charging() != battery->charging()) {
            constexpr double PerUnit = 1000.0;
            logLine(callbacks,
                    std::format("Glasses battery: {}%, {} ({:.2f} V, {:.1f} C).", battery->percent,
                                battery->charging() ? "charging" : "not charging",
                                static_cast<double>(battery->millivolts) / PerUnit,
                                static_cast<double>(battery->millicelsius) / PerUnit));
            if (callbacks.on_battery) {
                callbacks.on_battery(*battery);
            }
        }
        battery_ = battery;
        return true;
    }

    [[nodiscard]] bool stalled(bool streaming) const {
        constexpr auto StallTimeout = 8s;
        return streaming && std::chrono::steady_clock::now() - last_media_at_ > StallTimeout;
    }
    // Records a delivered unit; true the first time the wanted medium arrives after a start.
    bool record(MediaEvent event, bool video, bool audio) {
        const bool paced = (event == MediaEvent::Frame && video) ||
                           (event == MediaEvent::AudioUnit && !video && audio);
        if (!paced) {
            return false;
        }
        last_media_at_ = std::chrono::steady_clock::now();
        const bool first = !flowing_;
        flowing_ = true;
        streamed_ = true;
        return first;
    }

    Keepalive keepalive_;
    Reassembly units_;
    std::set<std::uint16_t> noted_media_;
    std::set<std::uint32_t> noted_other_;
    std::optional<stream::BatteryState> battery_; // the last battery report
    bool flowing_ = false;                        // the wanted medium arrived since the last start
    bool streamed_ = false;                       // media arrived at some point in this session
    std::chrono::steady_clock::time_point last_media_at_ = std::chrono::steady_clock::now();
};
// Waits briefly for the glasses to confirm their access point is gone. True once they answer.
bool awaitAccessPointClosed(GlassesSession& session) {
    const auto deadline = std::chrono::steady_clock::now() + 3s;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto message = session.receiveRaw(500ms);
        if (!message) {
            if (session.linkLost()) {
                return false;
            }
            continue;
        }
        if (message->service == stream::ServiceAtc &&
            message->type == stream::MtDestroySoftApNetworkResult) {
            return true;
        }
    }
    return false;
}

// Asks the glasses to close their access point over session and logs the outcome. False when the
// link was already gone, so the request could not be sent.
bool closeAccessPoint(GlassesSession& session, const MediaCallbacks& callbacks) {
    const Bytes empty;
    if (session.linkLost() ||
        !session.sendOnService(stream::ServiceAtc, stream::MtDestroySoftApNetwork, viewOf(empty))) {
        return false;
    }
    logLine(callbacks, awaitAccessPointClosed(session)
                           ? "Closed the glasses' Wi-Fi network."
                           : "The glasses did not confirm closing their Wi-Fi network.");
    return true;
}
} // namespace

struct MediaSession::MediaLink {
    std::unique_ptr<Transport> transport;
    std::unique_ptr<GlassesSession> session; // declared last so it is destroyed first
};

struct MediaSession::ControlLink {
    std::unique_ptr<Transport> transport;
    std::unique_ptr<GlassesSession> session; // declared after the transport so it closes first
    stream::SoftApInfo access_point;
};

MediaSession::MediaSession(MediaCallbacks callbacks) : callbacks_(std::move(callbacks)) {}

MediaSession::~MediaSession() { stop(); }

void MediaSession::setState(MediaState next) {
    state_.store(next);
    if (callbacks_.on_state) {
        callbacks_.on_state(next);
    }
}

void MediaSession::log(const std::string& message) {
    if (callbacks_.on_log) {
        callbacks_.on_log(message);
    }
}

void MediaSession::start() {
    if (worker_.joinable()) {
        return;
    }
    stop_.store(false);
    worker_ = std::thread([this] { run(); });
}

void MediaSession::requestStop() noexcept {
    stop_.store(true);
    demand_changed_.notify_all();
}

void MediaSession::setDemand(const MediaDemand& demand) {
    {
        const std::scoped_lock lock(demand_mutex_);
        demand_ = demand;
    }
    demand_changed_.notify_all();
}

MediaDemand MediaSession::currentDemand() const {
    const std::scoped_lock lock(demand_mutex_);
    return demand_;
}

bool MediaSession::awaitDemand() {
    if (!currentDemand().wantsLink() && !stop_.load()) {
        setState(MediaState::Idle);
    }
    std::unique_lock lock(demand_mutex_);
    // A bounded wait, so a stop request that lands between the check and the wait is still seen.
    while (!stop_.load() && !demand_.wantsLink()) {
        (void)demand_changed_.wait_for(lock, StopPollInterval);
    }
    return !stop_.load();
}

void MediaSession::stop() {
    requestStop();
    if (worker_.joinable()) {
        worker_.join();
    }
    if (state_.load() != MediaState::Failed) {
        setState(MediaState::Stopped);
    }
}

bool MediaSession::interruptibleSleep(std::chrono::milliseconds total) {
    return sleepUnlessStopped(total, stop_);
}

std::string MediaSession::elapsed() const {
    const auto taken = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - attempt_started_);
    return std::format("{:.1f} s", static_cast<double>(taken.count()) / 1000.0);
}

void MediaSession::run() {
    if (!radioLease().tryAcquire(this)) {
        setState(MediaState::Waiting);
        log("Another glasses link in this program is using the glasses; waiting for it to finish.");
        if (!radioLease().acquire(this, stop_)) {
            return;
        }
    }
    const LeaseHold hold(this);
    runConnected();
}

void MediaSession::runConnected() {
    // Keep the link up across drops while the demand wants it: each attempt runs until the link
    // ends, then reconnects with a short backoff. With nothing wanted the worker waits, connected
    // to nothing, until the demand changes, the session is stopped or a fatal error occurs.
    const std::array<std::chrono::milliseconds, 4> backoff{2s, 4s, 8s, 15s};
    std::size_t failures = 0;
    while (awaitDemand()) {
        const SessionEnd end = attemptSession();
        if (end == SessionEnd::Fatal) {
            ended_.store(true);
            return;
        }
        if (stop_.load()) {
            return;
        }
        if (end == SessionEnd::Released) {
            failures = 0;
            continue;
        }
        // Media that flowed and then dropped reconnects quickly; repeated failures back off.
        failures = end == SessionEnd::Dropped ? 0 : failures + 1;
        const auto wait = backoff.at(std::min(failures, backoff.size() - 1));
        setState(MediaState::Reconnecting);
        log(std::format("Reconnecting to the glasses in {} s.",
                        std::chrono::duration_cast<std::chrono::seconds>(wait).count()));
        if (!interruptibleSleep(wait)) {
            return;
        }
    }
}

MediaSession::SessionEnd MediaSession::attemptSession() {
    // The worker thread is the boundary: an exception escaping it would call std::terminate and
    // take the host process (OBS) down. A failed attempt is logged and retried like a dropped link.
    try {
        return runSession();
    } catch (const std::exception& error) {
        log(std::string("Connection attempt failed: ") + error.what());
        return SessionEnd::Failed;
    } catch (const winrt::hresult_error& error) {
        log(std::format("Connection attempt failed with Windows error 0x{:08X}.",
                        static_cast<std::uint32_t>(error.code().value)));
        return SessionEnd::Failed;
    } catch (...) {
        log("Connection attempt failed with an unrecognised error; the camera stops.");
        setState(MediaState::Failed);
        return SessionEnd::Fatal;
    }
}

MediaSession::SessionEnd MediaSession::runSession() {
    attempt_started_ = std::chrono::steady_clock::now();
    setState(MediaState::Connecting);
    const auto identity = identity::loadOwnerIdentity();
    if (!identity) {
        log("No stored glasses identity; run setup first.");
        setState(MediaState::Failed);
        return SessionEnd::Fatal;
    }
    const auto owner = crypto::OwnerKey::importPrivate(
        ByteView(identity->owner_private.data(), identity->owner_private.size()));
    if (!owner) {
        log("Stored glasses identity is unreadable.");
        setState(MediaState::Failed);
        return SessionEnd::Fatal;
    }

    // The adapter is fixed for the attempt; choosing another one releases the link and reconnects.
    const std::string wifi_adapter = currentDemand().wifi_adapter;
    // The control link outlives the rest of the attempt so it can close the glasses' access point
    // if the attempt fails before the media link opens. Locals unwind in reverse order: the media
    // link closes, then this PC leaves the glasses' network, then the control link goes.
    auto failure = SessionEnd::Failed;
    const auto control = openControlLink(*owner, failure);
    if (!control) {
        return failure;
    }
    log(std::format("The glasses' Wi-Fi network is up after {}; joining it from this PC.",
                    elapsed()));
    WifiStation station;
    const auto station_link =
        connectWifiStation(control->access_point.ssid, control->access_point.passphrase,
                           wifi_adapter, control->access_point.address, station);
    if (!station_link) {
        log(describeStationFailure(station));
        abandonAccessPoint(*control);
        return SessionEnd::Failed;
    }
    log(std::format(
        "Joined the glasses' Wi-Fi network after {} (associated in {} ms, address in {} "
        "ms); opening the media connection.",
        elapsed(), station.associated_after.count(),
        (station.addressed_after - station.associated_after).count()));
    const auto link = openMediaLink(*owner, control->access_point);
    if (!link) {
        abandonAccessPoint(*control);
        return SessionEnd::Failed;
    }
    // From here the glasses have left Bluetooth. They close their network on their own within
    // about 15 s of this PC leaving (measured); a close sent over the Wi-Fi link was never
    // acknowledged, so none is sent.
    nonce_ = 1;
    if (!prepareMedia(*link->session)) {
        return SessionEnd::Failed;
    }
    return serve(*link->session, wifi_adapter);
}

std::unique_ptr<MediaSession::ControlLink>
MediaSession::openControlLink(const crypto::OwnerKey& owner, SessionEnd& failure) {
    // The glasses allow one RFCOMM session at a time and need a moment between them, so a connect
    // right after a previous session can fail transiently.
    constexpr int RfcommAttempts = 6;
    RfcommOpen bt;
    for (int attempt = 0; attempt < RfcommAttempts && !bt.transport; ++attempt) {
        if (attempt > 0 && !interruptibleSleep(3s)) {
            return nullptr;
        }
        bt = openBondedMetaRfcomm();
    }
    if (!bt.transport) {
        log("The glasses did not accept a Bluetooth connection.");
        return nullptr;
    }
    auto control = std::make_unique<ControlLink>();
    control->transport = std::move(bt.transport);
    control->session = std::make_unique<GlassesSession>(*control->transport, &owner);
    const auto outcome = control->session->open();
    if (outcome.stage != LinkStage::Ready) {
        log("The authenticated Bluetooth session did not reach ready.");
        return nullptr;
    }
    // The session reaches Ready even when the glasses refuse the owner key (the ownership setup
    // relies on that), but nothing else works then. Retrying cannot help: the owner changed.
    if (!outcome.trust_tx_ok) {
        log(std::format("The glasses did not accept this PC as their owner (status 0x{:04X}). "
                        "Another PC or the phone may have taken ownership; run the setup again.",
                        outcome.trust_status));
        setState(MediaState::Failed);
        failure = SessionEnd::Fatal;
        return nullptr;
    }
    log(std::format("Authenticated after {}; asking the glasses to start their Wi-Fi network.",
                    elapsed()));
    auto access_point = hostAccessPoint(*control->session, stop_, callbacks_);
    if (!access_point) {
        log(stop_.load() ? "Stopped before the glasses started their Wi-Fi network."
                         : "The glasses did not start their Wi-Fi network.");
        return nullptr;
    }
    control->access_point = std::move(*access_point);
    return control;
}

void MediaSession::abandonAccessPoint(ControlLink& control) {
    if (!closeAccessPoint(*control.session, callbacks_)) {
        log("The Bluetooth link to the glasses had closed; their Wi-Fi network was left for them "
            "to close.");
    }
}

std::unique_ptr<MediaSession::MediaLink>
MediaSession::openMediaLink(const crypto::OwnerKey& owner, const stream::SoftApInfo& access_point) {
    // The glasses host the network, so their TCP server is normally up before this PC has an
    // address. Connect at once and retry the whole TCP AirShield session on a short, growing delay
    // in case it is not.
    const std::array<std::chrono::milliseconds, 7> delays{0ms, 250ms, 500ms, 1s, 2s, 3s, 4s};
    LinkOutcome outcome;
    for (const auto delay : delays) {
        if (!interruptibleSleep(delay)) {
            return nullptr;
        }
        auto tcp = openTcp(access_point.address, static_cast<std::uint16_t>(access_point.port));
        if (!tcp.transport) {
            continue;
        }
        auto link = std::make_unique<MediaLink>();
        link->transport = std::move(tcp.transport);
        link->session = std::make_unique<GlassesSession>(*link->transport, &owner);
        outcome = link->session->open();
        if (outcome.stage == LinkStage::Ready) {
            log(std::format("Media session ready after {}; starting the camera.", elapsed()));
            return link;
        }
        // The failed link drops here, session first, closing its connection.
    }
    log("The media session over Wi-Fi did not reach ready (stage " +
        std::to_string(static_cast<int>(outcome.stage)) + ", error " +
        std::to_string(static_cast<int>(outcome.error)) + ").");
    return nullptr;
}

bool MediaSession::sendStep(GlassesSession& media, std::string_view name, std::uint16_t service,
                            std::uint16_t type, const Bytes& payload) {
    if (media.sendOnService(service, type, viewOf(payload))) {
        return true;
    }
    log(std::format("Sending {} failed; the link dropped.", name));
    return false;
}

MediaSession::Step MediaSession::awaitStep(GlassesSession& media, std::string_view name,
                                           std::uint16_t service, std::uint16_t type,
                                           const Bytes& payload, std::uint16_t response,
                                           std::chrono::seconds wait) {
    if (!sendStep(media, name, service, type, payload)) {
        return Step::Failed;
    }
    const auto result = awaitMedia(media, service, response, wait, stop_);
    if (result.ack == Ack::LinkLost) {
        log(std::format("The link dropped while waiting for {}.", name));
        return Step::Failed;
    }
    if (result.ack == Ack::Missing) {
        if (stop_.load()) {
            return Step::Failed;
        }
        log(std::format("No acknowledgement for {}; continuing.", name));
        return Step::Done;
    }
    // The configure and stream-control answers carry an error code; the glasses refuse, for
    // example, a video profile they cannot encode.
    if (response == stream::MtConfigureVideoResponse ||
        response == stream::MtConfigureAudioResponse ||
        response == stream::MtControlStreamResponse) {
        if (const auto answer = stream::parseStreamAnswer(viewOf(result.payload));
            answer && answer->error != 0) {
            log(std::format("The glasses refused {} (error {}).", name, answer->error));
            return Step::Refused;
        }
    }
    return stop_.load() ? Step::Failed : Step::Done;
}

bool MediaSession::prepareMedia(GlassesSession& media) {
    return sendStep(media, "Subscribe", stream::ServiceStatus, stream::MtInit,
                    stream::buildSubscribe()) &&
           awaitStep(media, "StartApp", stream::ServiceStatus, stream::MtStartApp,
                     stream::buildStartApp(stream::AppLivestream), stream::MtStartAppResponse,
                     7s) == Step::Done &&
           sendStep(media, "SessionSettings", stream::ServiceMedia, stream::MtSessionSettings,
                    stream::buildSessionSettings(3, /*bypass_don=*/true));
}

MediaSession::StreamSetup MediaSession::setupFor(const MediaDemand& demand,
                                                 const Configured& configured) {
    StreamSetup setup;
    if (demand.video) {
        setup.video = demand.video_config;
    }
    setup.audio = demand.audio && !configured.audio_refused;
    return setup;
}

std::optional<MediaSession::StreamSetup> MediaSession::applySetup(GlassesSession& media,
                                                                  const StreamSetup& running,
                                                                  const StreamSetup& wanted,
                                                                  Configured& configured) {
    constexpr auto AnswerWait = 5s;
    if (running.streams() &&
        awaitStep(media, "ControlStream STOP", stream::ServiceMedia, stream::MtControlStream,
                  stream::buildControlStream(nonce_++, stream::ActionStop),
                  stream::MtControlStreamResponse, AnswerWait) == Step::Failed) {
        return std::nullopt;
    }
    if (!wanted.streams()) {
        return StreamSetup{};
    }
    // Video goes out only when the profile or its on/off state differs from what the glasses
    // hold; a video stream switched off keeps its last profile.
    if (wanted.video) {
        if (!configured.video_enabled || configured.video != wanted.video) {
            if (awaitStep(media, "ConfigureVideo", stream::ServiceMedia, stream::MtConfigureVideo,
                          stream::buildConfigureVideo(nonce_++, *wanted.video, true),
                          stream::MtConfigureVideoResponse, AnswerWait) != Step::Done) {
                return std::nullopt;
            }
            configured.video = wanted.video;
            configured.video_enabled = true;
        }
    } else if (configured.video_enabled && configured.video) {
        if (awaitStep(media, "ConfigureVideo off", stream::ServiceMedia, stream::MtConfigureVideo,
                      stream::buildConfigureVideo(nonce_++, *configured.video, false),
                      stream::MtConfigureVideoResponse, AnswerWait) != Step::Done) {
            return std::nullopt;
        }
        configured.video_enabled = false;
    }
    StreamSetup effective = wanted;
    if (configured.audio != wanted.audio) {
        const Step step =
            awaitStep(media, "ConfigureAudio", stream::ServiceMedia, stream::MtConfigureAudio,
                      stream::buildConfigureAudio(nonce_++, wanted.audio),
                      stream::MtConfigureAudioResponse, AnswerWait);
        if (step == Step::Failed) {
            return std::nullopt;
        }
        if (step == Step::Refused) {
            // Not fatal: video goes on without it, and this session stops asking for audio.
            log("The glasses refused the audio stream; continuing without glasses audio.");
            configured.audio_refused = true;
            configured.audio = false;
            effective.audio = false;
        } else {
            configured.audio = wanted.audio;
        }
    }
    if (!effective.streams()) {
        return StreamSetup{};
    }
    if (effective.audio && callbacks_.on_audio_config) {
        // The glasses send raw AAC with no AudioSpecificConfig, so describe the format asked of
        // them. A configuration the glasses do send later replaces it.
        const Bytes config = stream::requestedAudioSpecificConfig();
        callbacks_.on_audio_config(viewOf(config));
    }
    if (awaitStep(media, "ControlStream START", stream::ServiceMedia, stream::MtControlStream,
                  stream::buildControlStream(nonce_++, stream::ActionStart),
                  stream::MtControlStreamResponse, AnswerWait) != Step::Done) {
        return std::nullopt;
    }
    return effective;
}

void MediaSession::stopBeforeLeaving(GlassesSession& media, const StreamSetup& running) {
    if (!running.streams() || media.linkLost()) {
        return;
    }
    // The link may already be failing, so no answer is awaited; the pause lets the request out.
    constexpr auto SendPause = 300ms;
    (void)media.sendOnService(stream::ServiceMedia, stream::MtControlStream,
                              viewOf(stream::buildControlStream(nonce_++, stream::ActionStop)));
    std::this_thread::sleep_for(SendPause);
}

const char* MediaSession::releaseReason(const MediaDemand& demand,
                                        const std::string& wifi_adapter) const {
    if (stop_.load()) {
        return "Glasses link stopped.";
    }
    if (!demand.wantsLink()) {
        return "No glasses source needs the glasses now; disconnecting.";
    }
    if (demand.wifi_adapter != wifi_adapter) {
        return "The Wi-Fi adapter choice changed; reconnecting with the new adapter.";
    }
    return nullptr;
}

namespace {
// How a pump result ends the attempt: the log line, and whether the running stream should be
// stopped first. nullopt keeps serving.
struct Ending {
    const char* why = nullptr;
    bool stop_stream = false;
};

std::optional<Ending> endingFor(StreamWatch::Pump pump, bool flowing) {
    switch (pump) {
    case StreamWatch::Pump::Continue:
    case StreamWatch::Pump::FirstMedia:
        return std::nullopt;
    case StreamWatch::Pump::LinkLost:
        return Ending{.why = "The media link to the glasses dropped.", .stop_stream = false};
    case StreamWatch::Pump::Silent:
        return Ending{.why = "The glasses stopped answering over Wi-Fi.", .stop_stream = true};
    case StreamWatch::Pump::Stalled:
        return Ending{.why = flowing ? "The stream stalled; the link may have dropped."
                                     : "The glasses accepted the stream but sent nothing.",
                      .stop_stream = true};
    case StreamWatch::Pump::StreamEnded:
        return Ending{.why = "The stream ended; reconnecting.", .stop_stream = false};
    }
    return std::nullopt;
}
} // namespace

void MediaSession::announceReady(bool restarted) {
    setState(MediaState::Ready);
    log(restarted ? std::string("Camera off; the glasses stay connected, so a glasses source "
                                "going live starts it in about a second.")
                  : std::format("Connected to the glasses after {}, camera off; a glasses source "
                                "going live starts it in about a second.",
                                elapsed()));
}

void MediaSession::announceFlow(bool video, bool restarted,
                                std::chrono::steady_clock::time_point change_began) {
    setState(MediaState::Streaming);
    const char* medium = video ? "Video" : "Audio";
    if (!restarted) {
        log(std::format("{} is flowing after {}.", medium, elapsed()));
        return;
    }
    const auto taken = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - change_began);
    log(std::format("{} is flowing {:.1f} s after the change.", medium,
                    static_cast<double>(taken.count()) / 1000.0));
}

MediaSession::SessionEnd MediaSession::serve(GlassesSession& media,
                                             const std::string& wifi_adapter) {
    StreamWatch watch;
    Configured configured;
    StreamSetup running;
    bool announced_ready = false;
    // When the latest change of stream began, and whether media had flowed before it: a restart
    // reports its own duration rather than the time since the connection began.
    auto change_began = std::chrono::steady_clock::now();
    bool restarted = false;
    const auto finish = [&](std::string_view why) {
        log(std::string(why));
        return watch.streamed() ? SessionEnd::Dropped : SessionEnd::Failed;
    };
    while (true) {
        const MediaDemand demand = currentDemand();
        if (const char* reason = releaseReason(demand, wifi_adapter); reason != nullptr) {
            stopBeforeLeaving(media, running);
            log(reason);
            return SessionEnd::Released;
        }
        if (const StreamSetup wanted = setupFor(demand, configured); wanted != running) {
            change_began = std::chrono::steady_clock::now();
            // A start from the connected, camera-off state reports its own time too.
            restarted = watch.streamed() || announced_ready;
            const auto applied = applySetup(media, running, wanted, configured);
            if (!applied) {
                return finish("The glasses did not accept the stream change; reconnecting.");
            }
            running = *applied;
            watch.restart();
            announced_ready = false;
            setState(running.streams() ? MediaState::Connecting : MediaState::Ready);
        }
        if (!running.streams() && !announced_ready) {
            announced_ready = true;
            announceReady(restarted);
        }
        const auto pumped = watch.pump(media, running.video.has_value(), running.audio, callbacks_);
        if (pumped == StreamWatch::Pump::FirstMedia) {
            announceFlow(running.video.has_value(), restarted, change_began);
        }
        if (const auto ending = endingFor(pumped, watch.flowing())) {
            if (ending->stop_stream) {
                stopBeforeLeaving(media, running);
            }
            return finish(ending->why);
        }
    }
}
} // namespace mgl