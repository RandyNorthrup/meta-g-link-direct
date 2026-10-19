// SPDX-License-Identifier: AGPL-3.0-only
// Port of Visor core/session SecureLink.kt, GlassesSession.kt, DataXConnection.kt and
// core/identity Trust.kt at e7077b4051b1e3e605c667aa2a010fbe41f346a0. See THIRD_PARTY_NOTICES.md.
#include "mgl/session.hpp"

#include "mgl/airshield.hpp"
#include "mgl/bytes.hpp"
#include "mgl/crypto.hpp"
#include "mgl/datax.hpp"
#include "mgl/sdu.hpp"
#include "mgl/transport.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace mgl {
namespace {
using namespace std::chrono_literals;

constexpr std::uint16_t IdentityService = 36;
constexpr std::uint16_t IdentityEnableTrust = 0x1000;
constexpr std::uint16_t IdentityEnableTrustEc = 0x1001;
constexpr int HandshakeReads = 8;
constexpr auto HandshakeTimeout = 10s;
constexpr auto TrustTimeout = 5s;
constexpr auto EndLinkSetupTimeout = 5s;

// One channel per service for this session.
std::unordered_map<std::uint16_t, std::uint16_t>& serviceChannels() {
    thread_local std::unordered_map<std::uint16_t, std::uint16_t> channels;
    return channels;
}
} // namespace

GlassesSession::GlassesSession(Transport& transport, const crypto::OwnerKey* owner)
    : transport_(transport), owner_(owner) {
    serviceChannels().clear();
}

bool GlassesSession::sendPlaintextSetup(std::uint16_t channel, std::uint16_t type,
                                        bool with_service, ByteView payload) {
    datax::Tags tags;
    tags.type = type;
    if (with_service) {
        tags.service = airshield::LinkSetupService;
    }
    const auto message = datax::buildMessage(channel, tags, payload);
    return message && transport_.send(*message);
}

bool GlassesSession::sealAndSend(ByteView plaintext) {
    Bytes frame;
    frame.reserve(1 + plaintext.size());
    frame.push_back(AirshieldData); // stream 0
    const auto sealed = tx_->seal(plaintext);
    append(frame, sealed);
    return transport_.send(frame);
}

std::optional<datax::Message> GlassesSession::nextMessage(std::chrono::milliseconds timeout) {
    if (!inbox_.empty()) {
        auto message = std::move(inbox_.front());
        inbox_.erase(inbox_.begin());
        return message;
    }
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        auto sdu = transport_.receive(LinkPhase::Encrypted, remaining);
        if (sdu.status != TransportStatus::Ok) {
            return std::nullopt;
        }
        const ByteView frame(sdu.frame);
        if ((frame.front() & AirshieldTypeMask) != AirshieldData) {
            // A COMMAND frame (e.g. stream-0 LOST): cleartext control, not DataX.
            continue;
        }
        auto plaintext = rx_->open(frame.subspan(1), /*strip_pad=*/false);
        if (!plaintext) {
            return std::nullopt;
        }
        std::vector<datax::Message> messages;
        if (!decoder_.decode(*plaintext, messages)) {
            return std::nullopt;
        }
        for (auto& message : messages) {
            inbox_.push_back(std::move(message));
        }
        if (!inbox_.empty()) {
            auto message = std::move(inbox_.front());
            inbox_.erase(inbox_.begin());
            return message;
        }
    }
    return std::nullopt;
}

LinkOutcome GlassesSession::open() {
    auto outcome = openHandshake();
    if (outcome.stage != LinkStage::KeysDerived) {
        return outcome;
    }
    if (owner_ != nullptr) {
        if (!runTrust(outcome)) {
            return outcome;
        }
    }
    if (!finishLinkSetup()) {
        outcome.error = LinkError::TransportFailed;
        return outcome;
    }
    if (!answerPeerEndLinkSetup(EndLinkSetupTimeout)) {
        outcome.error = LinkError::EndLinkSetupMissing;
        return outcome;
    }
    outcome.stage = LinkStage::Ready;
    return outcome;
}

