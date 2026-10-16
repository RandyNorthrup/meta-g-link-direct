# Changelog

All notable changes to this project will be documented here. The project uses semantic versions and has not published a release.

## [Unreleased]

### Added

- Confirmed personal Windows OBS/Gen 2 contract, including optional glasses audio, adjustable video, latest stable firmware targeting, and a minimum 120-minute final acceptance run.
- Native CMake project, pinned OBS 32.2.2 development inputs, AGPL-3.0 license and Visor attribution.
- Portable frame-prefix library with incomplete/invalid/ready results and independent boundary examples.
- Read-only Windows radio diagnostics and an OBS metadata module that explicitly does not implement a camera source.
- Wi-Fi association counts and an explicit bounded passive BLE discovery command, with redacted report counts, native failure propagation, seven real CLI contract tests and an invalid-argument regression drill. The current Windows BLE scan returns `TransportNotSupported`; authenticated glasses operation remains unproved.
- Native controller feature inspection, explicit Classic inquiry/registered Meta-service discovery, and a separate bounded RFCOMM socket open/close check restricted to one Windows-paired service. Reuse one private apartment lifetime. Ten CLI tests and a connection-claim drill protect diagnostics. Current controller reports LE unsupported; Classic inquiry works but no registered Meta service is present, so no glasses socket connection was attempted.
- Existing-adapter Classic investigation: uncached per-peer SDP with explicit error/completion counters, a separate opt-in Windows pairing picker, and direct private-address Winsock UUID/channel-7 hello probes with no OS bond prerequisite or application writes. No glasses connection is claimed: uncached queries found no Meta service with one timeout, the picker was canceled, and the glasses address remains unidentified.
- Bounded hidden address input, a portable validated address parser, requested redacted JSON export and write-error propagation. Shared process-test support covers invalid/private input without contacting guessed devices; separate live SDP testing and address/skipped-query drills extend the maintained checks.
- Isolated native module loading, parser tests, disposable-resource PowerShell runner tests, separate Windows AddressSanitizer build, local quality tasks and defect/restoration drills.
- Local pre-commit checks and a hardened portable CI workflow definition. Hosted CI has not run.

### Engineering decisions

- Use C17/MSVC for OBS ABI glue after real clang-cl build failures exposed unsupported `_udiv128`; retain C++20 for reusable code.
- Normalize Cppcheck's MSVC compilation database and model real dynamic OBS entry points. Keep analysis limitations explicit.
- Preserve original failed canary evidence, correct the AWS canary to the scanner's documented base32 pattern, and retain exact-restoration checks.

Authentication, direct glasses sessions, camera/audio decoding, source UI, latest-firmware proof and long-session acceptance remain future work. The earlier foundation ZIP is historical and does not contain the new discovery CLI.
