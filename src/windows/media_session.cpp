// SPDX-License-Identifier: AGPL-3.0-only
// Worker-thread media pipeline. The step order mirrors the proven --stream probe path and Visor's
// LiveStream bring-up at the revision in THIRD_PARTY_NOTICES.md.
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
#include "mgl/wifi_group.hpp"

#include <thread>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace mgl {
namespace {
using namespace std::chrono_literals;

ByteView viewOf(const Bytes& bytes) { return {bytes.data(), bytes.size()}; }

// Sends the Wi-Fi join and returns the glasses' TCP endpoint, retrying the INTERNAL_ERROR window.
std::optional<stream::JoinResult> joinWifi(GlassesSession& session, const WifiGroup& group,
                                           const std::atomic<bool>& stop) {
    for (int attempt = 0; attempt < 8 && !stop.load(); ++attempt) {
        if (attempt > 0) {
            std::this_thread::sleep_for(4s);
        }
        const auto uuid = airshield::randomUuid();
        const auto join = stream::buildJoinAccessPoint(group.ssid, group.passphrase, uuid,
                                                       stream::Bandwidth20MHz);
        if (!session.sendOnService(stream::ServiceAtc, stream::MtJoinAccessPoint, viewOf(join))) {
            return std::nullopt;
        }
        const auto deadline = std::chrono::steady_clock::now() + 12s;
        while (std::chrono::steady_clock::now() < deadline && !stop.load()) {
            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::steady_clock::now());
            auto message = session.receiveRaw(remaining);
            if (!message) {
                break;
            }
            if (message->service == stream::ServiceAtc &&
                message->type == stream::MtJoinAccessPointResult) {
                if (const auto parsed = stream::parseJoinResult(viewOf(message->payload))) {
                    if (parsed->ok()) {
                        return parsed;
                    }
                    break;
                }
            }
        }
    }
    return std::nullopt;
}
} // namespace

MediaSession::MediaSession(MediaCallbacks callbacks, stream::VideoConfig config)
    : callbacks_(std::move(callbacks)), config_(config) {}

MediaSession::~MediaSession() { stop(); }

