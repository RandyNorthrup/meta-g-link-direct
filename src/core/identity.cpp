// SPDX-License-Identifier: AGPL-3.0-only
// The owner identity and token store over a platform's sealed storage (identity_store.hpp).
#include "mgl/identity.hpp"

#include "identity_store.hpp"
#include "mgl/bytes.hpp"
#include "mgl/platform.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace mgl::identity {
namespace {
// Reads a little-endian u32 at offset and advances past it; nullopt if fewer than four bytes
// remain.
std::optional<std::uint32_t> readU32(ByteView data, std::size_t& offset) {
    if (data.size() - offset < sizeof(std::uint32_t)) {
        return std::nullopt;
    }
    std::uint32_t value = 0;
    unsigned shift = 0;
    for (const std::uint8_t byte : data.subspan(offset, sizeof(std::uint32_t))) {
        value |= static_cast<std::uint32_t>(byte) << shift;
        shift += 8U;
    }
    offset += sizeof(std::uint32_t);
    return value;
}

void appendString(Bytes& out, const std::string& text) {
    appendLittleEndian(out, static_cast<std::uint32_t>(text.size()));
    out.insert(out.end(), text.begin(), text.end());
}

bool readString(ByteView data, std::size_t& offset, std::string& out) {
    const auto size = readU32(data, offset);
    if (!size || data.size() - offset < *size) {
        return false;
    }
    const auto text = data.subspan(offset, *size);
    out.assign(text.begin(), text.end());
    offset += *size;
    return true;
}

std::string& lastProblem() {
    thread_local std::string problem;
    return problem;
}
} // namespace

void store::noteProblem(std::string problem) { lastProblem() = std::move(problem); }

std::string lastStorageProblem() { return lastProblem(); }

bool saveOwnerIdentity(const OwnerIdentity& identity) {
    store::noteProblem({});
    Bytes blob;
    appendLittleEndian(blob, static_cast<std::uint32_t>(identity.owner_private.size()));
    append(blob, identity.owner_private);
    appendString(blob, identity.serial);
    appendString(blob, identity.user_id);
    const bool ok = store::save(store::IdentityBlob, blob);
    platform::secureZero(blob.data(), blob.size());
    return ok;
}

std::optional<OwnerIdentity> loadOwnerIdentity() {
    store::noteProblem({});
    auto blob = store::load(store::IdentityBlob);
    if (!blob) {
        return std::nullopt;
    }
    std::size_t offset = 0;
    const ByteView view(*blob);
    const auto key_size = readU32(view, offset);
    if (!key_size || view.size() - offset < *key_size) {
        return std::nullopt;
    }
    OwnerIdentity identity;
    const auto key = view.subspan(offset, *key_size);
    identity.owner_private.assign(key.begin(), key.end());
    offset += *key_size;
    if (!readString(view, offset, identity.serial) || !readString(view, offset, identity.user_id)) {
        return std::nullopt;
    }
    return identity;
}

bool forgetOwnerIdentity() {
    // Both run even if the first fails, so a partial Forget removes all it can.
    const bool identity_gone = store::remove(store::IdentityBlob);
    const bool tokens_gone = store::remove(store::TokenBlob);
    return identity_gone && tokens_gone;
}

bool saveTokenBlob(const std::string& text) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) — byte view of the token text.
    const ByteView bytes(reinterpret_cast<const std::uint8_t*>(text.data()), text.size());
    return store::save(store::TokenBlob, bytes);
}

std::optional<std::string> loadTokenBlob() {
    auto blob = store::load(store::TokenBlob);
    if (!blob) {
        return std::nullopt;
    }
    return std::string(blob->begin(), blob->end());
}
} // namespace mgl::identity