LinkOutcome GlassesSession::openHandshake() {
    LinkOutcome outcome;
    outcome.stage = LinkStage::Connected;

    auto ephemeral = crypto::EcdhKey::generate();
    std::array<std::uint8_t, airshield::ChallengeSize> my_challenge{};
    std::array<std::uint8_t, airshield::SeedSize> my_seed{};
    std::array<std::uint8_t, airshield::IvSize> my_iv{};
    std::array<std::uint8_t, sizeof(std::uint32_t)> seq_bytes{};
    crypto::randomBytes(my_challenge);
    crypto::randomBytes(my_seed);
    crypto::randomBytes(my_iv);
    crypto::randomBytes(seq_bytes);
    std::uint32_t my_sequence = 0;
    for (std::size_t index = 0; index < seq_bytes.size(); ++index) {
        my_sequence |= static_cast<std::uint32_t>(seq_bytes[index]) << (index * 8U);
    }

    const auto request = airshield::encodeRequestEncryption(ephemeral.publicKey(), my_challenge);
    const auto enable =
        airshield::encodeEnableEncryption(ephemeral.publicKey(), my_seed, my_iv, my_sequence);
    if (!sendPlaintextSetup(airshield::LinkSetupChannel, airshield::RequestEncryptionType, true,
                            request) ||
        !sendPlaintextSetup(airshield::EnableChannel, airshield::EnableEncryptionType, false,
                            enable)) {
        outcome.error = LinkError::TransportFailed;
        return outcome;
    }
    outcome.stage = LinkStage::HandshakeSent;

    airshield::PeerHandshake peer;
    const auto deadline = std::chrono::steady_clock::now() + HandshakeTimeout;
    for (int attempt = 0; attempt < HandshakeReads && !peer.complete(); ++attempt) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        if (remaining <= 0ms) {
            break;
        }
        auto sdu = transport_.receive(LinkPhase::Plaintext, remaining);
        if (sdu.status != TransportStatus::Ok) {
            outcome.error = LinkError::TransportFailed;
            return outcome;
        }
        const ByteView frame(sdu.frame);
        if ((frame.front() & AirshieldTypeMask) == AirshieldData) {
            // An early encrypted frame; it belongs after keys exist. Drop it: the glasses resend
            // nothing critical before EndLinkSetup, and keeping it would need the ciphers we are
            // still deriving.
            continue;
        }
        const auto header = datax::parseHeader(frame);
        if (!header) {
            outcome.error = LinkError::HandshakeIncomplete;
            return outcome;
        }
        const auto payload = frame.subspan(header->header_size, header->dataSize());
        if (!airshield::readHandshakeMessage(header->type, payload, peer)) {
            // A message type we do not model during setup; ignore it.
            continue;
        }
    }
    if (!peer.complete()) {
        outcome.error = LinkError::HandshakeIncomplete;
        return outcome;
    }

    const auto secret = ephemeral.sharedSecret(peer.enable_public_key);
    if (!secret) {
        outcome.error = LinkError::BadPeerKey;
        return outcome;
    }

    const auto rx_keys = airshield::deriveDirectionKeys(*secret, my_challenge, peer.seed, peer.iv);
    const auto tx_keys = airshield::deriveDirectionKeys(*secret, peer.challenge, my_seed, my_iv);
    rx_ = std::make_unique<airshield::FrameCrypto>(rx_keys, peer.base);
    tx_ = std::make_unique<airshield::FrameCrypto>(tx_keys, my_sequence);
    binding_ = airshield::channelBinding(peer.challenge, peer.request_public_key, my_seed,
                                         ephemeral.publicKey());
    have_binding_ = true;
    outcome.stage = LinkStage::KeysDerived;
    return outcome;
}

bool GlassesSession::runTrust(LinkOutcome& outcome) {
    const auto channel = registry_.openLocal(IdentityService);
    if (!channel) {
        outcome.error = LinkError::TransportFailed;
        return false;
    }
    const auto body = airshield::encodeEnableTrust(*owner_, binding_);
    datax::Tags tags;
    tags.service = IdentityService;
    tags.type = IdentityEnableTrust;
    const auto trust_message = datax::buildMessage(*channel, tags, body);
    if (!trust_message || !sealAndSend(*trust_message)) {
        outcome.error = LinkError::TransportFailed;
        return false;
    }
    const auto our_status_channel = datax::mirror(*channel);

    const auto deadline = std::chrono::steady_clock::now() + TrustTimeout;
    std::vector<datax::Message> deferred;
    while (!(outcome.trust_tx_ok && outcome.trust_rx_ok)) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        if (remaining <= 0ms) {
            break;
        }
        auto message = nextMessage(remaining);
        if (!message) {
            break;
        }
        const auto& info = message->header;
        if (info.has_error && info.channel == our_status_channel) {
            outcome.trust_status = info.error;
            if (info.error == datax::StatusSuccess) {
                outcome.trust_tx_ok = true;
            }
            continue;
        }
        const bool peer_opened = (info.channel & datax::OriginatorBit) != 0;
        if (peer_opened && info.service == IdentityService &&
            (info.type == IdentityEnableTrust || info.type == IdentityEnableTrustEc)) {
            datax::Tags status;
            status.error = datax::StatusSuccess;
            const auto ack = datax::buildMessage(datax::mirror(info.channel), status, {});
            if (ack && sealAndSend(*ack)) {
                outcome.trust_rx_ok = true;
            }
            continue;
        }
        deferred.push_back(std::move(*message));
    }
    // Anything not consumed belongs to the caller, in order.
    for (auto it = deferred.rbegin(); it != deferred.rend(); ++it) {
        inbox_.insert(inbox_.begin(), std::move(*it));
    }
    if (outcome.trust_tx_ok && outcome.trust_rx_ok) {
        outcome.stage = LinkStage::TrustAccepted;
    } else {
        outcome.error = LinkError::TrustRejected;
    }
    // Trust failure is not fatal to the link: the glasses still answer EndLinkSetup, so go on.
    return true;
}

