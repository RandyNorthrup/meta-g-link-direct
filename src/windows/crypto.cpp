// SPDX-License-Identifier: AGPL-3.0-only
// Windows CNG implementation of mgl/crypto.hpp.
#include "mgl/crypto.hpp"

#include "mgl/bytes.hpp"

#include <windows.h> // IWYU pragma: keep — SDK umbrella prerequisites for bcrypt.h.

#include <bcrypt.h>
#include <minwindef.h>
#include <winbase.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace mgl::crypto {
namespace {
constexpr ULONG P256Bits = 256;

void check(NTSTATUS status, const char* operation) {
    if (!BCRYPT_SUCCESS(status)) {
        throw CryptoError(std::string(operation) + " failed with NTSTATUS " +
                          std::to_string(static_cast<std::uint32_t>(status)));
    }
}

ULONG length(std::size_t size) {
    if (size > std::numeric_limits<ULONG>::max()) {
        throw CryptoError("input too large for CNG");
    }
    return static_cast<ULONG>(size);
}

// CNG takes non-const input pointers for read-only buffers.
PUCHAR input(ByteView data) noexcept {
    return const_cast<PUCHAR>(data.data()); // NOLINT(cppcoreguidelines-pro-type-const-cast)
}

struct HashHandle {
    BCRYPT_HASH_HANDLE value = nullptr;
    HashHandle() = default;
    HashHandle(const HashHandle&) = delete;
    HashHandle& operator=(const HashHandle&) = delete;
    HashHandle(HashHandle&&) = delete;
    HashHandle& operator=(HashHandle&&) = delete;
    ~HashHandle() {
        if (value != nullptr) {
            BCryptDestroyHash(value);
        }
    }
};

struct KeyHandle {
    BCRYPT_KEY_HANDLE value = nullptr;
    KeyHandle() = default;
    explicit KeyHandle(BCRYPT_KEY_HANDLE handle) noexcept : value(handle) {}
    // Gives up ownership without destroying the key.
    [[nodiscard]] BCRYPT_KEY_HANDLE release() noexcept { return std::exchange(value, nullptr); }
    KeyHandle(const KeyHandle&) = delete;
    KeyHandle& operator=(const KeyHandle&) = delete;
    KeyHandle(KeyHandle&&) = delete;
    KeyHandle& operator=(KeyHandle&&) = delete;
    ~KeyHandle() {
        if (value != nullptr) {
            BCryptDestroyKey(value);
        }
    }
};

struct SecretHandle {
    BCRYPT_SECRET_HANDLE value = nullptr;
    SecretHandle() = default;
    SecretHandle(const SecretHandle&) = delete;
    SecretHandle& operator=(const SecretHandle&) = delete;
    SecretHandle(SecretHandle&&) = delete;
    SecretHandle& operator=(SecretHandle&&) = delete;
    ~SecretHandle() {
        if (value != nullptr) {
            BCryptDestroySecret(value);
        }
    }
};

Digest finishHash(BCRYPT_HASH_HANDLE hash, std::initializer_list<ByteView> parts) {
    for (const auto part : parts) {
        check(BCryptHashData(hash, input(part), length(part.size()), 0), "BCryptHashData");
    }
    Digest digest{};
    check(BCryptFinishHash(hash, digest.data(), length(digest.size()), 0), "BCryptFinishHash");
    return digest;
}

// BCRYPT_ECCKEY_BLOB followed by big-endian coordinates (and the private scalar when present).
Bytes eccBlob(ULONG magic, ByteView material) {
    BCRYPT_ECCKEY_BLOB header{.dwMagic = magic, .cbKey = ScalarSize};
    Bytes blob(sizeof(header) + material.size());
    std::memcpy(blob.data(), &header, sizeof(header));
    std::ranges::copy(material, blob.begin() + sizeof(header));
    return blob;
}

PublicKey exportPublic(BCRYPT_KEY_HANDLE key) {
    std::array<std::uint8_t, sizeof(BCRYPT_ECCKEY_BLOB) + PublicKeySize> blob{};
    ULONG written = 0;
    check(BCryptExportKey(key, nullptr, BCRYPT_ECCPUBLIC_BLOB, blob.data(), length(blob.size()),
                          &written, 0),
          "BCryptExportKey(public)");
    if (written != blob.size()) {
        throw CryptoError("unexpected P-256 public blob size");
    }
    PublicKey out{};
    std::ranges::copy(std::span(blob).subspan(sizeof(BCRYPT_ECCKEY_BLOB)), out.begin());
    return out;
}

BCRYPT_KEY_HANDLE generatePair(BCRYPT_ALG_HANDLE algorithm) {
    BCRYPT_KEY_HANDLE key = nullptr;
    check(BCryptGenerateKeyPair(algorithm, &key, P256Bits, 0), "BCryptGenerateKeyPair");
    KeyHandle owned(key); // destroys the key if finalising throws
    check(BCryptFinalizeKeyPair(key, 0), "BCryptFinalizeKeyPair");
    return owned.release();
}

// P-256 group order n and n / 2, big-endian.
constexpr std::array<std::uint8_t, ScalarSize> GroupOrder{
    0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84, 0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51};
constexpr std::array<std::uint8_t, ScalarSize> HalfOrder{
    0x7F, 0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x00, 0x00, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xDE, 0x73, 0x7D, 0x56, 0xD3, 0x8B, 0xCF, 0x42, 0x79, 0xDC, 0xE5, 0x61, 0x7E, 0x31, 0x92, 0xA8};
} // namespace

