// SPDX-License-Identifier: AGPL-3.0-only
// Port of Visor core/protocol Kdf.kt, Cipher.kt, Messages.kt, Binding.kt and core/identity
// Trust.kt at e7077b4051b1e3e605c667aa2a010fbe41f346a0. See THIRD_PARTY_NOTICES.md.
#include "mgl/airshield.hpp"

#include "mgl/bytes.hpp"
#include "mgl/crypto.hpp"
#include "mgl/protobuf.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace mgl::airshield {
namespace {
constexpr std::uint8_t PadOffset = 0xC0;
constexpr std::size_t BlockSize = crypto::AesBlockSize;
constexpr std::uint32_t CounterPrefix = 0x00000202;
constexpr std::uint8_t ProvisioningCapabilities = 2;

const std::array<std::uint8_t, 9> AirshieldInfo{'A', 'i', 'r', 'S', 'h', 'i', 'e', 'l', 'd'};
const std::array<std::uint8_t, 11> HmacDeriveLabel{'h', 'm', 'a', 'c', '_', 'd',
                                                   'e', 'r', 'i', 'v', 'e'};

ByteView view(const auto& container) { return ByteView(container.data(), container.size()); }

// HKDF-Expand for exactly one SHA-256 block (the only length AirShield asks for).
crypto::Digest hkdfExpand(const crypto::Digest& prk, ByteView info) {
    const std::array<std::uint8_t, 1> counter{1};
    return crypto::hmacSha256(view(prk), {info, {counter.data(), counter.size()}});
}

void writeLittle32(Bytes& out, std::uint32_t value) {
    for (std::size_t index = 0; index < sizeof(value); ++index) {
        out.push_back(static_cast<std::uint8_t>(value >> (index * 8U)));
    }
}

Bytes modifiedPad(ByteView data) {
    const auto pad = BlockSize - (data.size() % BlockSize);
    const auto marker = static_cast<std::uint8_t>(pad + PadOffset);
    Bytes out(data.begin(), data.end());
    out.insert(out.end(), pad, marker);
    return out;
}
} // namespace

DirectionKeys deriveDirectionKeys(const crypto::Digest& shared_secret, ByteView nonce,
                                  ByteView seed, ByteView iv) {
    const auto aes_salt = crypto::sha256({nonce, seed});
    const auto hmac_salt = crypto::sha256({seed, nonce, view(HmacDeriveLabel)});
    const auto aes_prk =
        crypto::hmacSha256(view(aes_salt), {{shared_secret.data(), shared_secret.size()}});
    const auto hmac_prk =
        crypto::hmacSha256(view(hmac_salt), {{shared_secret.data(), shared_secret.size()}});
    const auto aes_key = hkdfExpand(aes_prk, view(AirshieldInfo));
    const auto hmac_key = hkdfExpand(hmac_prk, view(AirshieldInfo));
    DirectionKeys keys;
    keys.aes = aes_key;
    keys.hmac = hmac_key;
    std::ranges::copy(iv.first(IvSize), keys.iv.begin());
    return keys;
}

FrameCrypto::FrameCrypto(const DirectionKeys& keys, std::uint32_t initial_sequence)
    : aes_(view(keys.aes)), hmac_(view(keys.hmac)), tx_iv_(keys.iv), rx_iv_(keys.iv),
      tx_sequence_(initial_sequence), rx_sequence_(initial_sequence) {}

crypto::Digest FrameCrypto::tag(std::uint32_t sequence, std::uint8_t block_count,
                                ByteView ciphertext) const {
    Bytes counter;
    counter.reserve(sizeof(std::uint32_t) * 2 + 1);
    writeLittle32(counter, CounterPrefix);
    writeLittle32(counter, sequence);
    counter.push_back(block_count);
    return hmac_.compute({view(counter), ciphertext});
}

Bytes FrameCrypto::seal(ByteView plaintext) {
    const auto padded = modifiedPad(plaintext);
    const auto ciphertext = aes_.encrypt(view(tx_iv_), padded);
    const auto block_count = static_cast<std::uint8_t>((ciphertext.size() / BlockSize) - 1);
    const auto mac = tag(tx_sequence_, block_count, ciphertext);
    Bytes frame;
    frame.reserve(TagSize + 1 + ciphertext.size());
    append(frame, {mac.data(), TagSize});
    frame.push_back(block_count);
    append(frame, ciphertext);
    std::ranges::copy(std::span(ciphertext).last(BlockSize), tx_iv_.begin());
    ++tx_sequence_;
    return frame;
}