bool GlassesSession::finishLinkSetup() {
    const auto scope = airshield::randomUuid();
    const auto lease = airshield::randomUuid();
    const auto body = airshield::encodeEndLinkSetup(scope, lease);
    datax::Tags tags;
    tags.type = airshield::EndLinkSetupType;
    const auto message = datax::buildMessage(airshield::LinkSetupChannel, tags, body);
    return message && sealAndSend(*message);
}

bool GlassesSession::answerPeerEndLinkSetup(std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        auto message = nextMessage(remaining);
        if (!message) {
            return false;
        }
        const auto& info = message->header;
        if (info.channel == airshield::LinkSetupChannel &&
            info.type == airshield::EndLinkSetupType) {
            datax::Tags status;
            status.error = datax::StatusSuccess;
            const auto ack = datax::buildMessage(datax::mirror(info.channel), status, {});
            return ack && sealAndSend(*ack);
        }
        // Record any service a peer channel names, then drop; nothing else is needed here.
        if (info.service != 0) {
            registry_.bindInbound(info.channel, info.service);
        }
    }
    return false;
}

bool GlassesSession::sendOnService(std::uint16_t service, std::uint16_t type, ByteView payload) {
    if (!tx_) {
        return false;
    }
    auto& channels = serviceChannels();
    const auto known = channels.find(service);
    if (known != channels.end()) {
        datax::Tags tags;
        tags.type = type;
        const auto message = datax::buildMessage(known->second, tags, payload);
        return message && sealAndSend(*message);
    }
    const auto channel = registry_.openLocal(service);
    if (!channel) {
        return false;
    }
    channels[service] = *channel;
    datax::Tags tags;
    tags.service = service;
    tags.type = type;
    const auto message = datax::buildMessage(*channel, tags, payload);
    return message && sealAndSend(*message);
}

std::optional<RawMessage> GlassesSession::receiveRaw(std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        auto message = nextMessage(remaining);
        if (!message) {
            return std::nullopt;
        }
        auto& info = message->header;
        if (info.service != 0) {
            registry_.bindInbound(info.channel, info.service);
        }
        const auto service = registry_.serviceForInbound(info.channel);
        const bool peer_opened = (info.channel & datax::OriginatorBit) != 0;
        if ((service == 30 || service == 79) && peer_opened && !info.has_error) {
            // Keep the link healthy without surfacing these device-peer/constellation pushes.
            datax::Tags status;
            status.error = datax::StatusServiceNotFound;
            const auto refusal = datax::buildMessage(datax::mirror(info.channel), status, {});
            if (refusal) {
                (void)sealAndSend(*refusal);
            }
            continue;
        }
        return RawMessage{.channel = info.channel,
                          .service = service,
                          .type = info.type,
                          .error = info.error,
                          .has_error = info.has_error,
                          .closed = info.closed,
                          .payload = std::move(message->payload)};
    }
    return std::nullopt;
}

std::optional<ServiceMessage> GlassesSession::receive(std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        auto message = nextMessage(remaining);
        if (!message) {
            return std::nullopt;
        }
        auto& info = message->header;
        if (info.service != 0) {
            registry_.bindInbound(info.channel, info.service);
        }
        const auto service = registry_.serviceForInbound(info.channel);
        const bool peer_opened = (info.channel & datax::OriginatorBit) != 0;
        if ((service == 30 || service == 79) && peer_opened && !info.has_error) {
            datax::Tags status;
            status.error = datax::StatusServiceNotFound;
            const auto refusal = datax::buildMessage(datax::mirror(info.channel), status, {});
            if (refusal) {
                (void)sealAndSend(*refusal);
            }
            continue;
        }
        if (info.has_error) {
            continue;
        }
        if (info.closed && message->payload.empty()) {
            continue;
        }
        return ServiceMessage{
            .service = service, .type = info.type, .payload = std::move(message->payload)};
    }
    return std::nullopt;
}
} // namespace mgl
