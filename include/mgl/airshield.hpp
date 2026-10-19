// SPDX-License-Identifier: AGPL-3.0-only
// AirShield link encryption and link-setup messages. Ported from Visor core/protocol Kdf.kt,
// Cipher.kt, Messages.kt, Binding.kt and core/identity Trust.kt at the revision in
// THIRD_PARTY_NOTICES.md.
#ifndef MGL_AIRSHIELD_HPP
#define MGL_AIRSHIELD_HPP

#include "mgl/bytes.hpp"
#include "mgl/crypto.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace mgl::airshield {
inline constexpr std::size_t ChallengeSize = 16;
inline constexpr std::size_t SeedSize = 32;
inline constexpr std::size_t IvSize = 16;
inline constexpr std::size_t UuidSize = 16;
inline constexpr std::size_t TagSize = 8;
inline constexpr std::size_t MaximumPlaintext = 4095;

inline constexpr std::uint16_t LinkSetupChannel = 0x8001;
inline constexpr std::uint16_t EnableChannel = 0x0001;
inline constexpr std::uint16_t LinkSetupService = 5;
inline constexpr std::uint16_t RequestEncryptionType = 1;
inline constexpr std::uint16_t EnableEncryptionType = 2;
inline constexpr std::uint16_t EndLinkSetupType = 0x1000;

using Uuid = std::array<std::uint8_t, UuidSize>;

struct DirectionKeys {
    std::array<std::uint8_t, crypto::AesKeySize> aes{};
    std::array<std::uint8_t, crypto::DigestSize> hmac{};
    std::array<std::uint8_t, IvSize> iv{};
};

// HKDF-SHA-256 with the AirShield salts: nonce is the receiver's challenge, seed and IV the
// sender's.
[[nodiscard]] DirectionKeys deriveDirectionKeys(const crypto::Digest& shared_secret, ByteView nonce,
                                                ByteView seed, ByteView iv);

// The per-direction frame cipher: AES-256-CBC with a hand-rolled modified PKCS#7 pad and a
// truncated HMAC-SHA-256 tag over a counter, the block count and the ciphertext. The IV chains
// from the last ciphertext block and the sequence counter advances once per frame.
class FrameCrypto {
  public:
    FrameCrypto(const DirectionKeys& keys, std::uint32_t initial_sequence);

    // Seal one plaintext into a DATA frame body: tag || block-count || ciphertext. The plaintext
    // must be at most MaximumPlaintext bytes.
    [[nodiscard]] Bytes seal(ByteView plaintext);

    // Verify and decrypt one DATA frame body. nullopt on a bad tag, a length mismatch or a short
    // frame; on failure no state advances and the link must close.
    [[nodiscard]] std::optional<Bytes> open(ByteView frame, bool strip_pad);

  private:
    crypto::AesCbc aes_;
    crypto::HmacSha256 hmac_;
    std::array<std::uint8_t, IvSize> tx_iv_;
    std::array<std::uint8_t, IvSize> rx_iv_;
    std::uint32_t tx_sequence_;
    std::uint32_t rx_sequence_;

    [[nodiscard]] crypto::Digest tag(std::uint32_t sequence, std::uint8_t block_count,
                                     ByteView ciphertext) const;
};

// The four fields the client sends in RequestEncryption, with Visor's fixed defaults.
[[nodiscard]] Bytes encodeRequestEncryption(const crypto::PublicKey& public_key,
                                            ByteView challenge);
[[nodiscard]] Bytes encodeEnableEncryption(const crypto::PublicKey& public_key, ByteView seed,
                                           ByteView iv, std::uint32_t base);
[[nodiscard]] Bytes encodeEndLinkSetup(const Uuid& scope, const Uuid& lease);

struct PeerHandshake {
    crypto::PublicKey request_public_key{};
    crypto::PublicKey enable_public_key{};
    std::array<std::uint8_t, ChallengeSize> challenge{};
    std::array<std::uint8_t, SeedSize> seed{};
    std::array<std::uint8_t, IvSize> iv{};
    std::uint32_t base = 0;
    bool have_request = false;
    bool have_enable = false;
    [[nodiscard]] bool complete() const noexcept { return have_request && have_enable; }
};

// Reads one RequestEncryption or EnableEncryption payload into peer; returns false if malformed.
[[nodiscard]] bool readHandshakeMessage(std::uint16_t message_type, ByteView payload,
                                        PeerHandshake& peer);

// A version-4 UUID drawn so it passes the firmware's validity test.
[[nodiscard]] Uuid randomUuid();
[[nodiscard]] bool validUuid(const Uuid& uuid) noexcept;

// The 32-byte channel binding the trust signature covers:
// SHA-256(SHA-256(glasses challenge || glasses RequestEncryption key) ||
//         SHA-256(our seed || our public key)).
[[nodiscard]] crypto::Digest channelBinding(ByteView glasses_challenge,
                                            const crypto::PublicKey& glasses_request_key,
                                            ByteView our_seed, const crypto::PublicKey& our_public);

// The svc36 ENABLE_TRUST body: field 1 SHA-256(public key), field 2 the low-s r||s signature over
// the binding, field 3 the provisioning-capabilities varint.
[[nodiscard]] Bytes encodeEnableTrust(const crypto::OwnerKey& owner, const crypto::Digest& binding);
} // namespace mgl::airshield
#endif
