// SPDX-License-Identifier: AGPL-3.0-only
// Where a platform keeps the sealed identity and token blobs: Windows DPAPI files, the Secret
// Service on Linux, the login keychain on macOS. Every implementation ties a blob to the current
// user and never writes it in the clear.
#ifndef MGL_CORE_IDENTITY_STORE_HPP
#define MGL_CORE_IDENTITY_STORE_HPP

#include "mgl/bytes.hpp"

#include <optional>
#include <string>

namespace mgl::identity::store {
inline constexpr const char* IdentityBlob = "identity.bin";
inline constexpr const char* TokenBlob = "tokens.bin";

[[nodiscard]] bool save(const char* name, ByteView plaintext);
// nullopt when the blob is absent or cannot be opened.
[[nodiscard]] std::optional<Bytes> load(const char* name);
// True when the blob is gone, including when it never existed.
[[nodiscard]] bool remove(const char* name);
// Keeps the system's reason when one of the above fails for a reason other than an absent blob.
void noteProblem(std::string problem);
} // namespace mgl::identity::store
#endif
