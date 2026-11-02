// SPDX-License-Identifier: AGPL-3.0-only
// mbedTLS implementation of mgl/crypto.hpp for Linux and macOS. The pinned static build is hidden
// inside this project's binaries, so OBS's own copy, if it has one, is not disturbed.
#include "mgl/bytes.hpp"
#include "mgl/crypto.hpp"
#include "mgl/platform.hpp"

#include <mbedtls/aes.h>
#include <mbedtls/bignum.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/ecp.h>
#include <mbedtls/md.h>
#include <mbedtls/sha256.h>

#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>

#ifdef __APPLE__
#include <sys/random.h> // getentropy
#endif

namespace mgl::crypto {
namespace {
constexpr unsigned AesKeyBits = AesKeySize * 8U;
constexpr std::size_t UncompressedPointSize = 1 + PublicKeySize;
constexpr std::uint8_t UncompressedMarker = 0x04;
constexpr std::size_t GetEntropyLimit = 256; // the most getentropy gives in one call

void check(int status, const char* operation) {
    if (status != 0) {
        throw CryptoError(std::string(operation) + " failed with mbedTLS error " +
                          std::to_string(status));
    }
}

// The random source every mbedTLS operation that needs one uses, in the shape it expects.
int systemRandom(void* /*context*/, unsigned char* out, std::size_t size) {
    std::size_t filled = 0;
    while (filled < size) {
        const std::size_t step = std::min(size - filled, GetEntropyLimit);
        if (getentropy(out + filled, step) != 0) {
            return MBEDTLS_ERR_ECP_RANDOM_FAILED;
        }
        filled += step;
    }
    return 0;
}

// RAII over the mbedTLS context pairs of init and free.
class Mpi {
  public:
    Mpi() { mbedtls_mpi_init(&value_); }
    Mpi(const Mpi&) = delete;
    Mpi& operator=(const Mpi&) = delete;
    Mpi(Mpi&&) = delete;
    Mpi& operator=(Mpi&&) = delete;
    ~Mpi() { mbedtls_mpi_free(&value_); }
    [[nodiscard]] mbedtls_mpi* get() noexcept { return &value_; }
    [[nodiscard]] const mbedtls_mpi* get() const noexcept { return &value_; }

  private:
    mbedtls_mpi value_{};
};

class Point {
  public:
    Point() { mbedtls_ecp_point_init(&value_); }
    Point(const Point&) = delete;
    Point& operator=(const Point&) = delete;
    Point(Point&&) = delete;
    Point& operator=(Point&&) = delete;
    ~Point() { mbedtls_ecp_point_free(&value_); }
    [[nodiscard]] mbedtls_ecp_point* get() noexcept { return &value_; }
    [[nodiscard]] const mbedtls_ecp_point* get() const noexcept { return &value_; }

  private:
    mbedtls_ecp_point value_{};
};

class P256Group {
  public:
    P256Group() {
        mbedtls_ecp_group_init(&value_);
        const int loaded = mbedtls_ecp_group_load(&value_, MBEDTLS_ECP_DP_SECP256R1);
        if (loaded != 0) {
            mbedtls_ecp_group_free(&value_);
            check(loaded, "mbedtls_ecp_group_load");
        }
    }
    P256Group(const P256Group&) = delete;
    P256Group& operator=(const P256Group&) = delete;
    P256Group(P256Group&&) = delete;
    P256Group& operator=(P256Group&&) = delete;
    ~P256Group() { mbedtls_ecp_group_free(&value_); }
    [[nodiscard]] mbedtls_ecp_group* get() noexcept { return &value_; }

