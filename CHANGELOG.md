# Changelog

All notable changes to this project will be documented here. The project uses semantic versions and has not published a release.

## [Unreleased]

### Added

- Confirmed personal Windows OBS/Gen 2 contract, including optional glasses audio, adjustable video, latest stable firmware targeting, and a minimum 120-minute final acceptance run.
- Native CMake project, pinned OBS 32.2.2 development inputs, AGPL-3.0 license and Visor attribution.
- Portable frame-prefix library with incomplete/invalid/ready results and independent boundary examples.
- Read-only Windows radio diagnostics and an OBS metadata module that explicitly does not implement a camera source.
- Isolated native module loading, parser tests, disposable-resource PowerShell runner tests, separate Windows AddressSanitizer build, local quality tasks and defect/restoration drills.
- Local pre-commit checks and a hardened portable CI workflow definition. Hosted CI has not run.

### Engineering decisions

- Use C17/MSVC for OBS ABI glue after real clang-cl build failures exposed unsupported `_udiv128`; retain C++20 for reusable code.
- Normalize Cppcheck's MSVC compilation database and model real dynamic OBS entry points. Keep analysis limitations explicit.
- Preserve original failed canary evidence, correct the AWS canary to the scanner's documented base32 pattern, and retain exact-restoration checks.

Authentication, direct glasses sessions, camera/audio decoding, source UI, latest-firmware proof and long-session acceptance remain future work.
