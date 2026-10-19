// SPDX-License-Identifier: AGPL-3.0-only
// Credentials for the short-lived access point the glasses host for this PC.
#ifndef MGL_WINDOWS_WIFI_CREDENTIALS_HPP
#define MGL_WINDOWS_WIFI_CREDENTIALS_HPP

#include "mgl/crypto.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace mgl {
// Names of the access point the glasses host for this PC.
inline constexpr std::string_view GlassesAccessPointPrefix = "mgl-";

struct WifiCredentials {
    std::string ssid;
    std::string passphrase;
};

// Fresh random credentials from the system CSPRNG: unambiguous symbols only, a 6-symbol name
// suffix and a 20-symbol passphrase (about 116 bits). Throws crypto::CryptoError if the generator
// fails, so a weak or empty passphrase is never used.
[[nodiscard]] inline WifiCredentials randomWifiCredentials() {
    constexpr std::string_view Alphabet =
        "abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    constexpr std::size_t SuffixLength = 6;
    constexpr std::size_t PassphraseLength = 20;
    return {.ssid =
                std::string(GlassesAccessPointPrefix) + crypto::randomText(Alphabet, SuffixLength),
            .passphrase = crypto::randomText(Alphabet, PassphraseLength)};
}
} // namespace mgl
#endif
