# Certification matrix

"Certified" in this project means this table: each gate from `AGENTS.md`, per platform, with its evidence or a stated gap. No outside body certifies anything, and one platform's result says nothing about another's.

Version 0.5.0, 2026-11-01. One pair of Ray-Ban Meta Gen 2 glasses. Details: [`CROSS_PLATFORM_VERIFICATION.md`](CROSS_PLATFORM_VERIFICATION.md).

| Gate | Windows 11 x64 | Ubuntu 26.04, native OBS | Linux, OBS Flatpak | macOS, Intel | macOS, Apple Silicon |
|---|---|---|---|---|---|
| **Requirements readiness** | Pass ([brief](../PROJECT_BRIEF.md)) | Pass (D26) | Pass (D26) | Pass (D27) | Pass (D27) |
| **Build and unit tests** | Pass: MSVC `/W4 /WX`, 7 of 7 CTest suites | Pass: warnings as errors, 6 of 6 | Pass: built in the SDK, warnings as errors, 6 of 6 | Pass: warnings as errors, 6 of 6 | Pass in CI: arm64 build against OBS 33.0.1, 6 of 6 |
| **Other software gates** | Pass: the `package` gate ran format, clang-tidy, Cppcheck, jscpd, Gitleaks, Semgrep, npm and OSV audits, workflow linters, Pester, ASan and all 27 red drills | Shared gates cover the shared code. Also built on Arch Linux with OBS 33.0.1, FFmpeg 9 and GCC 16: 6 of 6. **Gap:** clang-tidy isn't in the gate for POSIX-only sources; a first run on 11 files gave about 160 open findings, mostly strict rules against C APIs (curl, GLib, `fcntl`). No sanitizer run | Same as native | Shared gates cover the shared code. **Gap:** no clang-tidy on `.mm` files; no sanitizer run | Same as Intel |
| **Glasses proof** | Pass on the released zip: owner change, video in 16.3–16.5 s, audio, battery | Pass on the released bundle: handshake, owner change, video in 17.2 s, audio, battery | Pass on the released bundle: same, with its own identity, video in 15.9 s | Pass on the released bundle: 5 of 5 starts, video in 14.7–36.2 s. Earlier builds that day: 16 of 17 first try, the other recovered by itself | **Not tested** |
| **OBS behaviour** | Pass on 33.0.1; earlier on 32.2.2. Non-blocking start and stop, quality in 1.2 s, two adapters with a missing-adapter error | Pass on 33.0.0: quality, audio, battery request, stop returns to the home network. **Gap:** OBS 32, keep-connected, microphone source, long run | Pass on 33.0.1: same. **Gap:** same | Pass on 33.0.1: quality in 1.2–1.3 s, audio, battery file and request, 3 minutes at 30.0 fps from the glasses and 30.1 shown, 1 render frame skipped of 8,547. **Gap:** OBS 32, keep-connected, microphone source | **Not tested** |
| **Installer** | Pass: real install on OBS 33.0.1 | Pass: dry run, install, uninstall, build-fit check not triggered | Pass: permissions added and taken back | Pass: install, permission reset and questions. **Gap:** a download with its quarantine mark | **Not tested** |
| **Setup window** | Pass: full login in WebView2, browser fallback | Opens with its text and buttons. **Gap:** a full login through the window; `--provision` was used | Opens from the Flatpak, 640 × 760. **Gap:** as native | Opens at 640 × 788, centred. **Gap:** a full login through the window | **Not tested** |
| **Accessibility** | Partial: keyboard and names through UI Automation at 100%. **Gap:** high contrast, 150–200% scaling, Narrator by ear | **Not reviewed** | **Not reviewed** | **Not reviewed** | **Not reviewed** |
| **Latest firmware** | Firmware 20.3.6 recorded. **Gap:** whether it is the latest needs the phone app | Same glasses | Same glasses | Same glasses | Same glasses |
| **Full-charge endurance** | **Not run.** Longest run 3 min 14 s | **Not run.** Longest run about 1.5 min | **Not run.** Longest run about 1.5 min | **Not run.** Longest run about 4 min | **Not run** |

**Released builds.** The glasses results above were run on the files from the GitHub release, downloaded and checked against their SHA-256 files, except Apple Silicon, which has no glasses run. The installed plugin was compared byte for byte with the bundle's on Linux and macOS.

## Also open, on every platform

- The glasses' own identity signature isn't verified during the handshake.
- The trip back to the Meta AI phone app hasn't been tried.
- Walking out of range and back hasn't been tried; a lost media session recovered in 21.5 s on Windows.
- A second Wi-Fi adapter that stays online during a stream hasn't been tried; each of two adapters carried the stream on Windows with the other left alone.
- CPU, memory and thermal limits over a long run, and a long-run latency percentile.
- Why the glasses mostly didn't answer on the Mac before they were charged. Unproven; see [the report](CROSS_PLATFORM_VERIFICATION.md#the-failed-runs-before-charging).
- Releases aren't code-signed or notarized.
