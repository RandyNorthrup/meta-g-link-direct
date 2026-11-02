// SPDX-License-Identifier: AGPL-3.0-only
// mgl/platform.hpp on Linux, and on macOS apart from the region (src/macos/region.mm).
#ifdef __APPLE__
#define __STDC_WANT_LIB_EXT1__ 1 // memset_s; must come before any system header
#endif
#include "mgl/platform.hpp"

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>

namespace mgl::platform {
#ifndef __APPLE__
std::optional<std::string> userRegion() {
    // A locale name such as en_US.UTF-8 carries the region after the underscore.
    for (const char* variable : {"LC_ALL", "LC_MESSAGES", "LANG"}) {
        const char* value = std::getenv(variable); // NOLINT(concurrency-mt-unsafe)
        if (value == nullptr || *value == '\0') {
            continue;
        }
        const std::string_view locale(value);
        const auto underscore = locale.find('_');
        constexpr std::size_t CodeLength = 2;
        if (underscore == std::string_view::npos || locale.size() < underscore + 1 + CodeLength) {
            return std::nullopt;
        }
        const auto code = locale.substr(underscore + 1, CodeLength);
        const auto upper = [](char letter) { return letter >= 'A' && letter <= 'Z'; };
        if (!upper(code[0]) || !upper(code[1])) {
            return std::nullopt;
        }
        return std::string(code);
    }
    return std::nullopt;
}
#endif

bool environmentFlag(const char* name) {
    return std::getenv(name) != nullptr; // NOLINT(concurrency-mt-unsafe)
}

void secureZero(void* memory, std::size_t size) noexcept {
#ifdef __APPLE__
    (void)memset_s(memory, size, 0, size);
#else
    explicit_bzero(memory, size);
#endif
}

std::optional<std::string> describeCurrentException() { return std::nullopt; }
} // namespace mgl::platform