void MediaSession::setState(MediaState state) {
    state_.store(state);
    if (callbacks_.on_state) {
        callbacks_.on_state(state);
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

void MediaSession::stop() {
    stop_.store(true);
    if (worker_.joinable()) {
        worker_.join();
    }
    if (state_.load() != MediaState::Failed) {
        setState(MediaState::Stopped);
    }
}

void MediaSession::interruptibleSleep(std::chrono::milliseconds total) {
    const auto until = std::chrono::steady_clock::now() + total;
    while (std::chrono::steady_clock::now() < until && !stop_.load()) {
        std::this_thread::sleep_for(200ms);
    }
}

void MediaSession::run() {
    // Keep the camera up across drops: each attempt runs until the stream ends, then reconnects
    // with a short backoff, until the source is stopped or a fatal error occurs.
    const std::array<std::chrono::milliseconds, 4> backoff{2s, 4s, 8s, 15s};
    for (int attempt = 0; !stop_.load(); ++attempt) {
        if (runSession()) {
            return; // fatal; do not retry
        }
        if (stop_.load()) {
            return;
        }
        setState(MediaState::Reconnecting);
        log("Reconnecting to the glasses.");
        interruptibleSleep(backoff.at(static_cast<std::size_t>(
            std::min<int>(attempt, static_cast<int>(backoff.size()) - 1))));
    }
}

bool MediaSession::runSession() {
    setState(MediaState::Connecting);
    const auto identity = identity::loadOwnerIdentity();
    if (!identity) {
        log("No stored glasses identity; run setup first.");
        setState(MediaState::Failed);
        return true;
    }
    const auto owner = crypto::OwnerKey::importPrivate(
        ByteView(identity->owner_private.data(), identity->owner_private.size()));
    if (!owner) {
        log("Stored glasses identity is unreadable.");
        setState(MediaState::Failed);
        return true;
    }

    WifiGroup group;
    const auto group_owner = WifiGroupOwner::start(group);
    if (!group_owner) {
        log("Could not start the Wi-Fi group for the glasses.");
        return false;
    }
    log("Wi-Fi group up; connecting to the glasses over Bluetooth.");

    stream::JoinResult join;
    {
        RfcommOpen bt;
        for (int attempt = 0; attempt < 6 && !bt.transport && !stop_.load(); ++attempt) {
            bt = openBondedMetaRfcomm();
            if (!bt.transport) {
                std::this_thread::sleep_for(3s);
            }
        }
        if (!bt.transport) {
            log("The glasses did not accept a Bluetooth connection.");
            return false;
        }
        GlassesSession bt_session(*bt.transport, &*owner);
        if (bt_session.open().stage != LinkStage::Ready) {
            log("The authenticated Bluetooth session did not reach ready.");
            return false;
        }
        log("Authenticated; asking the glasses to join Wi-Fi.");
        const auto result = joinWifi(bt_session, group, stop_);
        if (!result) {
            log("The glasses did not join the Wi-Fi group.");
            return false;
        }
        join = *result;
    }
    if (stop_.load()) {
        return false;
    }

    log("Glasses joined Wi-Fi at port " + std::to_string(join.port) + "; opening media TCP.");
    // The glasses' TCP server is not ready the instant the join succeeds, so settle and retry the
    // whole TCP AirShield session a few times rather than racing it once.
    // tcp_transport outlives media so the session's transport reference stays valid; declare it
    // first for correct destruction order.
    std::unique_ptr<Transport> tcp_transport;
    std::unique_ptr<GlassesSession> media;
    LinkOutcome media_link;
    for (int attempt = 0; attempt < 5 && !stop_.load(); ++attempt) {
        std::this_thread::sleep_for(attempt == 0 ? 1500ms : 2500ms);
        auto tcp = openTcp(join.address, static_cast<std::uint16_t>(join.port));
        if (!tcp.transport) {
            continue;
        }
        auto session = std::make_unique<GlassesSession>(*tcp.transport, &*owner);
        media_link = session->open();
        if (media_link.stage == LinkStage::Ready) {
            tcp_transport = std::move(tcp.transport);
            media = std::move(session);
            break;
        }
        // session drops first, then tcp.transport, closing the failed connection.
    }
    if (!media) {
        log("The media session over Wi-Fi did not reach ready (stage " +
            std::to_string(static_cast<int>(media_link.stage)) + ", error " +
            std::to_string(static_cast<int>(media_link.error)) + ").");
        return false;
    }
    log("Media session ready; starting the camera.");

    // Each step must be acknowledged before the next, or the glasses drop the stream setup.
    const auto await_media = [&](std::uint16_t service, std::uint16_t type,
                                 std::chrono::seconds timeout) {
        const auto until = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < until && !stop_.load()) {
            const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
                until - std::chrono::steady_clock::now());
            auto message = media->receiveRaw(remaining);
            if (!message) {
                return;
            }
            if (message->service == service && message->type == type) {
                return;
            }
        }
    };

    std::uint32_t nonce = 1;
    (void)media->sendOnService(stream::ServiceStatus, stream::MtInit,
                               viewOf(stream::buildSubscribe()));
    (void)media->sendOnService(stream::ServiceStatus, stream::MtStartApp,
                               viewOf(stream::buildStartApp(stream::AppLivestream)));
    await_media(stream::ServiceStatus, stream::MtStartAppResponse, 7s);
    (void)media->sendOnService(stream::ServiceMedia, stream::MtSessionSettings,
                               viewOf(stream::buildSessionSettings(3, /*bypass_don=*/true)));
    (void)media->sendOnService(stream::ServiceMedia, stream::MtConfigureVideo,
                               viewOf(stream::buildConfigureVideo(nonce++, config_)));
    await_media(stream::ServiceMedia, stream::MtConfigureVideoResponse, 5s);
    (void)media->sendOnService(stream::ServiceMedia, stream::MtConfigureAudio,
                               viewOf(stream::buildConfigureAudio(nonce++, /*enabled=*/false)));
    await_media(stream::ServiceMedia, stream::MtConfigureAudioResponse, 5s);
    (void)media->sendOnService(stream::ServiceMedia, stream::MtControlStream,
                               viewOf(stream::buildControlStream(nonce++, stream::ActionStart)));
    await_media(stream::ServiceMedia, stream::MtControlStreamResponse, 5s);
    setState(MediaState::Streaming);

    Bytes frame;
    auto last_frame_at = std::chrono::steady_clock::now();
    constexpr auto StallTimeout = 8s;
    while (!stop_.load()) {
        auto message = media->receiveRaw(2s);
        if (std::chrono::steady_clock::now() - last_frame_at > StallTimeout) {
            log("Video stalled; the link may have dropped.");
            break; // reconnect
        }
        if (!message) {
            continue; // a quiet gap within the stall window; keep reading
        }
        if (message->service != stream::ServiceMedia) {
            continue;
        }
        const ByteView payload(message->payload.data(), message->payload.size());
        if (message->type == stream::MtVideoChunk) {
            append(frame, payload);
        } else if (message->type == stream::MtVideoChunkEnd) {
            append(frame, payload);
            if (frame.size() > 8 && callbacks_.on_frame) {
                const ByteView annex(frame.data() + 8, frame.size() - 8);
                std::uint64_t pts = 0;
                for (int i = 0; i < 8; ++i) {
                    pts |= static_cast<std::uint64_t>(frame[static_cast<std::size_t>(i)])
                           << (i * 8);
                }
                callbacks_.on_frame(pts, annex, hevc::hasKeyframe(annex));
                last_frame_at = std::chrono::steady_clock::now();
            }
            frame.clear();
        } else if (message->type == stream::MtCodecConfig) {
            if (const auto codec = stream::parseCodecConfig(payload)) {
                if (!codec->data.empty() && callbacks_.on_parameters) {
                    callbacks_.on_parameters(viewOf(codec->data));
                }
            }
        }
    }

    (void)media->sendOnService(stream::ServiceMedia, stream::MtControlStream,
                               viewOf(stream::buildControlStream(nonce++, stream::ActionStop)));
    // Leave the access point before the group goes, as the Meta app does, so the glasses drop off
    // cleanly and the next session's Wi-Fi join and TCP handshake are not left wedged.
    const Bytes leave;
    (void)media->sendOnService(stream::ServiceAtc, stream::MtLeaveAccessPoint, viewOf(leave));
    std::this_thread::sleep_for(300ms);
    log(stop_.load() ? "Camera stopped." : "Stream ended; will reconnect.");
    return false;
}
} // namespace mgl
