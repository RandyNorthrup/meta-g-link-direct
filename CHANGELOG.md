# Changelog

All notable changes are recorded here. The project follows [semantic versioning](https://semver.org/); versions below 1.0 are pre-releases.

## [Unreleased]

## [0.1.0] - 2026-10-19 (pre-release)

First public build. Tested on one Windows 11 PC (OBS 32.2.2) with one pair of Ray-Ban Meta Gen 2 glasses.

### Added

- **OBS source "Ray-Ban Meta Glasses (Direct)"**: live 720 × 1280 HEVC video at 30 fps, decoded with the FFmpeg libraries that ship with OBS. It starts when its scene goes live and stops when the scene leaves the program output. The properties offer a quality list, a Wi-Fi adapter choice (Automatic, or a named adapter; an unplugged choice fails with a log line instead of switching adapters) and a status line. Verified in OBS 32.2.2: two minutes at 30 fps; across start, stream and stop OBS skipped 1 render frame in 9,794, and that skip happened before the scene went live.
- **Direct link with no phone and no network changes**: an encrypted Bluetooth session (AirShield) authenticates with the owner key; the glasses then host their own 2.4 GHz / 20 MHz Wi-Fi network (svc10 `CreateSoftApNetwork`), the PC joins it with a temporary Windows Wi-Fi profile, and video arrives over a second encrypted session on TCP. Verified on hardware: about 179 s of video with no stall in a test harness and about 104 s in the shipped media session, back-to-back stop/start cycles, and a 4.1 s network join.
- **One-time Meta setup** that makes the PC the glasses' owner (Meta login, ownership exchange). The owner key is sealed with Windows DPAPI. The Meta AI app's two public client tokens are built in; `--import-tokens` overrides them if Meta rotates them.
- **Installer**: a release zip with `Install.cmd` and `Uninstall.cmd`. It finds OBS, requires version 32.x with its FFmpeg libraries and the Visual C++ runtime, waits for OBS to be closed (it never closes OBS), copies the plugin (elevating only that step when OBS is in a protected folder such as Program Files) and the setup tool, and offers the setup. Uninstall elevates the same way and still removes the setup tool when OBS is already gone.
- **Setup tool** `meta-g-link-probe.exe`: `--provision`, `--link`, a 60-second `--stream` check, `--forget`, and Bluetooth radio diagnostics.
- **Robustness**: if the glasses refuse this PC's owner key (another PC or the phone took them over), the source stops with *Failed* and a log line asking for the setup again instead of retrying forever (not yet exercised with glasses owned elsewhere). Start and stop never block OBS's video thread; teardown runs in the background and module unload waits for it. One source drives the glasses at a time (others show *Waiting*). The worker reconnects with a 2–15 s backoff, catches every exception at its thread boundary, reports *Streaming* only once frames arrive, sends a keepalive and logs the glasses' stream-stop reasons.
- **Quality gates**: MSVC `/W4 /WX`, strict clang-tidy, Cppcheck, jscpd, Gitleaks, Semgrep, npm audit and OSV-Scanner, four CTest suites (including 68 protocol cases) and Pester suites, and 20 green → red → restored drills.

### Changed

- The glasses host the Wi-Fi network instead of joining one. With the glasses joining an access point (a Windows hotspot pinned to 2.4 GHz, or a dual-band home router), video stopped 8–54 s into every run regardless of bitrate, adapter channel-width setting, audio or Bluetooth setting. See [the Wi-Fi link report](reports/WIFI_LINK_VERIFICATION.md).
- The endurance target is now one full glasses battery charge with no drop or reconnect, measured in minutes, instead of 120 minutes, because streaming appears to drain the battery in roughly 35–40 minutes.

### Fixed

- A reconnect crashed in C++/WinRT activation; the factory cache is now cleared before the apartment closes.
- A wedged Windows Wi-Fi call could hang the worker; WinRT waits are bounded.
- The Wi-Fi passphrase generator ignored a random-number failure and could yield a weak passphrase; it now uses the system CSPRNG and stops on failure.
- An exception on the media worker thread would have terminated OBS.
- The HEVC SPS parser could shift a 32-bit value by 32 bits on malformed input.
- `datax::Decoder` left a fragment length uninitialized.
- JSON parsing is limited to 128 nesting levels.
- A decoder that fails to open is reported instead of ignored.

### Known issues

- Battery: one reading suggests about 35–40 minutes of streaming per charge (unconfirmed).
- While the camera runs, its Wi-Fi adapter is on the glasses' network, which has no internet. Keeping a second Wi-Fi adapter online at the same time is untested.
- Only 720 × 1280 is verified; the other quality profiles are untested. Glasses audio is not implemented.
- Reconnection after a real dropped link is implemented but not yet exercised on hardware.
- The PC does not yet verify the glasses' trust signature.
- Keyboard, Narrator, theme and scaling checks of the source properties have not been done.
- The binaries are not code-signed, so SmartScreen may warn.

[Unreleased]: https://github.com/RandyNorthrup/meta-g-link-direct/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/RandyNorthrup/meta-g-link-direct/releases/tag/v0.1.0
