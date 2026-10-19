// SPDX-License-Identifier: AGPL-3.0-only
// The AirShield session over a Transport: handshake, trust, EndLinkSetup, then DataX messages.
// Behaviour follows Visor core/session SecureLink.kt and GlassesSession.kt at the revision in
// THIRD_PARTY_NOTICES.md. This layer is synchronous and owns one link attempt.
#ifndef MGL_SESSION_HPP
#define MGL_SESSION_HPP

#include "mgl/airshield.hpp"
#include "mgl/bytes.hpp"
#include "mgl/crypto.hpp"
#include "mgl/datax.hpp"
#include "mgl/transport.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace mgl {
// How far a link attempt reached, reported without leaking key material.
enum class LinkStage : std::uint8_t {
    NotStarted,
    Connected,     // transport open, nothing exchanged
    HandshakeSent, // our RequestEncryption and EnableEncryption written
    KeysDerived,   // the glasses answered and the per-direction ciphers are installed
    TrustAccepted, // the glasses accepted our owner key (svc36 success both ways)
    Ready          // EndLinkSetup exchanged; encrypted DataX traffic can flow
};

enum class LinkError : std::uint8_t {
    None,
    TransportFailed,
    HandshakeIncomplete, // the glasses did not send both handshake messages
    BadPeerKey,          // their public key is not on P-256
    FrameRejected,       // a MAC or framing failure on an encrypted frame
    TrustRejected,       // the glasses refused our owner key (status reported separately)
    EndLinkSetupMissing  // no EndLinkSetup came back
};

struct LinkOutcome {
    LinkStage stage = LinkStage::NotStarted;
    LinkError error = LinkError::None;
    bool trust_tx_ok = false;       // the glasses accepted our credential
    bool trust_rx_ok = false;       // the glasses sent and we acknowledged theirs
    std::uint16_t trust_status = 0; // the last svc36 status code seen, 0 if none
};

// One decoded service message handed up from the session.
struct ServiceMessage {
    std::uint16_t service = 0;
    std::uint16_t type = 0;
    Bytes payload;
};

// A decoded message with its framing kept, for flows (change-owner) that must see status frames.
struct RawMessage {
    std::uint16_t channel = 0;
    std::uint16_t service = 0;
    std::uint16_t type = 0;
    std::uint16_t error = 0;
    bool has_error = false;
    bool closed = false;
    Bytes payload;
};

// Drives one connection. Construct with an open transport; call open() once.
class GlassesSession {
  public:
    // owner may be null to run the handshake only (no trust exchange); used to prove the link
    // before an owner key exists.
    GlassesSession(Transport& transport, const crypto::OwnerKey* owner);

    // Run handshake, trust (if an owner key was given) and EndLinkSetup. The outcome says how far
    // it got and why it stopped.
    [[nodiscard]] LinkOutcome open();

    // Run only the AirShield handshake and key schedule, stopping at KeysDerived. The identity
    // service (svc36) is reachable without a scope, so change-owner runs from here.
    [[nodiscard]] LinkOutcome openHandshake();

    // After the keys exist: open a channel for a service and send one typed message on it.
    [[nodiscard]] bool sendOnService(std::uint16_t service, std::uint16_t type, ByteView payload);

    // After Ready: the next service message, or nullopt on timeout/close. Status frames and
    // refused peer-service pushes are handled internally and not returned.
    [[nodiscard]] std::optional<ServiceMessage> receive(std::chrono::milliseconds timeout);

    // After the keys exist: the next decoded message with its framing, including status frames.
    // Used by the change-owner flow, which must observe svc36 status codes.
    [[nodiscard]] std::optional<RawMessage> receiveRaw(std::chrono::milliseconds timeout);

  private:
    Transport& transport_;
    const crypto::OwnerKey* owner_;
    datax::ChannelRegistry registry_;
    datax::Decoder decoder_;
    std::unique_ptr<airshield::FrameCrypto> tx_;
    std::unique_ptr<airshield::FrameCrypto> rx_;
    std::vector<Bytes> pending_plaintext_;
    std::vector<datax::Message> inbox_;
    airshield::Uuid last_binding_scope_{};
    crypto::Digest binding_{};
    bool have_binding_ = false;

    [[nodiscard]] bool sendPlaintextSetup(std::uint16_t channel, std::uint16_t type,
                                          bool with_service, ByteView payload);
    [[nodiscard]] bool sealAndSend(ByteView plaintext);
    [[nodiscard]] std::optional<datax::Message> nextMessage(std::chrono::milliseconds timeout);
    [[nodiscard]] bool runTrust(LinkOutcome& outcome);
    [[nodiscard]] bool finishLinkSetup();
    [[nodiscard]] bool answerPeerEndLinkSetup(std::chrono::milliseconds timeout);
};
} // namespace mgl
#endif