Digest sha256(std::initializer_list<ByteView> parts) {
    HashHandle hash;
    check(BCryptCreateHash(BCRYPT_SHA256_ALG_HANDLE, &hash.value, nullptr, 0, nullptr, 0, 0),
          "BCryptCreateHash(SHA256)");
    return finishHash(hash.value, parts);
}

Digest hmacSha256(ByteView key, std::initializer_list<ByteView> parts) {
    return HmacSha256(key).compute(parts);
}

struct HmacSha256::Handle {
    HashHandle hash;
};

HmacSha256::HmacSha256(ByteView key) : handle_(std::make_unique<Handle>()) {
    check(BCryptCreateHash(BCRYPT_HMAC_SHA256_ALG_HANDLE, &handle_->hash.value, nullptr, 0,
                           input(key), length(key.size()), BCRYPT_HASH_REUSABLE_FLAG),
          "BCryptCreateHash(HMAC)");
}

Digest HmacSha256::compute(std::initializer_list<ByteView> parts) const {
    return finishHash(handle_->hash.value, parts);
}

HmacSha256::HmacSha256(HmacSha256&&) noexcept = default;
HmacSha256& HmacSha256::operator=(HmacSha256&&) noexcept = default;
HmacSha256::~HmacSha256() = default;

struct AesCbc::Handle {
    KeyHandle key;
};

AesCbc::AesCbc(ByteView key) : handle_(std::make_unique<Handle>()) {
    if (key.size() != AesKeySize) {
        throw CryptoError("AES-256 needs a 32-byte key");
    }
    check(BCryptGenerateSymmetricKey(BCRYPT_AES_CBC_ALG_HANDLE, &handle_->key.value, nullptr, 0,
                                     input(key), length(key.size()), 0),
          "BCryptGenerateSymmetricKey");
}

namespace {
Bytes runCbc(BCRYPT_KEY_HANDLE key, ByteView iv, ByteView data, bool encrypt) {
    if (iv.size() != AesBlockSize || data.size() % AesBlockSize != 0) {
        throw CryptoError("AES-CBC needs a one-block IV and whole blocks");
    }
    // CNG updates the IV buffer in place; work on a copy so callers own their chaining.
    std::array<std::uint8_t, AesBlockSize> chain{};
    std::ranges::copy(iv, chain.begin());
    Bytes out(data.size());
    ULONG written = 0;
    const auto status =
        encrypt ? BCryptEncrypt(key, input(data), length(data.size()), nullptr, chain.data(),
                                length(chain.size()), out.data(), length(out.size()), &written, 0)
                : BCryptDecrypt(key, input(data), length(data.size()), nullptr, chain.data(),
                                length(chain.size()), out.data(), length(out.size()), &written, 0);
    check(status, encrypt ? "BCryptEncrypt" : "BCryptDecrypt");
    if (written != out.size()) {
        throw CryptoError("AES-CBC produced an unexpected length");
    }
    return out;
}
} // namespace