std::optional<Bytes> FrameCrypto::open(ByteView frame, bool strip_pad) {
    if (frame.size() < TagSize + 1 + BlockSize) {
        return std::nullopt;
    }
    const auto received_tag = frame.first(TagSize);
    const auto block_count = frame[TagSize];
    const auto ciphertext = frame.subspan(TagSize + 1);
    if (ciphertext.size() != (static_cast<std::size_t>(block_count) + 1) * BlockSize) {
        return std::nullopt;
    }
    const auto expected = tag(rx_sequence_, block_count, ciphertext);
    if (!crypto::constantTimeEqual(received_tag, {expected.data(), TagSize})) {
        return std::nullopt;
    }
    auto plaintext = aes_.decrypt(view(rx_iv_), ciphertext);
    std::ranges::copy(ciphertext.last(BlockSize), rx_iv_.begin());
    ++rx_sequence_;
    if (strip_pad && !plaintext.empty()) {
        const auto pad =
            static_cast<std::size_t>(static_cast<std::uint8_t>(plaintext.back() - PadOffset));
        if (pad >= 1 && pad <= BlockSize && pad <= plaintext.size()) {
            plaintext.resize(plaintext.size() - pad);
        }
    }
    return plaintext;
}

Bytes encodeRequestEncryption(const crypto::PublicKey& public_key, ByteView challenge) {
    Bytes out;
    proto::appendBytesField(out, 1, view(public_key));
    proto::appendBytesField(out, 2, challenge);
    proto::appendVarintField(out, 3, 0);  // curve 0 = secp256r1
    proto::appendVarintField(out, 4, 31); // supported parameters
    proto::appendVarintField(out, 7, 16); // AirShield version
    return out;
}

Bytes encodeEnableEncryption(const crypto::PublicKey& public_key, ByteView seed, ByteView iv,
                             std::uint32_t base) {
    Bytes out;
    proto::appendBytesField(out, 1, view(public_key));
    proto::appendBytesField(out, 2, seed);
    proto::appendBytesField(out, 3, iv);
    proto::appendVarintField(out, 4, base);
    proto::appendVarintField(out, 5, 31); // parameters; the glasses drop our frames when this is 1
    return out;
}

Bytes encodeEndLinkSetup(const Uuid& scope, const Uuid& lease) {
    Bytes out;
    proto::appendVarintField(out, 1, 1); // asMain
    proto::appendBytesField(out, 2, view(scope));
    proto::appendBytesField(out, 3, view(lease));
    return out;
}

namespace {
template <std::size_t N>
bool copyFixed(std::optional<ByteView> field, std::array<std::uint8_t, N>& out) {
    if (!field || field->size() != N) {
        return false;
    }
    std::ranges::copy(*field, out.begin());
    return true;
}
} // namespace

bool readHandshakeMessage(std::uint16_t message_type, ByteView payload, PeerHandshake& peer) {
    const auto message = proto::Message::parse(payload);
    if (!message) {
        return false;
    }
    if (message_type == RequestEncryptionType) {
        if (!copyFixed(message->bytes(1), peer.request_public_key) ||
            !copyFixed(message->bytes(2), peer.challenge)) {
            return false;
        }
        peer.have_request = true;
        return true;
    }
    if (message_type == EnableEncryptionType) {
        if (!copyFixed(message->bytes(1), peer.enable_public_key) ||
            !copyFixed(message->bytes(2), peer.seed) || !copyFixed(message->bytes(3), peer.iv)) {
            return false;
        }
        peer.base = static_cast<std::uint32_t>(message->varintOr(4, 0));
        peer.have_enable = true;
        return true;
    }
    return false;
}

Uuid randomUuid() {
    Uuid uuid{};
    do {
        crypto::randomBytes(uuid);
        constexpr std::uint8_t VersionMask = 0x0F;
        constexpr std::uint8_t Version4 = 0x40;
        constexpr std::uint8_t VariantMask = 0x3F;
        constexpr std::uint8_t Variant10 = 0x80;
        uuid[6] = static_cast<std::uint8_t>((uuid[6] & VersionMask) | Version4);
        uuid[8] = static_cast<std::uint8_t>((uuid[8] & VariantMask) | Variant10);
    } while (!validUuid(uuid));
    return uuid;
}

bool validUuid(const Uuid& uuid) noexcept {
    return (uuid[6] & 0xF0) == 0x40 && (uuid[8] & 0xC0) == 0x80;
}

crypto::Digest channelBinding(ByteView glasses_challenge,
                              const crypto::PublicKey& glasses_request_key, ByteView our_seed,
                              const crypto::PublicKey& our_public) {
    const auto left = crypto::sha256({glasses_challenge, view(glasses_request_key)});
    const auto right = crypto::sha256({our_seed, view(our_public)});
    return crypto::sha256({view(left), view(right)});
}

Bytes encodeEnableTrust(const crypto::OwnerKey& owner, const crypto::Digest& binding) {
    const auto signature = crypto::lowS(owner.signDigest(binding));
    const auto key_hash = crypto::sha256({view(owner.publicKey())});
    Bytes out;
    proto::appendBytesField(out, 1, view(key_hash));
    proto::appendBytesField(out, 2, view(signature));
    proto::appendVarintField(out, 3, ProvisioningCapabilities);
    return out;
}
} // namespace mgl::airshield
