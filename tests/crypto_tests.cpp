// SPDX-License-Identifier: AGPL-3.0-only
// Known answers for mgl/crypto.hpp, run against each platform's provider (Windows CNG, mbedTLS) so
// a provider cannot quietly differ. The vectors are public: FIPS 180-4 SHA-256, RFC 4231 HMAC, NIST
// SP 800-38A AES-256-CBC and the RFC 6979 A.2.5 P-256 key pair. The glasses themselves check the
// ECDSA signatures: they accept the owner key only if they verify.
#include "mgl/bytes.hpp"
#include "mgl/crypto.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
namespace crypto = mgl::crypto;

// Fails the run on the first false case and counts every case checked.
class Checks {
  public:
    void operator()(bool ok, const char* name) {
        ++count_;
        if (!ok) {
            throw std::runtime_error(std::string(name) + ": failed");
        }
    }
    // A case that holds when the optional has a value; returns that value.
    template <typename T>
    [[nodiscard]] const T& present(const std::optional<T>& value, const char* name) {
        ++count_;
        if (!value) {
            throw std::runtime_error(std::string(name) + ": failed");
        }
        return *value;
    }
    [[nodiscard]] int count() const noexcept { return count_; }

  private:
    int count_ = 0;
};

std::vector<std::uint8_t> fromHex(std::string_view text) {
    const auto nibble = [](char digit) {
        return static_cast<std::uint32_t>(digit <= '9' ? digit - '0' : digit - 'a' + 10);
    };
    std::vector<std::uint8_t> out;
    out.reserve(text.size() / 2);
    for (std::size_t index = 0; index + 1 < text.size(); index += 2) {
        out.push_back(
            static_cast<std::uint8_t>((nibble(text.at(index)) << 4U) | nibble(text.at(index + 1))));
    }
    return out;
}

mgl::ByteView view(const std::vector<std::uint8_t>& bytes) { return {bytes.data(), bytes.size()}; }

template <typename Container> bool equalHex(const Container& bytes, std::string_view expected) {
    const auto wanted = fromHex(expected);
    return wanted.size() == bytes.size() && std::equal(wanted.begin(), wanted.end(), bytes.begin());
}

void testHashes(Checks& check) {
    const std::string_view abc = "abc";
    const auto abc_bytes = std::vector<std::uint8_t>(abc.begin(), abc.end());
    check(equalHex(crypto::sha256({view(abc_bytes)}),
                   "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"),
          "SHA-256 of abc");
    // Split input gives the same digest.
    const std::vector<std::uint8_t> a{'a'};
    const std::vector<std::uint8_t> bc{'b', 'c'};
    check(equalHex(crypto::sha256({view(a), view(bc)}),
                   "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"),
          "SHA-256 over parts");

    // RFC 4231 test case 2.
    const std::string_view key = "Jefe";
    const std::string_view data = "what do ya want for nothing?";
    const auto key_bytes = std::vector<std::uint8_t>(key.begin(), key.end());
    const auto data_bytes = std::vector<std::uint8_t>(data.begin(), data.end());
    constexpr std::string_view Expected =
        "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843";
    check(equalHex(crypto::hmacSha256(view(key_bytes), {view(data_bytes)}), Expected),
          "HMAC-SHA-256 RFC 4231 case 2");
    const crypto::HmacSha256 reusable(view(key_bytes));
    (void)reusable.compute({view(key_bytes)});
    check(equalHex(reusable.compute({view(data_bytes)}), Expected),
          "a reused HMAC key starts fresh");
}

void testAes(Checks& check) {
    // NIST SP 800-38A F.2.5, CBC-AES256.Encrypt, the first two blocks.
    const auto key = fromHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
    const auto iv = fromHex("000102030405060708090a0b0c0d0e0f");
    const auto plain = fromHex("6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e51");
    constexpr std::string_view Cipher =
        "f58c4c04d6e5f1ba779eabfb5f7bfbd69cfc4e967edb808d679f777bc6702c7d";
    const crypto::AesCbc aes(view(key));
    const auto encrypted = aes.encrypt(view(iv), view(plain));
    check(equalHex(encrypted, Cipher), "AES-256-CBC encrypt");
    const auto decrypted = aes.decrypt(view(iv), view(encrypted));
    check(std::ranges::equal(plain, decrypted), "AES-256-CBC decrypt");
    // The caller owns chaining: the same IV gives the same result again.
    check(aes.encrypt(view(iv), view(plain)) == encrypted, "AES-CBC leaves the IV to the caller");
    bool refused = false;
    try {
        (void)aes.encrypt(view(iv), mgl::ByteView(plain.data(), 15));
    } catch (const crypto::CryptoError&) {
        refused = true;
    }
    check(refused, "AES-CBC refuses a partial block");
}