Bytes AesCbc::encrypt(ByteView iv, ByteView plaintext) const {
    return runCbc(handle_->key.value, iv, plaintext, true);
}

Bytes AesCbc::decrypt(ByteView iv, ByteView ciphertext) const {
    return runCbc(handle_->key.value, iv, ciphertext, false);
}

AesCbc::AesCbc(AesCbc&&) noexcept = default;
AesCbc& AesCbc::operator=(AesCbc&&) noexcept = default;
AesCbc::~AesCbc() = default;

void randomBytes(std::span<std::uint8_t> out) {
    check(BCryptGenRandom(nullptr, out.data(), length(out.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG),
          "BCryptGenRandom");
}

std::string randomText(std::string_view alphabet, std::size_t length) {
    constexpr std::size_t ByteValues = 256;
    if (alphabet.empty() || alphabet.size() > ByteValues) {
        throw CryptoError("randomText needs an alphabet of 1 to 256 symbols");
    }
    // Bytes at or above the largest multiple of the alphabet size are rejected, so every symbol is
    // equally likely; modulo alone would favour the first symbols.
    const std::size_t accepted = ByteValues - (ByteValues % alphabet.size());
    std::string out;
    out.reserve(length);
    std::array<std::uint8_t, 64> pool{};
    while (out.size() < length) {
        randomBytes(pool);
        for (const std::uint8_t byte : pool) {
            if (byte < accepted && out.size() < length) {
                out.push_back(alphabet.at(byte % alphabet.size()));
            }
        }
    }
    return out;
}

bool constantTimeEqual(ByteView left, ByteView right) noexcept {
    if (left.size() != right.size()) {
        return false;
    }
    std::uint8_t difference = 0;
    auto right_byte = right.begin();
    for (const auto left_byte : left) {
        difference |= static_cast<std::uint8_t>(left_byte ^ *right_byte);
        ++right_byte;
    }
    return difference == 0;
}

struct EcdhKey::Handle {
    KeyHandle key;
};

EcdhKey::EcdhKey(std::unique_ptr<Handle> handle, const PublicKey& public_key)
    : handle_(std::move(handle)), public_key_(public_key) {}

EcdhKey EcdhKey::generate() {
    auto handle = std::make_unique<Handle>();
    handle->key.value = generatePair(BCRYPT_ECDH_P256_ALG_HANDLE);
    const auto public_key = exportPublic(handle->key.value);
    return {std::move(handle), public_key};
}

std::optional<Digest> EcdhKey::sharedSecret(ByteView peer_public_key) const {
    if (peer_public_key.size() != PublicKeySize) {
        return std::nullopt;
    }
    auto blob = eccBlob(BCRYPT_ECDH_PUBLIC_P256_MAGIC, peer_public_key);
    KeyHandle peer;
    if (!BCRYPT_SUCCESS(BCryptImportKeyPair(BCRYPT_ECDH_P256_ALG_HANDLE, nullptr,
                                            BCRYPT_ECCPUBLIC_BLOB, &peer.value, blob.data(),
                                            length(blob.size()), 0))) {
        return std::nullopt;
    }
    SecretHandle secret;
    check(BCryptSecretAgreement(handle_->key.value, peer.value, &secret.value, 0),
          "BCryptSecretAgreement");
    Digest raw{};
    ULONG written = 0;
    check(BCryptDeriveKey(secret.value, BCRYPT_KDF_RAW_SECRET, nullptr, raw.data(),
                          length(raw.size()), &written, 0),
          "BCryptDeriveKey(raw)");
    if (written != raw.size()) {
        throw CryptoError("unexpected ECDH secret size");
    }
    // BCRYPT_KDF_RAW_SECRET returns the x coordinate little-endian.
    std::ranges::reverse(raw);
    return raw;
}

EcdhKey::EcdhKey(EcdhKey&&) noexcept = default;
EcdhKey& EcdhKey::operator=(EcdhKey&&) noexcept = default;
EcdhKey::~EcdhKey() = default;

struct OwnerKey::Handle {
    KeyHandle key;
};

OwnerKey::OwnerKey(std::unique_ptr<Handle> handle, const PublicKey& public_key)
    : handle_(std::move(handle)), public_key_(public_key) {}

OwnerKey OwnerKey::generate() {
    auto handle = std::make_unique<Handle>();
    handle->key.value = generatePair(BCRYPT_ECDSA_P256_ALG_HANDLE);
    const auto public_key = exportPublic(handle->key.value);
    return {std::move(handle), public_key};
}

std::optional<OwnerKey> OwnerKey::importPrivate(ByteView material) {
    if (material.size() != PublicKeySize + ScalarSize) {
        return std::nullopt;
    }
    auto blob = eccBlob(BCRYPT_ECDSA_PRIVATE_P256_MAGIC, material);
    auto handle = std::make_unique<Handle>();
    const auto status =
        BCryptImportKeyPair(BCRYPT_ECDSA_P256_ALG_HANDLE, nullptr, BCRYPT_ECCPRIVATE_BLOB,
                            &handle->key.value, blob.data(), length(blob.size()), 0);
    SecureZeroMemory(blob.data(), blob.size());
    if (!BCRYPT_SUCCESS(status)) {
        return std::nullopt;
    }
    PublicKey public_key{};
    std::ranges::copy(material.first(PublicKeySize), public_key.begin());
    if (exportPublic(handle->key.value) != public_key) {
        return std::nullopt;
    }
    return OwnerKey(std::move(handle), public_key);
}

Bytes OwnerKey::exportPrivate() const {
    std::array<std::uint8_t, sizeof(BCRYPT_ECCKEY_BLOB) + PublicKeySize + ScalarSize> blob{};
    ULONG written = 0;
    check(BCryptExportKey(handle_->key.value, nullptr, BCRYPT_ECCPRIVATE_BLOB, blob.data(),
                          length(blob.size()), &written, 0),
          "BCryptExportKey(private)");
    if (written != blob.size()) {
        SecureZeroMemory(blob.data(), blob.size());
        throw CryptoError("unexpected P-256 private blob size");
    }
    const auto material = std::span(blob).subspan(sizeof(BCRYPT_ECCKEY_BLOB));
    Bytes out(material.begin(), material.end());
    SecureZeroMemory(blob.data(), blob.size());
    return out;
}

Signature OwnerKey::signDigest(const Digest& digest) const {
    Signature signature{};
    ULONG written = 0;
    check(BCryptSignHash(handle_->key.value, nullptr, input(digest), length(digest.size()),
                         signature.data(), length(signature.size()), &written, 0),
          "BCryptSignHash");
    if (written != signature.size()) {
        throw CryptoError("unexpected ECDSA signature size");
    }
    return signature;
}

OwnerKey::OwnerKey(OwnerKey&&) noexcept = default;
OwnerKey& OwnerKey::operator=(OwnerKey&&) noexcept = default;
OwnerKey::~OwnerKey() = default;

Signature lowS(const Signature& signature) noexcept {
    const auto s = std::span(signature).subspan<ScalarSize>();
    if (!std::ranges::lexicographical_compare(HalfOrder, s)) {
        return signature;
    }
    Signature out = signature;
    unsigned borrow = 0;
    for (std::size_t index = ScalarSize; index-- > 0;) {
        const auto minuend = static_cast<unsigned>(GroupOrder.at(index));
        const auto subtrahend = static_cast<unsigned>(signature.at(ScalarSize + index)) + borrow;
        borrow = minuend < subtrahend ? 1U : 0U;
        constexpr unsigned ByteRange = 0x100;
        out.at(ScalarSize + index) =
            static_cast<std::uint8_t>(minuend + (borrow * ByteRange) - subtrahend);
    }
    return out;
}
} // namespace mgl::crypto
