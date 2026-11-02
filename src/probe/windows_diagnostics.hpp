// SPDX-License-Identifier: AGPL-3.0-only
// The probe's radio diagnostics that use Windows-only Bluetooth and Wi-Fi APIs.
#ifndef MGL_PROBE_WINDOWS_DIAGNOSTICS_HPP
#define MGL_PROBE_WINDOWS_DIAGNOSTICS_HPP

#include <string_view>

namespace mgl::probe {
// Whether mode names one of these diagnostics. No arguments at all runs the default one.
[[nodiscard]] bool isWindowsDiagnostic(std::string_view mode);
[[nodiscard]] int runWindowsDiagnostic(std::string_view mode);
} // namespace mgl::probe
#endif
