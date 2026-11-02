// SPDX-License-Identifier: AGPL-3.0-only
#include "mgl/platform.hpp"

#include <windows.h> // IWYU pragma: keep — SDK umbrella prerequisites.

#include <winbase.h>
#include <winnls.h>

#include <winrt/base.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace mgl::platform {
std::optional<std::string> userRegion() {
    constexpr int RegionCodeLength = 3; // two letters and the terminator
    std::array<wchar_t, RegionCodeLength> name{};
    if (GetUserDefaultGeoName(name.data(), RegionCodeLength) != RegionCodeLength) {
        return std::nullopt;
    }
    std::string code;
    for (const wchar_t letter : std::wstring_view(name.data(), RegionCodeLength - 1)) {
        if (letter < L'A' || letter > L'Z') {
            return std::nullopt;
        }
        code.push_back(static_cast<char>(letter));
    }
    return code;
}

bool environmentFlag(const char* name) {
    char* raw = nullptr;
    std::size_t size = 0;
    // NOLINTNEXTLINE(misc-include-cleaner) - UCRT-only; declared in <stdlib.h>, under <cstdlib>.
    const auto status = _dupenv_s(&raw, &size, name);
    // _dupenv_s hands back a CRT allocation the caller must release with free.
    const std::unique_ptr<char, decltype(&std::free)> value(raw, &std::free);
    return status == 0 && value != nullptr;
}

void secureZero(void* memory, std::size_t size) noexcept { SecureZeroMemory(memory, size); }

std::optional<std::string> describeCurrentException() {
    try {
        throw;
    } catch (const winrt::hresult_error& error) {
        return std::format("Windows error 0x{:08X}",
                           static_cast<std::uint32_t>(error.code().value));
    } catch (...) {
        return std::nullopt;
    }
}
} // namespace mgl::platform
