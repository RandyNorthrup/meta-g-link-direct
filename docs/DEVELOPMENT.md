# Development

How to build, check and package Meta G Link Direct from source. Project rules are in [AGENTS.md](../AGENTS.md); the product contract is [PROJECT_BRIEF.md](../PROJECT_BRIEF.md); tasks, evidence and open gates are in [PLAN.md](../PLAN.md).

## Toolchain

The build is verified with this toolchain on Windows 11 Pro x64 build 26200:

| Tool | Version |
|---|---|
| Visual Studio 2022 C++ build tools (MSVC) | 19.44.35222.0, toolset 14.44.35207 |
| Windows SDK | 10.0.26100.0 |
| CMake / Ninja | 3.31.3 / the Ninja bundled with Visual Studio |
| LLVM (clang-format, clang-tidy) | 22.1.1 |
| PowerShell | 7.6.6 (the installer itself runs on Windows PowerShell 5.1) |
| Python, tar | Python 3.14.0 and the Windows `tar` (used by `bootstrap`) |
| Pester / PSScriptAnalyzer | 5.7.1 / 1.25.0 |
| Node / npm | 24.20.0 / 11.19.0 (development tools only; nothing ships) |
| Gitleaks, Semgrep, actionlint, zizmor, OSV-Scanner, pre-commit | 8.30.1, 1.177.0, 1.7.12, 1.25.2, 2.6.0, 4.5.1 |
| OBS Studio (runtime for the module test) | 32.2.2 (also checked: 33.0.0-rc2) |

