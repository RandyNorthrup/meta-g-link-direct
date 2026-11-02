// SPDX-License-Identifier: AGPL-3.0-only
// The small things the portable core asks of the operating system, one implementation per platform.
#ifndef MGL_PLATFORM_HPP
#define MGL_PLATFORM_HPP

#include <cstddef>
#include <optional>
#include <string>

namespace mgl::platform {
// The user's region as an ISO 3166 two-letter code, or nullopt when the system does not say.
[[nodiscard]] std::optional<std::string> userRegion();

// Whether the named environment variable is set, whatever its value.
[[nodiscard]] bool environmentFlag(const char* name);

// Overwrites memory that held key material; the compiler may not remove the write.
void secureZero(void* memory, std::size_t size) noexcept;

// Called inside a catch-all: describes an exception of the platform's own kind that does not derive
// from std::exception, or nullopt when it is not one.
[[nodiscard]] std::optional<std::string> describeCurrentException();
} // namespace mgl::platform
#endif