void testEcdh(Checks& check) {
    // Each side's generated key agrees with the other's, and a point off the curve is refused.
    const auto left = crypto::EcdhKey::generate();
    const auto right = crypto::EcdhKey::generate();
    const auto from_left = left.sharedSecret(mgl::ByteView(right.publicKey()));
    const auto from_right = right.sharedSecret(mgl::ByteView(left.publicKey()));
    check(from_left.has_value() && from_right.has_value() && *from_left == *from_right,
          "ECDH sides agree");
    std::array<std::uint8_t, crypto::PublicKeySize> off_curve{};
    off_curve.fill(0x11);
    check(!left.sharedSecret(mgl::ByteView(off_curve)).has_value(),
          "ECDH refuses a point off the curve");
    check(!left.sharedSecret(mgl::ByteView(off_curve).first(10)).has_value(),
          "ECDH refuses a short key");
}

void testOwnerKey(Checks& check) {
    // RFC 6979 A.2.5: the P-256 public point and its private scalar.
    const auto x = fromHex("60fed4ba255a9d31c961eb74c6356d68c049b8923b61fa6ce669622e60f29fb6");
    const auto y = fromHex("7903fe1008b8bc99a41ae9e95628bc64f2f1b20c2d7e9f5177a3c294d4462299");
    const auto d = fromHex("c9afa9d845ba75166b5c215767b1d6934e50c3db36e89b127b8a622b120f6721");
    std::vector<std::uint8_t> material(x);
    material.insert(material.end(), y.begin(), y.end());
    material.insert(material.end(), d.begin(), d.end());
    const auto imported = crypto::OwnerKey::importPrivate(view(material));
    const auto& owner = check.present(imported, "owner key imports");
    check(owner.exportPrivate() == material, "owner key exports the same material");
    auto mismatched = material;
    mismatched.at(crypto::PublicKeySize) ^= 0x01U; // the first byte of the scalar
    check(!crypto::OwnerKey::importPrivate(view(mismatched)).has_value(),
          "owner key refuses a scalar that does not match its point");
    check(!crypto::OwnerKey::importPrivate(mgl::ByteView(material).first(40)).has_value(),
          "owner key refuses short material");

    const auto generated = crypto::OwnerKey::generate();
    const auto round_trip =
        crypto::OwnerKey::importPrivate(mgl::ByteView(generated.exportPrivate()));
    check(round_trip.has_value() && round_trip->publicKey() == generated.publicKey(),
          "a generated owner key survives export and import");

    // Both halves are present and the low-S normalisation stays within half the group order.
    const crypto::Digest digest = crypto::sha256({view(x)});
    const auto signature = crypto::lowS(owner.signDigest(digest));
    constexpr std::size_t Half = crypto::ScalarSize;
    const bool r_set = std::any_of(signature.begin(), signature.begin() + Half,
                                   [](std::uint8_t byte) { return byte != 0; });
    const bool s_set = std::any_of(signature.begin() + Half, signature.end(),
                                   [](std::uint8_t byte) { return byte != 0; });
    check(r_set && s_set, "an ECDSA signature has both halves");
    check(signature.at(Half) <= 0x7F, "low-S keeps s in the lower half");
}

void testRandom(Checks& check) {
    std::array<std::uint8_t, 64> first{};
    std::array<std::uint8_t, 64> second{};
    crypto::randomBytes(first);
    crypto::randomBytes(second);
    check(first != second, "the random source does not repeat");
    const auto text = crypto::randomText("abc", 32);
    check(text.size() == 32 && text.find_first_not_of("abc") == std::string::npos,
          "random text stays in its alphabet");
}
} // namespace

int main() {
    try {
        Checks check;
        testHashes(check);
        testAes(check);
        testEcdh(check);
        testOwnerKey(check);
        testRandom(check);
        std::cout << check.count() << " crypto provider cases passed\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        constexpr int DiagnosticWriteFailed = 4;
        if (std::fputs(error.what(), stderr) == EOF || std::fputc('\n', stderr) == EOF) {
            return DiagnosticWriteFailed;
        }
        return EXIT_FAILURE;
    }
}