`dependencies.lock.json` pins the downloadable inputs. `bootstrap` downloads the OBS source, the Microsoft Edge WebView2 SDK 1.0.2903.40 (NuGet package; headers and the static loader for the setup window) and the Cppcheck 2.22.0 installer into the ignored `.deps/` and `.tools/` and checks each against its SHA-256 hash, builds the OBS development SDK (OBS's own build then fetches its prebuilt dependency package), and restores the locked npm packages. Nothing is installed globally. The other tools must already be on `PATH`; a missing tool fails its gate.

To check the plugin against another OBS version without installing it, extract that version's official Windows zip and run `out\windows\mgl-module-tests.exe out\windows\meta-g-link-direct.dll <obs>\bin\64bit`; for a full load, add an empty `obs_portable_mode.txt` to the extracted folder, put the plugin in `<obs>\plugins\meta-g-link-direct\` (OBS 33 layout) and start it with `--multi` so it runs beside another OBS.

The runner needs the installed OBS runtime for the module test. OBS is found at its per-user location (`%LOCALAPPDATA%\Programs\obs-studio`); for another location set `MGL_OBS_RUNTIME` to that installation's `bin\64bit` folder before running any task, including `bootstrap`.

## Tasks

Run everything through the task runner from the repository root, in PowerShell 7:

```powershell
pwsh -NoProfile -File scripts/Invoke-Project.ps1 bootstrap   # once
pwsh -NoProfile -File scripts/Invoke-Project.ps1 build
pwsh -NoProfile -File scripts/Invoke-Project.ps1 test
pwsh -NoProfile -File scripts/Invoke-Project.ps1 quality     # every gate, tests and red drills
pwsh -NoProfile -File scripts/Invoke-Project.ps1 package     # quality, then the release zip
```

The runner enters the Visual Studio developer environment only when needed. Every native failure is propagated as a task failure.

| Task | What it runs |
|---|---|
| `configure`, `build`, `typecheck` | CMake configure and the MSVC build (`/W4 /WX`, C files also `/analyze`) |
| `format`, `format:check` | clang-format on owned `.c`, `.cpp` and `.hpp` files |
| `lint` | `lint:cpp` (clang-tidy over the compilation database), `lint:powershell` (PSScriptAnalyzer), `lint:workflow` (actionlint, zizmor) |
| `deadcode` | `deadcode:cpp` (Cppcheck) and `deadcode:copies` (jscpd, zero-clone threshold) |
| `security:audit` | `security:secrets` (Gitleaks over the working tree), `security:sast` (Semgrep), `security:npm`, `security:osv`, then Gitleaks over the full Git history |
| `test` | CTest (frame prefix, private address, protocol, OBS module ABI) and Pester (runner, CLI and installer contracts) |
| `test:asan` | All CTest suites in a separate AddressSanitizer build |
| `test:live` | A live uncached Bluetooth service query (needs a Bluetooth radio) |
| `test:red` | All 25 green → red → exact-restoration → green drills over the gates; `-Drill changed` runs only the drills whose inputs changed, `-Drill <id>` one drill |
| `quality` | `configure`, `build`, `format:check`, `lint`, `deadcode`, `security:audit`, `test`, `test:live`, then the drills whose inputs changed |
| `package` | `quality` with every drill, then `dist/meta-g-link-direct-<version>-windows-x64.zip` and its `.sha256` file |

A drill proves that its gate still fails on a planted defect, so it only needs to run again when something that decides that gate's result changed. Each drill's inputs are the file it mutates, `scripts/Invoke-Project.ps1` and its gate's configuration (`$drillGateInputs` in the runner, for example `.clang-tidy` for `lint:cpp` or the CMake files and `tests/` for `test`). `reports/red-drills.json` records each passing drill with a SHA-256 over those inputs. `quality` reruns a drill when that fingerprint differs or no passing result is recorded, and prints `UNCHANGED drill <id>` for each of the others, then a run/unchanged count. Tool versions installed on the PC are not part of the fingerprint; `package` reruns every drill before a release.

`package` stages the user bundle: `Install.cmd`, `Uninstall.cmd`, `Install-MetaGLinkDirect.ps1`, `plugin\meta-g-link-direct.dll`, `plugin\locale\en-US.ini`, `tools\meta-g-link-setup.exe`, `tools\meta-g-link-probe.exe`, `README.txt`, `LICENSE` and `THIRD_PARTY_NOTICES.md`. `Install-MetaGLinkDirect.ps1 -WhatIf` lists the installer's actions without changing anything (it does not show the elevation prompt, which it skips). The installer takes every system folder from Windows (`Get-KnownFolder`) and never assumes a drive letter; `tests/installer.Tests.ps1` fails on a drive-letter path in it. To try an OBS on another drive, map a spare letter to a folder with a portable OBS (`subst M: <folder>`, removed with `subst M: /D`).

The GitHub workflow (`.github/workflows/quality.yml`) builds the portable protocol core on Linux with clang under ASan and UBSan, scans the history with Gitleaks and audits the npm development tools. The Windows, OBS and hardware checks run locally.

## Layout

| Path | Contents |
|---|---|
| `include/mgl/`, `src/protocol/` | Protocol: SDU framing, protobuf, DataX, stream and Wi-Fi link messages, HEVC helpers, JSON, encodings and the playout clock that times bursty media (portable, built on Linux in CI), and AirShield (`airshield.cpp`, built with the Windows library because it uses CNG) |
| `src/windows/` | CNG crypto, DPAPI identity and token storage, Meta setup over WinHTTP and the ownership change shared by both setup programs (`setup.cpp`), RFCOMM and TCP transports, the Wi-Fi client connection, the demand-driven media session, UTF-8/UTF-16 conversion (`win_text.cpp`), radio diagnostics |
| `src/plugin/` | OBS glue in C17 (`module.c` with the Tools menu entry, `source.c` with the camera and microphone sources, `link_settings.c` for the settings shared by all glasses sources, `battery_status.c` for the battery file and the obs-websocket vendor), the C++20 process-wide glasses link (`glasses_link.cpp`, `glasses_link.h`), the video pacer that decodes and presents each picture on time (`video_pacer.cpp`), and the FFmpeg HEVC and AAC decoders (`hevc_decoder.cpp`, `aac_decoder.cpp`, with shared handles in `ffmpeg_handles.hpp`) |
| `src/setup/` | `meta-g-link-setup.exe`: the setup window (Win32 and WebView2) |
| `src/probe/` | `meta-g-link-probe.exe`: `--provision`, stream check and diagnostics |
| `tests/` | Native test executables and Pester suites |
| `scripts/` | `Invoke-Project.ps1` (task runner) and `Install-MetaGLinkDirect.ps1` (end-user installer) |
| `packaging/` | Files added to the release bundle |
| `assets/` | README artwork |
| `reports/` | Hardware observations and gate evidence referenced by `PLAN.md` |

Only the C files include OBS headers: an OBS header needs the MSVC `_udiv128` intrinsic, which clang cannot compile (LLVM issue 59168). The C++ glasses link sits behind the small C interface in `src/plugin/glasses_link.h`, so clang-tidy covers all C++ code. Nothing replaces OBS declarations with a private ABI.

The glasses accept one session, so the plugin holds one `MediaSession` for the whole OBS process. Each glasses source registers with the link and says whether it is live and wants video or audio; the link turns that into a `MediaDemand` (video, audio, keep the link up, video profile from the camera source that went live first, Wi-Fi adapter). The session connects when the demand wants the link and, when the demand changes, stops the stream, configures video and audio again and starts it inside the same session. With video turned off in `ConfigureVideoStreamRequest`, the glasses stream audio alone and keep the camera off. A changed Wi-Fi adapter releases the link and reconnects with the new adapter. The setup window holds the link while it runs, because it needs the glasses' Bluetooth session itself.

The glasses' radio delivers 30 fps in bursts of four or five frames ([report](../reports/FRAME_RATE_VERIFICATION.md)). On the media worker, `mgl::playout::PlayoutClock` gives every video and audio unit a presentation time: its capture time on the PC's clock plus an adaptive delay (20–500 ms). Audio is decoded there and handed to OBS stamped with that time. Video units go to the session's `VideoPacer`, which decodes each one on its own thread 15 ms before it is due. The C side turns each presentation offset into a timestamp on OBS's clock, and OBS's default buffered async mode shows each picture on its frame.

## Protocol overview

0. **Framing, both links**: each AirShield frame's plaintext is padded with 1–16 bytes of (0xC0 + count). `datax::Decoder` strips the pad and splits DataX messages; the glasses fill frames without regard to message boundaries, so a frame may end inside a payload or a header, and the decoder finishes either from the next frame.
1. **Bluetooth**: `openBondedMetaRfcomm` connects to the paired glasses' Meta RFCOMM service. `GlassesSession` runs the AirShield handshake (ECDH P-256, HKDF-SHA256, AES-256-CBC with HMAC-SHA256 per frame), proves the owner key (svc36 trust) and ends link setup.
2. **Wi-Fi**: svc10 `CreateSoftApNetwork` (0x1000) asks the glasses to host a 2.4 GHz / 20 MHz network; `SoftApNetworkInfo` (0x1001) returns its address and port. `connectWifiStation` joins it with the adapter chosen in the source properties (`listWifiAdapters`; empty means the first adapter) and a temporary Native Wi-Fi profile, then waits for a DHCP address. If this step or the media connection fails, the glasses' network is closed over Bluetooth.
3. **Media**: a second `GlassesSession` over TCP; svc28 starts the livestream app; svc51350 configures and starts the stream from the current `MediaDemand`, and stops, reconfigures and restarts it when the demand changes. svc28 device-status pushes (types 2005–2044) carry the battery report (`stream::parseBatteryState`). Video arrives as HEVC chunks (types 10 and 12) and audio as AAC chunks (types 9 and 11); each reassembled unit starts with an 8-byte little-endian presentation time (`stream::splitTimedUnit`). The glasses send raw AAC-LC with no AudioSpecificConfig, so `MediaSession` hands the decoder `stream::requestedAudioSpecificConfig()` for the format it asked for. svc100 serves as a keepalive.
4. **Decode and present**: each unit is timed by `PlayoutClock`. FFmpeg (the `avcodec`/`avutil` that ship with OBS) decodes video on the pacer thread to I420 for `obs_source_output_video`, and audio to float planar samples for `obs_source_output_audio`, both stamped with their presentation time on OBS's clock.

The message numbers and fields for step 2 come from the Meta AI app's `com.oculus.atc` protocol classes; the rest follows Visor. See [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md) and [the Wi-Fi link report](../reports/WIFI_LINK_VERIFICATION.md).

## Setup tool commands

`meta-g-link-setup.exe` is the setup window that the installer and the OBS *Set up glasses…* button open. It hosts Meta's login in WebView2 (private mode, a browser profile of that run only, deleted afterwards), recognises the final redirect to `LoginRedirectUri` in `NavigationStarting`, cancels it and takes the code from its query (`queryParameter`), then runs `setup::changeOwner` on a worker thread. It exits 0 when the owner changed, 8 when the user cancelled and 3 when the setup failed; the window itself says which step stopped.

`meta-g-link-probe.exe --help` lists every command and the diagnostic exit codes; setup commands also return 1 on an unexpected error.

| Command | Purpose |
|---|---|
| `--auth-url` | Print the Meta login address used by `--provision` |
| `--provision` | Read the login code on stdin and make this PC the glasses' owner (the Meta AI app loses the glasses) |
| `--import-tokens` | Seal Meta app tokens from stdin; they override the built-in ones |
| `--link` | Open the authenticated Bluetooth session and report the stage reached |
| `--stream` | Run the OBS source's media session for 60 s; status on stderr, summary on stdout |
| `--forget` | Delete this PC's stored identity and imported tokens |
| `--handshake` | AirShield handshake only, without the owner key or any ownership change |
| `--discover`, `--discover-classic`, `--connect-classic`, `--discover-classic-live`, `--connect-classic-live`, `--pair-classic`, `--direct-classic-uuid`, `--direct-classic-channel7` | Bluetooth radio diagnostics; they make no Meta login and record no identifiers |

Setting `MGL_HTTP_DEBUG` (to any value) traces the Meta setup requests by URL, field names, reply status, reply keys and Meta's error message; it never prints tokens or receipts.

## Analysis decisions

- clang-tidy uses the strict preset with one style conflict disabled (`llvm-prefer-static-over-anonymous-namespace`, which contradicts the retained `misc-use-anonymous-namespace`). Line-scoped annotations cover the C++/WinRT projection headers that LLVM cannot map back to the public SDK, and each unavoidable C ABI cast at a Windows API boundary.
- Cppcheck runs on an analysis copy of the compilation database with MSVC `-external:I` paths normalized and the compiler builtins added. `.cppcheck-obs.cfg` models the eleven entry points OBS loads dynamically (eight module functions and three locale functions). Suppressed: system-header and third-party `.deps/` notices, and four inline `constParameterCallback` findings in `src/plugin/source.c` (three `obs_source_info` callbacks and one `obs_property_clicked_t` button callback), whose `void *data` parameter type OBS fixes.
- Two line-scoped `gitleaks:allow` markers in `include/mgl/provision.hpp` cover the Meta AI app's public client tokens (AGENTS.md revision 3).

## Releasing

Releases follow AGENTS.md: they are tagged and published only after `package` passes.

1. Update `CHANGELOG.md` and the version in `CMakeLists.txt`.
2. Run `package` and check the zip and `.sha256` file under `dist/`.
3. Tag `v<version>`, push, and create the GitHub release with both files attached, marked Latest. Below 1.0 the release title and notes say beta; it is not flagged as a GitHub pre-release, because the repository page lists only a Latest release.
