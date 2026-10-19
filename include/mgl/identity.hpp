// SPDX-License-Identifier: AGPL-3.0-only
// Local, user-scoped protected storage for the glasses owner identity and the Meta client tokens.
// Everything is sealed with Windows DPAPI (CryptProtectData, current-user scope) under
// %LOCALAPPDATA%\MetaGLinkDirect. Nothing here is ever written to the repo, logs or diagnostics.
#ifndef MGL_IDENTITY_HPP
#define MGL_IDENTITY_HPP

#include "mgl/bytes.hpp"

#include <optional>
#include <string>

namespace mgl::identity {
struct OwnerIdentity {
    Bytes owner_private; // 96 bytes: x || y || d of the P-256 owner key
    std::string serial;  // glasses serial from the final ownership receipt
    std::string user_id; // Meta user id from the final ownership receipt
};

// true on success. The blob is DPAPI-sealed before it touches disk.
[[nodiscard]] bool saveOwnerIdentity(const OwnerIdentity& identity);
[[nodiscard]] std::optional<OwnerIdentity> loadOwnerIdentity();
// Forget deletes this plugin's stored owner identity and any imported token store, nothing else.
// true if both are removed or already absent.
[[nodiscard]] bool forgetOwnerIdentity();

// Imported Meta app tokens, which override the built-in ones: an "HW|..." line and "AR|..." or
// bare "<id>|<secret>" lines, sealed as given.
[[nodiscard]] bool saveTokenBlob(const std::string& text);
[[nodiscard]] std::optional<std::string> loadTokenBlob();

// The directory the sealed files live in, created on demand.
[[nodiscard]] std::optional<std::string> storageDirectory();
} // namespace mgl::identity
#endif
