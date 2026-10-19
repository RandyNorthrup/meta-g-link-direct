// SPDX-License-Identifier: AGPL-3.0-only
// Cryptographic primitives the glasses protocol needs. The Windows implementation uses CNG;
// failures of the platform provider throw CryptoError rather than producing partial output.
#ifndef MGL_CRYPTO_HPP
#define MGL_CRYPTO_HPP

#include "mgl/bytes.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace mgl::crypto {
inline constexpr std::size_t DigestSize = 32;
inline constexpr std::size_t PublicKeySize = 64; // x || y, big-endian, no 0x04 prefix
inline constexpr std::size_t ScalarSize = 32;
inline constexpr std::size_t SignatureSize = 64; // r || s, big-endian
inline constexpr std::size_t AesKeySize = 32;
inline constexpr std::size_t AesBlockSize = 16;

using Digest = std::array<std::uint8_t, DigestSize>;
using PublicKey = std::array<std::uint8_t, PublicKeySize>;
using Signature = std::array<std::uint8_t, SignatureSize>;

class CryptoError : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

[[nodiscard]] Digest sha256(std::initializer_list<ByteView> parts);
[[nodiscard]] Digest hmacSha256(ByteView key, std::initializer_list<ByteView> parts);

// A reusable HMAC-SHA-256 key for per-frame tags.
class HmacSha256 {
  public:
    explicit HmacSha256(ByteView key);
    [[nodiscard]] Digest compute(std::initializer_list<ByteView> parts) const;

    HmacSha256(HmacSha256&&) noexcept;
    HmacSha256& operator=(HmacSha256&&) noexcept;
    HmacSha256(const HmacSha256&) = delete;
    HmacSha256& operator=(const HmacSha256&) = delete;
    ~HmacSha256();

  private:
    struct Handle;
    std::unique_ptr<Handle> handle_;
};

// A reusable AES-256-CBC key without padding. Inputs must be whole blocks; the IV is one block.
class AesCbc {
  public:
    explicit AesCbc(ByteView key);
    [[nodiscard]] Bytes encrypt(ByteView iv, ByteView plaintext) const;
    [[nodiscard]] Bytes decrypt(ByteView iv, ByteView ciphertext) const;

    AesCbc(AesCbc&&) noexcept;
    AesCbc& operator=(AesCbc&&) noexcept;
    AesCbc(const AesCbc&) = delete;
    AesCbc& operator=(const AesCbc&) = delete;
    ~AesCbc();

  private:
    struct Handle;
    std::unique_ptr<Handle> handle_;
};

// Fills out from the system CSPRNG. Throws CryptoError if the generator fails.
void randomBytes(std::span<std::uint8_t> out);
// length symbols drawn uniformly from alphabet (1 to 256 distinct symbols) with the system CSPRNG.
// Throws CryptoError on a bad alphabet or a generator failure; never returns weak output.
[[nodiscard]] std::string randomText(std::string_view alphabet, std::size_t length);

[[nodiscard]] bool constantTimeEqual(ByteView left, ByteView right) noexcept;

// An ephemeral P-256 key for one link handshake.
class EcdhKey {
  public:
    [[nodiscard]] static EcdhKey generate();
    [[nodiscard]] const PublicKey& publicKey() const noexcept { return public_key_; }
    // The big-endian x coordinate of the shared point; nullopt if the peer key is not on P-256.
    [[nodiscard]] std::optional<Digest> sharedSecret(ByteView peer_public_key) const;

    EcdhKey(EcdhKey&&) noexcept;
    EcdhKey& operator=(EcdhKey&&) noexcept;
    EcdhKey(const EcdhKey&) = delete;
    EcdhKey& operator=(const EcdhKey&) = delete;
    ~EcdhKey();

  private:
    struct Handle;
    EcdhKey(std::unique_ptr<Handle> handle, const PublicKey& public_key);
    std::unique_ptr<Handle> handle_;
    PublicKey public_key_{};
};

// The persistent P-256 owner key the glasses know this computer by.
class OwnerKey {
  public:
    [[nodiscard]] static OwnerKey generate();
    // Private material: x || y || d, 96 bytes. nullopt when malformed.
    [[nodiscard]] static std::optional<OwnerKey> importPrivate(ByteView material);
    [[nodiscard]] Bytes exportPrivate() const;
    [[nodiscard]] const PublicKey& publicKey() const noexcept { return public_key_; }
    // ECDSA over an already-hashed 32-byte value (NONEwithECDSA), r || s, s not yet normalised.
    [[nodiscard]] Signature signDigest(const Digest& digest) const;

    OwnerKey(OwnerKey&&) noexcept;
    OwnerKey& operator=(OwnerKey&&) noexcept;
    OwnerKey(const OwnerKey&) = delete;
    OwnerKey& operator=(const OwnerKey&) = delete;
    ~OwnerKey();

  private:
    struct Handle;
    OwnerKey(std::unique_ptr<Handle> handle, const PublicKey& public_key);
    std::unique_ptr<Handle> handle_;
    PublicKey public_key_{};
};

// s replaced by n - s when s is above half the P-256 group order, as the glasses require.
[[nodiscard]] Signature lowS(const Signature& signature) noexcept;
} // namespace mgl::crypto
#endif
