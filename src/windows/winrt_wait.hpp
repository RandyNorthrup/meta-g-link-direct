// SPDX-License-Identifier: AGPL-3.0-only
// Bounded waits on C++/WinRT async operations. An operation that never completes would otherwise
// block its thread forever (seen with Windows radio calls during a reconnect), so every wait has a
// deadline, cancels on time-out, and fails explicitly.
#ifndef MGL_WINDOWS_WINRT_WAIT_HPP
#define MGL_WINDOWS_WINRT_WAIT_HPP

#include <winrt/Windows.Foundation.h> // IWYU pragma: keep

#include <chrono>
#include <cstdint>

namespace mgl {
// HRESULT_FROM_WIN32(ERROR_TIMEOUT), reported when a bounded wait runs out.
inline constexpr std::uint32_t TimedOutHresult = 0x800705B4;

// Waits up to timeout for the operation; on time-out it is cancelled. True only if it completed.
template <typename Operation>
bool awaitOperation(const Operation& operation, std::chrono::milliseconds timeout) {
    using winrt::Windows::Foundation::AsyncStatus; // NOLINT(misc-include-cleaner)
    if (operation.wait_for(timeout) == AsyncStatus::Started) {
        operation.Cancel();
        return false;
    }
    return operation.Status() == AsyncStatus::Completed;
}
} // namespace mgl
#endif