  private:
    mbedtls_ecp_group value_{};
};

PublicKey exportPoint(const mbedtls_ecp_group* group, const mbedtls_ecp_point* point) {
    std::array<unsigned char, UncompressedPointSize> raw{};
    std::size_t written = 0;
    check(mbedtls_ecp_point_write_binary(group, point, MBEDTLS_ECP_PF_UNCOMPRESSED, &written,
                                         raw.data(), raw.size()),
          "mbedtls_ecp_point_write_binary");
    if (written != raw.size()) {
        throw CryptoError("unexpected P-256 point size");
    }
    PublicKey out{};
    std::ranges::copy(std::span(raw).subspan(1), out.begin());
    return out;
}

// Reads x || y into point and checks it lies on the curve. False when it does not.
bool importPoint(mbedtls_ecp_group* group, ByteView x_and_y, mbedtls_ecp_point* point) {
    std::array<unsigned char, UncompressedPointSize> raw{};
    raw[0] = UncompressedMarker;
    std::ranges::copy(x_and_y, raw.begin() + 1);
    return mbedtls_ecp_point_read_binary(group, point, raw.data(), raw.size()) == 0 &&
           mbedtls_ecp_check_pubkey(group, point) == 0;
}

struct ShaContext {
    ShaContext() { mbedtls_sha256_init(&value); }
    ShaContext(const ShaContext&) = delete;
    ShaContext& operator=(const ShaContext&) = delete;
    ShaContext(ShaContext&&) = delete;
    ShaContext& operator=(ShaContext&&) = delete;
    ~ShaContext() { mbedtls_sha256_free(&value); }
    mbedtls_sha256_context value{};
};
} // namespace

Digest sha256(std::initializer_list<ByteView> parts) {
    ShaContext context;
    constexpr int NotSha224 = 0;
    check(mbedtls_sha256_starts(&context.value, NotSha224), "mbedtls_sha256_starts");
    for (const auto part : parts) {
        check(mbedtls_sha256_update(&context.value, part.data(), part.size()),
              "mbedtls_sha256_update");
    }
    Digest digest{};
    check(mbedtls_sha256_finish(&context.value, digest.data()), "mbedtls_sha256_finish");
    return digest;
}

struct HmacSha256::Handle {
    Handle() { mbedtls_md_init(&context); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    Handle(Handle&&) = delete;
    Handle& operator=(Handle&&) = delete;
    ~Handle() { mbedtls_md_free(&context); }
    mbedtls_md_context_t context{};
};

HmacSha256::HmacSha256(ByteView key) : handle_(std::make_unique<Handle>()) {
    constexpr int UseHmac = 1;
    check(
        mbedtls_md_setup(&handle_->context, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), UseHmac),
        "mbedtls_md_setup");
    check(mbedtls_md_hmac_starts(&handle_->context, key.data(), key.size()),
          "mbedtls_md_hmac_starts");
}

Digest HmacSha256::compute(std::initializer_list<ByteView> parts) const {
    check(mbedtls_md_hmac_reset(&handle_->context), "mbedtls_md_hmac_reset");
    for (const auto part : parts) {
        check(mbedtls_md_hmac_update(&handle_->context, part.data(), part.size()),
              "mbedtls_md_hmac_update");
    }
    Digest digest{};
    check(mbedtls_md_hmac_finish(&handle_->context, digest.data()), "mbedtls_md_hmac_finish");
    return digest;
}

HmacSha256::HmacSha256(HmacSha256&&) noexcept = default;
HmacSha256& HmacSha256::operator=(HmacSha256&&) noexcept = default;
HmacSha256::~HmacSha256() = default;

struct AesCbc::Handle {
    Handle() {
        mbedtls_aes_init(&encrypting);
        mbedtls_aes_init(&decrypting);
    }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    Handle(Handle&&) = delete;
    Handle& operator=(Handle&&) = delete;
    ~Handle() {
        mbedtls_aes_free(&encrypting);
        mbedtls_aes_free(&decrypting);
    }
    mbedtls_aes_context encrypting{};
    mbedtls_aes_context decrypting{};
};

AesCbc::AesCbc(ByteView key) : handle_(std::make_unique<Handle>()) {
    if (key.size() != AesKeySize) {
        throw CryptoError("AES-256 needs a 32-byte key");
    }
    check(mbedtls_aes_setkey_enc(&handle_->encrypting, key.data(), AesKeyBits),
          "mbedtls_aes_setkey_enc");
    check(mbedtls_aes_setkey_dec(&handle_->decrypting, key.data(), AesKeyBits),
          "mbedtls_aes_setkey_dec");
}

namespace {
Bytes runCbc(mbedtls_aes_context& key, int mode, ByteView iv, ByteView data) {
    if (iv.size() != AesBlockSize || data.size() % AesBlockSize != 0) {
        throw CryptoError("AES-CBC needs a one-block IV and whole blocks");
    }
    // mbedTLS updates the IV buffer in place; work on a copy so callers own their chaining.
    std::array<unsigned char, AesBlockSize> chain{};
    std::ranges::copy(iv, chain.begin());
    Bytes out(data.size());
    check(mbedtls_aes_crypt_cbc(&key, mode, data.size(), chain.data(), data.data(), out.data()),
          "mbedtls_aes_crypt_cbc");
    return out;
}
} // namespace

Bytes AesCbc::encrypt(ByteView iv, ByteView plaintext) const {
    return runCbc(handle_->encrypting, MBEDTLS_AES_ENCRYPT, iv, plaintext);
}

Bytes AesCbc::decrypt(ByteView iv, ByteView ciphertext) const {
    return runCbc(handle_->decrypting, MBEDTLS_AES_DECRYPT, iv, ciphertext);
}

AesCbc::AesCbc(AesCbc&&) noexcept = default;
AesCbc& AesCbc::operator=(AesCbc&&) noexcept = default;
AesCbc::~AesCbc() = default;

void randomBytes(std::span<std::uint8_t> out) {
    if (systemRandom(nullptr, out.data(), out.size()) != 0) {
        throw CryptoError("the system random source failed");
    }
}

// A P-256 key pair; the handle of both key classes.
struct KeyPair {
    P256Group group;
    Mpi secret;
    Point public_point;
};

namespace {
void generateInto(KeyPair& pair) {
    check(mbedtls_ecp_gen_keypair(pair.group.get(), pair.secret.get(), pair.public_point.get(),
                                  &systemRandom, nullptr),
          "mbedtls_ecp_gen_keypair");
}
} // namespace

struct EcdhKey::Handle {
    KeyPair pair;
};

EcdhKey::EcdhKey(std::unique_ptr<Handle> handle, const PublicKey& public_key)
    : handle_(std::move(handle)), public_key_(public_key) {}

EcdhKey EcdhKey::generate() {
    auto handle = std::make_unique<Handle>();
    generateInto(handle->pair);
    const auto public_key = exportPoint(handle->pair.group.get(), handle->pair.public_point.get());
    return {std::move(handle), public_key};
}

std::optional<Digest> EcdhKey::sharedSecret(ByteView peer_public_key) const {
    if (peer_public_key.size() != PublicKeySize) {
        return std::nullopt;
    }
    Point peer;
    if (!importPoint(handle_->pair.group.get(), peer_public_key, peer.get())) {
        return std::nullopt;
    }
    Mpi shared;
    check(mbedtls_ecdh_compute_shared(handle_->pair.group.get(), shared.get(), peer.get(),
                                      handle_->pair.secret.get(), &systemRandom, nullptr),
          "mbedtls_ecdh_compute_shared");
    Digest raw{};
    check(mbedtls_mpi_write_binary(shared.get(), raw.data(), raw.size()),
          "mbedtls_mpi_write_binary");
    return raw;
}

EcdhKey::EcdhKey(EcdhKey&&) noexcept = default;
EcdhKey& EcdhKey::operator=(EcdhKey&&) noexcept = default;
EcdhKey::~EcdhKey() = default;

struct OwnerKey::Handle {
    KeyPair pair;
};

OwnerKey::OwnerKey(std::unique_ptr<Handle> handle, const PublicKey& public_key)
    : handle_(std::move(handle)), public_key_(public_key) {}

OwnerKey OwnerKey::generate() {
    auto handle = std::make_unique<Handle>();
    generateInto(handle->pair);
    const auto public_key = exportPoint(handle->pair.group.get(), handle->pair.public_point.get());
    return {std::move(handle), public_key};
}

std::optional<OwnerKey> OwnerKey::importPrivate(ByteView material) {
    if (material.size() != PublicKeySize + ScalarSize) {
        return std::nullopt;
    }
    auto handle = std::make_unique<Handle>();
    auto& pair = handle->pair;
    if (!importPoint(pair.group.get(), material.first(PublicKeySize), pair.public_point.get())) {
        return std::nullopt;
    }
    const auto scalar = material.subspan(PublicKeySize);
    if (mbedtls_mpi_read_binary(pair.secret.get(), scalar.data(), scalar.size()) != 0 ||
        mbedtls_ecp_check_privkey(pair.group.get(), pair.secret.get()) != 0) {
        return std::nullopt;
    }
    // The public half must be the one the private scalar gives.
    Point derived;
    auto* const generator = &pair.group.get()->G;
    if (mbedtls_ecp_mul(pair.group.get(), derived.get(), pair.secret.get(), generator,
                        &systemRandom, nullptr) != 0 ||
        mbedtls_ecp_point_cmp(derived.get(), pair.public_point.get()) != 0) {
        return std::nullopt;
    }
    PublicKey public_key{};
    std::ranges::copy(material.first(PublicKeySize), public_key.begin());
    return OwnerKey(std::move(handle), public_key);
}

Bytes OwnerKey::exportPrivate() const {
    Bytes out(public_key_.begin(), public_key_.end());
    std::array<unsigned char, ScalarSize> scalar{};
    check(mbedtls_mpi_write_binary(handle_->pair.secret.get(), scalar.data(), scalar.size()),
          "mbedtls_mpi_write_binary");
    out.insert(out.end(), scalar.begin(), scalar.end());
    platform::secureZero(scalar.data(), scalar.size());
    return out;
}

Signature OwnerKey::signDigest(const Digest& digest) const {
    Mpi r;
    Mpi s;
    check(mbedtls_ecdsa_sign(handle_->pair.group.get(), r.get(), s.get(),
                             handle_->pair.secret.get(), digest.data(), digest.size(),
                             &systemRandom, nullptr),
          "mbedtls_ecdsa_sign");
    Signature signature{};
    const auto bytes = std::span(signature);
    check(mbedtls_mpi_write_binary(r.get(), bytes.first<ScalarSize>().data(), ScalarSize),
          "mbedtls_mpi_write_binary");
    check(mbedtls_mpi_write_binary(s.get(), bytes.subspan<ScalarSize>().data(), ScalarSize),
          "mbedtls_mpi_write_binary");
    return signature;
}

OwnerKey::OwnerKey(OwnerKey&&) noexcept = default;
OwnerKey& OwnerKey::operator=(OwnerKey&&) noexcept = default;
OwnerKey::~OwnerKey() = default;
} // namespace mgl::crypto
