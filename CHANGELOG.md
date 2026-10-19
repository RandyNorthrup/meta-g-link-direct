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
- Ported protocol stack (C++20): strict protobuf, DataX header/channel-registry/frame-splitter, AirShield KDF and per-frame AES-256-CBC + HMAC-SHA-256 cipher, handshake messages, channel binding and svc36 trust, with Windows CNG crypto. Independent protocol/DataX/protobuf/encoding test vectors (42 cases) alongside the existing suites.
- Synchronous `GlassesSession` over a whole-SDU transport: AirShield handshake, trust with the owner key, EndLinkSetup, and a DataX receive loop. Proven on hardware — `--handshake` reaches key derivation and `--link` reaches READY with trust success in both directions, reproducibly.
- Meta-authenticated change of owner: OIDC browser login to the FRLA, the AR-universe user-token exchange, and the svc36 change-owner flow (`--provision`). Verified on hardware: this computer now holds the glasses owner key, sealed locally with DPAPI. Tokens and identity never enter the repo, logs or diagnostics.
- A TCP transport and the svc51350 stream control. First verified end to end (`--stream`) with a Windows-hosted network the glasses joined, delivering live 720x1280 HEVC frames at ~30 fps; that network has since been replaced (see Removed).
- HEVC Annex-B helpers (frame/keyframe detection, SPS picture size) used to confirm real decoded camera frames.
- The OBS camera source "Ray-Ban Meta Glasses (Direct)": FFmpeg HEVC decode to I420, a quality list and a live status line in its properties. Verified live in OBS 32.2.2 at 720x1280.
- `scripts/Install-MetaGLinkDirect.ps1`, an end-user installer for a setup bundle (Windows PowerShell 5.1, `-WhatIf`, `-Uninstall`). It never closes OBS and elevates only the plugin copy. Verified by real installs on the development PC. The `package` task does not yet build that bundle layout.
- `--forget` deletes this PC's sealed glasses identity and imported tokens.
- The Meta AI app's two public client tokens are built in as defaults (AGENTS.md revision 3), so setup needs no token step; `--import-tokens` still overrides them.
- The glasses host their own Wi-Fi network for the camera stream: svc10 `CreateSoftApNetwork` (0x1000) on 2.4 GHz at 20 MHz, channel 6, with the Windows region as country code, answered by `SoftApNetworkInfo` (0x1001). Message numbers and fields come from the Meta AI app's `com.oculus.atc` protocol classes. A Native Wi-Fi client connection (`connectWifiStation`) joins the PC to that network with a temporary profile. Verified on hardware: 180 s (5,370 frames) and 120 s runs through the production media session with no stall, two back-to-back stop/start cycles, and two minutes live in OBS 32.2.2 at 30 fps with no render frames skipped through start, stream and stop.
- Protocol tests for the glasses-hosted network messages (26 new cases, 68 in that suite), with a port-field red drill.

### Changed

- OBS headers are now included only by C17 files (`module.c`, `source.c`, both under `/analyze`). The camera core is C++20 behind `src/plugin/camera.h`, so clang-tidy covers all C++.
- Starting and stopping the camera no longer blocks OBS's video thread. A stopped capture is cut off from OBS at once and tears the link down on a background thread; module unload waits up to ten seconds for it. The previous build stalled OBS rendering for 29.7 s in one observed scene switch.
- One process-wide radio lease: a second camera source waits ("Waiting") instead of idling forever.
- The media worker now:
  - reconnects with a 2–15 s backoff that resets after video flows;
  - detects a dropped link immediately;
  - stops promptly during every wait;
  - reports "Streaming" only once frames arrive;
  - logs a stream configuration the glasses reject.
- The camera no longer uses a PC-hosted network. With the glasses joining an access point (Windows Mobile Hotspot pinned to 2.4 GHz, or a dual-band home router), video stopped 8–54 s into every run, whatever the bitrate (600 kbit/s to 4 Mbit/s), adapter channel-width setting, audio or Bluetooth setting. On the glasses' own network the stream holds. See `reports/WIFI_LINK_VERIFICATION.md`.
- If the PC cannot join the glasses' network or open the media connection, the glasses' network is closed over Bluetooth, which the glasses acknowledge. A failed join also deletes the temporary Windows Wi-Fi profile. After a stream the glasses have left Bluetooth and close the network themselves within about 15 s (measured); a close sent over the Wi-Fi link was never acknowledged, so none is sent.
- The media status log no longer includes the glasses' network address.
- `--stream` runs the same media session as the OBS source instead of a separate copy.
- `MGL_HTTP_DEBUG` prints field names, reply keys and status only, never token or receipt values.
- The strict clang-tidy, Cppcheck and jscpd gates pass on all new code. Duplicate helpers were merged: little-endian writing, the Meta RFCOMM UUID (four copies), C++/WinRT Bluetooth names, and session receive handling.

### Removed

- The Windows Mobile Hotspot and Wi-Fi Direct group owners, the svc10 `JoinAccessPoint` path, and the probe's `--wifi` and `--wifi-join` diagnostics. None of them carried a stream past a minute, and the hotspot path rewrote the user's Mobile Hotspot settings.

### Fixed

- The Wi-Fi passphrase came from `CoCreateGuid` bytes with the call's failure ignored, so a failure produced an all-`a` passphrase. It is now 20 symbols drawn uniformly from the system CSPRNG, and an RNG failure stops the start.
- Any exception on the media worker thread (a CNG failure, for example) would have terminated OBS. The worker now catches it at its boundary, logs it and retries.
- The SPS parser could shift a 32-bit value by 32 bits (undefined behaviour) on malformed input. Out-of-range picture sizes are now rejected.
- `datax::Decoder` left a fragment length uninitialised.
- JSON parsing is now limited to 128 nesting levels, so hostile input cannot overflow the stack.
- A decoder that fails to open is reported, not ignored.

### Known gaps

- The PC acknowledges the glasses' trust message without verifying its signature. The Bluetooth bond and the private network passphrase limit the exposure, but verification against the glasses' device-certificate key is still to be built and proven with hardware.
- While the camera runs, the PC's Wi-Fi adapter is on the glasses' network, which has no internet. A PC online only through that adapter is offline until the camera stops.
- A glasses health reading that looks like battery fell about 4 points in 90 s of streaming (unconfirmed), which would limit an unpowered session to roughly 35–40 minutes.

### Engineering decisions

- Use C17/MSVC for OBS ABI glue after real clang-cl build failures exposed unsupported `_udiv128`; retain C++20 for reusable code.
- Normalize Cppcheck's MSVC compilation database and model real dynamic OBS entry points. Keep analysis limitations explicit.
- Preserve original failed canary evidence, correct the AWS canary to the scanner's documented base32 pattern, and retain exact-restoration checks.

Direct authentication, the owner change, the glasses-hosted Wi-Fi network, live 720x1280 HEVC camera frames and the OBS source are proven on hardware. Remaining work:

- reconnection after a real link drop on hardware;
- the non-default quality presets;
- a Narrator and keyboard pass over the source properties;
- latency measurement;
- latest-firmware proof;
- glasses trust-signature verification;
- the 120-minute acceptance run.

The earlier foundation ZIP is historical and does not contain the new CLI or protocol stack.

Ownership note: provisioning moved the glasses' owner key to this PC, so the iPhone Meta AI app is no longer the owner until the glasses are re-added there.
