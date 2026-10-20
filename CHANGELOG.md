# Changelog

All notable changes are recorded here. The project follows [semantic versioning](https://semver.org/); versions below 1.0 are beta releases.

## [0.3.2] - 2026-10-20 (beta)

### Fixed

- **Glasses audio was never recorded or streamed from camera sources created with 0.1.0.** The camera source had no audio in 0.1.0, so OBS saved it with every audio track switched off and restores that on each load: glasses audio moved the mixer meter but reached no recording or stream. When OBS loads such a source, the plugin now turns on OBS's default tracks once and logs it; later track choices are kept. Verified in OBS 33.0.0-rc2 on a source from 0.1.0.
- **A frame boundary inside a message header garbled the stream.** The glasses pack messages into encrypted frames without regard to message boundaries. When a frame ended inside a message's header, the decoder lost its place until it happened to realign: the log filled with messages of nonsense types and the video nearly froze (4.5 pictures per second for several seconds in one recording). The decoder now carries the start of a header into the next frame. A new test splits a message stream at every byte position (194 of 637 positions failed before the fix), and a new red drill (24 in total) guards it. The decoder followed Visor's splitter at the revision this project ported; Visor's current splitter treats the frames as one stream in the same way.

### Measured

- **Glass-to-OBS latency: about 0.4 s** (0.36–0.45 s in five readings of an on-screen timer the glasses filmed), within the 0.5 s target. About 0.3 s of it is the playout delay of that run.
- **Audio/video sync: within one frame** (about ±33 ms) in a clap test, so no Sync Offset is needed.
- **Reconnecting:** after the glasses are powered on, the plugin connects them itself, without pressing Connect in Windows, and Windows' audio devices for the glasses come up with it. After a lost media session, video returned 21.5 s later.

See [the latency and sync report](reports/LATENCY_AND_SYNC_VERIFICATION.md).

## [0.3.1] - 2026-10-20 (beta)

### Fixed

- **OBS showed the glasses' video at about 13 fps instead of 30**, at every quality setting. The glasses encode a true 30 fps, but their radio delivers the frames in bursts: four or five at once, then 110–160 ms of nothing, whatever the bitrate and whatever Wi-Fi setting was tried on the PC or the glasses. The plugin handed each frame to OBS as it arrived, in OBS's unbuffered mode, which keeps only the newest frame per rendered frame and dropped most of every burst. The plugin now presents each frame at its own capture cadence after a short delay that covers the bursts, as the Meta AI app's own player does. That delay adapts to the link, between 20 and 500 ms, and was 188 ms in the test. Audio uses the same clock, so the two stay in step. Measured in OBS 33.0.0-rc2: 12.8 distinct glasses pictures per second in a recording before, 29.6 after. See [the frame-rate report](reports/FRAME_RATE_VERIFICATION.md).
- Video and audio units the decoders reject are now counted and logged; they were silently dropped before.

### Added

- Once a minute while video flows, the OBS log reports the frame rate received from the glasses and shown, the presentation delay, late frames, and undecodable or skipped frames, for example "Video over the last 60 s: 30.0 fps from the glasses, 30.0 fps shown, presentation delay 188 ms, 2 late, 0 undecodable, 0 skipped."

- README: a new 30 fps clip recorded in OBS with 0.3.1 (920 distinct pictures in 30.8 s), and the full 30-second clip as an MP4, both with the audio removed.
- README: **Hear OBS in your glasses**. OBS's audio monitor can use the glasses' speakers (the *Headphones* Bluetooth device) while their camera streams. Verified in OBS 33.0.0-rc2 with 0.3.0: a *Monitor Only* source played steadily in the glasses, not from the PC speakers, while OBS held 30 fps with 0 of 2,689 frames skipped; a plain player's tone stayed continuous and the stream check got its full 30 fps. Media apps playing through the glasses are paused by the glasses while their camera streams. No plugin change.

### Changed

- CI: `actions/checkout` 7.0.1 and `actions/setup-node` 7.1.0 (Dependabot #1 and #2), still pinned by commit SHA; both SHAs match the official release tags. The `workflow-security` red drill now unpins whichever commit `actions/checkout` is pinned to, instead of one hard-coded SHA, so a pin bump no longer leaves it without a target (it refused to run after #1, as designed).
- A new red drill (`playout-cadence`, 23 in total) proves that the tests fail if frames are presented on arrival again.

### Known issues

- The presentation delay adds about 0.2 s of latency; glass-to-OBS latency and audio/video sync have not been measured.

## [0.3.0] - 2026-10-19 (beta)

### Added

- **Microphone source "Ray-Ban Meta Glasses Microphone (Direct)"**: the glasses' microphones (48 kHz stereo) as an OBS audio source of their own, usable without the camera. While only microphone sources are live, the glasses stream audio alone and the camera and its light stay off (seen on the glasses). Verified in OBS 33.0.0-rc2: with the glasses connected and the camera off, audio flowed 0.5 s after the scene switch (mixer peak −26.7 dBFS); switching from a camera scene to a microphone-only scene, the stream changed to audio alone in 1.5 s (peak −12.9 dBFS), with no video requested.
- **Keep the glasses connected while OBS is open** (off by default): the glasses connect when OBS loads and stay connected with the camera off, so a glasses source going live starts in about a second instead of about 13 (measured in OBS 33.0.0-rc2: video 0.9 s and microphone audio 0.5 s after the scene switch). It uses the glasses' battery and keeps the chosen Wi-Fi adapter on the glasses' network, with no internet, until OBS closes.
- **Setup window** `meta-g-link-setup.exe`: explains the change of owner, shows Meta's login inside the window (Microsoft Edge WebView2, private mode, its browser data deleted afterwards), notices the end of the login by itself and makes this PC the owner. Nothing is copied or pasted. Without WebView2 it opens the login in the browser and offers a box for the final address. The installer and the new **Set up glasses…** button in OBS open it.
- **Glasses settings in OBS**: every glasses source's properties group the settings into *Video*, *Audio*, *Glasses connection* (shared by all glasses sources: Wi-Fi adapter, keep connected, status) and *Glasses setup* (Set up glasses…, and Forget these glasses behind a confirmation tick). Shared settings are saved in the plugin's own configuration and copied to every glasses source. **Tools → Ray-Ban Meta Glasses (Direct)** opens them, adding a camera source to the current scene first if there is none.
- **Battery level**: the status shows the glasses' battery percentage and whether they are charging, from the glasses' device-status reports, and the OBS log records each change (for example "Glasses battery: 57%, not charging (3.73 V, 30.2 C)").

### Changed

- **One connection for all glasses sources.** The plugin keeps one link to the glasses for the whole OBS process and asks it for what the live sources need. Several glasses sources can be live at once; they show the same video, and the camera source that went live first sets the quality. Quality and audio changes now apply inside the running session (stream stop, new configuration, start) instead of at the next start; changing the Wi-Fi adapter reconnects. Measured in OBS 33.0.0-rc2: 720 × 1280 → 504 × 896 in 1.4 s, audio on in 1.0 s, back to 720 × 1280 in 1.4 s, with no reconnect. 504 × 896 is now verified in OBS too.
- The camera starts sooner. The media connection now opens as soon as this PC has its address on the glasses' network, instead of after a fixed 1.5 s pause, and retries on a short, growing delay. The Wi-Fi join is checked every 100 ms instead of every 500 ms. Measured with the stream check: the media connection took 1.0 s after the join, down from 2.7 s. In OBS the first frame now comes about 13 s after the scene goes live.
- The OBS log shows how long each connection step took, for example "Joined the glasses' Wi-Fi network after 8.7 s (associated in 3273 ms, address in 737 ms)". A start from the connected, camera-off state reports its own time.
- The installer opens the setup window instead of asking for the login address in its console, and no longer asks for a typed confirmation: the window's first page explains the change and offers Continue or Cancel. `meta-g-link-probe.exe --provision` and the window share one implementation of the ownership change.
- `quality` reruns only the red drills whose inputs (the file the drill mutates, the task runner and the gate's configuration) changed since their recorded pass, and lists the others as unchanged; `package` and `test:red` still run all of them. `reports/red-drills.json` records each drill's inputs and their SHA-256. A new drill (22 in total) proves that a changed input forces a rerun.
- UTF-8 and UTF-16 conversion for Windows calls lives in one place (`win_text`) instead of five copies.
- The README shows a short clip from the glasses and screenshots of the setup window and the OBS properties.

### Fixed

- The Wi-Fi join could count an address the adapter still held from its previous network as the glasses' address. The media connection then aimed at the wrong network and waited about 21 s for each attempt. The join now waits for a DHCP address on the glasses' subnet, and each TCP connection attempt gives up after 3 s.

### Known issues

- Battery use with only the microphone source live has not been measured.
- The PC still does not verify the glasses' trust signature. The glasses sign with RSA-4096, and the exact verification the Meta app performs is not known yet.
- Audio and video sync has not been measured; OBS's *Sync Offset* can adjust it.
- OBS 33 is verified with release candidate 33.0.0-rc2; the final 33.0 release has not been tested yet.

## [0.2.0] - 2026-10-19 (beta)

### Added

- **Optional glasses audio**: a *Glasses microphone audio* setting (off by default) sends the glasses' microphones to OBS as the source's audio, 48 kHz stereo. The source now appears in the OBS audio mixer, silent while the setting is off. The glasses send raw AAC-LC with no AudioSpecificConfig, so the decoder is configured from the requested format; a configuration the glasses do send replaces it. If the glasses refuse audio, the camera continues with video only. Verified in OBS 32.2.2: decoded at 48 kHz stereo, peak −15 dBFS on OBS's mixer meter, 0 render frames skipped in 2,973.
- **OBS 33 support**: OBS 33 loads third-party plugins from a new folder layout and OBS 34 stops loading the old `obs-plugins` folder. Verified with OBS 33.0.0-rc2 installed on the development PC: the plugin loaded from the new layout, the Wi-Fi adapter list worked, and video and audio streamed (30 fps, mixer peak −29.7 dBFS, no render frames skipped while streaming). Its FFmpeg (`avcodec-62`, `avutil-60`) matches this build.
- The installer finds OBS on any drive: besides the OBS installer's registry entry, Program Files and the per-user Programs folder, it now checks a running OBS (for one extracted anywhere) and every Steam library folder, and says which one it uses when it finds several. Verified with a portable OBS 33 on a mapped drive letter: found while running, installed into, and loaded the plugin.
- `CONTRIBUTING.md`, `SECURITY.md` (private vulnerability reporting is now on) and `SUPPORT.md`; README sections for help, contributing and support; sponsor and donate links.
- Download and star badges in the README.

### Changed

- The installer takes ProgramData, Program Files and the user's local application data folder from Windows rather than from environment variables, so it follows the folders Windows and OBS actually use on any drive.
- The installer puts the plugin in `%ProgramData%\obs-studio\plugins\meta-g-link-direct`, the location OBS recommends, in both layouts OBS reads there: the flat layout that OBS 33 and later load, and `bin\64bit` for OBS 32, which OBS 33 skips. Upgrading OBS needs no reinstall, and OBS 32 and 33 can share a PC. A portable OBS gets the layout of its version inside its own folder. The installer removes copies an earlier install left elsewhere, including 0.1.0's copy in the old `obs-plugins` folder. It accepts OBS 32 and newer and notes when a version is newer than the ones this build was checked with. Uninstall also removes the ProgramData copy when OBS itself is gone.
- `MediaSession` takes one `MediaOptions` value (video profile, audio, Wi-Fi adapter). Video and audio share one parser for the glasses' timed access units, and both decoders share FFmpeg handle wrappers.
- The README no longer uses emoji.
- Releases are published as regular GitHub releases marked Latest, labelled beta, so the repository page lists them; 0.1.0 was relabelled the same way.

### Known issues

- Audio and video sync has not been measured; OBS's *Sync Offset* can adjust it.
- OBS 33 is verified with release candidate 33.0.0-rc2; the final 33.0 release has not been tested yet.

## [0.1.0] - 2026-10-19 (beta)

First public build. Tested on one Windows 11 PC (OBS 32.2.2) with one pair of Ray-Ban Meta Gen 2 glasses.

### Added

- **OBS source "Ray-Ban Meta Glasses (Direct)"**: live 720 × 1280 HEVC video at 30 fps, decoded with the FFmpeg libraries that ship with OBS. It starts when its scene goes live and stops when the scene leaves the program output. The properties offer a quality list, a Wi-Fi adapter choice (Automatic, or a named adapter; an unplugged choice fails with a log line instead of switching adapters) and a status line. Verified in OBS 32.2.2: two minutes at 30 fps; across start, stream and stop OBS skipped 1 render frame in 9,794, and that skip happened before the scene went live.
- **Direct link with no phone and no network changes**: an encrypted Bluetooth session (AirShield) authenticates with the owner key; the glasses then host their own 2.4 GHz / 20 MHz Wi-Fi network (svc10 `CreateSoftApNetwork`), the PC joins it with a temporary Windows Wi-Fi profile, and video arrives over a second encrypted session on TCP. Verified on hardware: about 179 s of video with no stall in a test harness and about 104 s in the shipped media session, back-to-back stop/start cycles, and a 4.1 s network join.
- **One-time Meta setup** that makes the PC the glasses' owner (Meta login, ownership exchange). The owner key is sealed with Windows DPAPI. The Meta AI app's two public client tokens are built in; `--import-tokens` overrides them if Meta rotates them.
- **Installer**: a release zip with `Install.cmd` and `Uninstall.cmd`. It finds OBS, requires version 32.x with its FFmpeg libraries and the Visual C++ runtime, waits for OBS to be closed (it never closes OBS), copies the plugin (elevating only that step when OBS is in a protected folder such as Program Files) and the setup tool, and offers the setup. Uninstall elevates the same way and still removes the setup tool when OBS is already gone.
- **Setup tool** `meta-g-link-probe.exe`: `--provision`, `--link`, a 60-second `--stream` check, `--forget`, and Bluetooth radio diagnostics.
- **Robustness**: if the glasses refuse this PC's owner key (another PC or the phone took them over), the source stops with *Failed* and a log line asking for the setup again instead of retrying forever (not yet exercised with glasses owned elsewhere). Start and stop never block OBS's video thread; teardown runs in the background and module unload waits for it. One source drives the glasses at a time (others show *Waiting*). The worker reconnects with a 2–15 s backoff, catches every exception at its thread boundary, reports *Streaming* only once frames arrive, sends a keepalive and logs the glasses' stream-stop reasons.
- **Quality gates**: MSVC `/W4 /WX`, strict clang-tidy, Cppcheck, jscpd, Gitleaks, Semgrep, npm audit and OSV-Scanner, four CTest suites (including 68 protocol cases) and Pester suites, and 20 green → red → restored drills (21 from 0.2.0, which adds installer tests).

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
- Only 720 × 1280 is verified; the other quality profiles are untested. Glasses audio is not implemented (added in 0.2.0).
- Reconnection after a real dropped link is implemented but not yet exercised on hardware.
- The PC does not yet verify the glasses' trust signature.
- Keyboard, Narrator, theme and scaling checks of the source properties have not been done.
- The binaries are not code-signed, so SmartScreen may warn.

[Unreleased]: https://github.com/RandyNorthrup/meta-g-link-direct/compare/v0.2.0...HEAD
[0.2.0]: https://github.com/RandyNorthrup/meta-g-link-direct/releases/tag/v0.2.0
[0.1.0]: https://github.com/RandyNorthrup/meta-g-link-direct/releases/tag/v0.1.0
