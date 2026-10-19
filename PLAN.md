# Delivery plan

## Contract and scope

The [project brief](PROJECT_BRIEF.md) is authoritative for the product. This plan covers the Windows 11 x64 OBS plugin, built for Randy first and published for other users as beta releases (brief D10, D17). The structured ledger at the end owns task and verification states; each state cites evidence under `reports/`.

## Milestones

| Milestone | State | Proven | Open |
|---|---|---|---|
| M0 · TASK-SETUP | Verified | Native build and quality foundation, portable frame-prefix parser, OBS module ABI test, gate drills | — |
| M1 · TASK-AUTH | In progress | AirShield session over Bluetooth with trust success; Meta owner change; DPAPI-sealed identity; Forget | Trust-signature verification; return-to-phone behaviour |
| M2 · TASK-MEDIA | In progress | Glasses-hosted 2.4 GHz / 20 MHz link; live 720 × 1280 HEVC at 30 fps; runs of about 179 s and 120 s with no stall; stop/start cycles; optional glasses audio (raw AAC-LC 48 kHz stereo, decoded) | Lower profiles; latency, CPU, memory and thermal measurement; audio/video sync; firmware build |
| M3 · TASK-OBS | In progress | Installed source live in OBS 32.2.2 for two minutes at 30 fps with no render frames skipped by the source; non-blocking start and stop; installer; Wi-Fi adapter choice, including the explicit failure and Bluetooth close for a missing adapter; glasses audio in the OBS mixer (peak −15 dBFS); installed from the ProgramData plugin folder in both layouts; OBS 33.0.0-rc2 installed, loading the new layout, streaming video and audio; installer finds and installs into an OBS on another drive letter | Reconnection after a real drop; the final OBS 33.0 release; the owner-key refusal path; quality changes in OBS; two Wi-Fi adapters at once; keyboard, Narrator, theme and scaling; the full-charge endurance run |

Endurance (brief D16): stream from a full glasses battery to the low-battery shutdown with no drop or reconnect and record the measured minutes. This replaced the 120-minute minimum on 2026-10-19 because a glasses health reading that looks like battery fell about 4 points in 90 s (unconfirmed), which suggests about 35–40 minutes per charge.

## Scan and reuse decisions

Reuse Visor's protocol and session behaviour at commit `e7077b4051b1e3e605c667aa2a010fbe41f346a0` with AGPL-3.0 attribution, and OBS's published module API with the pinned 32.2.2 development inputs. Do not create private OBS ABI declarations or a fake camera source. Rescan the canonical paths before every change.

- Wi-Fi link (2026-10-18): the glasses-hosted network extends the canonical paths. `src/protocol/stream.cpp` gains `CreateSoftApNetwork`/`SoftApNetworkInfo` beside the stream messages; `MediaSession` keeps its worker, lease, reconnect loop, stream setup and keepalive and swaps only the network step; `randomWifiCredentials` remains the single CSPRNG credential generator. One new responsibility, `src/windows/wifi_station.cpp`, joins a network as a Native Wi-Fi client. The Mobile Hotspot and Wi-Fi Direct owners, the `JoinAccessPoint` messages and the probe's `--wifi`/`--wifi-join` were deleted rather than kept as a fallback: no remaining consumer, none carried a stream past a minute, and the hotspot path rewrote the user's Mobile Hotspot settings.
- Wi-Fi adapter choice (2026-10-19): `wifi_station.cpp` gains `listWifiAdapters` and an adapter id on `connectWifiStation` instead of a second Wi-Fi component; `MediaSession` takes the id; the plugin exposes it through the existing C boundary (`mglWifiAdapterList`, `mglCameraSetWifiAdapter`) and the existing properties pattern used for quality. An id that matches no adapter fails explicitly (no silent fallback). The owner-key check lives in `MediaSession`, not `GlassesSession`, because the ownership setup needs sessions that reach Ready with the key refused.
- Glasses audio (2026-10-19): extends the existing stream path rather than adding a second session. `stream::splitTimedUnit` replaces the video-only presentation-time parser for both media; `MediaSession` gains audio reassembly and codec-configuration routing beside video; `MediaOptions` replaces the growing constructor argument list; `AacDecoder` sits beside `HevcDecoder`, both on shared FFmpeg handle wrappers (`ffmpeg_handles.hpp`) instead of duplicated cleanup. Audio is a setting on the camera source, not a second source, because the glasses accept one session (brief D18).
- Plugin location (2026-10-19): the installer's single layout function (`Get-PluginLayout`) chooses the folders per OBS version and portability (both layouts in ProgramData); the same copy/remove step and elevation path serve every layout (brief D19).
- Drive independence (2026-10-19): every system folder in the installer comes from one helper over the Windows known-folder call (`Get-KnownFolder`), the call OBS itself uses for ProgramData, instead of environment variables or drive letters. `Find-ObsInstallation` stays the single OBS lookup and gains a running OBS (`Get-RunningObsRoot`) and Steam's library list (`Get-SteamLibraryFolder`); registry reads share `Get-RegistryText`. `tests/installer.Tests.ps1` loads the installer's functions from its syntax tree, so the tests exercise the shipped code without a test switch in it.
- Community files (2026-10-19): the funding links come from the account's `.github` repository (`FUNDING.yml`), so this repository adds no copy; it adds `CONTRIBUTING.md`, `SECURITY.md` and `SUPPORT.md`, which that repository does not provide.
- Release packaging (2026-10-19): `package` stages exactly the bundle layout that `Install-MetaGLinkDirect.ps1` already installs from (`Build-ReleaseBundle`), instead of the CMake install layout that no installer consumed. The installer's copy and remove steps share one elevation path (`Invoke-PluginFileStep`), which also fixes uninstall for OBS under Program Files.

## Architecture and inputs

C++20 owned code; C17 for the two files that include OBS headers; CMake 3.31.3; MSVC 19.44.35222.0 (toolset 14.44.35207) with Windows SDK 10.0.26100.0; LLVM 22.1.1 for formatting and C++ analysis. The protocol core stays portable; transports, identity, Wi-Fi and the media session live under `src/windows/`; OBS is a thin consumer behind `src/plugin/camera.h`. `dependencies.lock.json` pins the downloadable inputs; the OBS 32.2.2 source tar SHA-256 is `ec81fb66b03e75ddb3076b576f62679c39262e0e9960cef3e17a40dc5d68e6b4`. References: [OBS template](https://github.com/obsproject/obs-plugintemplate/tree/3e7d7ac3b5342cd7d9b88890b9c70b472d1520fc), [OBS 32.2.2](https://github.com/obsproject/obs-studio/tree/ba2f32bdf791005443988a4955e963663e16b1ed), [source API](https://docs.obsproject.com/reference-sources).

## Gate policy and evidence

`scripts/Invoke-Project.ps1` exposes format, format:check, lint, typecheck, deadcode, security:audit, test, test:red, build, quality and package. A missing or unsupported tool is a failed gate, never a silent success. Native exit codes propagate. Drills record baseline, intended failure and exact restoration. The maintained set runs on affected changes and before each release; `package` runs `quality` first.

Detailed logs belong under `reports/` (indexed in `reports/README.md`). Requirements, software checks, glasses proof, OBS behaviour, accessibility, firmware compatibility and endurance stay separate statuses.

## Open verification

Reconnection after a real drop; the final OBS 33.0 release once it is out; trust-signature verification; lower quality profiles and quality changes in OBS; latency, CPU, memory and thermal measurement; audio/video sync and long-run audio; the keyboard, Narrator, theme and scaling pass; the firmware build; confirming the battery reading; and the full-charge endurance run. While streaming, the PC's Wi-Fi adapter has no internet (brief D15).

Deferred: code signing and provenance, a published SBOM and native advisory assessment, Windows jobs in hosted CI, and other platforms or glasses generations.

## Analysis decisions

- Both languages build with MSVC because CMake rejects a mixed clang-cl/MSVC project and OBS's `util_uint64.h` needs `_udiv128`, which clang-cl does not implement ([LLVM issue 59168](https://github.com/llvm/llvm-project/issues/59168)). No compiler macro is spoofed and no private OBS declaration is introduced.
- clang-tidy disables only `llvm-prefer-static-over-anonymous-namespace`, which contradicts the retained `misc-use-anonymous-namespace`. Line-scoped `misc-include-cleaner` annotations cover C++/WinRT projection headers that LLVM 22 cannot map back to the public SDK (in `src/windows/discovery.cpp` and `src/windows/classic.cpp`). Line-scoped cast annotations cover exact-signature `GetProcAddress` conversions in `tests/module_tests.cpp` and each unavoidable C ABI cast at a Windows API boundary. Revisit when the SDK or LLVM changes.
- Cppcheck runs on an analysis copy of the compilation database with MSVC `-external:I` paths normalized and the compiler builtins added. `.cppcheck-obs.cfg` models the six OBS entry points OBS loads dynamically. Only `missingIncludeSystem` notices and findings under `.deps/` are suppressed; owned header and unused-function findings fail.
- The first synthetic secret canary used the wrong alphabet; its miss is preserved in `reports/red-secret-canary-initial.txt` and the generator was corrected without changing scanner rules.
- The GitHub workflow builds the portable protocol core on Linux with clang under ASan and UBSan; Windows ASan runs locally (`test:asan`). TSan, fuzzing and a coverage floor are not established.
- OSV and npm audit cover the locked npm development packages only. The pinned native OBS archives have integrity and version evidence, not an advisory assessment.

## Ledger

```quality-ledger
{
  "schema_version": 1,
  "work": {
    "id": "MGL-DIRECT",
    "title": "Direct Gen 2 glasses source for personal Windows OBS use",
    "scope_revision": 3,
    "brief": "PROJECT_BRIEF.md",
    "brief_reason": null,
    "rules": [
      {
        "path": "AGENTS.md",
        "revision": "2"
      }
    ],
    "inputs": [
      "dependencies.lock.json",
      "package-lock.json",
      "CMakePresets.json"
    ],
    "environment": {
      "tools": {
        "python": "3.14.0",
        "cmake": "3.31.3",
        "llvm": "22.1.1",
        "powershell": "7.6.6",
        "gitleaks": "8.30.1",
        "node": "24.20.0",
        "npm": "11.19.0",
        "semgrep": "1.177.0",
        "actionlint": "1.7.12",
        "zizmor": "1.25.2",
        "osv_scanner": "2.6.0",
        "pester": "5.7.1",
        "psscriptanalyzer": "1.25.0",
        "jscpd": "5.4.1",
        "precommit": "4.5.1",
        "msvc": "19.44.35222.0",
        "cppcheck": "2.22.0"
      },
      "platform": "windows"
    }
  },
  "requirements": [
    {
      "id": "REQ-SETUP",
      "statement": "Provide a reproducible native Windows project foundation with honest boundaries and enforceable quality gates.",
      "priority": "critical",
      "acceptance": [
        "AC-FRAMING",
        "AC-MODULE",
        "AC-GATES"
      ],
      "superseded_by": null
    },
    {
      "id": "REQ-AUTH",
      "statement": "Establish a real authenticated direct Windows connection to Gen 2; Meta setup is allowed when required and streaming sessions should use local links.",
      "priority": "critical",
      "acceptance": [
        "AC-AUTH"
      ],
      "superseded_by": null
    },
    {
      "id": "REQ-MEDIA",
      "statement": "Receive and decode real video locally with adjustable quality and optional glasses audio on latest stable firmware.",
      "priority": "critical",
      "acceptance": [
        "AC-MEDIA"
      ],
      "superseded_by": null
    },
    {
      "id": "REQ-OBS",
      "statement": "Deliver an accessible responsive native OBS source and prove streaming for one full glasses battery charge with no drop or reconnect, recorded in measured minutes.",
      "priority": "critical",
      "acceptance": [
        "AC-OBS"
      ],
      "superseded_by": null
    }
  ],
  "acceptance": [
    {
      "id": "AC-FRAMING",
      "requirement": "REQ-SETUP",
      "given": "Pinned Visor framing rules and independent boundary/invalid prefix examples",
      "when": "The portable frame-prefix parser is built and its tests and mutation drill run",
      "then": "DataX uses four bytes plus declared length; AirShield uses ten bytes plus 16 times block count plus one; incomplete and invalid prefixes remain distinct; the intended defect is rejected.",
      "checks": [
        "behavior",
        "red"
      ],
      "manual_reason": null
    },
    {
      "id": "AC-MODULE",
      "requirement": "REQ-SETUP",
      "given": "OBS 32.2.2 headers/import libraries and installed matching runtime",
      "when": "The native module is built and loaded in an isolated test harness",
      "then": "The DLL resolves against the real OBS runtime and exports valid module metadata; no camera source or hardware success is falsely registered.",
      "checks": [
        "behavior",
        "red"
      ],
      "manual_reason": null
    },
    {
      "id": "AC-GATES",
      "requirement": "REQ-SETUP",
      "given": "Owned source, scripts, configuration, dependency pins and no real credentials",
      "when": "The quality runner and maintained red drills execute",
      "then": "Configured applicable checks fail on intended violations and restore cleanly; missing tools or unproven checks are explicit deferred gates, never reported as green.",
      "checks": [
        "behavior",
        "red"
      ],
      "manual_reason": null
    },
    {
      "id": "AC-AUTH",
      "requirement": "REQ-AUTH",
      "given": "Real Gen 2 glasses with latest stable firmware and Windows wireless hardware",
      "when": "Setup completes, phone is powered off, a new session starts and another starts after glasses restart",
      "then": "Authenticated device information is read directly on PC; local session proof and exact firmware are recorded; Meta setup credentials are protected and phone recovery is documented.",
      "checks": [
        "manual"
      ],
      "manual_reason": "Requires physical glasses, user-controlled login/ownership steps and radio behavior; mocks cannot establish this contract."
    },
    {
      "id": "AC-MEDIA",
      "requirement": "REQ-MEDIA",
      "given": "A proved direct authenticated session and latest stable firmware",
      "when": "A real camera stream and optional glasses audio are received and measured",
      "then": "Decode 720\u00d71280 nominal 30fps and at least one lower profile; target p95 glass-to-PC latency \u2264500ms without unbounded buffering; audio can remain disabled and its optional enable path is verified.",
      "checks": [
        "behavior",
        "red",
        "manual"
      ],
      "manual_reason": "Decoder/parser automated evidence must be complemented by actual camera, timing and audio observations."
    },
    {
      "id": "AC-OBS",
      "requirement": "REQ-OBS",
      "given": "A proved direct media path, OBS 32.2.2 on this PC and external audio normally selected",
      "when": "User pairs/connects, starts/stops, adjusts quality, toggles glasses audio, reconnects and streams from a full glasses battery until the low-battery shutdown",
      "then": "Native OBS source remains stable on latest stable firmware; startup \u226415s after ready connection, reconnect \u226430s after availability; no growing queue; keyboard/Narrator/theme checks pass at 480px+ and 100\u2013200% scaling; phone stays off during streaming.",
      "checks": [
        "behavior",
        "red",
        "manual"
      ],
      "manual_reason": "Requires a real OBS scene, actual glasses, assistive technology, visual inspection and a timed full-charge run."
    }
  ],
  "tasks": [
    {
      "id": "TASK-SETUP",
      "purpose": "M0: establish the native build, protocol foundation, diagnostics, module skeleton, rules and repeatable gates",
      "acceptance": [
        "AC-FRAMING",
        "AC-MODULE",
        "AC-GATES"
      ],
      "depends_on": [],
      "changes": [
        {
          "path": "AGENTS.md",
          "action": "create"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "action": "create"
        },
        {
          "path": ".gitignore",
          "action": "create"
        },
        {
          "path": "CMakeLists.txt",
          "action": "create"
        },
        {
          "path": "CMakePresets.json",
          "action": "create"
        },
        {
          "path": ".clang-format",
          "action": "create"
        },
        {
          "path": ".clang-tidy",
          "action": "create"
        },
        {
          "path": ".gitleaks.toml",
          "action": "create"
        },
        {
          "path": ".semgrep.yml",
          "action": "create"
        },
        {
          "path": ".pre-commit-config.yaml",
          "action": "create"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "action": "create"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "action": "create"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "action": "create"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "action": "create"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "action": "create"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "action": "create"
        },
        {
          "path": "src/probe/main.cpp",
          "action": "create"
        },
        {
          "path": "src/plugin/module.c",
          "action": "create"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "action": "create"
        },
        {
          "path": "tests/module_tests.cpp",
          "action": "create"
        },
        {
          "path": "dependencies.lock.json",
          "action": "create"
        },
        {
          "path": "LICENSE",
          "action": "create"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "action": "create"
        },
        {
          "path": "README.md",
          "action": "create"
        },
        {
          "path": "CHANGELOG.md",
          "action": "create"
        },
        {
          "path": ".github/workflows/quality.yml",
          "action": "create"
        },
        {
          "path": ".github/dependabot.yml",
          "action": "create"
        },
        {
          "path": ".gitattributes",
          "action": "create"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "action": "create"
        },
        {
          "path": ".jscpd.json",
          "action": "create"
        },
        {
          "path": "package.json",
          "action": "create"
        },
        {
          "path": "package-lock.json",
          "action": "create"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "action": "create"
        }
      ],
      "status": "verified",
      "evidence": [
        "EV-SOFTWARE-AGGREGATE",
        "EV-RED-PARSER-LENGTH",
        "EV-RED-MODULE-METADATA",
        "EV-RED-FORMAT",
        "EV-RED-OWNED-HEADER",
        "EV-RED-COMPILER-TYPES",
        "EV-RED-UNUSED-CODE",
        "EV-RED-SCRIPT-ANALYSIS",
        "EV-RED-WORKFLOW-SECURITY",
        "EV-RED-SECRET-SCANNER",
        "EV-RED-UNSAFE-COPY",
        "EV-RED-DUPLICATED-CODE",
        "EV-RED-ADDRESS-SANITIZER",
        "EV-RED-DEPENDENCY-NPM",
        "EV-RED-DEPENDENCY-OSV",
        "EV-RED-WHATIF-GUARD",
        "EV-RED-NATIVE-EXIT-PROPAGATION",
        "EV-DISCOVERY-SOFTWARE",
        "EV-DISCOVERY-GIT-INTEGRITY",
        "EV-DISCOVERY-RED-PARSER-LENGTH",
        "EV-DISCOVERY-RED-MODULE-METADATA",
        "EV-DISCOVERY-RED-PROBE-ARGUMENTS",
        "EV-DISCOVERY-RED-FORMAT",
        "EV-DISCOVERY-RED-OWNED-HEADER",
        "EV-DISCOVERY-RED-COMPILER-TYPES",
        "EV-DISCOVERY-RED-UNUSED-CODE",
        "EV-DISCOVERY-RED-SCRIPT-ANALYSIS",
        "EV-DISCOVERY-RED-WORKFLOW-SECURITY",
        "EV-DISCOVERY-RED-SECRET-SCANNER",
        "EV-DISCOVERY-RED-UNSAFE-COPY",
        "EV-DISCOVERY-RED-DUPLICATED-CODE",
        "EV-DISCOVERY-RED-ADDRESS-SANITIZER",
        "EV-DISCOVERY-RED-DEPENDENCY-NPM",
        "EV-DISCOVERY-RED-DEPENDENCY-OSV",
        "EV-DISCOVERY-RED-WHATIF-GUARD",
        "EV-DISCOVERY-RED-NATIVE-EXIT-PROPAGATION",
        "EV-CLASSIC-SOFTWARE",
        "EV-CLASSIC-RED-PARSER-LENGTH",
        "EV-CLASSIC-RED-MODULE-METADATA",
        "EV-CLASSIC-RED-PROBE-ARGUMENTS",
        "EV-CLASSIC-RED-CLASSIC-CONNECTION-CLAIM",
        "EV-CLASSIC-RED-FORMAT",
        "EV-CLASSIC-RED-OWNED-HEADER",
        "EV-CLASSIC-RED-COMPILER-TYPES",
        "EV-CLASSIC-RED-UNUSED-CODE",
        "EV-CLASSIC-RED-SCRIPT-ANALYSIS",
        "EV-CLASSIC-RED-WORKFLOW-SECURITY",
        "EV-CLASSIC-RED-SECRET-SCANNER",
        "EV-CLASSIC-RED-UNSAFE-COPY",
        "EV-CLASSIC-RED-DUPLICATED-CODE",
        "EV-CLASSIC-RED-ADDRESS-SANITIZER",
        "EV-CLASSIC-RED-DEPENDENCY-NPM",
        "EV-CLASSIC-RED-DEPENDENCY-OSV",
        "EV-CLASSIC-RED-WHATIF-GUARD",
        "EV-CLASSIC-RED-NATIVE-EXIT-PROPAGATION",
        "EV-EXISTING-ADAPTER-SOFTWARE",
        "EV-EXISTING-ADAPTER-RED-PARSER-LENGTH",
        "EV-EXISTING-ADAPTER-RED-MODULE-METADATA",
        "EV-EXISTING-ADAPTER-RED-PROBE-ARGUMENTS",
        "EV-EXISTING-ADAPTER-RED-CLASSIC-CONNECTION-CLAIM",
        "EV-EXISTING-ADAPTER-RED-PRIVATE-ADDRESS-VALIDATION",
        "EV-EXISTING-ADAPTER-RED-UNCACHED-QUERY-LOOP",
        "EV-EXISTING-ADAPTER-RED-FORMAT",
        "EV-EXISTING-ADAPTER-RED-OWNED-HEADER",
        "EV-EXISTING-ADAPTER-RED-COMPILER-TYPES",
        "EV-EXISTING-ADAPTER-RED-UNUSED-CODE",
        "EV-EXISTING-ADAPTER-RED-SCRIPT-ANALYSIS",
        "EV-EXISTING-ADAPTER-RED-WORKFLOW-SECURITY",
        "EV-EXISTING-ADAPTER-RED-SECRET-SCANNER",
        "EV-EXISTING-ADAPTER-RED-UNSAFE-COPY",
        "EV-EXISTING-ADAPTER-RED-DUPLICATED-CODE",
        "EV-EXISTING-ADAPTER-RED-ADDRESS-SANITIZER",
        "EV-EXISTING-ADAPTER-RED-DEPENDENCY-NPM",
        "EV-EXISTING-ADAPTER-RED-DEPENDENCY-OSV",
        "EV-EXISTING-ADAPTER-RED-WHATIF-GUARD",
        "EV-EXISTING-ADAPTER-RED-NATIVE-EXIT-PROPAGATION",
        "EV-EXISTING-ADAPTER-GIT-INTEGRITY"
      ],
      "blocker": null,
      "superseded_by": null
    },
    {
      "id": "TASK-AUTH",
      "purpose": "M1: port discovery, transport, protected identity and authorized Meta ownership setup; prove direct authentication",
      "acceptance": [
        "AC-AUTH"
      ],
      "depends_on": [
        "TASK-SETUP"
      ],
      "changes": [
        {
          "path": "src/windows/session.cpp",
          "action": "create"
        },
        {
          "path": "src/windows/identity.cpp",
          "action": "create"
        },
        {
          "path": "include/mgl/session.hpp",
          "action": "create"
        },
        {
          "path": "src/probe/main.cpp",
          "action": "modify"
        },
        {
          "path": "tests/session_tests.cpp",
          "action": "create"
        },
        {
          "path": "src/windows/discovery.cpp",
          "action": "create"
        },
        {
          "path": "include/mgl/discovery.hpp",
          "action": "create"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "action": "modify"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "action": "modify"
        },
        {
          "path": "CMakeLists.txt",
          "action": "modify"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "action": "modify"
        },
        {
          "path": "tests/probe.Tests.ps1",
          "action": "create"
        },
        {
          "path": "README.md",
          "action": "modify"
        },
        {
          "path": "CHANGELOG.md",
          "action": "modify"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "action": "modify"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "action": "modify"
        },
        {
          "path": ".gitattributes",
          "action": "modify"
        },
        {
          "path": "src/windows/classic.cpp",
          "action": "create"
        },
        {
          "path": "src/windows/apartment.hpp",
          "action": "create"
        },
        {
          "path": "include/mgl/address.hpp",
          "action": "create"
        },
        {
          "path": "src/protocol/address.cpp",
          "action": "create"
        },
        {
          "path": "tests/address_tests.cpp",
          "action": "create"
        },
        {
          "path": "include/mgl/rfcomm.hpp",
          "action": "create"
        },
        {
          "path": "src/windows/rfcomm.cpp",
          "action": "create"
        },
        {
          "path": "src/probe/private_input.cpp",
          "action": "create"
        },
        {
          "path": "src/probe/private_input.hpp",
          "action": "create"
        },
        {
          "path": "tests/probe_support.ps1",
          "action": "create"
        },
        {
          "path": "tests/live.Tests.ps1",
          "action": "create"
        }
      ],
      "status": "in_progress",
      "evidence": [
        "EV-DISCOVERY-CLI-SOFTWARE",
        "EV-DISCOVERY-RED-PROBE-ARGUMENTS",
        "EV-DISCOVERY-HARDWARE-DEFERRED",
        "EV-CLASSIC-SOFTWARE-PARTIAL",
        "EV-CLASSIC-RED-CLASSIC-CONNECTION-CLAIM",
        "EV-CLASSIC-HARDWARE-DEFERRED",
        "EV-EXISTING-ADAPTER-CLI-PARTIAL",
        "EV-EXISTING-ADAPTER-RED-PROBE-ARGUMENTS",
        "EV-EXISTING-ADAPTER-RED-CLASSIC-CONNECTION-CLAIM",
        "EV-EXISTING-ADAPTER-RED-PRIVATE-ADDRESS-VALIDATION",
        "EV-EXISTING-ADAPTER-RED-UNCACHED-QUERY-LOOP",
        "EV-EXISTING-ADAPTER-HARDWARE-DEFERRED"
      ],
      "blocker": "Owner change and authenticated session are proven on hardware; glasses trust-signature verification is not built, so AC-AUTH stays open.",
      "superseded_by": null
    },
    {
      "id": "TASK-MEDIA",
      "purpose": "M2: port local access-point negotiation, frame reception, real media decoding and quality/audio control",
      "acceptance": [
        "AC-MEDIA"
      ],
      "depends_on": [
        "TASK-AUTH"
      ],
      "changes": [
        {
          "path": "src/windows/wifi_station.cpp",
          "action": "create"
        },
        {
          "path": "include/mgl/wifi_station.hpp",
          "action": "create"
        },
        {
          "path": "src/protocol/stream.cpp",
          "action": "modify"
        },
        {
          "path": "include/mgl/stream.hpp",
          "action": "modify"
        },
        {
          "path": "src/windows/media_session.cpp",
          "action": "modify"
        },
        {
          "path": "tests/protocol_tests.cpp",
          "action": "modify"
        },
        {
          "path": "src/windows/wifi_group.cpp",
          "action": "delete"
        },
        {
          "path": "src/windows/wifi_hotspot.cpp",
          "action": "delete"
        },
        {
          "path": "include/mgl/wifi_group.hpp",
          "action": "delete"
        }
      ],
      "status": "in_progress",
      "evidence": [
        "EV-WIFI-LINK-HARDWARE",
        "EV-WIFI-LINK-SOFTWARE",
        "EV-WIFI-LINK-RED-SOFTAP-PORT",
        "EV-AUDIO-HARDWARE"
      ],
      "blocker": null,
      "superseded_by": null
    },
    {
      "id": "TASK-OBS",
      "purpose": "M3: integrate the proved connection as an OBS source and certify long-session/accessibility behavior",
      "acceptance": [
        "AC-OBS"
      ],
      "depends_on": [
        "TASK-MEDIA"
      ],
      "changes": [
        {
          "path": "src/plugin/glasses_source.cpp",
          "action": "create"
        },
        {
          "path": "src/plugin/module.c",
          "action": "modify"
        },
        {
          "path": "data/locale/en-US.ini",
          "action": "create"
        },
        {
          "path": "tests/source_tests.cpp",
          "action": "create"
        }
      ],
      "status": "in_progress",
      "evidence": [
        "EV-WIFI-LINK-OBS",
        "EV-WIFI-LINK-OBS-DEFERRED",
        "EV-AUDIO-HARDWARE",
        "EV-OBS33-LOAD",
        "EV-INSTALLER-DRIVES"
      ],
      "blocker": null,
      "superseded_by": null
    }
  ],
  "evidence": [
    {
      "id": "EV-READINESS-1",
      "acceptance": [
        "AC-AUTH",
        "AC-FRAMING",
        "AC-GATES",
        "AC-MEDIA",
        "AC-MODULE",
        "AC-OBS"
      ],
      "kind": "readiness",
      "status": "stale",
      "command": [],
      "method": "Independent review of confirmed scope, source scan, acceptance coverage, task dependencies, current tool observations and declared hardware/credential limits.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1"
        }
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/readiness.txt",
        "sha256": "b0569cc44129507979954b2b4ea85d5f92678265af6b1ae032274295951893e5"
      },
      "inputs": [
        {
          "path": "AGENTS.md",
          "sha256": "71fbdb1732ce2b68f11b7a92e8d3d6bbdc174885cd1eea36b81b770735936af9"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "11ba1d3922963c5a72e269035c3e6fb03804ef3ac92258b7247f3a222235e380"
        }
      ],
      "scope_sha256": "00703e3105d324d9d00af55d6c853b8730d7cb040a2e695a690b915bcf37013a",
      "red": null,
      "reason": "Rule revision 2, C17/MSVC ABI glue and additional dev-tool/runner-test scope supersede the initial review. Re-review before final aggregate verification."
    },
    {
      "id": "EV-READINESS-2",
      "acceptance": [
        "AC-AUTH",
        "AC-FRAMING",
        "AC-GATES",
        "AC-MEDIA",
        "AC-MODULE",
        "AC-OBS"
      ],
      "kind": "readiness",
      "status": "stale",
      "command": [],
      "method": "Re-review after observed compiler compatibility correction, scoped analysis configuration, added runner tests, source inventory and updated verification boundaries.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/readiness-2.txt",
        "sha256": "77c46bcf7b127e7e47cb3945ccf4fa5c561a23728569af42e7c6a59d258b71c3"
      },
      "inputs": [
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        }
      ],
      "scope_sha256": "bea9f9ba2d7cbe474ed1b216198e3b6f35e9fab8d4f128c7cc6a34c55aa08154",
      "red": null,
      "reason": "TASK-AUTH discovery paths expanded; re-review the direct discovery increment and refresh readiness from a new review."
    },
    {
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "reason": "Discovery increment changed source, diagnostic contract and plan prose; retained as historical evidence. Fresh aggregate/drills and readiness review are required.",
      "id": "EV-SOFTWARE-AGGREGATE",
      "acceptance": [
        "AC-FRAMING",
        "AC-MODULE",
        "AC-GATES"
      ],
      "kind": "behavior",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        ".\\scripts\\Invoke-Project.ps1",
        "package"
      ],
      "method": "Executed the configured quality aggregate and packaging: native build; real-runtime module metadata/ABI load; 83 frame cases; three Pester effects/failure tests; formatting/static/dead-code/security checks; all 16 intended-red/restored-green drills. Independently verified ZIP CRC, x64 PE, packaged/tested DLL equality and unchanged aggregate source fingerprints. No real glasses, source UI or hosted execution is claimed.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/quality-package.txt",
        "sha256": "50b0172b6be808fff7c2b24c1a5d2ad4dc3bf925cd9a2c5928209d566881f148"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": null
    },
    {
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "reason": "Discovery increment changed source, diagnostic contract and plan prose; retained as historical evidence. Fresh aggregate/drills and readiness review are required.",
      "id": "EV-RED-PARSER-LENGTH",
      "acceptance": [
        "AC-FRAMING"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "test"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/foundation-red/parser-length.txt",
        "sha256": "8bc8158e1d561ec3f1f243c3a94dc64ef83a75f8575d93d98e57218710b6900d"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Change DataxHeader from 4 to 5 without modifying the independent expected frame lengths.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "DataX empty has 4 bytes: unexpected prefix status or frame length",
        "observed_diagnostic": "DataX empty has 4 bytes: unexpected prefix status or frame length",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "fcea87b9c7e60b81822b0cadc2362def9e3cd413011a19130199b857b6660278"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      }
    },
    {
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "reason": "Discovery increment changed source, diagnostic contract and plan prose; retained as historical evidence. Fresh aggregate/drills and readiness review are required.",
      "id": "EV-RED-MODULE-METADATA",
      "acceptance": [
        "AC-MODULE"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "test"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/foundation-red/module-metadata.txt",
        "sha256": "567a809d4b60e91d028d2b27f45fa03b77af878d20010737e7a74f778a793a64"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Change the exported module name to Broken module name.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "OBS module metadata does not match development-foundation contract",
        "observed_diagnostic": "OBS module metadata does not match development-foundation contract",
        "before": [
          {
            "path": "src/plugin/module.c",
            "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
          }
        ],
        "mutated": [
          {
            "path": "src/plugin/module.c",
            "sha256": "608f7c48d09ad85b57cce8de7cc22fedeeea6017210dfd2cc37efc9da082004b"
          }
        ],
        "after": [
          {
            "path": "src/plugin/module.c",
            "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
          }
        ]
      }
    },
    {
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "reason": "Discovery increment changed source, diagnostic contract and plan prose; retained as historical evidence. Fresh aggregate/drills and readiness review are required.",
      "id": "EV-RED-FORMAT",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "format:check"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/foundation-red/format.txt",
        "sha256": "2709604244b5ea4ab4da49131d6d193bc361311b575793332a501dea5dee3c3b"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Append an intentionally unformatted function.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp:47:22: error: code should be clang-formatted [-Wclang-format-violations]",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp:47:22: error: code should be clang-formatted [-Wclang-format-violations]",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "c2a1d1a6c8b7489cbb25561dddadb31309d7ac5c15f880bbeb3c90288eabb038"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      }
    },
    {
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "reason": "Discovery increment changed source, diagnostic contract and plan prose; retained as historical evidence. Fresh aggregate/drills and readiness review are required.",
      "id": "EV-RED-OWNED-HEADER",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "lint:cpp"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/foundation-red/owned-header.txt",
        "sha256": "fac6e378d9eeee7b448697bceb5b0cbb8ec96484b25910eea094fa0d11ae0531"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Append a mutable inline global to an owned header.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\include\\mgl/sdu.hpp:23:12: error: variable 'red_drill_global' is non-const and globally accessible, consider making it const [cppcoreguidelines-avoid-non-const-global-variables,-warnings-as-errors]",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\include\\mgl/sdu.hpp:23:12: error: variable 'red_drill_global' is non-const and globally accessible, consider making it const [cppcoreguidelines-avoid-non-const-global-variables,-warnings-as-errors]",
        "before": [
          {
            "path": "include/mgl/sdu.hpp",
            "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
          }
        ],
        "mutated": [
          {
            "path": "include/mgl/sdu.hpp",
            "sha256": "220f387016dea2b2cb31874833a09fd753ad9998873937e5cdfba7cb762fe0ec"
          }
        ],
        "after": [
          {
            "path": "include/mgl/sdu.hpp",
            "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
          }
        ]
      }
    },
    {
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "reason": "Discovery increment changed source, diagnostic contract and plan prose; retained as historical evidence. Fresh aggregate/drills and readiness review are required.",
      "id": "EV-RED-COMPILER-TYPES",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "build"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/foundation-red/compiler-types.txt",
        "sha256": "a7e224c803008efad05581d907b1cf92d64ed5dc2b18ddc13edc3e61594ce36f"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Initialize an integer constant from a string literal.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp(47): error C2440: 'initializing': cannot convert from 'const char [16]' to 'const int'",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp(47): error C2440: 'initializing': cannot convert from 'const char [16]' to 'const int'",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "4d3f32e4a31af074ff96c5ef217d361573917be607688d49acd001562970bc96"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      }
    },
    {
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "reason": "Discovery increment changed source, diagnostic contract and plan prose; retained as historical evidence. Fresh aggregate/drills and readiness review are required.",
      "id": "EV-RED-UNUSED-CODE",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "deadcode:cpp"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/foundation-red/unused-code.txt",
        "sha256": "6644c6b1f7e244c71347df207b9fb8a9df6a4f0dda4a2fbe5ea19cf7198cc0d1"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Append an unreferenced function to owned protocol source.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp:47:5: style: The function 'red_drill_unused_function' is never used. [unusedFunction]",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp:47:5: style: The function 'red_drill_unused_function' is never used. [unusedFunction]",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "56c7aca4962c137d9b9bfddaedb756fb220fc33620f0f3888c0b25de525b780a"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      }
    },
    {
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "reason": "Discovery increment changed source, diagnostic contract and plan prose; retained as historical evidence. Fresh aggregate/drills and readiness review are required.",
      "id": "EV-RED-SCRIPT-ANALYSIS",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "lint:powershell"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/foundation-red/script-analysis.txt",
        "sha256": "5d46cad10861f1ca95791580adc151d2002908cf892a9ef48b9fdb4ea2f95398"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Append a function with an unapproved PowerShell verb.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "PSUseApprovedVerbs Warning  Invoke-Project.ps1 464  The cmdlet 'Badverb-RedDrill' uses an unapproved verb.",
        "observed_diagnostic": "PSUseApprovedVerbs Warning  Invoke-Project.ps1 464  The cmdlet 'Badverb-RedDrill' uses an unapproved verb.",
        "before": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
          }
        ],
        "mutated": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "9c32be0adda020562fa3727ccd34e64553120926c808daa86761731f29dd9bb0"
          }
        ],
        "after": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
          }
        ]
      }
    },
    {
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "reason": "Discovery increment changed source, diagnostic contract and plan prose; retained as historical evidence. Fresh aggregate/drills and readiness review are required.",
      "id": "EV-RED-WORKFLOW-SECURITY",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "lint:workflow"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/foundation-red/workflow-security.txt",
        "sha256": "19e4cb7fa962c8103d15f12ac99efbf79c284e97c1eaf4b251064f3e7892133b"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Replace the pinned checkout commit SHA with a movable version tag.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "error[unpinned-uses]: unpinned action reference",
        "observed_diagnostic": "error[unpinned-uses]: unpinned action reference",
        "before": [
          {
            "path": ".github/workflows/quality.yml",
            "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
          }
        ],
        "mutated": [
          {
            "path": ".github/workflows/quality.yml",
            "sha256": "a151bb006b4d7e858647ffe9c81521c98e2e2d9a3c12cfa60555e229a57afd76"
          }
        ],
        "after": [
          {
            "path": ".github/workflows/quality.yml",
            "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
          }
        ]
      }
    },
    {
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "reason": "Discovery increment changed source, diagnostic contract and plan prose; retained as historical evidence. Fresh aggregate/drills and readiness review are required.",
      "id": "EV-RED-SECRET-SCANNER",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "security:secrets"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/foundation-red/secret-scanner.txt",
        "sha256": "2e1fd97782814a8372e4c997e16aa9dd39e35d50eaadc8590dbfd818b2d7f94c"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Append a randomized base32 AWS-key-shaped synthetic canary; scanner output is redacted.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "1:07PM WRN leaks found: 1",
        "observed_diagnostic": "1:07PM WRN leaks found: 1",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "cf9ff27c94759ecb629d586ecc3b4aa0442274170c2cfd180ceb73276f9f4999"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      }
    },
    {
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "reason": "Discovery increment changed source, diagnostic contract and plan prose; retained as historical evidence. Fresh aggregate/drills and readiness review are required.",
      "id": "EV-RED-UNSAFE-COPY",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "security:sast"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/foundation-red/unsafe-copy.txt",
        "sha256": "c8db8607f34ac8e2a1f449b36873aab4f0f3e17d4ad1a299520171cac8ed5277"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Append an unbounded strcpy call.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "\u276f\u276f\u2771 mgl-unsafe-string-copy",
        "observed_diagnostic": "\u276f\u276f\u2771 mgl-unsafe-string-copy",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "8c973ff6b7b7e0be339225d7c4a427203011da68c5fda6414bea6ec6c0ea0b03"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      }
    },
    {
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "reason": "Discovery increment changed source, diagnostic contract and plan prose; retained as historical evidence. Fresh aggregate/drills and readiness review are required.",
      "id": "EV-RED-DUPLICATED-CODE",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "deadcode:copies"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/foundation-red/duplicated-code.txt",
        "sha256": "34aa70eeccf7c3c218914d604a72805519c10d0ab96c565a3e27d098df8e4895"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Insert a duplicate frame-prefix implementation under another function name.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "ERROR: jscpd found too many duplicates (7.0%) over threshold (0.0%)",
        "observed_diagnostic": "ERROR: jscpd found too many duplicates (7.0%) over threshold (0.0%)",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "21406acfb851f1a583b2a8e742638e4eb5e5f367c7195ef607eb5862fcd8d6df"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      }
    },
    {
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "reason": "Discovery increment changed source, diagnostic contract and plan prose; retained as historical evidence. Fresh aggregate/drills and readiness review are required.",
      "id": "EV-RED-ADDRESS-SANITIZER",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "test:asan"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/foundation-red/address-sanitizer.txt",
        "sha256": "537d18b20f17cd3ebf3c8c1b76c7824c976514202361c4427adbfc4f5470f436"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Inject a runtime heap-buffer read past a one-element allocation in test code.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "==8120==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x120083ca0034 at pc 0x7ff736b753de bp 0x00441939f800 sp 0x00441939f808",
        "observed_diagnostic": "==8120==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x120083ca0034 at pc 0x7ff736b753de bp 0x00441939f800 sp 0x00441939f808",
        "before": [
          {
            "path": "tests/sdu_tests.cpp",
            "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
          }
        ],
        "mutated": [
          {
            "path": "tests/sdu_tests.cpp",
            "sha256": "f0b8a6f71a8f96f071d86024029c536756dd654d9beaea73b9a22d9535147d2e"
          }
        ],
        "after": [
          {
            "path": "tests/sdu_tests.cpp",
            "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
          }
        ]
      }
    },
    {
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "reason": "Discovery increment changed source, diagnostic contract and plan prose; retained as historical evidence. Fresh aggregate/drills and readiness review are required.",
      "id": "EV-RED-DEPENDENCY-NPM",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "security:npm"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/foundation-red/dependency-npm.txt",
        "sha256": "02d6d1b6cfe118449d8746927bd3e88e5cd96acd688c11dee6c8ce730f364c6b"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Temporarily add vulnerable lodash 4.17.20 metadata to the lockfile without installing it.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Command Injection in lodash - https://github.com/advisories/GHSA-35jh-r3h4-6jhm",
        "observed_diagnostic": "Command Injection in lodash - https://github.com/advisories/GHSA-35jh-r3h4-6jhm",
        "before": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ],
        "mutated": [
          {
            "path": "package-lock.json",
            "sha256": "55a94e0ca609f9c43263164d98163a9057daf13c65dd980eccc570ad31f8c11f"
          }
        ],
        "after": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ]
      }
    },
    {
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "reason": "Discovery increment changed source, diagnostic contract and plan prose; retained as historical evidence. Fresh aggregate/drills and readiness review are required.",
      "id": "EV-RED-DEPENDENCY-OSV",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "security:osv"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/foundation-red/dependency-osv.txt",
        "sha256": "0f4ec7667ba7545976e6f4b89b37630ae4c4a9c839353177cbb935a04b1b161b"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Temporarily add vulnerable lodash 4.17.20 metadata to the lockfile without installing it.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "| https://osv.dev/GHSA-35jh-r3h4-6jhm | 8.1  | npm       | lodash (dev) | 4.17.20 | 4.17.21       | package-lock.json |",
        "observed_diagnostic": "| https://osv.dev/GHSA-35jh-r3h4-6jhm | 8.1  | npm       | lodash (dev) | 4.17.20 | 4.17.21       | package-lock.json |",
        "before": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ],
        "mutated": [
          {
            "path": "package-lock.json",
            "sha256": "55a94e0ca609f9c43263164d98163a9057daf13c65dd980eccc570ad31f8c11f"
          }
        ],
        "after": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ]
      }
    },
    {
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "reason": "Discovery increment changed source, diagnostic contract and plan prose; retained as historical evidence. Fresh aggregate/drills and readiness review are required.",
      "id": "EV-RED-WHATIF-GUARD",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "test:runner"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/foundation-red/whatif-guard.txt",
        "sha256": "37f13af9529fe320c7d2027a04ff846aae24a13df23399ae7b1fe1dae8788b24"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Bypass the common ShouldProcess guard; the disposable WhatIf effect assertion must fail.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Expected strings to be the same, but they were different.",
        "observed_diagnostic": "Expected strings to be the same, but they were different.",
        "before": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
          }
        ],
        "mutated": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "f984215ef641a3fd7cbf07a4a75c8a0ca293cc75a2e510623b9872acea52f9ee"
          }
        ],
        "after": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
          }
        ]
      }
    },
    {
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "reason": "Discovery increment changed source, diagnostic contract and plan prose; retained as historical evidence. Fresh aggregate/drills and readiness review are required.",
      "id": "EV-RED-NATIVE-EXIT-PROPAGATION",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\scripts\\Invoke-Project.ps1",
        "test:runner"
      ],
      "method": "Executed as part of the complete maintained drill set inside the successful package aggregate; reviewed the intended diagnostic and independent assertion or scanner mechanism.",
      "exit_code": 0,
      "artifact": {
        "path": "reports/foundation-red/native-exit-propagation.txt",
        "sha256": "75d8d8a9c5714edc20a16f9b9472d43f46e7a57a0288593186708f32cf88c81c"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": {
        "mutation": "Bypass the native exit-code guard; the real formatter-failure assertion must fail.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Expected 0 to be different from the actual value, but got the same value.",
        "observed_diagnostic": "Expected 0 to be different from the actual value, but got the same value.",
        "before": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
          }
        ],
        "mutated": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "6317091c63172acbb754e1704c06ba88dc870ecaea9805583166c6dfdf6eed7f"
          }
        ],
        "after": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
          }
        ]
      }
    },
    {
      "id": "EV-PORTABLE-COVERAGE-DEFERRED",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "behavior",
      "status": "deferred",
      "command": [],
      "method": "Boundary review; this gate has not run and is not counted as a passing check.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/VERIFICATION.md",
        "sha256": "4b1c390345a4e2bb445ffd96faaca26b9b5226190e2368420f9c0023f41965a2"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": null,
      "reason": "Owner: technical implementation. No portable coverage baseline/floor, fuzz or hosted/Linux sanitizer result exists. Next: establish supported portable-core execution before session/media parser certification; gcovr does not cover the current MSVC build. No calendar deadline supplied."
    },
    {
      "id": "EV-NATIVE-AUDIT-DEFERRED",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "behavior",
      "status": "deferred",
      "command": [],
      "method": "Boundary review; this gate has not run and is not counted as a passing check.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/VERIFICATION.md",
        "sha256": "4b1c390345a4e2bb445ffd96faaca26b9b5226190e2368420f9c0023f41965a2"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "ee43d446d24f0d2bdcbed79675723d60ce40dc884f727fe66e93a4adee84fda8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "ab623d802989bc87f67cf8942c822959d46ea34d92d8a9bcedebe1eec456ae76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "ccc5cde490041c4086e9f076482ae31af2334d3d0132162109bb021aed490ae3"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "0244903cb9ac5f49cf5fc7e5c1ea48117c04ee84082d7d61bd1b3d526b5957b6"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "1dd8521c61703b03143386b81923cb8840d4777fdc65d2ae1ce40c63728ea1d6"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "1aa70c0fd03120ae45b53da26fc11f7cd81c6b6255031683e92ce0c7365e0d8e"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "f34458187fdd9585bba2d58ed8d2533d79b0ea45e9c27f3e70141df27a40eb85"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "1799a1e768ac3b121fe93084f901bc869827b63ea3da2111f074f9a56fe50080"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "d7c5a85a690218456d9d9b3b0ce19c4b47d108cba4665fb474343c9af68db6d4"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "4949ca6b56599220219cfe4da1a57f8620b11118cf1c81c80acedd944e6d7b3c",
      "red": null,
      "reason": "Owner: technical/security implementation. npm/OSV cover the locked development packages, not a complete native OBS/dependency component audit. Next: inventory native components and assess advisories before accepting authentication/media dependencies. No calendar deadline supplied."
    },
    {
      "id": "EV-READINESS-DISCOVERY",
      "acceptance": [
        "AC-AUTH",
        "AC-FRAMING",
        "AC-GATES",
        "AC-MEDIA",
        "AC-MODULE",
        "AC-OBS"
      ],
      "kind": "readiness",
      "status": "stale",
      "command": [],
      "method": "Reviewed the concrete read-only discovery increment, failure/lifetime/privacy contract, canonical reuse and real-hardware boundaries. See artifact.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/discovery-readiness.txt",
        "sha256": "d6e208f3fff6c712ca97dd774688a396e6f2b05a4bc28b81550c627c355d57f0"
      },
      "inputs": [
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9f42752e917cd5cf9a77abffaf4ad4648ed79dac4a5354a9f1c287388b3a6f87"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        }
      ],
      "scope_sha256": "1f130947fe316163e31c08c46ac7c1fdb735161898028021bda2f1b2a8e21139",
      "red": null,
      "reason": "Discovery increment changed source, diagnostic contract and plan prose; retained as historical evidence. Fresh aggregate/drills and readiness review are required."
    },
    {
      "id": "EV-READINESS-DISCOVERY-FINAL",
      "acceptance": [
        "AC-AUTH",
        "AC-FRAMING",
        "AC-GATES",
        "AC-MEDIA",
        "AC-MODULE",
        "AC-OBS"
      ],
      "kind": "readiness",
      "status": "stale",
      "command": [],
      "method": "Reviewed current requirements, canonical reuse, native error/lifetime/privacy behavior, SDK analysis boundary, hardware limitations and evidence storage. Readiness only.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/discovery-readiness-final.txt",
        "sha256": "8e94d073aa2e205c586103445b9f9c9858b55453f941924589fd3915c06f297f"
      },
      "inputs": [
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        }
      ],
      "scope_sha256": "9370e4308f66344887276f29998e98d3b1a91edc9647b889dd44f8ddc823690c",
      "red": null,
      "reason": "TASK-AUTH expands to explicit Classic inquiry/service/connection checks and shared apartment lifetime. New readiness review required."
    },
    {
      "id": "EV-DISCOVERY-SOFTWARE",
      "acceptance": [
        "AC-FRAMING",
        "AC-GATES",
        "AC-MODULE"
      ],
      "kind": "behavior",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "quality"
      ],
      "method": "Actual full quality aggregate exited 0: two CTest suites, 3 runner plus 7 CLI Pester tests, configured analysis/security, Windows ASan and 17 defect/restoration drills. The subsequent reports-only Git text-conversion change was separately verified by the Git integrity receipt; executable sources are unchanged. No hardware success or refreshed ZIP claim.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/quality-discovery.txt",
        "sha256": "9bd7cf92572afddc1cbff0e819fdd11b375b5c46c35d8e901e157b49c0cc8750"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "7617422964d1b91ba5e497bd6c1d8fa9e5e9361baec02c04cd81cf7cbe5f6d74",
      "red": null,
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-DISCOVERY-CLI-SOFTWARE",
      "acceptance": [
        "AC-AUTH"
      ],
      "kind": "behavior",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "quality"
      ],
      "method": "Partial enabling evidence for TASK-AUTH only: native diagnostic CLI, association counts and real scan failure reporting. Same full aggregate; not proof of AC-AUTH authenticated device information or phone-off/current-firmware behavior.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/quality-discovery.txt",
        "sha256": "9bd7cf92572afddc1cbff0e819fdd11b375b5c46c35d8e901e157b49c0cc8750"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/discovery.hpp",
          "sha256": "bd5393d7bdd19d2de67308be56c8a7c2f46492607be7b161779176822fc5570f"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "include/mgl/session.hpp",
          "sha256": null
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "src/windows/discovery.cpp",
          "sha256": "adea862c34b41e7877a75e7d4d8f9b2aa0eecb541fdf4ec11994ccae61f9cabc"
        },
        {
          "path": "src/windows/identity.cpp",
          "sha256": null
        },
        {
          "path": "src/windows/session.cpp",
          "sha256": null
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/probe.Tests.ps1",
          "sha256": "f75abfbfd1ab1bc8f263805e87cacb4cf036fec7e69cd7f6f4edde2ccd8c7879"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        },
        {
          "path": "tests/session_tests.cpp",
          "sha256": null
        }
      ],
      "scope_sha256": "1da1f86af911fd12f5f6264eefa89edd8d42c03dd2ecfdc6d793ed3abd7a107f",
      "red": null,
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-DISCOVERY-GIT-INTEGRITY",
      "acceptance": [
        "AC-FRAMING",
        "AC-GATES",
        "AC-MODULE"
      ],
      "kind": "behavior",
      "status": "stale",
      "command": [
        "python",
        "-"
      ],
      "method": "Executed Python stdin verification of git hash-object --no-filters versus --path for every immutable receipt artifact other than this report. All 41 matched. Reports-only text conversion was disabled after the software aggregate; executable source inputs are unchanged.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/evidence-git-integrity.json",
        "sha256": "c141c694d5c04f1435bd7c4fb7cc5dbf99aafe48c40f81ed4396ba7026f501c8"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "7617422964d1b91ba5e497bd6c1d8fa9e5e9361baec02c04cd81cf7cbe5f6d74",
      "red": null,
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-DISCOVERY-RED-PARSER-LENGTH",
      "acceptance": [
        "AC-FRAMING"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "parser-length"
      ],
      "method": "Inspected actual baseline, intended diagnostic and restored run; independently verified exact restoration. Change DataxHeader from 4 to 5 without modifying the independent expected frame lengths.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/discovery-red/parser-length.txt",
        "sha256": "f4b55da49810882c3a99360adcb35a9223960cc304ee3354e076020489ac6bfa"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "7617422964d1b91ba5e497bd6c1d8fa9e5e9361baec02c04cd81cf7cbe5f6d74",
      "red": {
        "mutation": "Change DataxHeader from 4 to 5 without modifying the independent expected frame lengths.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "DataX empty has 4 bytes: unexpected prefix status or frame length",
        "observed_diagnostic": "DataX empty has 4 bytes: unexpected prefix status or frame length",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "fcea87b9c7e60b81822b0cadc2362def9e3cd413011a19130199b857b6660278"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-DISCOVERY-RED-MODULE-METADATA",
      "acceptance": [
        "AC-MODULE"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "module-metadata"
      ],
      "method": "Inspected actual baseline, intended diagnostic and restored run; independently verified exact restoration. Change the exported module name to Broken module name.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/discovery-red/module-metadata.txt",
        "sha256": "d79998c6522d0140b4b0c4111467f5d80b33b9c52fa9eadbfdc56ecd49ba8109"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "7617422964d1b91ba5e497bd6c1d8fa9e5e9361baec02c04cd81cf7cbe5f6d74",
      "red": {
        "mutation": "Change the exported module name to Broken module name.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "OBS module metadata does not match development-foundation contract",
        "observed_diagnostic": "OBS module metadata does not match development-foundation contract",
        "before": [
          {
            "path": "src/plugin/module.c",
            "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
          }
        ],
        "mutated": [
          {
            "path": "src/plugin/module.c",
            "sha256": "608f7c48d09ad85b57cce8de7cc22fedeeea6017210dfd2cc37efc9da082004b"
          }
        ],
        "after": [
          {
            "path": "src/plugin/module.c",
            "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
          }
        ]
      },
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-DISCOVERY-RED-PROBE-ARGUMENTS",
      "acceptance": [
        "AC-AUTH",
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "probe-arguments"
      ],
      "method": "Inspected actual baseline, intended diagnostic and restored run; independently verified exact restoration. Return exit 0 for rejected native probe arguments while preserving independent CLI assertions expecting exit 5.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/discovery-red/probe-arguments.txt",
        "sha256": "a6b9a0ebb1a7c6465e53326e579fb57c32ad052261cfdd1ef86b4bfe2d525f87"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/discovery.hpp",
          "sha256": "bd5393d7bdd19d2de67308be56c8a7c2f46492607be7b161779176822fc5570f"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "include/mgl/session.hpp",
          "sha256": null
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "src/windows/discovery.cpp",
          "sha256": "adea862c34b41e7877a75e7d4d8f9b2aa0eecb541fdf4ec11994ccae61f9cabc"
        },
        {
          "path": "src/windows/identity.cpp",
          "sha256": null
        },
        {
          "path": "src/windows/session.cpp",
          "sha256": null
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/probe.Tests.ps1",
          "sha256": "f75abfbfd1ab1bc8f263805e87cacb4cf036fec7e69cd7f6f4edde2ccd8c7879"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        },
        {
          "path": "tests/session_tests.cpp",
          "sha256": null
        }
      ],
      "scope_sha256": "1da1f86af911fd12f5f6264eefa89edd8d42c03dd2ecfdc6d793ed3abd7a107f",
      "red": {
        "mutation": "Return exit 0 for rejected native probe arguments while preserving independent CLI assertions expecting exit 5.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Expected 5, because invalid probe arguments must fail, but got 0.",
        "observed_diagnostic": "Expected 5, because invalid probe arguments must fail, but got 0.",
        "before": [
          {
            "path": "src/probe/main.cpp",
            "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
          }
        ],
        "mutated": [
          {
            "path": "src/probe/main.cpp",
            "sha256": "6400301fed1c9b2c2098c0fc3df457813b4afbc74d1ece68a075add4ac462dec"
          }
        ],
        "after": [
          {
            "path": "src/probe/main.cpp",
            "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
          }
        ]
      },
      "reason": "This combined AC-AUTH/AC-GATES receipt depends on expanded TASK-AUTH paths. Preserve prior result; rerun after Classic implementation."
    },
    {
      "id": "EV-DISCOVERY-RED-FORMAT",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "format"
      ],
      "method": "Inspected actual baseline, intended diagnostic and restored run; independently verified exact restoration. Append an intentionally unformatted function.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/discovery-red/format.txt",
        "sha256": "2709604244b5ea4ab4da49131d6d193bc361311b575793332a501dea5dee3c3b"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "7617422964d1b91ba5e497bd6c1d8fa9e5e9361baec02c04cd81cf7cbe5f6d74",
      "red": {
        "mutation": "Append an intentionally unformatted function.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp:47:22: error: code should be clang-formatted [-Wclang-format-violations]",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp:47:22: error: code should be clang-formatted [-Wclang-format-violations]",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "c2a1d1a6c8b7489cbb25561dddadb31309d7ac5c15f880bbeb3c90288eabb038"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-DISCOVERY-RED-OWNED-HEADER",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "owned-header"
      ],
      "method": "Inspected actual baseline, intended diagnostic and restored run; independently verified exact restoration. Append a mutable inline global to an owned header.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/discovery-red/owned-header.txt",
        "sha256": "c2473b7818ccdca3d372677f0df20c2d8fd1961b326696dcac1a909f947676c8"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "7617422964d1b91ba5e497bd6c1d8fa9e5e9361baec02c04cd81cf7cbe5f6d74",
      "red": {
        "mutation": "Append a mutable inline global to an owned header.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\include\\mgl/sdu.hpp:23:12: error: variable 'red_drill_global' is non-const and globally accessible, consider making it const [cppcoreguidelines-avoid-non-const-global-variables,-warnings-as-errors]",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\include\\mgl/sdu.hpp:23:12: error: variable 'red_drill_global' is non-const and globally accessible, consider making it const [cppcoreguidelines-avoid-non-const-global-variables,-warnings-as-errors]",
        "before": [
          {
            "path": "include/mgl/sdu.hpp",
            "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
          }
        ],
        "mutated": [
          {
            "path": "include/mgl/sdu.hpp",
            "sha256": "220f387016dea2b2cb31874833a09fd753ad9998873937e5cdfba7cb762fe0ec"
          }
        ],
        "after": [
          {
            "path": "include/mgl/sdu.hpp",
            "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
          }
        ]
      },
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-DISCOVERY-RED-COMPILER-TYPES",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "compiler-types"
      ],
      "method": "Inspected actual baseline, intended diagnostic and restored run; independently verified exact restoration. Initialize an integer constant from a string literal.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/discovery-red/compiler-types.txt",
        "sha256": "a7e224c803008efad05581d907b1cf92d64ed5dc2b18ddc13edc3e61594ce36f"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "7617422964d1b91ba5e497bd6c1d8fa9e5e9361baec02c04cd81cf7cbe5f6d74",
      "red": {
        "mutation": "Initialize an integer constant from a string literal.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp(47): error C2440: 'initializing': cannot convert from 'const char [16]' to 'const int'",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp(47): error C2440: 'initializing': cannot convert from 'const char [16]' to 'const int'",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "4d3f32e4a31af074ff96c5ef217d361573917be607688d49acd001562970bc96"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-DISCOVERY-RED-UNUSED-CODE",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "unused-code"
      ],
      "method": "Inspected actual baseline, intended diagnostic and restored run; independently verified exact restoration. Append an unreferenced function to owned protocol source.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/discovery-red/unused-code.txt",
        "sha256": "15d8d4407e78506c93dc673ea632fb05454034aafc78d5055743ba56b26dd9a2"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "7617422964d1b91ba5e497bd6c1d8fa9e5e9361baec02c04cd81cf7cbe5f6d74",
      "red": {
        "mutation": "Append an unreferenced function to owned protocol source.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp:47:5: style: The function 'red_drill_unused_function' is never used. [unusedFunction]",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp:47:5: style: The function 'red_drill_unused_function' is never used. [unusedFunction]",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "56c7aca4962c137d9b9bfddaedb756fb220fc33620f0f3888c0b25de525b780a"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-DISCOVERY-RED-SCRIPT-ANALYSIS",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "script-analysis"
      ],
      "method": "Inspected actual baseline, intended diagnostic and restored run; independently verified exact restoration. Append a function with an unapproved PowerShell verb.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/discovery-red/script-analysis.txt",
        "sha256": "574acb09fa37c4e046e1ddf80cedb0f053649f1f7dc2d83f7040d03bd76ca97f"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "7617422964d1b91ba5e497bd6c1d8fa9e5e9361baec02c04cd81cf7cbe5f6d74",
      "red": {
        "mutation": "Append a function with an unapproved PowerShell verb.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "PSUseApprovedVerbs Warning  Invoke-Project.ps1 481  The cmdlet 'Badverb-RedDrill' uses an unapproved verb.",
        "observed_diagnostic": "PSUseApprovedVerbs Warning  Invoke-Project.ps1 481  The cmdlet 'Badverb-RedDrill' uses an unapproved verb.",
        "before": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
          }
        ],
        "mutated": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "0ce13f6cb38db07c7f6ded1d6899b533e7eeb5138b0dd6c50a6ecdf531149820"
          }
        ],
        "after": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
          }
        ]
      },
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-DISCOVERY-RED-WORKFLOW-SECURITY",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "workflow-security"
      ],
      "method": "Inspected actual baseline, intended diagnostic and restored run; independently verified exact restoration. Replace the pinned checkout commit SHA with a movable version tag.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/discovery-red/workflow-security.txt",
        "sha256": "19e4cb7fa962c8103d15f12ac99efbf79c284e97c1eaf4b251064f3e7892133b"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "7617422964d1b91ba5e497bd6c1d8fa9e5e9361baec02c04cd81cf7cbe5f6d74",
      "red": {
        "mutation": "Replace the pinned checkout commit SHA with a movable version tag.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "error[unpinned-uses]: unpinned action reference",
        "observed_diagnostic": "error[unpinned-uses]: unpinned action reference",
        "before": [
          {
            "path": ".github/workflows/quality.yml",
            "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
          }
        ],
        "mutated": [
          {
            "path": ".github/workflows/quality.yml",
            "sha256": "a151bb006b4d7e858647ffe9c81521c98e2e2d9a3c12cfa60555e229a57afd76"
          }
        ],
        "after": [
          {
            "path": ".github/workflows/quality.yml",
            "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
          }
        ]
      },
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-DISCOVERY-RED-SECRET-SCANNER",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "secret-scanner"
      ],
      "method": "Inspected actual baseline, intended diagnostic and restored run; independently verified exact restoration. Append a randomized base32 AWS-key-shaped synthetic canary; scanner output is redacted.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/discovery-red/secret-scanner.txt",
        "sha256": "0957f74d758aaea77e980d9bc32d36045ae948404aef1f8d618e5cca90035de3"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "7617422964d1b91ba5e497bd6c1d8fa9e5e9361baec02c04cd81cf7cbe5f6d74",
      "red": {
        "mutation": "Append a randomized base32 AWS-key-shaped synthetic canary; scanner output is redacted.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "4:56PM WRN leaks found: 1",
        "observed_diagnostic": "4:56PM WRN leaks found: 1",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "2d041a3361ab6d524c60a217c4881d9dbe553bd4f7b87574c8d87ef4a4a71dce"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-DISCOVERY-RED-UNSAFE-COPY",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "unsafe-copy"
      ],
      "method": "Inspected actual baseline, intended diagnostic and restored run; independently verified exact restoration. Append an unbounded strcpy call.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/discovery-red/unsafe-copy.txt",
        "sha256": "99f887c35e19935b50a5d4d23c7f2338618679b35e9d216f534d5a2274ccab25"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "7617422964d1b91ba5e497bd6c1d8fa9e5e9361baec02c04cd81cf7cbe5f6d74",
      "red": {
        "mutation": "Append an unbounded strcpy call.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "\u276f\u276f\u2771 mgl-unsafe-string-copy",
        "observed_diagnostic": "\u276f\u276f\u2771 mgl-unsafe-string-copy",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "8c973ff6b7b7e0be339225d7c4a427203011da68c5fda6414bea6ec6c0ea0b03"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-DISCOVERY-RED-DUPLICATED-CODE",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "duplicated-code"
      ],
      "method": "Inspected actual baseline, intended diagnostic and restored run; independently verified exact restoration. Insert a duplicate frame-prefix implementation under another function name.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/discovery-red/duplicated-code.txt",
        "sha256": "a84e87de08359e5a39846eade11df7f3c1ffa402c936f75843fb1392d7ad9f65"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "7617422964d1b91ba5e497bd6c1d8fa9e5e9361baec02c04cd81cf7cbe5f6d74",
      "red": {
        "mutation": "Insert a duplicate frame-prefix implementation under another function name.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Found 1 clones.",
        "observed_diagnostic": "Found 1 clones.",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "21406acfb851f1a583b2a8e742638e4eb5e5f367c7195ef607eb5862fcd8d6df"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-DISCOVERY-RED-ADDRESS-SANITIZER",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "address-sanitizer"
      ],
      "method": "Inspected actual baseline, intended diagnostic and restored run; independently verified exact restoration. Inject a runtime heap-buffer read past a one-element allocation in test code.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/discovery-red/address-sanitizer.txt",
        "sha256": "f26f74cac5eacb68cebac2487f9971eb0c8914f35afc8d1109e084d55317a0ae"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "7617422964d1b91ba5e497bd6c1d8fa9e5e9361baec02c04cd81cf7cbe5f6d74",
      "red": {
        "mutation": "Inject a runtime heap-buffer read past a one-element allocation in test code.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "==3660==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x121cb6ca0034 at pc 0x7ff736c653de bp 0x007bd59ff6e0 sp 0x007bd59ff6e8",
        "observed_diagnostic": "==3660==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x121cb6ca0034 at pc 0x7ff736c653de bp 0x007bd59ff6e0 sp 0x007bd59ff6e8",
        "before": [
          {
            "path": "tests/sdu_tests.cpp",
            "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
          }
        ],
        "mutated": [
          {
            "path": "tests/sdu_tests.cpp",
            "sha256": "f0b8a6f71a8f96f071d86024029c536756dd654d9beaea73b9a22d9535147d2e"
          }
        ],
        "after": [
          {
            "path": "tests/sdu_tests.cpp",
            "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
          }
        ]
      },
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-DISCOVERY-RED-DEPENDENCY-NPM",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "dependency-npm"
      ],
      "method": "Inspected actual baseline, intended diagnostic and restored run; independently verified exact restoration. Temporarily add vulnerable lodash 4.17.20 metadata to the lockfile without installing it.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/discovery-red/dependency-npm.txt",
        "sha256": "02d6d1b6cfe118449d8746927bd3e88e5cd96acd688c11dee6c8ce730f364c6b"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "7617422964d1b91ba5e497bd6c1d8fa9e5e9361baec02c04cd81cf7cbe5f6d74",
      "red": {
        "mutation": "Temporarily add vulnerable lodash 4.17.20 metadata to the lockfile without installing it.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Command Injection in lodash - https://github.com/advisories/GHSA-35jh-r3h4-6jhm",
        "observed_diagnostic": "Command Injection in lodash - https://github.com/advisories/GHSA-35jh-r3h4-6jhm",
        "before": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ],
        "mutated": [
          {
            "path": "package-lock.json",
            "sha256": "55a94e0ca609f9c43263164d98163a9057daf13c65dd980eccc570ad31f8c11f"
          }
        ],
        "after": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ]
      },
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-DISCOVERY-RED-DEPENDENCY-OSV",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "dependency-osv"
      ],
      "method": "Inspected actual baseline, intended diagnostic and restored run; independently verified exact restoration. Temporarily add vulnerable lodash 4.17.20 metadata to the lockfile without installing it.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/discovery-red/dependency-osv.txt",
        "sha256": "abf65731d3a8283dc39247b2f25bed39fd3fed130f04e3c9bac02d501fdbd54e"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "7617422964d1b91ba5e497bd6c1d8fa9e5e9361baec02c04cd81cf7cbe5f6d74",
      "red": {
        "mutation": "Temporarily add vulnerable lodash 4.17.20 metadata to the lockfile without installing it.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "| https://osv.dev/GHSA-35jh-r3h4-6jhm | 8.1  | npm       | lodash (dev) | 4.17.20 | 4.17.21       | package-lock.json |",
        "observed_diagnostic": "| https://osv.dev/GHSA-35jh-r3h4-6jhm | 8.1  | npm       | lodash (dev) | 4.17.20 | 4.17.21       | package-lock.json |",
        "before": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ],
        "mutated": [
          {
            "path": "package-lock.json",
            "sha256": "55a94e0ca609f9c43263164d98163a9057daf13c65dd980eccc570ad31f8c11f"
          }
        ],
        "after": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ]
      },
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-DISCOVERY-RED-WHATIF-GUARD",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "whatif-guard"
      ],
      "method": "Inspected actual baseline, intended diagnostic and restored run; independently verified exact restoration. Bypass the common ShouldProcess guard; the disposable WhatIf effect assertion must fail.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/discovery-red/whatif-guard.txt",
        "sha256": "8703f68c1c57632c622d4ee812aecdaeb78ad4e8e4691a351d7d8d3dbd236852"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "7617422964d1b91ba5e497bd6c1d8fa9e5e9361baec02c04cd81cf7cbe5f6d74",
      "red": {
        "mutation": "Bypass the common ShouldProcess guard; the disposable WhatIf effect assertion must fail.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Expected strings to be the same, but they were different.",
        "observed_diagnostic": "Expected strings to be the same, but they were different.",
        "before": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
          }
        ],
        "mutated": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "120fcc5952c9ee26a1dc9306825eff2cd573e77ac173dc4d1486cd516d4327a8"
          }
        ],
        "after": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
          }
        ]
      },
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-DISCOVERY-RED-NATIVE-EXIT-PROPAGATION",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "native-exit-propagation"
      ],
      "method": "Inspected actual baseline, intended diagnostic and restored run; independently verified exact restoration. Bypass the native exit-code guard; the real formatter-failure assertion must fail.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/discovery-red/native-exit-propagation.txt",
        "sha256": "568e9558ee370f7254eb81dfb6c69b7a46388ce50399320a9813ba435322d379"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "7617422964d1b91ba5e497bd6c1d8fa9e5e9361baec02c04cd81cf7cbe5f6d74",
      "red": {
        "mutation": "Bypass the native exit-code guard; the real formatter-failure assertion must fail.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Expected 0 to be different from the actual value, but got the same value.",
        "observed_diagnostic": "Expected 0 to be different from the actual value, but got the same value.",
        "before": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
          }
        ],
        "mutated": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "a9b187aa1b03c9bf32afd65453dd28deb05662dd79c099d3f13fd84dd579f7a6"
          }
        ],
        "after": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
          }
        ]
      },
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-DISCOVERY-HARDWARE-DEFERRED",
      "acceptance": [
        "AC-AUTH"
      ],
      "kind": "manual",
      "status": "stale",
      "command": [],
      "method": "Observed the actual redacted native scan and independent adapter/PnP-name summary. Native exit 3, Bluetooth error 9, 27 ms, no completed scan. No real authentication/ownership or phone-off test was attempted.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        },
        "platform": "windows"
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/DISCOVERY_VERIFICATION.md",
        "sha256": "9bd7741f62d028b3806cca60f93f02ff1602a8417448b686f57866599ffabfea"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "5955536ae2525d156a58713c5553067dc8d884ebdad9548f2add75b354e4dd19"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "27213f1d7ac878840fd67c71b76f461e94d3ce2e8293a24fa495155260e58329"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "3374f9bb5b18ae2efae65f26b79e2c79700aaec77598d5f0b075567c95c2d23f"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "65748fd4ba4368a75c5ebe73197e9b6669ac1b5f0da4530f28a45a1c1b0d8ae0"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "8ef4bd4ff07c056768c65fe23db496cd44c7b317eddb1f35f97c26d28258c0da"
        },
        {
          "path": "include/mgl/discovery.hpp",
          "sha256": "bd5393d7bdd19d2de67308be56c8a7c2f46492607be7b161779176822fc5570f"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "include/mgl/session.hpp",
          "sha256": null
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "e7d38dd33f0aadf2f7c2b982462f6aea86878bcb0c3455fbee69903d67caa6d2"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "a057a54f9c429cd802c9c2d8bb2516df1519bff55943e63f5e9751889fce2c11"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "c5d20224035f211a156ab4af6cbdc298429cc704918db92e636abf1201afbf9b"
        },
        {
          "path": "src/windows/discovery.cpp",
          "sha256": "adea862c34b41e7877a75e7d4d8f9b2aa0eecb541fdf4ec11994ccae61f9cabc"
        },
        {
          "path": "src/windows/identity.cpp",
          "sha256": null
        },
        {
          "path": "src/windows/session.cpp",
          "sha256": null
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/probe.Tests.ps1",
          "sha256": "f75abfbfd1ab1bc8f263805e87cacb4cf036fec7e69cd7f6f4edde2ccd8c7879"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        },
        {
          "path": "tests/session_tests.cpp",
          "sha256": null
        }
      ],
      "scope_sha256": "1da1f86af911fd12f5f6264eefa89edd8d42c03dd2ecfdc6d793ed3abd7a107f",
      "red": null,
      "reason": "Historical discovery snapshot. Replacement-controller recommendation superseded by the existing-adapter constraint and new targeted Classic investigation; see EV-EXISTING-ADAPTER-HARDWARE-DEFERRED."
    },
    {
      "id": "EV-CLASSIC-READINESS",
      "acceptance": [
        "AC-AUTH",
        "AC-FRAMING",
        "AC-GATES",
        "AC-MEDIA",
        "AC-MODULE",
        "AC-OBS"
      ],
      "kind": "readiness",
      "status": "stale",
      "command": [],
      "method": "Reviewed the concrete controller/Classic transport increment, safety/failure/lifetime and test contracts; no authenticated-device-info proof claimed.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/classic-readiness.txt",
        "sha256": "b3347aa98f827f91f8f8292fc29f23a8d2be19419dfaa8c418a00be39993ef2d"
      },
      "inputs": [
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "9df26e151e5b4903db5f9e1783c29f373701a1eff9556d641660ec29cbfb9fc5"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        }
      ],
      "scope_sha256": "7ee960dad2b4fbb39122aeb6d411870ecf3bbb3d1b00d30c0b8fe18c345878d4",
      "red": null,
      "reason": "Classic/controller increment changed source, observation inputs and plan prose. Retain prior proof; fresh affected software/drills/readiness required."
    },
    {
      "id": "EV-CLASSIC-READINESS-FINAL",
      "acceptance": [
        "AC-AUTH",
        "AC-FRAMING",
        "AC-GATES",
        "AC-MEDIA",
        "AC-MODULE",
        "AC-OBS"
      ],
      "kind": "readiness",
      "status": "stale",
      "command": [],
      "method": "Reviewed the completed controller/Classic increment, canonical reuse, explicit connection and failure/lifetime contracts, actual hardware blocker and unchanged full acceptance; readiness only.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/classic-readiness-final.txt",
        "sha256": "1175576c1b90d40dba9fe3a5065628bb414c8f5a6627b8faea30aa297562f230"
      },
      "inputs": [
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        }
      ],
      "scope_sha256": "01cf799d672aba8cf8101db16f84ae9f7bced698e8c85fabd293b4d9afa436fb",
      "red": null,
      "reason": "Direct private-address RFCOMM probe extends TASK-AUTH paths. Keep prior source-bound result; re-review/reverify new scope."
    },
    {
      "id": "EV-CLASSIC-SOFTWARE",
      "acceptance": [
        "AC-FRAMING",
        "AC-GATES",
        "AC-MODULE"
      ],
      "kind": "behavior",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "quality"
      ],
      "method": "Actual full quality aggregate exited 0: two CTest suites, 3 runner and 10 CLI tests, configured analysis/security, Windows ASan and all 18 defect/restoration drills. Scope is software foundation/diagnostics, not current glasses or refreshed ZIP.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/quality-classic.txt",
        "sha256": "ac9ad85cee977eb799613116868c874dec5f86266ef5869ac4d6be5d682b61b1"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "a7e10dfc390635734e0a1978de9c102afc08ef00acdd09c73a3ef6a10929ddf9",
      "red": null,
      "reason": "Existing-adapter Classic increment changes source, task inputs and current documentation; retain prior proof and run fresh affected checks."
    },
    {
      "id": "EV-CLASSIC-SOFTWARE-PARTIAL",
      "acceptance": [
        "AC-AUTH"
      ],
      "kind": "behavior",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "quality"
      ],
      "method": "Partial enabling diagnostic/Classic code evidence only. Actual aggregate passed; current registered Meta service count is zero and no real socket was opened. No authenticated device-info, identity/setup, phone-off or firmware claim.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/quality-classic.txt",
        "sha256": "ac9ad85cee977eb799613116868c874dec5f86266ef5869ac4d6be5d682b61b1"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/discovery.hpp",
          "sha256": "bb88b637251d5047c072be3313b8e95961ec9125aab550ea7cded2a9cdba3fdf"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "include/mgl/session.hpp",
          "sha256": null
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/apartment.hpp",
          "sha256": "c392d69883555a5da3111f7e5a2f916a0e36b812ad609494f8fac00cf0d3ef29"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "src/windows/classic.cpp",
          "sha256": "acc4fc04304855d957d6b7ec2fe98d920c68371bf2e47b7acb6871cfa20cded5"
        },
        {
          "path": "src/windows/discovery.cpp",
          "sha256": "5066920f2c8bb1fc0dd143857978b72668e3e41c18aedb7af8ba95f4145fcccf"
        },
        {
          "path": "src/windows/identity.cpp",
          "sha256": null
        },
        {
          "path": "src/windows/session.cpp",
          "sha256": null
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/probe.Tests.ps1",
          "sha256": "f09f29301bf1b36a7289c3ff6c1798d6c50ef50a1a9f14244447b9cd100791fa"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        },
        {
          "path": "tests/session_tests.cpp",
          "sha256": null
        }
      ],
      "scope_sha256": "ee96b278f196fcb30773677615edb7b4cd5be2f4f987167d0fea867b99e33081",
      "red": null,
      "reason": "Direct private-address RFCOMM probe extends TASK-AUTH paths. Keep prior source-bound result; re-review/reverify new scope."
    },
    {
      "id": "EV-CLASSIC-RED-PARSER-LENGTH",
      "acceptance": [
        "AC-FRAMING"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "parser-length"
      ],
      "method": "Inspected actual intended diagnostic and failure, then independently verified exact current byte/hash restoration. Change DataxHeader from 4 to 5 without modifying the independent expected frame lengths.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/classic-red/parser-length.txt",
        "sha256": "3f44e5b99d0bee827fe3f0a016133feaa38297471d4171a652d05aaa10c6e422"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "a7e10dfc390635734e0a1978de9c102afc08ef00acdd09c73a3ef6a10929ddf9",
      "red": {
        "mutation": "Change DataxHeader from 4 to 5 without modifying the independent expected frame lengths.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "DataX empty has 4 bytes: unexpected prefix status or frame length",
        "observed_diagnostic": "DataX empty has 4 bytes: unexpected prefix status or frame length",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "fcea87b9c7e60b81822b0cadc2362def9e3cd413011a19130199b857b6660278"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": "Existing-adapter Classic increment changes source, task inputs and current documentation; retain prior proof and run fresh affected checks."
    },
    {
      "id": "EV-CLASSIC-RED-MODULE-METADATA",
      "acceptance": [
        "AC-MODULE"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "module-metadata"
      ],
      "method": "Inspected actual intended diagnostic and failure, then independently verified exact current byte/hash restoration. Change the exported module name to Broken module name.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/classic-red/module-metadata.txt",
        "sha256": "5f5bcf19e162247d90d6fe539b62272ee5547b5cab0236de909294d2a8905458"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "a7e10dfc390635734e0a1978de9c102afc08ef00acdd09c73a3ef6a10929ddf9",
      "red": {
        "mutation": "Change the exported module name to Broken module name.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "OBS module metadata does not match development-foundation contract",
        "observed_diagnostic": "OBS module metadata does not match development-foundation contract",
        "before": [
          {
            "path": "src/plugin/module.c",
            "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
          }
        ],
        "mutated": [
          {
            "path": "src/plugin/module.c",
            "sha256": "608f7c48d09ad85b57cce8de7cc22fedeeea6017210dfd2cc37efc9da082004b"
          }
        ],
        "after": [
          {
            "path": "src/plugin/module.c",
            "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
          }
        ]
      },
      "reason": "Existing-adapter Classic increment changes source, task inputs and current documentation; retain prior proof and run fresh affected checks."
    },
    {
      "id": "EV-CLASSIC-RED-PROBE-ARGUMENTS",
      "acceptance": [
        "AC-AUTH",
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "probe-arguments"
      ],
      "method": "Inspected actual intended diagnostic and failure, then independently verified exact current byte/hash restoration. Return exit 0 for rejected native probe arguments while preserving independent CLI assertions expecting exit 5.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/classic-red/probe-arguments.txt",
        "sha256": "6925a39ebc56fec5b35749f42ca0b84c694abca7517d684a0a300c7cc23e5295"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/discovery.hpp",
          "sha256": "bb88b637251d5047c072be3313b8e95961ec9125aab550ea7cded2a9cdba3fdf"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "include/mgl/session.hpp",
          "sha256": null
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/apartment.hpp",
          "sha256": "c392d69883555a5da3111f7e5a2f916a0e36b812ad609494f8fac00cf0d3ef29"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "src/windows/classic.cpp",
          "sha256": "acc4fc04304855d957d6b7ec2fe98d920c68371bf2e47b7acb6871cfa20cded5"
        },
        {
          "path": "src/windows/discovery.cpp",
          "sha256": "5066920f2c8bb1fc0dd143857978b72668e3e41c18aedb7af8ba95f4145fcccf"
        },
        {
          "path": "src/windows/identity.cpp",
          "sha256": null
        },
        {
          "path": "src/windows/session.cpp",
          "sha256": null
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/probe.Tests.ps1",
          "sha256": "f09f29301bf1b36a7289c3ff6c1798d6c50ef50a1a9f14244447b9cd100791fa"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        },
        {
          "path": "tests/session_tests.cpp",
          "sha256": null
        }
      ],
      "scope_sha256": "ee96b278f196fcb30773677615edb7b4cd5be2f4f987167d0fea867b99e33081",
      "red": {
        "mutation": "Return exit 0 for rejected native probe arguments while preserving independent CLI assertions expecting exit 5.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Expected 5, because invalid probe arguments must fail, but got 0.",
        "observed_diagnostic": "Expected 5, because invalid probe arguments must fail, but got 0.",
        "before": [
          {
            "path": "src/probe/main.cpp",
            "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
          }
        ],
        "mutated": [
          {
            "path": "src/probe/main.cpp",
            "sha256": "5aef00e35f3bac7f73790627efb088ec21ca7e1ce99852948f04c8426991b7fe"
          }
        ],
        "after": [
          {
            "path": "src/probe/main.cpp",
            "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
          }
        ]
      },
      "reason": "Direct private-address RFCOMM probe extends TASK-AUTH paths. Keep prior source-bound result; re-review/reverify new scope."
    },
    {
      "id": "EV-CLASSIC-RED-CLASSIC-CONNECTION-CLAIM",
      "acceptance": [
        "AC-AUTH",
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "classic-connection-claim"
      ],
      "method": "Inspected actual intended diagnostic and failure, then independently verified exact current byte/hash restoration. Initialize Classic diagnostic result with rfcomm_connected true; independent query-only CLI assertion must reject the false connection claim.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/classic-red/classic-connection-claim.txt",
        "sha256": "faf0c58d203eeb8f351ac9366b829f7cdb5aedd25931dccb1bc287e9aeeb724f"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/discovery.hpp",
          "sha256": "bb88b637251d5047c072be3313b8e95961ec9125aab550ea7cded2a9cdba3fdf"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "include/mgl/session.hpp",
          "sha256": null
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/apartment.hpp",
          "sha256": "c392d69883555a5da3111f7e5a2f916a0e36b812ad609494f8fac00cf0d3ef29"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "src/windows/classic.cpp",
          "sha256": "acc4fc04304855d957d6b7ec2fe98d920c68371bf2e47b7acb6871cfa20cded5"
        },
        {
          "path": "src/windows/discovery.cpp",
          "sha256": "5066920f2c8bb1fc0dd143857978b72668e3e41c18aedb7af8ba95f4145fcccf"
        },
        {
          "path": "src/windows/identity.cpp",
          "sha256": null
        },
        {
          "path": "src/windows/session.cpp",
          "sha256": null
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/probe.Tests.ps1",
          "sha256": "f09f29301bf1b36a7289c3ff6c1798d6c50ef50a1a9f14244447b9cd100791fa"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        },
        {
          "path": "tests/session_tests.cpp",
          "sha256": null
        }
      ],
      "scope_sha256": "ee96b278f196fcb30773677615edb7b4cd5be2f4f987167d0fea867b99e33081",
      "red": {
        "mutation": "Initialize Classic diagnostic result with rfcomm_connected true; independent query-only CLI assertion must reject the false connection claim.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Expected $false, because Classic discovery must not claim an RFCOMM connection, but got $true.",
        "observed_diagnostic": "Expected $false, because Classic discovery must not claim an RFCOMM connection, but got $true.",
        "before": [
          {
            "path": "src/windows/classic.cpp",
            "sha256": "acc4fc04304855d957d6b7ec2fe98d920c68371bf2e47b7acb6871cfa20cded5"
          }
        ],
        "mutated": [
          {
            "path": "src/windows/classic.cpp",
            "sha256": "6adfa1beee9641c6be93619deaf105b2da90b6e4b925ecd7c225bd770e210ed4"
          }
        ],
        "after": [
          {
            "path": "src/windows/classic.cpp",
            "sha256": "acc4fc04304855d957d6b7ec2fe98d920c68371bf2e47b7acb6871cfa20cded5"
          }
        ]
      },
      "reason": "Direct private-address RFCOMM probe extends TASK-AUTH paths. Keep prior source-bound result; re-review/reverify new scope."
    },
    {
      "id": "EV-CLASSIC-RED-FORMAT",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "format"
      ],
      "method": "Inspected actual intended diagnostic and failure, then independently verified exact current byte/hash restoration. Append an intentionally unformatted function.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/classic-red/format.txt",
        "sha256": "2709604244b5ea4ab4da49131d6d193bc361311b575793332a501dea5dee3c3b"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "a7e10dfc390635734e0a1978de9c102afc08ef00acdd09c73a3ef6a10929ddf9",
      "red": {
        "mutation": "Append an intentionally unformatted function.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp:47:22: error: code should be clang-formatted [-Wclang-format-violations]",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp:47:22: error: code should be clang-formatted [-Wclang-format-violations]",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "c2a1d1a6c8b7489cbb25561dddadb31309d7ac5c15f880bbeb3c90288eabb038"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": "Existing-adapter Classic increment changes source, task inputs and current documentation; retain prior proof and run fresh affected checks."
    },
    {
      "id": "EV-CLASSIC-RED-OWNED-HEADER",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "owned-header"
      ],
      "method": "Inspected actual intended diagnostic and failure, then independently verified exact current byte/hash restoration. Append a mutable inline global to an owned header.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/classic-red/owned-header.txt",
        "sha256": "a4069569c2f35b911ebd4d44bb106999eadca9d59eb95028260f93c882d41ca2"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "a7e10dfc390635734e0a1978de9c102afc08ef00acdd09c73a3ef6a10929ddf9",
      "red": {
        "mutation": "Append a mutable inline global to an owned header.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\include\\mgl/sdu.hpp:23:12: error: variable 'red_drill_global' is non-const and globally accessible, consider making it const [cppcoreguidelines-avoid-non-const-global-variables,-warnings-as-errors]",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\include\\mgl/sdu.hpp:23:12: error: variable 'red_drill_global' is non-const and globally accessible, consider making it const [cppcoreguidelines-avoid-non-const-global-variables,-warnings-as-errors]",
        "before": [
          {
            "path": "include/mgl/sdu.hpp",
            "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
          }
        ],
        "mutated": [
          {
            "path": "include/mgl/sdu.hpp",
            "sha256": "220f387016dea2b2cb31874833a09fd753ad9998873937e5cdfba7cb762fe0ec"
          }
        ],
        "after": [
          {
            "path": "include/mgl/sdu.hpp",
            "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
          }
        ]
      },
      "reason": "Existing-adapter Classic increment changes source, task inputs and current documentation; retain prior proof and run fresh affected checks."
    },
    {
      "id": "EV-CLASSIC-RED-COMPILER-TYPES",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "compiler-types"
      ],
      "method": "Inspected actual intended diagnostic and failure, then independently verified exact current byte/hash restoration. Initialize an integer constant from a string literal.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/classic-red/compiler-types.txt",
        "sha256": "a7e224c803008efad05581d907b1cf92d64ed5dc2b18ddc13edc3e61594ce36f"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "a7e10dfc390635734e0a1978de9c102afc08ef00acdd09c73a3ef6a10929ddf9",
      "red": {
        "mutation": "Initialize an integer constant from a string literal.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp(47): error C2440: 'initializing': cannot convert from 'const char [16]' to 'const int'",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp(47): error C2440: 'initializing': cannot convert from 'const char [16]' to 'const int'",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "4d3f32e4a31af074ff96c5ef217d361573917be607688d49acd001562970bc96"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": "Existing-adapter Classic increment changes source, task inputs and current documentation; retain prior proof and run fresh affected checks."
    },
    {
      "id": "EV-CLASSIC-RED-UNUSED-CODE",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "unused-code"
      ],
      "method": "Inspected actual intended diagnostic and failure, then independently verified exact current byte/hash restoration. Append an unreferenced function to owned protocol source.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/classic-red/unused-code.txt",
        "sha256": "cc104eeecdc5dfaaeadc7ee06971302147aa1b0d154c23e59ba5062d39ca5eb1"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "a7e10dfc390635734e0a1978de9c102afc08ef00acdd09c73a3ef6a10929ddf9",
      "red": {
        "mutation": "Append an unreferenced function to owned protocol source.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp:47:5: style: The function 'red_drill_unused_function' is never used. [unusedFunction]",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp:47:5: style: The function 'red_drill_unused_function' is never used. [unusedFunction]",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "56c7aca4962c137d9b9bfddaedb756fb220fc33620f0f3888c0b25de525b780a"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": "Existing-adapter Classic increment changes source, task inputs and current documentation; retain prior proof and run fresh affected checks."
    },
    {
      "id": "EV-CLASSIC-RED-SCRIPT-ANALYSIS",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "script-analysis"
      ],
      "method": "Inspected actual intended diagnostic and failure, then independently verified exact current byte/hash restoration. Append a function with an unapproved PowerShell verb.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/classic-red/script-analysis.txt",
        "sha256": "33fe20ff3bc402ecd9615bf6fc97fe10d4edbef721f4877bb6a881adff8522d2"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "a7e10dfc390635734e0a1978de9c102afc08ef00acdd09c73a3ef6a10929ddf9",
      "red": {
        "mutation": "Append a function with an unapproved PowerShell verb.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "PSUseApprovedVerbs Warning  Invoke-Project.ps1 492  The cmdlet 'Badverb-RedDrill' uses an unapproved verb.",
        "observed_diagnostic": "PSUseApprovedVerbs Warning  Invoke-Project.ps1 492  The cmdlet 'Badverb-RedDrill' uses an unapproved verb.",
        "before": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
          }
        ],
        "mutated": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "a42e3eab7a57b5c28bf42c62def3bd82b9b995cb3dd3d21a18d3fdec559bfcf0"
          }
        ],
        "after": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
          }
        ]
      },
      "reason": "Existing-adapter Classic increment changes source, task inputs and current documentation; retain prior proof and run fresh affected checks."
    },
    {
      "id": "EV-CLASSIC-RED-WORKFLOW-SECURITY",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "workflow-security"
      ],
      "method": "Inspected actual intended diagnostic and failure, then independently verified exact current byte/hash restoration. Replace the pinned checkout commit SHA with a movable version tag.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/classic-red/workflow-security.txt",
        "sha256": "19e4cb7fa962c8103d15f12ac99efbf79c284e97c1eaf4b251064f3e7892133b"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "a7e10dfc390635734e0a1978de9c102afc08ef00acdd09c73a3ef6a10929ddf9",
      "red": {
        "mutation": "Replace the pinned checkout commit SHA with a movable version tag.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "error[unpinned-uses]: unpinned action reference",
        "observed_diagnostic": "error[unpinned-uses]: unpinned action reference",
        "before": [
          {
            "path": ".github/workflows/quality.yml",
            "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
          }
        ],
        "mutated": [
          {
            "path": ".github/workflows/quality.yml",
            "sha256": "a151bb006b4d7e858647ffe9c81521c98e2e2d9a3c12cfa60555e229a57afd76"
          }
        ],
        "after": [
          {
            "path": ".github/workflows/quality.yml",
            "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
          }
        ]
      },
      "reason": "Existing-adapter Classic increment changes source, task inputs and current documentation; retain prior proof and run fresh affected checks."
    },
    {
      "id": "EV-CLASSIC-RED-SECRET-SCANNER",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "secret-scanner"
      ],
      "method": "Inspected actual intended diagnostic and failure, then independently verified exact current byte/hash restoration. Append a randomized base32 AWS-key-shaped synthetic canary; scanner output is redacted.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/classic-red/secret-scanner.txt",
        "sha256": "5905311978ec592f3778b1949b8e04a13feb723ec252d83bedb41b0759fe43e5"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "a7e10dfc390635734e0a1978de9c102afc08ef00acdd09c73a3ef6a10929ddf9",
      "red": {
        "mutation": "Append a randomized base32 AWS-key-shaped synthetic canary; scanner output is redacted.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "5:27PM WRN leaks found: 1",
        "observed_diagnostic": "5:27PM WRN leaks found: 1",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "f0ebd63ca32fd73579696a76480919f96355d11dd02914da5106e08a51828188"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": "Existing-adapter Classic increment changes source, task inputs and current documentation; retain prior proof and run fresh affected checks."
    },
    {
      "id": "EV-CLASSIC-RED-UNSAFE-COPY",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "unsafe-copy"
      ],
      "method": "Inspected actual intended diagnostic and failure, then independently verified exact current byte/hash restoration. Append an unbounded strcpy call.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/classic-red/unsafe-copy.txt",
        "sha256": "cdff4eaecd0f629359cab9a72b22153d6c3737e24b28ff56fe4508d705d79b7c"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "a7e10dfc390635734e0a1978de9c102afc08ef00acdd09c73a3ef6a10929ddf9",
      "red": {
        "mutation": "Append an unbounded strcpy call.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "\u276f\u276f\u2771 mgl-unsafe-string-copy",
        "observed_diagnostic": "\u276f\u276f\u2771 mgl-unsafe-string-copy",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "8c973ff6b7b7e0be339225d7c4a427203011da68c5fda6414bea6ec6c0ea0b03"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": "Existing-adapter Classic increment changes source, task inputs and current documentation; retain prior proof and run fresh affected checks."
    },
    {
      "id": "EV-CLASSIC-RED-DUPLICATED-CODE",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "duplicated-code"
      ],
      "method": "Inspected actual intended diagnostic and failure, then independently verified exact current byte/hash restoration. Insert a duplicate frame-prefix implementation under another function name.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/classic-red/duplicated-code.txt",
        "sha256": "f1612962fd7241445290f4eb551503161d0cca9b197412e2431541cbc783dc4b"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "a7e10dfc390635734e0a1978de9c102afc08ef00acdd09c73a3ef6a10929ddf9",
      "red": {
        "mutation": "Insert a duplicate frame-prefix implementation under another function name.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "ERROR: jscpd found too many duplicates (2.9%) over threshold (0.0%)",
        "observed_diagnostic": "ERROR: jscpd found too many duplicates (2.9%) over threshold (0.0%)",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "21406acfb851f1a583b2a8e742638e4eb5e5f367c7195ef607eb5862fcd8d6df"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": "Existing-adapter Classic increment changes source, task inputs and current documentation; retain prior proof and run fresh affected checks."
    },
    {
      "id": "EV-CLASSIC-RED-ADDRESS-SANITIZER",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "address-sanitizer"
      ],
      "method": "Inspected actual intended diagnostic and failure, then independently verified exact current byte/hash restoration. Inject a runtime heap-buffer read past a one-element allocation in test code.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/classic-red/address-sanitizer.txt",
        "sha256": "b143217a1dbd668ff88504950d32ef8e66a43e72a8fef6af86793591630f4da9"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "a7e10dfc390635734e0a1978de9c102afc08ef00acdd09c73a3ef6a10929ddf9",
      "red": {
        "mutation": "Inject a runtime heap-buffer read past a one-element allocation in test code.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "==17628==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x11a9d56a0034 at pc 0x7ff726e053de bp 0x0073b65dfc70 sp 0x0073b65dfc78",
        "observed_diagnostic": "==17628==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x11a9d56a0034 at pc 0x7ff726e053de bp 0x0073b65dfc70 sp 0x0073b65dfc78",
        "before": [
          {
            "path": "tests/sdu_tests.cpp",
            "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
          }
        ],
        "mutated": [
          {
            "path": "tests/sdu_tests.cpp",
            "sha256": "f0b8a6f71a8f96f071d86024029c536756dd654d9beaea73b9a22d9535147d2e"
          }
        ],
        "after": [
          {
            "path": "tests/sdu_tests.cpp",
            "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
          }
        ]
      },
      "reason": "Existing-adapter Classic increment changes source, task inputs and current documentation; retain prior proof and run fresh affected checks."
    },
    {
      "id": "EV-CLASSIC-RED-DEPENDENCY-NPM",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "dependency-npm"
      ],
      "method": "Inspected actual intended diagnostic and failure, then independently verified exact current byte/hash restoration. Temporarily add vulnerable lodash 4.17.20 metadata to the lockfile without installing it.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/classic-red/dependency-npm.txt",
        "sha256": "02d6d1b6cfe118449d8746927bd3e88e5cd96acd688c11dee6c8ce730f364c6b"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "a7e10dfc390635734e0a1978de9c102afc08ef00acdd09c73a3ef6a10929ddf9",
      "red": {
        "mutation": "Temporarily add vulnerable lodash 4.17.20 metadata to the lockfile without installing it.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Command Injection in lodash - https://github.com/advisories/GHSA-35jh-r3h4-6jhm",
        "observed_diagnostic": "Command Injection in lodash - https://github.com/advisories/GHSA-35jh-r3h4-6jhm",
        "before": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ],
        "mutated": [
          {
            "path": "package-lock.json",
            "sha256": "7b71deea65dd1a92bd9d3da7a5a5073784dfcdc976cc9a7958d545444376b122"
          }
        ],
        "after": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ]
      },
      "reason": "Existing-adapter Classic increment changes source, task inputs and current documentation; retain prior proof and run fresh affected checks."
    },
    {
      "id": "EV-CLASSIC-RED-DEPENDENCY-OSV",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "dependency-osv"
      ],
      "method": "Inspected actual intended diagnostic and failure, then independently verified exact current byte/hash restoration. Temporarily add vulnerable lodash 4.17.20 metadata to the lockfile without installing it.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/classic-red/dependency-osv.txt",
        "sha256": "5b4e0cc057aa573956f29b56807754217c671c49218b94519cbe330362d27e67"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "a7e10dfc390635734e0a1978de9c102afc08ef00acdd09c73a3ef6a10929ddf9",
      "red": {
        "mutation": "Temporarily add vulnerable lodash 4.17.20 metadata to the lockfile without installing it.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "| https://osv.dev/GHSA-35jh-r3h4-6jhm | 8.1  | npm       | lodash (dev) | 4.17.20 | 4.17.21       | package-lock.json |",
        "observed_diagnostic": "| https://osv.dev/GHSA-35jh-r3h4-6jhm | 8.1  | npm       | lodash (dev) | 4.17.20 | 4.17.21       | package-lock.json |",
        "before": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ],
        "mutated": [
          {
            "path": "package-lock.json",
            "sha256": "7b71deea65dd1a92bd9d3da7a5a5073784dfcdc976cc9a7958d545444376b122"
          }
        ],
        "after": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ]
      },
      "reason": "Existing-adapter Classic increment changes source, task inputs and current documentation; retain prior proof and run fresh affected checks."
    },
    {
      "id": "EV-CLASSIC-RED-WHATIF-GUARD",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "whatif-guard"
      ],
      "method": "Inspected actual intended diagnostic and failure, then independently verified exact current byte/hash restoration. Bypass the common ShouldProcess guard; the disposable WhatIf effect assertion must fail.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/classic-red/whatif-guard.txt",
        "sha256": "ea7e1b0d01788932fe4415ea6e6c3f1aa7d74179c01ebdc82bdfd5c790e5803a"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "a7e10dfc390635734e0a1978de9c102afc08ef00acdd09c73a3ef6a10929ddf9",
      "red": {
        "mutation": "Bypass the common ShouldProcess guard; the disposable WhatIf effect assertion must fail.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Expected strings to be the same, but they were different.",
        "observed_diagnostic": "Expected strings to be the same, but they were different.",
        "before": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
          }
        ],
        "mutated": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "c3e141c6870f1a1e0a59d402bc8d825c4159ce222c359f471e89f4373d6392e8"
          }
        ],
        "after": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
          }
        ]
      },
      "reason": "Existing-adapter Classic increment changes source, task inputs and current documentation; retain prior proof and run fresh affected checks."
    },
    {
      "id": "EV-CLASSIC-RED-NATIVE-EXIT-PROPAGATION",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "stale",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "native-exit-propagation"
      ],
      "method": "Inspected actual intended diagnostic and failure, then independently verified exact current byte/hash restoration. Bypass the native exit-code guard; the real formatter-failure assertion must fail.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/classic-red/native-exit-propagation.txt",
        "sha256": "3e90a622279e1b762b563c4592660140082d27c96b72ccc79ff5d9d1201a9abd"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "a7e10dfc390635734e0a1978de9c102afc08ef00acdd09c73a3ef6a10929ddf9",
      "red": {
        "mutation": "Bypass the native exit-code guard; the real formatter-failure assertion must fail.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Expected 0 to be different from the actual value, but got the same value.",
        "observed_diagnostic": "Expected 0 to be different from the actual value, but got the same value.",
        "before": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
          }
        ],
        "mutated": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "e6695e155c57a62cdbb07987fe01fa0a3b552a4177efb16c5f96a00907164e02"
          }
        ],
        "after": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
          }
        ]
      },
      "reason": "Existing-adapter Classic increment changes source, task inputs and current documentation; retain prior proof and run fresh affected checks."
    },
    {
      "id": "EV-CLASSIC-HARDWARE-DEFERRED",
      "acceptance": [
        "AC-AUTH"
      ],
      "kind": "manual",
      "status": "stale",
      "command": [],
      "method": "Actual native feature query/inquiry/connection checks and user pairing/controller responses are recorded in the report and classic-observation-final.json. No actual socket, authentication or phone-off session occurred.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/CLASSIC_VERIFICATION.md",
        "sha256": "71d35e6d3a62023a816db7398d913c8d8858c3ad781d338702d12e7ab5feabec"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "88bafdc9b7bd3137d745d5b878b0659b4ef0a62b08ed1823e862ae517f530c76"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "fbe5a8b24c73db413803cecf3b92e021d31c6f18f257e18973e180a9e6db6816"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "f1b4656a1cf03c28eec4f130c288c9aeace0af403b6268297996aaaaac124f8c"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "59164c6e7cf8ca233d358995d20ade680da3d520d3e79a56014bf6b35b9df2d4"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/discovery.hpp",
          "sha256": "bb88b637251d5047c072be3313b8e95961ec9125aab550ea7cded2a9cdba3fdf"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "include/mgl/session.hpp",
          "sha256": null
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "44131bcfa9133afa43d91eeefba4e79e3b5c875c03c74341f4229357b3230f0c"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6f82802c41680e45a76227cbe2144bb0cab4c4ebee17015daf813c958c091f93"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/apartment.hpp",
          "sha256": "c392d69883555a5da3111f7e5a2f916a0e36b812ad609494f8fac00cf0d3ef29"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "src/windows/classic.cpp",
          "sha256": "acc4fc04304855d957d6b7ec2fe98d920c68371bf2e47b7acb6871cfa20cded5"
        },
        {
          "path": "src/windows/discovery.cpp",
          "sha256": "5066920f2c8bb1fc0dd143857978b72668e3e41c18aedb7af8ba95f4145fcccf"
        },
        {
          "path": "src/windows/identity.cpp",
          "sha256": null
        },
        {
          "path": "src/windows/session.cpp",
          "sha256": null
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/probe.Tests.ps1",
          "sha256": "f09f29301bf1b36a7289c3ff6c1798d6c50ef50a1a9f14244447b9cd100791fa"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        },
        {
          "path": "tests/session_tests.cpp",
          "sha256": null
        }
      ],
      "scope_sha256": "ee96b278f196fcb30773677615edb7b4cd5be2f4f987167d0fea867b99e33081",
      "red": null,
      "reason": "Historical discovery snapshot. Replacement-controller recommendation superseded by the existing-adapter constraint and new targeted Classic investigation; see EV-EXISTING-ADAPTER-HARDWARE-DEFERRED."
    },
    {
      "id": "EV-LIVE-SDP-READINESS",
      "acceptance": [
        "AC-AUTH",
        "AC-FRAMING",
        "AC-GATES",
        "AC-MEDIA",
        "AC-MODULE",
        "AC-OBS"
      ],
      "kind": "readiness",
      "status": "stale",
      "command": [],
      "method": "Reviewed existing-adapter requirement and uncached Classic probe/connection increment, canonical reuse, deadline/privacy/error contracts and actual-host tests. Full glasses acceptance unchanged.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/live-sdp-readiness.txt",
        "sha256": "cbe4ce8a5e745087cc467a2cd17169ec7d24c3f48e09e267e8d9974bd11f9464"
      },
      "inputs": [
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        }
      ],
      "scope_sha256": "01cf799d672aba8cf8101db16f84ae9f7bced698e8c85fabd293b4d9afa436fb",
      "red": null,
      "reason": "Direct private-address RFCOMM probe extends TASK-AUTH paths. Keep prior source-bound result; re-review/reverify new scope."
    },
    {
      "id": "EV-DIRECT-RFCOMM-READINESS",
      "acceptance": [
        "AC-AUTH",
        "AC-FRAMING",
        "AC-GATES",
        "AC-MEDIA",
        "AC-MODULE",
        "AC-OBS"
      ],
      "kind": "readiness",
      "status": "stale",
      "command": [],
      "method": "Reviewed private-address/direct Winsock transport increment, trustworthy target requirement, no-frame-write behavior, protocol reference, native boundaries and tests; no actual connection claimed.",
      "environment": {
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "msvc": "19.44.35222.0",
          "cppcheck": "2.22.0"
        },
        "platform": "windows"
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/direct-rfcomm-readiness.txt",
        "sha256": "846afbd9826c724276f68464c8544092c5570973484dfff6665eaca2ed036f98"
      },
      "inputs": [
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "6ca6ac0e1a29f6c3603fdcbf5b63ac3b87682eb44c12821d297370923aa59b98"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        }
      ],
      "scope_sha256": "015f0a71d87306f61ea53ba2586e1ad482c3e5d75abd707bfb499af496639fed",
      "red": null,
      "reason": "Existing-adapter Classic increment changes source, task inputs and current documentation; retain prior proof and run fresh affected checks."
    },
    {
      "id": "EV-EXISTING-ADAPTER-READINESS",
      "acceptance": [
        "AC-AUTH",
        "AC-FRAMING",
        "AC-GATES",
        "AC-MEDIA",
        "AC-MODULE",
        "AC-OBS"
      ],
      "kind": "readiness",
      "status": "stale",
      "command": [],
      "method": "Reviewed existing-adapter constraint, canonical ownership/reuse, actual failed discovery/picker observations, private input, unpaired Winsock probe design, native lifetime/error boundaries, test/drill coverage and remaining hardware gates.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/existing-adapter-readiness.txt",
        "sha256": "946ac54c8625bcdadc0621d6493ce6b6141a2f06212ea0680f232eba3662b130"
      },
      "inputs": [
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "25b1f068dedca3e61deacbe6e7ccd08a8a54b00c0e783025707f0e3898aaa358"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        }
      ],
      "scope_sha256": "a3529185a96542b04ea84569e420c8d57d7390efa0a8d6f482bdcb2de07bb871",
      "red": null,
      "reason": "Normalized owned documentation and test source to declared Git LF bytes before final evidence binding; review refreshed without changing requirements."
    },
    {
      "id": "EV-EXISTING-ADAPTER-READINESS-FINAL",
      "acceptance": [
        "AC-AUTH",
        "AC-FRAMING",
        "AC-GATES",
        "AC-MEDIA",
        "AC-MODULE",
        "AC-OBS"
      ],
      "kind": "readiness",
      "status": "pass",
      "command": [],
      "method": "Re-reviewed the same concrete design and requirements after Git LF normalization; existing-adapter, trustworthy-target, private-input and unresolved authentication/hardware boundaries remain unchanged.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/existing-adapter-readiness.txt",
        "sha256": "946ac54c8625bcdadc0621d6493ce6b6141a2f06212ea0680f232eba3662b130"
      },
      "inputs": [
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        }
      ],
      "scope_sha256": "a3529185a96542b04ea84569e420c8d57d7390efa0a8d6f482bdcb2de07bb871",
      "red": null,
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-SOFTWARE",
      "acceptance": [
        "AC-FRAMING",
        "AC-MODULE",
        "AC-GATES"
      ],
      "kind": "behavior",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "quality"
      ],
      "method": "Observed full quality exit 0: build/format/strict analysis/dead-code/security, three CTest suites, 3 runner + 20 CLI + 1 live Pester cases and all 20 defect/restoration drills including Windows ASan. No real camera/source/firmware proof.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/existing-adapter-quality.txt",
        "sha256": "a73f031427c1d7c71ae19d7aee3a2948b1dcc6579422991caed2bca64452772a"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "d70cca17793217ef03029c6825bfec8e1e743fbee2e1b86cdccd7a34a37dbf4d",
      "red": null,
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-CLI-PARTIAL",
      "acceptance": [
        "AC-AUTH"
      ],
      "kind": "behavior",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "quality"
      ],
      "method": "Partial enabling-code evidence only: real diagnostic/invalid-private-input/query tests and maintained drills passed. No valid-address direct connection or authenticated device info was obtained; TASK-AUTH remains blocked.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/existing-adapter-quality.txt",
        "sha256": "a73f031427c1d7c71ae19d7aee3a2948b1dcc6579422991caed2bca64452772a"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/address.hpp",
          "sha256": "9889fa8f7c51d4236dc5d3e00c46167952f2879478da0842e0335eca28ebc6ed"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/discovery.hpp",
          "sha256": "7a262e29abb007fadad655d5d172cd138d46485162dd651d26deb26ea2b74094"
        },
        {
          "path": "include/mgl/rfcomm.hpp",
          "sha256": "66d2fc76d1d8e2e74049c3e33304298e5b08cfcaa5b34da2bc5b80d551003483"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "include/mgl/session.hpp",
          "sha256": null
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/probe/private_input.cpp",
          "sha256": "5c251d6eacf7d06803f085505aeb485f7627620ee989f13925a85fc1304d4e4c"
        },
        {
          "path": "src/probe/private_input.hpp",
          "sha256": "b90870ed2085b7aebe46726f517252c8fe2c168d15341b01eb16fdda0654cc43"
        },
        {
          "path": "src/protocol/address.cpp",
          "sha256": "c3004bc33bb615eee1e285a7666fb28bff020a22f412f366381be8aa065d5eaf"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/apartment.hpp",
          "sha256": "c392d69883555a5da3111f7e5a2f916a0e36b812ad609494f8fac00cf0d3ef29"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "src/windows/classic.cpp",
          "sha256": "ece9d6024a38f9d81fab43bdba941173b47153d0b9bacab449bc8f1b2c4fea13"
        },
        {
          "path": "src/windows/discovery.cpp",
          "sha256": "5066920f2c8bb1fc0dd143857978b72668e3e41c18aedb7af8ba95f4145fcccf"
        },
        {
          "path": "src/windows/identity.cpp",
          "sha256": null
        },
        {
          "path": "src/windows/rfcomm.cpp",
          "sha256": "9da6dbd1321ba3be1d7746e65a56ea9fa02354eedfb385c36bbce0f11742ae20"
        },
        {
          "path": "src/windows/session.cpp",
          "sha256": null
        },
        {
          "path": "tests/address_tests.cpp",
          "sha256": "e9e3311b0609a4eef7ab4fcfa3a55943ae20600fd07978606cdfadc65412e901"
        },
        {
          "path": "tests/live.Tests.ps1",
          "sha256": "efd413d3f5bfc27aa403fcfd99f61da5fc58271ba90527d57fb9914945bfd65f"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/probe.Tests.ps1",
          "sha256": "6e241cf9154c67044628d35fa5f3b0c6b92c9139dc666d6ed9adfab082ddc064"
        },
        {
          "path": "tests/probe_support.ps1",
          "sha256": "ad53ccd95cc106348768cb5889de2be8608401934064ec3c361466e66ed8c4a2"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        },
        {
          "path": "tests/session_tests.cpp",
          "sha256": null
        }
      ],
      "scope_sha256": "1cffc8d0c3343417798b228b939daca34c529768b6b1048f7978056d9f9b4bc7",
      "red": null,
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-PARSER-LENGTH",
      "acceptance": [
        "AC-FRAMING"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "parser-length"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Change DataxHeader from 4 to 5 without modifying the independent expected frame lengths.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/parser-length.txt",
        "sha256": "5ef897da3df647a40ab32dd6fbc885b3c3ba05d5f8582426cb1a5eabfeed915c"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "d70cca17793217ef03029c6825bfec8e1e743fbee2e1b86cdccd7a34a37dbf4d",
      "red": {
        "mutation": "Change DataxHeader from 4 to 5 without modifying the independent expected frame lengths.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "DataX empty has 4 bytes",
        "observed_diagnostic": "DataX empty has 4 bytes: unexpected prefix status or frame length",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "fcea87b9c7e60b81822b0cadc2362def9e3cd413011a19130199b857b6660278"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-MODULE-METADATA",
      "acceptance": [
        "AC-MODULE"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "module-metadata"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Change the exported module name to Broken module name.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/module-metadata.txt",
        "sha256": "136bae4dd2087a886ebaee2df06bd643265844d1a7a995df4d8816ea32bf7d63"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "d70cca17793217ef03029c6825bfec8e1e743fbee2e1b86cdccd7a34a37dbf4d",
      "red": {
        "mutation": "Change the exported module name to Broken module name.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "OBS module metadata does not match",
        "observed_diagnostic": "OBS module metadata does not match development-foundation contract",
        "before": [
          {
            "path": "src/plugin/module.c",
            "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
          }
        ],
        "mutated": [
          {
            "path": "src/plugin/module.c",
            "sha256": "608f7c48d09ad85b57cce8de7cc22fedeeea6017210dfd2cc37efc9da082004b"
          }
        ],
        "after": [
          {
            "path": "src/plugin/module.c",
            "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-PROBE-ARGUMENTS",
      "acceptance": [
        "AC-AUTH",
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "probe-arguments"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Return exit 0 for rejected native probe arguments while preserving independent CLI assertions expecting exit 5.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/probe-arguments.txt",
        "sha256": "397b7bd7b3e2bdff4d6f37f5edc63b5695b72479f3fd435a4141cd9bdc580424"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/address.hpp",
          "sha256": "9889fa8f7c51d4236dc5d3e00c46167952f2879478da0842e0335eca28ebc6ed"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/discovery.hpp",
          "sha256": "7a262e29abb007fadad655d5d172cd138d46485162dd651d26deb26ea2b74094"
        },
        {
          "path": "include/mgl/rfcomm.hpp",
          "sha256": "66d2fc76d1d8e2e74049c3e33304298e5b08cfcaa5b34da2bc5b80d551003483"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "include/mgl/session.hpp",
          "sha256": null
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/probe/private_input.cpp",
          "sha256": "5c251d6eacf7d06803f085505aeb485f7627620ee989f13925a85fc1304d4e4c"
        },
        {
          "path": "src/probe/private_input.hpp",
          "sha256": "b90870ed2085b7aebe46726f517252c8fe2c168d15341b01eb16fdda0654cc43"
        },
        {
          "path": "src/protocol/address.cpp",
          "sha256": "c3004bc33bb615eee1e285a7666fb28bff020a22f412f366381be8aa065d5eaf"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/apartment.hpp",
          "sha256": "c392d69883555a5da3111f7e5a2f916a0e36b812ad609494f8fac00cf0d3ef29"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "src/windows/classic.cpp",
          "sha256": "ece9d6024a38f9d81fab43bdba941173b47153d0b9bacab449bc8f1b2c4fea13"
        },
        {
          "path": "src/windows/discovery.cpp",
          "sha256": "5066920f2c8bb1fc0dd143857978b72668e3e41c18aedb7af8ba95f4145fcccf"
        },
        {
          "path": "src/windows/identity.cpp",
          "sha256": null
        },
        {
          "path": "src/windows/rfcomm.cpp",
          "sha256": "9da6dbd1321ba3be1d7746e65a56ea9fa02354eedfb385c36bbce0f11742ae20"
        },
        {
          "path": "src/windows/session.cpp",
          "sha256": null
        },
        {
          "path": "tests/address_tests.cpp",
          "sha256": "e9e3311b0609a4eef7ab4fcfa3a55943ae20600fd07978606cdfadc65412e901"
        },
        {
          "path": "tests/live.Tests.ps1",
          "sha256": "efd413d3f5bfc27aa403fcfd99f61da5fc58271ba90527d57fb9914945bfd65f"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/probe.Tests.ps1",
          "sha256": "6e241cf9154c67044628d35fa5f3b0c6b92c9139dc666d6ed9adfab082ddc064"
        },
        {
          "path": "tests/probe_support.ps1",
          "sha256": "ad53ccd95cc106348768cb5889de2be8608401934064ec3c361466e66ed8c4a2"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        },
        {
          "path": "tests/session_tests.cpp",
          "sha256": null
        }
      ],
      "scope_sha256": "1cffc8d0c3343417798b228b939daca34c529768b6b1048f7978056d9f9b4bc7",
      "red": {
        "mutation": "Return exit 0 for rejected native probe arguments while preserving independent CLI assertions expecting exit 5.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "invalid probe arguments must fail",
        "observed_diagnostic": "Expected 5, because invalid probe arguments must fail, but got 0.",
        "before": [
          {
            "path": "src/probe/main.cpp",
            "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
          }
        ],
        "mutated": [
          {
            "path": "src/probe/main.cpp",
            "sha256": "5cd0c1be2b217474a26731591506ae9eb603f8125d5eedeca4886a83820d24ee"
          }
        ],
        "after": [
          {
            "path": "src/probe/main.cpp",
            "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-CLASSIC-CONNECTION-CLAIM",
      "acceptance": [
        "AC-AUTH",
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "classic-connection-claim"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Initialize Classic diagnostic result with rfcomm_connected true; independent query-only CLI assertion must reject the false connection claim.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/classic-connection-claim.txt",
        "sha256": "6dc8271cf21af53e0c3e0a595a29a51c66086867d4e690e8aef0d2a76c904c88"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/address.hpp",
          "sha256": "9889fa8f7c51d4236dc5d3e00c46167952f2879478da0842e0335eca28ebc6ed"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/discovery.hpp",
          "sha256": "7a262e29abb007fadad655d5d172cd138d46485162dd651d26deb26ea2b74094"
        },
        {
          "path": "include/mgl/rfcomm.hpp",
          "sha256": "66d2fc76d1d8e2e74049c3e33304298e5b08cfcaa5b34da2bc5b80d551003483"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "include/mgl/session.hpp",
          "sha256": null
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/probe/private_input.cpp",
          "sha256": "5c251d6eacf7d06803f085505aeb485f7627620ee989f13925a85fc1304d4e4c"
        },
        {
          "path": "src/probe/private_input.hpp",
          "sha256": "b90870ed2085b7aebe46726f517252c8fe2c168d15341b01eb16fdda0654cc43"
        },
        {
          "path": "src/protocol/address.cpp",
          "sha256": "c3004bc33bb615eee1e285a7666fb28bff020a22f412f366381be8aa065d5eaf"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/apartment.hpp",
          "sha256": "c392d69883555a5da3111f7e5a2f916a0e36b812ad609494f8fac00cf0d3ef29"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "src/windows/classic.cpp",
          "sha256": "ece9d6024a38f9d81fab43bdba941173b47153d0b9bacab449bc8f1b2c4fea13"
        },
        {
          "path": "src/windows/discovery.cpp",
          "sha256": "5066920f2c8bb1fc0dd143857978b72668e3e41c18aedb7af8ba95f4145fcccf"
        },
        {
          "path": "src/windows/identity.cpp",
          "sha256": null
        },
        {
          "path": "src/windows/rfcomm.cpp",
          "sha256": "9da6dbd1321ba3be1d7746e65a56ea9fa02354eedfb385c36bbce0f11742ae20"
        },
        {
          "path": "src/windows/session.cpp",
          "sha256": null
        },
        {
          "path": "tests/address_tests.cpp",
          "sha256": "e9e3311b0609a4eef7ab4fcfa3a55943ae20600fd07978606cdfadc65412e901"
        },
        {
          "path": "tests/live.Tests.ps1",
          "sha256": "efd413d3f5bfc27aa403fcfd99f61da5fc58271ba90527d57fb9914945bfd65f"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/probe.Tests.ps1",
          "sha256": "6e241cf9154c67044628d35fa5f3b0c6b92c9139dc666d6ed9adfab082ddc064"
        },
        {
          "path": "tests/probe_support.ps1",
          "sha256": "ad53ccd95cc106348768cb5889de2be8608401934064ec3c361466e66ed8c4a2"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        },
        {
          "path": "tests/session_tests.cpp",
          "sha256": null
        }
      ],
      "scope_sha256": "1cffc8d0c3343417798b228b939daca34c529768b6b1048f7978056d9f9b4bc7",
      "red": {
        "mutation": "Initialize Classic diagnostic result with rfcomm_connected true; independent query-only CLI assertion must reject the false connection claim.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Classic discovery must not claim an RFCOMM connection",
        "observed_diagnostic": "Expected $false, because Classic discovery must not claim an RFCOMM connection, but got $true.",
        "before": [
          {
            "path": "src/windows/classic.cpp",
            "sha256": "ece9d6024a38f9d81fab43bdba941173b47153d0b9bacab449bc8f1b2c4fea13"
          }
        ],
        "mutated": [
          {
            "path": "src/windows/classic.cpp",
            "sha256": "b9e2e406914924ef1ef9cc3056d9300a9ed406a6647dfb7af9e1816524cec56c"
          }
        ],
        "after": [
          {
            "path": "src/windows/classic.cpp",
            "sha256": "ece9d6024a38f9d81fab43bdba941173b47153d0b9bacab449bc8f1b2c4fea13"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-PRIVATE-ADDRESS-VALIDATION",
      "acceptance": [
        "AC-AUTH",
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "private-address-validation"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Remove zero-address rejection while independent reserved-address examples remain unchanged.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/private-address-validation.txt",
        "sha256": "dc4eb9b44928eb3190583c6150bde95e859c269624f08cde8af9443e692efd88"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/address.hpp",
          "sha256": "9889fa8f7c51d4236dc5d3e00c46167952f2879478da0842e0335eca28ebc6ed"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/discovery.hpp",
          "sha256": "7a262e29abb007fadad655d5d172cd138d46485162dd651d26deb26ea2b74094"
        },
        {
          "path": "include/mgl/rfcomm.hpp",
          "sha256": "66d2fc76d1d8e2e74049c3e33304298e5b08cfcaa5b34da2bc5b80d551003483"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "include/mgl/session.hpp",
          "sha256": null
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/probe/private_input.cpp",
          "sha256": "5c251d6eacf7d06803f085505aeb485f7627620ee989f13925a85fc1304d4e4c"
        },
        {
          "path": "src/probe/private_input.hpp",
          "sha256": "b90870ed2085b7aebe46726f517252c8fe2c168d15341b01eb16fdda0654cc43"
        },
        {
          "path": "src/protocol/address.cpp",
          "sha256": "c3004bc33bb615eee1e285a7666fb28bff020a22f412f366381be8aa065d5eaf"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/apartment.hpp",
          "sha256": "c392d69883555a5da3111f7e5a2f916a0e36b812ad609494f8fac00cf0d3ef29"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "src/windows/classic.cpp",
          "sha256": "ece9d6024a38f9d81fab43bdba941173b47153d0b9bacab449bc8f1b2c4fea13"
        },
        {
          "path": "src/windows/discovery.cpp",
          "sha256": "5066920f2c8bb1fc0dd143857978b72668e3e41c18aedb7af8ba95f4145fcccf"
        },
        {
          "path": "src/windows/identity.cpp",
          "sha256": null
        },
        {
          "path": "src/windows/rfcomm.cpp",
          "sha256": "9da6dbd1321ba3be1d7746e65a56ea9fa02354eedfb385c36bbce0f11742ae20"
        },
        {
          "path": "src/windows/session.cpp",
          "sha256": null
        },
        {
          "path": "tests/address_tests.cpp",
          "sha256": "e9e3311b0609a4eef7ab4fcfa3a55943ae20600fd07978606cdfadc65412e901"
        },
        {
          "path": "tests/live.Tests.ps1",
          "sha256": "efd413d3f5bfc27aa403fcfd99f61da5fc58271ba90527d57fb9914945bfd65f"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/probe.Tests.ps1",
          "sha256": "6e241cf9154c67044628d35fa5f3b0c6b92c9139dc666d6ed9adfab082ddc064"
        },
        {
          "path": "tests/probe_support.ps1",
          "sha256": "ad53ccd95cc106348768cb5889de2be8608401934064ec3c361466e66ed8c4a2"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        },
        {
          "path": "tests/session_tests.cpp",
          "sha256": null
        }
      ],
      "scope_sha256": "1cffc8d0c3343417798b228b939daca34c529768b6b1048f7978056d9f9b4bc7",
      "red": {
        "mutation": "Remove zero-address rejection while independent reserved-address examples remain unchanged.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Malformed/reserved Bluetooth address accepted",
        "observed_diagnostic": "Malformed/reserved Bluetooth address accepted",
        "before": [
          {
            "path": "src/protocol/address.cpp",
            "sha256": "c3004bc33bb615eee1e285a7666fb28bff020a22f412f366381be8aa065d5eaf"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/address.cpp",
            "sha256": "5a034e94fa76c3213a1d534cba9143cb20e59c5d2311a11ba869b102946ad25f"
          }
        ],
        "after": [
          {
            "path": "src/protocol/address.cpp",
            "sha256": "c3004bc33bb615eee1e285a7666fb28bff020a22f412f366381be8aa065d5eaf"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-UNCACHED-QUERY-LOOP",
      "acceptance": [
        "AC-AUTH",
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "uncached-query-loop"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Skip all collected Classic addresses; the actual live-query assertion must reject zero queries when inquiry reports peers.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/uncached-query-loop.txt",
        "sha256": "726aa29c11e70de243525859a94d0149b8523bf6797e2f23e301906bbcc4de2d"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/address.hpp",
          "sha256": "9889fa8f7c51d4236dc5d3e00c46167952f2879478da0842e0335eca28ebc6ed"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/discovery.hpp",
          "sha256": "7a262e29abb007fadad655d5d172cd138d46485162dd651d26deb26ea2b74094"
        },
        {
          "path": "include/mgl/rfcomm.hpp",
          "sha256": "66d2fc76d1d8e2e74049c3e33304298e5b08cfcaa5b34da2bc5b80d551003483"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "include/mgl/session.hpp",
          "sha256": null
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/probe/private_input.cpp",
          "sha256": "5c251d6eacf7d06803f085505aeb485f7627620ee989f13925a85fc1304d4e4c"
        },
        {
          "path": "src/probe/private_input.hpp",
          "sha256": "b90870ed2085b7aebe46726f517252c8fe2c168d15341b01eb16fdda0654cc43"
        },
        {
          "path": "src/protocol/address.cpp",
          "sha256": "c3004bc33bb615eee1e285a7666fb28bff020a22f412f366381be8aa065d5eaf"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/apartment.hpp",
          "sha256": "c392d69883555a5da3111f7e5a2f916a0e36b812ad609494f8fac00cf0d3ef29"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "src/windows/classic.cpp",
          "sha256": "ece9d6024a38f9d81fab43bdba941173b47153d0b9bacab449bc8f1b2c4fea13"
        },
        {
          "path": "src/windows/discovery.cpp",
          "sha256": "5066920f2c8bb1fc0dd143857978b72668e3e41c18aedb7af8ba95f4145fcccf"
        },
        {
          "path": "src/windows/identity.cpp",
          "sha256": null
        },
        {
          "path": "src/windows/rfcomm.cpp",
          "sha256": "9da6dbd1321ba3be1d7746e65a56ea9fa02354eedfb385c36bbce0f11742ae20"
        },
        {
          "path": "src/windows/session.cpp",
          "sha256": null
        },
        {
          "path": "tests/address_tests.cpp",
          "sha256": "e9e3311b0609a4eef7ab4fcfa3a55943ae20600fd07978606cdfadc65412e901"
        },
        {
          "path": "tests/live.Tests.ps1",
          "sha256": "efd413d3f5bfc27aa403fcfd99f61da5fc58271ba90527d57fb9914945bfd65f"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/probe.Tests.ps1",
          "sha256": "6e241cf9154c67044628d35fa5f3b0c6b92c9139dc666d6ed9adfab082ddc064"
        },
        {
          "path": "tests/probe_support.ps1",
          "sha256": "ad53ccd95cc106348768cb5889de2be8608401934064ec3c361466e66ed8c4a2"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        },
        {
          "path": "tests/session_tests.cpp",
          "sha256": null
        }
      ],
      "scope_sha256": "1cffc8d0c3343417798b228b939daca34c529768b6b1048f7978056d9f9b4bc7",
      "red": {
        "mutation": "Skip all collected Classic addresses; the actual live-query assertion must reject zero queries when inquiry reports peers.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Live discovery must query available Classic peers",
        "observed_diagnostic": "Expected the actual value to be greater than 0, because Live discovery must query available Classic peers, but got 0.",
        "before": [
          {
            "path": "src/windows/classic.cpp",
            "sha256": "ece9d6024a38f9d81fab43bdba941173b47153d0b9bacab449bc8f1b2c4fea13"
          }
        ],
        "mutated": [
          {
            "path": "src/windows/classic.cpp",
            "sha256": "a4ed5b4d963ba79277e5149b1efcfeefb1fa78526a9bad943c89e600a2c2d2a4"
          }
        ],
        "after": [
          {
            "path": "src/windows/classic.cpp",
            "sha256": "ece9d6024a38f9d81fab43bdba941173b47153d0b9bacab449bc8f1b2c4fea13"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-FORMAT",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "format"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Append an intentionally unformatted function.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/format.txt",
        "sha256": "2709604244b5ea4ab4da49131d6d193bc361311b575793332a501dea5dee3c3b"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "d70cca17793217ef03029c6825bfec8e1e743fbee2e1b86cdccd7a34a37dbf4d",
      "red": {
        "mutation": "Append an intentionally unformatted function.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "clang-format-violations",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp:47:22: error: code should be clang-formatted [-Wclang-format-violations]",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "c2a1d1a6c8b7489cbb25561dddadb31309d7ac5c15f880bbeb3c90288eabb038"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-OWNED-HEADER",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "owned-header"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Append a mutable inline global to an owned header.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/owned-header.txt",
        "sha256": "54ad09bd143fda5c64a6f81de8c8690c5bc6dde921b1341f551502b14c75ef4e"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "d70cca17793217ef03029c6825bfec8e1e743fbee2e1b86cdccd7a34a37dbf4d",
      "red": {
        "mutation": "Append a mutable inline global to an owned header.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "cppcoreguidelines-avoid-non-const-global-variables",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\include\\mgl/sdu.hpp:23:12: error: variable 'red_drill_global' is non-const and globally accessible, consider making it const [cppcoreguidelines-avoid-non-const-global-variables,-warnings-as-errors]",
        "before": [
          {
            "path": "include/mgl/sdu.hpp",
            "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
          }
        ],
        "mutated": [
          {
            "path": "include/mgl/sdu.hpp",
            "sha256": "220f387016dea2b2cb31874833a09fd753ad9998873937e5cdfba7cb762fe0ec"
          }
        ],
        "after": [
          {
            "path": "include/mgl/sdu.hpp",
            "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-COMPILER-TYPES",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "compiler-types"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Initialize an integer constant from a string literal.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/compiler-types.txt",
        "sha256": "c8ff9c58f75c4bb99604940bb5fb1192e3d85bc2d05cbcb4c228aa74b9dace72"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "d70cca17793217ef03029c6825bfec8e1e743fbee2e1b86cdccd7a34a37dbf4d",
      "red": {
        "mutation": "Initialize an integer constant from a string literal.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "cannot convert",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp(47): error C2440: 'initializing': cannot convert from 'const char [16]' to 'const int'",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "4d3f32e4a31af074ff96c5ef217d361573917be607688d49acd001562970bc96"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-UNUSED-CODE",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "unused-code"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Append an unreferenced function to owned protocol source.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/unused-code.txt",
        "sha256": "acd186564cc229e1b6ff0bc80493edd2d40fe9f964ac3efee1e703373c240d25"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "d70cca17793217ef03029c6825bfec8e1e743fbee2e1b86cdccd7a34a37dbf4d",
      "red": {
        "mutation": "Append an unreferenced function to owned protocol source.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "unusedFunction",
        "observed_diagnostic": "C:\\Users\\Randy\\Coding\\meta_g_link_direct\\src\\protocol\\sdu.cpp:47:5: style: The function 'red_drill_unused_function' is never used. [unusedFunction]",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "56c7aca4962c137d9b9bfddaedb756fb220fc33620f0f3888c0b25de525b780a"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-SCRIPT-ANALYSIS",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "script-analysis"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Append a function with an unapproved PowerShell verb.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/script-analysis.txt",
        "sha256": "9f09f502ed3e675dd5298a937d14a18f4a13e1da98956865e97c5ae2e8587c23"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "d70cca17793217ef03029c6825bfec8e1e743fbee2e1b86cdccd7a34a37dbf4d",
      "red": {
        "mutation": "Append a function with an unapproved PowerShell verb.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "PSUseApprovedVerbs",
        "observed_diagnostic": "PSUseApprovedVerbs Warning  Invoke-Project.ps1 525  The cmdlet 'Badverb-RedDrill' uses an unapproved verb.",
        "before": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
          }
        ],
        "mutated": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "b1725ff57af94a3f017d8be7999bf368a739b184506942d7daf7fa00ba379835"
          }
        ],
        "after": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-WORKFLOW-SECURITY",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "workflow-security"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Replace the pinned checkout commit SHA with a movable version tag.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/workflow-security.txt",
        "sha256": "19e4cb7fa962c8103d15f12ac99efbf79c284e97c1eaf4b251064f3e7892133b"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "d70cca17793217ef03029c6825bfec8e1e743fbee2e1b86cdccd7a34a37dbf4d",
      "red": {
        "mutation": "Replace the pinned checkout commit SHA with a movable version tag.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "unpinned-uses",
        "observed_diagnostic": "error[unpinned-uses]: unpinned action reference",
        "before": [
          {
            "path": ".github/workflows/quality.yml",
            "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
          }
        ],
        "mutated": [
          {
            "path": ".github/workflows/quality.yml",
            "sha256": "a151bb006b4d7e858647ffe9c81521c98e2e2d9a3c12cfa60555e229a57afd76"
          }
        ],
        "after": [
          {
            "path": ".github/workflows/quality.yml",
            "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-SECRET-SCANNER",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "secret-scanner"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Append a randomized base32 AWS-key-shaped synthetic canary; scanner output is redacted.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/secret-scanner.txt",
        "sha256": "bd2d98f743c99d695942884b4a8d765cc453324a9756238ea4ca612bd20a6e61"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "d70cca17793217ef03029c6825bfec8e1e743fbee2e1b86cdccd7a34a37dbf4d",
      "red": {
        "mutation": "Append a randomized base32 AWS-key-shaped synthetic canary; scanner output is redacted.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "leaks found",
        "observed_diagnostic": "7:56PM WRN leaks found: 1",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "580afd006194e758422130003b587130953e92507ba6e4fa02c90daf1e5878a8"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-UNSAFE-COPY",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "unsafe-copy"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Append an unbounded strcpy call.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/unsafe-copy.txt",
        "sha256": "1fb6e8bd116024b4aada448ed818f1cc8f01945ff39b9a65bc87e913107c76bc"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "d70cca17793217ef03029c6825bfec8e1e743fbee2e1b86cdccd7a34a37dbf4d",
      "red": {
        "mutation": "Append an unbounded strcpy call.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "mgl-unsafe-string-copy",
        "observed_diagnostic": "\u276f\u276f\u2771 mgl-unsafe-string-copy",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "8c973ff6b7b7e0be339225d7c4a427203011da68c5fda6414bea6ec6c0ea0b03"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-DUPLICATED-CODE",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "duplicated-code"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Insert a duplicate frame-prefix implementation under another function name.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/duplicated-code.txt",
        "sha256": "8cf5926e14a608a0087dd891e5d41ff6e8ff6a5ff2d8eb99167f4983f2245910"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "d70cca17793217ef03029c6825bfec8e1e743fbee2e1b86cdccd7a34a37dbf4d",
      "red": {
        "mutation": "Insert a duplicate frame-prefix implementation under another function name.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Found 1 clones",
        "observed_diagnostic": "Found 1 clones.",
        "before": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "21406acfb851f1a583b2a8e742638e4eb5e5f367c7195ef607eb5862fcd8d6df"
          }
        ],
        "after": [
          {
            "path": "src/protocol/sdu.cpp",
            "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-ADDRESS-SANITIZER",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "address-sanitizer"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Inject a runtime heap-buffer read past a one-element allocation in test code.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/address-sanitizer.txt",
        "sha256": "bacbfb82e271083784584ec4219d769f23023e0be65230c03c2bc940e7d6c57f"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "d70cca17793217ef03029c6825bfec8e1e743fbee2e1b86cdccd7a34a37dbf4d",
      "red": {
        "mutation": "Inject a runtime heap-buffer read past a one-element allocation in test code.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "AddressSanitizer: heap-buffer-overflow",
        "observed_diagnostic": "==6888==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x124f8b1a0034 at pc 0x7ff6657f53de bp 0x00b1124ff980 sp 0x00b1124ff988",
        "before": [
          {
            "path": "tests/sdu_tests.cpp",
            "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
          }
        ],
        "mutated": [
          {
            "path": "tests/sdu_tests.cpp",
            "sha256": "f0b8a6f71a8f96f071d86024029c536756dd654d9beaea73b9a22d9535147d2e"
          }
        ],
        "after": [
          {
            "path": "tests/sdu_tests.cpp",
            "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-DEPENDENCY-NPM",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "dependency-npm"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Temporarily add vulnerable lodash 4.17.20 metadata to the lockfile without installing it.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/dependency-npm.txt",
        "sha256": "02d6d1b6cfe118449d8746927bd3e88e5cd96acd688c11dee6c8ce730f364c6b"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "d70cca17793217ef03029c6825bfec8e1e743fbee2e1b86cdccd7a34a37dbf4d",
      "red": {
        "mutation": "Temporarily add vulnerable lodash 4.17.20 metadata to the lockfile without installing it.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "GHSA-35jh-r3h4-6jhm",
        "observed_diagnostic": "Command Injection in lodash - https://github.com/advisories/GHSA-35jh-r3h4-6jhm",
        "before": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ],
        "mutated": [
          {
            "path": "package-lock.json",
            "sha256": "55a94e0ca609f9c43263164d98163a9057daf13c65dd980eccc570ad31f8c11f"
          }
        ],
        "after": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-DEPENDENCY-OSV",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "dependency-osv"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Temporarily add vulnerable lodash 4.17.20 metadata to the lockfile without installing it.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/dependency-osv.txt",
        "sha256": "f69a751cc156fac41ba1870517348840467545934df5b45820112a96ad32fd58"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "d70cca17793217ef03029c6825bfec8e1e743fbee2e1b86cdccd7a34a37dbf4d",
      "red": {
        "mutation": "Temporarily add vulnerable lodash 4.17.20 metadata to the lockfile without installing it.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "GHSA-35jh-r3h4-6jhm",
        "observed_diagnostic": "| https://osv.dev/GHSA-35jh-r3h4-6jhm | 8.1  | npm       | lodash (dev) | 4.17.20 | 4.17.21       | package-lock.json |",
        "before": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ],
        "mutated": [
          {
            "path": "package-lock.json",
            "sha256": "55a94e0ca609f9c43263164d98163a9057daf13c65dd980eccc570ad31f8c11f"
          }
        ],
        "after": [
          {
            "path": "package-lock.json",
            "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-WHATIF-GUARD",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "whatif-guard"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Bypass the common ShouldProcess guard; the disposable WhatIf effect assertion must fail.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/whatif-guard.txt",
        "sha256": "78c2080832117d93ca4205fd9bc7e90ca45facfb2349efca4e543fefa143cbb3"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "d70cca17793217ef03029c6825bfec8e1e743fbee2e1b86cdccd7a34a37dbf4d",
      "red": {
        "mutation": "Bypass the common ShouldProcess guard; the disposable WhatIf effect assertion must fail.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Expected strings to be the same",
        "observed_diagnostic": "Expected strings to be the same, but they were different.",
        "before": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
          }
        ],
        "mutated": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "46085646b830cbf8934fd2b6828292d63d691d4981217b8ecbda296961adf3e3"
          }
        ],
        "after": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-RED-NATIVE-EXIT-PROPAGATION",
      "acceptance": [
        "AC-GATES"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "test:red",
        "-Drill",
        "native-exit-propagation"
      ],
      "method": "Inspected actual intended diagnostic/nonzero failure and independently verified exact current-byte restoration. Bypass the native exit-code guard; the real formatter-failure assertion must fail.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/native-exit-propagation.txt",
        "sha256": "68df529b6e99d6fe3e383a2c46371431d901bf38c2eae18462b3f96ae8cf937e"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "d70cca17793217ef03029c6825bfec8e1e743fbee2e1b86cdccd7a34a37dbf4d",
      "red": {
        "mutation": "Bypass the native exit-code guard; the real formatter-failure assertion must fail.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "Expected 0 to be different from the actual value",
        "observed_diagnostic": "Expected 0 to be different from the actual value, but got the same value.",
        "before": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
          }
        ],
        "mutated": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "addca31cab3aef953146883e7b6626ce9196baa65e684080acc5ed4737ef3d9b"
          }
        ],
        "after": [
          {
            "path": "scripts/Invoke-Project.ps1",
            "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
          }
        ]
      },
      "reason": null
    },
    {
      "id": "EV-EXISTING-ADAPTER-HARDWARE-DEFERRED",
      "acceptance": [
        "AC-AUTH"
      ],
      "kind": "manual",
      "status": "deferred",
      "command": [],
      "method": "Actual initial/controlled/longer uncached results and canceled native picker are recorded. Two peers queried, zero Meta services and one timeout; paired headphones are separate. No trustworthy glasses address, direct socket/hello, authenticated device info, firmware or phone-off session proof.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/EXISTING_ADAPTER_VERIFICATION.md",
        "sha256": "da6e0dfb00e71f0e402b4c863b3672b21174f087b21613d42c120966a7612a48"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/address.hpp",
          "sha256": "9889fa8f7c51d4236dc5d3e00c46167952f2879478da0842e0335eca28ebc6ed"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/discovery.hpp",
          "sha256": "7a262e29abb007fadad655d5d172cd138d46485162dd651d26deb26ea2b74094"
        },
        {
          "path": "include/mgl/rfcomm.hpp",
          "sha256": "66d2fc76d1d8e2e74049c3e33304298e5b08cfcaa5b34da2bc5b80d551003483"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "include/mgl/session.hpp",
          "sha256": null
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/probe/private_input.cpp",
          "sha256": "5c251d6eacf7d06803f085505aeb485f7627620ee989f13925a85fc1304d4e4c"
        },
        {
          "path": "src/probe/private_input.hpp",
          "sha256": "b90870ed2085b7aebe46726f517252c8fe2c168d15341b01eb16fdda0654cc43"
        },
        {
          "path": "src/protocol/address.cpp",
          "sha256": "c3004bc33bb615eee1e285a7666fb28bff020a22f412f366381be8aa065d5eaf"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/apartment.hpp",
          "sha256": "c392d69883555a5da3111f7e5a2f916a0e36b812ad609494f8fac00cf0d3ef29"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "src/windows/classic.cpp",
          "sha256": "ece9d6024a38f9d81fab43bdba941173b47153d0b9bacab449bc8f1b2c4fea13"
        },
        {
          "path": "src/windows/discovery.cpp",
          "sha256": "5066920f2c8bb1fc0dd143857978b72668e3e41c18aedb7af8ba95f4145fcccf"
        },
        {
          "path": "src/windows/identity.cpp",
          "sha256": null
        },
        {
          "path": "src/windows/rfcomm.cpp",
          "sha256": "9da6dbd1321ba3be1d7746e65a56ea9fa02354eedfb385c36bbce0f11742ae20"
        },
        {
          "path": "src/windows/session.cpp",
          "sha256": null
        },
        {
          "path": "tests/address_tests.cpp",
          "sha256": "e9e3311b0609a4eef7ab4fcfa3a55943ae20600fd07978606cdfadc65412e901"
        },
        {
          "path": "tests/live.Tests.ps1",
          "sha256": "efd413d3f5bfc27aa403fcfd99f61da5fc58271ba90527d57fb9914945bfd65f"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/probe.Tests.ps1",
          "sha256": "6e241cf9154c67044628d35fa5f3b0c6b92c9139dc666d6ed9adfab082ddc064"
        },
        {
          "path": "tests/probe_support.ps1",
          "sha256": "ad53ccd95cc106348768cb5889de2be8608401934064ec3c361466e66ed8c4a2"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        },
        {
          "path": "tests/session_tests.cpp",
          "sha256": null
        }
      ],
      "scope_sha256": "1cffc8d0c3343417798b228b939daca34c529768b6b1048f7978056d9f9b4bc7",
      "red": null,
      "reason": "Glasses Classic address is unidentified; Meta AI Bluetooth-address availability question pending. Existing adapter remains required. Application authentication/identity/Meta setup and all media/OBS gates remain incomplete; do not guess an address or change ownership."
    },
    {
      "id": "EV-EXISTING-ADAPTER-GIT-INTEGRITY",
      "acceptance": [
        "AC-FRAMING",
        "AC-MODULE",
        "AC-GATES"
      ],
      "kind": "behavior",
      "status": "pass",
      "command": [
        "gitleaks",
        "git",
        "--redact",
        "--no-banner",
        "."
      ],
      "method": "After source commit, independently verified 136 committed source/artifact byte sets against ledger digests and observed a successful full reachable-history secret scan. Source commit staged-secrets and format hooks passed. No new hardware claim.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/existing-adapter-git-integrity.txt",
        "sha256": "8367b0eb3b5c13bced6726a714716fbeec81ac8696de229fdf23f07ca78497e1"
      },
      "inputs": [
        {
          "path": ".clang-format",
          "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
        },
        {
          "path": ".clang-tidy",
          "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
        },
        {
          "path": ".cppcheck-obs.cfg",
          "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
        },
        {
          "path": ".gitattributes",
          "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
        },
        {
          "path": ".github/dependabot.yml",
          "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
        },
        {
          "path": ".github/workflows/quality.yml",
          "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
        },
        {
          "path": ".gitignore",
          "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
        },
        {
          "path": ".gitleaks.toml",
          "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
        },
        {
          "path": ".jscpd.json",
          "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
        },
        {
          "path": ".pre-commit-config.yaml",
          "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
        },
        {
          "path": ".semgrep.yml",
          "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
        },
        {
          "path": "AGENTS.md",
          "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
        },
        {
          "path": "CHANGELOG.md",
          "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
        },
        {
          "path": "CMakeLists.txt",
          "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
        },
        {
          "path": "CMakePresets.json",
          "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
        },
        {
          "path": "LICENSE",
          "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
        },
        {
          "path": "PROJECT_BRIEF.md",
          "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
        },
        {
          "path": "PSScriptAnalyzerSettings.psd1",
          "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
        },
        {
          "path": "README.md",
          "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
        },
        {
          "path": "THIRD_PARTY_NOTICES.md",
          "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
        },
        {
          "path": "dependencies.lock.json",
          "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
        },
        {
          "path": "include/mgl/capabilities.hpp",
          "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
        },
        {
          "path": "include/mgl/sdu.hpp",
          "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
        },
        {
          "path": "package-lock.json",
          "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
        },
        {
          "path": "package.json",
          "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
        },
        {
          "path": "src/plugin/module.c",
          "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
        },
        {
          "path": "src/protocol/sdu.cpp",
          "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
        },
        {
          "path": "src/windows/capabilities.cpp",
          "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
        },
        {
          "path": "tests/module_tests.cpp",
          "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
        },
        {
          "path": "tests/runner.Tests.ps1",
          "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
        },
        {
          "path": "tests/sdu_tests.cpp",
          "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
        }
      ],
      "scope_sha256": "d70cca17793217ef03029c6825bfec8e1e743fbee2e1b86cdccd7a34a37dbf4d",
      "red": null,
      "reason": null
    },
    {
      "id": "EV-WIFI-LINK-HARDWARE",
      "acceptance": [
        "AC-MEDIA"
      ],
      "kind": "manual",
      "status": "pass",
      "command": [],
      "method": "Observed on real glasses, phone Bluetooth off: glasses-hosted 2.4 GHz / 20 MHz network via CreateSoftApNetwork; 5,370 frames in about 179 s (standalone harness), 3,127 frames in a 120 s production MediaSession run, two back-to-back 30 s runs, Bluetooth close acknowledged, network gone 8-14 s after a stream on fresh scans; on 2026-10-19 the DHCP-detection rewrite joined in 4.1 s and streamed 1,104 frames in 60 s. Every earlier join-an-access-point path stopped video within 8-54 s.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/WIFI_LINK_VERIFICATION.md",
        "sha256": "d0d6a035809793e86fbace8c9827f5252b62256ec0d434d77ea140c470f7b464"
      },
      "inputs": [
        {
          "path": "CMakeLists.txt",
          "sha256": "edf508bd8b91c85951f555ede63c42436b9cf8935c8d7dde09ea70b81b681108"
        },
        {
          "path": "include/mgl/stream.hpp",
          "sha256": "0588814c1b39e08aec407542a2d973c71cfb442f72e941081c00be45d686508a"
        },
        {
          "path": "src/protocol/stream.cpp",
          "sha256": "4f58f94c3ecfd5c06ed85a78d0ef8b7e9e163641bf9d727f43d7f93e511345a5"
        },
        {
          "path": "include/mgl/wifi_station.hpp",
          "sha256": "ccc16d781a97b2cad0c0491543e25ea7a3137b795fadf2d37b72c556e2be9151"
        },
        {
          "path": "src/windows/wifi_station.cpp",
          "sha256": "4c9ebc5d0e62a91ffe0613a3a9eb616f78161609a6f0854bc1bea03c1f3b01e1"
        },
        {
          "path": "src/windows/wifi_credentials.hpp",
          "sha256": "fdaaedd93bc48b18f0b5833f1ae7abde37d4035ad42ba610b9dc72f9e4fd9fe6"
        },
        {
          "path": "include/mgl/media_session.hpp",
          "sha256": "f72e0e09aa11a591a69d54c4d91447d81fae13906cb812db26a2c670e105a10c"
        },
        {
          "path": "src/windows/media_session.cpp",
          "sha256": "d1c912e636038eb4c96db2b105ad07855ac46d35f56dd530c0ef74e467c6fcc3"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6fb691e1b34541ba2cb19cacae66942682047ef9f60fe3536d5d2c8f38cf778b"
        },
        {
          "path": "tests/protocol_tests.cpp",
          "sha256": "f56c414cc9b814861aaffcbb40d35983e4cd6e6cbc1858716eec986816ecb7f9"
        },
        {
          "path": "include/mgl/wifi_group.hpp",
          "sha256": null
        },
        {
          "path": "src/windows/wifi_group.cpp",
          "sha256": null
        },
        {
          "path": "src/windows/wifi_hotspot.cpp",
          "sha256": null
        }
      ],
      "scope_sha256": null,
      "red": null,
      "reason": "The scope fingerprint tool behind earlier entries is not available in this session; inputs carry their own hashes (null = deleted file)."
    },
    {
      "id": "EV-WIFI-LINK-SOFTWARE",
      "acceptance": [
        "AC-MEDIA",
        "AC-GATES"
      ],
      "kind": "behavior",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "scripts/Invoke-Project.ps1",
        "package"
      ],
      "method": "The 0.2.0 release gate (package): configure, build, format:check, lint, deadcode, security:audit, test, test:live, test:red each exited 0; 4 CTest suites (90 protocol cases), 28 Pester cases and 21 red drills passed; release zip SHA-256 debb454b0990d21902acbf30ebb077c244873a78089d3b3ff5d144b8a9c7b693.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/wifi-link-gates.txt",
        "sha256": "bd3b3cc2b66bfab56affe8fd1d7614c2d298f94852615ce7616374ed8db5c537"
      },
      "inputs": [
        {
          "path": "CMakeLists.txt",
          "sha256": "b903d25825336f84b5a2f596ce6f3a9c86fd1e93a15e7139a313ec35dda77d11"
        },
        {
          "path": "include/mgl/stream.hpp",
          "sha256": "5cf40f97cac5c92a4197173585e6d421c63b709cf9b5c3468883f6ec38d99f65"
        },
        {
          "path": "src/protocol/stream.cpp",
          "sha256": "6514fd1d35dd9c344a3abeac53ea44590f00a725ca2e2aae721e08478655e576"
        },
        {
          "path": "include/mgl/media_session.hpp",
          "sha256": "c6a01be1339133961b2b20f6c5a2a0f21d574cad8d66466ec627b50e8b2f4397"
        },
        {
          "path": "src/windows/media_session.cpp",
          "sha256": "f09f551bf388fba131cd5db7c4c9c2efba455c2a5b14889a1e981117f1bb1207"
        },
        {
          "path": "include/mgl/wifi_station.hpp",
          "sha256": "ac6c3df9b17f8a7bc2fbbf92f918008bd7b2929d76dd2f71dfe28ec29d344636"
        },
        {
          "path": "src/windows/wifi_station.cpp",
          "sha256": "a3b0c56d87d04f9ab4afc33ad7986d1d861621b3250db7a36fc563e10f71dcb2"
        },
        {
          "path": "src/plugin/camera.h",
          "sha256": "354cca54429f5bf9acc282c3b4b3f78d51f26ef5d093b09ad76a1b2929bf6d9d"
        },
        {
          "path": "src/plugin/camera.cpp",
          "sha256": "aeb22cca3d7b2e9ab366360e3109ab9217240e9c92dd931cc4eba40ff2b2b786"
        },
        {
          "path": "src/plugin/source.c",
          "sha256": "de457fc283bbb1924b36cd530e6a04b8954501e4e0e9f8c7b74e393bbcaa8448"
        },
        {
          "path": "src/plugin/aac_decoder.hpp",
          "sha256": "fd2e73957c3b38db215cc996640cd89f20365092104ce6a790f321e357dc975b"
        },
        {
          "path": "src/plugin/aac_decoder.cpp",
          "sha256": "ae0c98f51187ec61df74f6575f666abd43479425f3ee6743b39cf3fa97ef9946"
        },
        {
          "path": "src/plugin/hevc_decoder.cpp",
          "sha256": "f99c0a6ce28a9e0bd196dca0c9fcb30293eb3a349bad48876a7780d4f86f54dd"
        },
        {
          "path": "src/plugin/ffmpeg_handles.hpp",
          "sha256": "c2e5c0ffd2dc1ac7ae0a6fe91093233afe845cc76e2039d5849c689879bb2d9d"
        },
        {
          "path": "data/locale/en-US.ini",
          "sha256": "47e4deecb8f18e755e1ac994f14c7b66792457207ba5a7db025995f140d5edf5"
        },
        {
          "path": "tests/protocol_tests.cpp",
          "sha256": "7a8ef0e13ccfe887d37bce17ec30663aff5467af4d5e3fee679bca04bea979b8"
        },
        {
          "path": "scripts/Install-MetaGLinkDirect.ps1",
          "sha256": "3b45b7023416a047163d7edabeba7413cfb2b9d7e073983f02eb0910284a7865"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "840ad4ffbe235c6458119df5bc59dc96012396525257d02cf0fc356866cb0eb6"
        },
        {
          "path": "tests/installer.Tests.ps1",
          "sha256": "99300ab966bda583e33c60bf464dbcdd876aec131606f25ce53d93313409b238"
        },
        {
          "path": "packaging/README.txt",
          "sha256": "c409052cefc9859ae65d41f8d617cca1f87cb576edc84dd744eb4fa2fe35e1e4"
        }
      ],
      "scope_sha256": null,
      "red": null,
      "reason": "The scope fingerprint tool behind earlier entries is not available in this session; inputs carry their own hashes (null = deleted file)."
    },
    {
      "id": "EV-WIFI-LINK-RED-SOFTAP-PORT",
      "acceptance": [
        "AC-MEDIA"
      ],
      "kind": "red",
      "status": "pass",
      "command": [
        "pwsh",
        "-NoProfile",
        "-File",
        "red-softap-port.ps1",
        "-Repo",
        "."
      ],
      "method": "Inspected the recorded intended failure and exact byte restoration.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/red/softap-port.txt",
        "sha256": "5d3f7e7f0a29496e07e6f83892c7406f70f12d5e001d2d6e42ded8a9de375686"
      },
      "inputs": [
        {
          "path": "src/protocol/stream.cpp",
          "sha256": "4f58f94c3ecfd5c06ed85a78d0ef8b7e9e163641bf9d727f43d7f93e511345a5"
        }
      ],
      "scope_sha256": null,
      "red": {
        "mutation": "parseSoftApNetworkInfo reads the port from field 3 instead of field 4; the protocol tests must fail.",
        "baseline_exit": 0,
        "mutated_exit": 1,
        "restored_exit": 0,
        "expected_diagnostic": "a success with address and port is usable: failed",
        "observed_diagnostic": "1  output: a success with address and port is usable: failed",
        "before": [
          {
            "path": "src/protocol/stream.cpp",
            "sha256": "4f58f94c3ecfd5c06ed85a78d0ef8b7e9e163641bf9d727f43d7f93e511345a5"
          }
        ],
        "mutated": [
          {
            "path": "src/protocol/stream.cpp",
            "sha256": "03ef10629aaf09134248e4db4f613dc6e1035078292143103078932097e525d5"
          }
        ],
        "after": [
          {
            "path": "src/protocol/stream.cpp",
            "sha256": "4f58f94c3ecfd5c06ed85a78d0ef8b7e9e163641bf9d727f43d7f93e511345a5"
          }
        ]
      },
      "reason": "The scope fingerprint tool behind earlier entries is not available in this session; inputs carry their own hashes (null = deleted file)."
    },
    {
      "id": "EV-WIFI-LINK-OBS-DEFERRED",
      "acceptance": [
        "AC-OBS"
      ],
      "kind": "manual",
      "status": "deferred",
      "command": [],
      "method": "Not yet run: reconnection after a real drop, quality changes in OBS, the keyboard/Narrator/theme/scaling pass, latency measurement and the full-charge endurance run.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": null,
      "artifact": null,
      "inputs": [],
      "scope_sha256": null,
      "red": null,
      "reason": "Next hardware sessions; a health reading that looks like battery suggests a session lasts roughly 35-40 minutes per charge."
    },
    {
      "id": "EV-WIFI-LINK-OBS",
      "acceptance": [
        "AC-OBS"
      ],
      "kind": "manual",
      "status": "pass",
      "command": [],
      "method": "Installed this build with scripts/Install-MetaGLinkDirect.ps1 (setup declined), started OBS 32.2.2 and switched the test scene live over the local OBS WebSocket. 720x1280 video about 18 s after the scene went live; two minutes at 30.0 fps; OBS render skips 1 of 9,794 frames (present before the scene went live) through start, stream and stop; CPU about 1.3%; memory flat; source screenshots at 0 s and 2 min showed live first-person frames and were deleted; Camera stopped on scene switch with no Wi-Fi profile left. Not covered: reconnection after a real drop, quality changes, accessibility, the full-charge endurance run. On 2026-10-19 the Wi-Fi adapter property listed Automatic and the USB adapter by name; video came up with that adapter chosen explicitly; an id matching no adapter failed explicitly, used no other adapter and closed the glasses network over Bluetooth (acknowledged).",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/WIFI_LINK_VERIFICATION.md",
        "sha256": "d0d6a035809793e86fbace8c9827f5252b62256ec0d434d77ea140c470f7b464"
      },
      "inputs": [
        {
          "path": "CMakeLists.txt",
          "sha256": "edf508bd8b91c85951f555ede63c42436b9cf8935c8d7dde09ea70b81b681108"
        },
        {
          "path": "include/mgl/stream.hpp",
          "sha256": "0588814c1b39e08aec407542a2d973c71cfb442f72e941081c00be45d686508a"
        },
        {
          "path": "src/protocol/stream.cpp",
          "sha256": "4f58f94c3ecfd5c06ed85a78d0ef8b7e9e163641bf9d727f43d7f93e511345a5"
        },
        {
          "path": "include/mgl/wifi_station.hpp",
          "sha256": "ac6c3df9b17f8a7bc2fbbf92f918008bd7b2929d76dd2f71dfe28ec29d344636"
        },
        {
          "path": "src/windows/wifi_station.cpp",
          "sha256": "a3b0c56d87d04f9ab4afc33ad7986d1d861621b3250db7a36fc563e10f71dcb2"
        },
        {
          "path": "src/windows/wifi_credentials.hpp",
          "sha256": "fdaaedd93bc48b18f0b5833f1ae7abde37d4035ad42ba610b9dc72f9e4fd9fe6"
        },
        {
          "path": "include/mgl/media_session.hpp",
          "sha256": "26697ce17c19d0ba0888eca7a11c5e7ce09a7c8d2ea27fe8b1116d059093a5c0"
        },
        {
          "path": "src/windows/media_session.cpp",
          "sha256": "c4490464219de062d591774239a45d506410b4a9ccc3ce712ef9d4f0a07624bb"
        },
        {
          "path": "src/plugin/camera.h",
          "sha256": "c14b40b631f48974f6d0ae066d4420d53295c1187e5253d9bb8334488cceeb8b"
        },
        {
          "path": "src/plugin/camera.cpp",
          "sha256": "f057f97c16dad8e1679189f52f300f68049c6efd6cf974f5aaa2e353775ad3ed"
        },
        {
          "path": "src/plugin/source.c",
          "sha256": "f47d8c734cec4a1f82d8a4417f9cd016e41e58a424e00dad303115ab995d8ea7"
        },
        {
          "path": "data/locale/en-US.ini",
          "sha256": "70580c88b13e2c3e8ee858397e613985a4c429113fbbdb338c9caf82305edd2a"
        },
        {
          "path": "src/probe/main.cpp",
          "sha256": "6fb691e1b34541ba2cb19cacae66942682047ef9f60fe3536d5d2c8f38cf778b"
        },
        {
          "path": "tests/protocol_tests.cpp",
          "sha256": "f56c414cc9b814861aaffcbb40d35983e4cd6e6cbc1858716eec986816ecb7f9"
        },
        {
          "path": "scripts/Install-MetaGLinkDirect.ps1",
          "sha256": "ecc3c16028d3a1ae7f4de453f6b30eb6ac4636c9e0c752c82527b000835c9ef9"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "de78c654f25a0ad94f656ad78b5bd6e1aee0f8dba3423574089510fbfb5b0d1c"
        },
        {
          "path": "packaging/Install.cmd",
          "sha256": "1bad1b9185b1328e783d01bc881e9a9060263f8cbc7c3973291ee40ec4227039"
        },
        {
          "path": "packaging/Uninstall.cmd",
          "sha256": "041c25669c21aa56cac8ef3af8d138abaddb0d85bcc2b3eeefbd81b7868bed3b"
        },
        {
          "path": "packaging/README.txt",
          "sha256": "a1828052143a56a979e082de1b79952e36508369d74eabaabe8bbad27acef0b4"
        }
      ],
      "scope_sha256": null,
      "red": null,
      "reason": "The scope fingerprint tool behind earlier entries is not available in this session; inputs carry their own hashes (null = deleted file)."
    },
    {
      "id": "EV-AUDIO-HARDWARE",
      "acceptance": [
        "AC-MEDIA",
        "AC-OBS"
      ],
      "kind": "manual",
      "status": "pass",
      "command": [],
      "method": "Real glasses: with audio requested the glasses sent raw AAC-LC stereo units (~47/s) and no audio codec configuration; with the requested-format AudioSpecificConfig the plugin decoded 48 kHz stereo in OBS 32.2.2, OBS InputVolumeMeters showed a -15 dBFS peak, and OBS skipped 0 of 2,973 render frames. Sync offset not measured.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": null,
      "artifact": {
        "path": "reports/AUDIO_AND_OBS33_VERIFICATION.md",
        "sha256": "cd54b464fd53b9aad34148a468b8cad98ae55d3b98a17e46a79b155fb414e124"
      },
      "inputs": [
        {
          "path": "CMakeLists.txt",
          "sha256": "b903d25825336f84b5a2f596ce6f3a9c86fd1e93a15e7139a313ec35dda77d11"
        },
        {
          "path": "include/mgl/stream.hpp",
          "sha256": "5cf40f97cac5c92a4197173585e6d421c63b709cf9b5c3468883f6ec38d99f65"
        },
        {
          "path": "src/protocol/stream.cpp",
          "sha256": "6514fd1d35dd9c344a3abeac53ea44590f00a725ca2e2aae721e08478655e576"
        },
        {
          "path": "include/mgl/media_session.hpp",
          "sha256": "c6a01be1339133961b2b20f6c5a2a0f21d574cad8d66466ec627b50e8b2f4397"
        },
        {
          "path": "src/windows/media_session.cpp",
          "sha256": "f09f551bf388fba131cd5db7c4c9c2efba455c2a5b14889a1e981117f1bb1207"
        },
        {
          "path": "include/mgl/wifi_station.hpp",
          "sha256": "ac6c3df9b17f8a7bc2fbbf92f918008bd7b2929d76dd2f71dfe28ec29d344636"
        },
        {
          "path": "src/windows/wifi_station.cpp",
          "sha256": "a3b0c56d87d04f9ab4afc33ad7986d1d861621b3250db7a36fc563e10f71dcb2"
        },
        {
          "path": "src/plugin/camera.h",
          "sha256": "354cca54429f5bf9acc282c3b4b3f78d51f26ef5d093b09ad76a1b2929bf6d9d"
        },
        {
          "path": "src/plugin/camera.cpp",
          "sha256": "aeb22cca3d7b2e9ab366360e3109ab9217240e9c92dd931cc4eba40ff2b2b786"
        },
        {
          "path": "src/plugin/source.c",
          "sha256": "de457fc283bbb1924b36cd530e6a04b8954501e4e0e9f8c7b74e393bbcaa8448"
        },
        {
          "path": "src/plugin/aac_decoder.hpp",
          "sha256": "fd2e73957c3b38db215cc996640cd89f20365092104ce6a790f321e357dc975b"
        },
        {
          "path": "src/plugin/aac_decoder.cpp",
          "sha256": "ae0c98f51187ec61df74f6575f666abd43479425f3ee6743b39cf3fa97ef9946"
        },
        {
          "path": "src/plugin/hevc_decoder.cpp",
          "sha256": "f99c0a6ce28a9e0bd196dca0c9fcb30293eb3a349bad48876a7780d4f86f54dd"
        },
        {
          "path": "src/plugin/ffmpeg_handles.hpp",
          "sha256": "c2e5c0ffd2dc1ac7ae0a6fe91093233afe845cc76e2039d5849c689879bb2d9d"
        },
        {
          "path": "data/locale/en-US.ini",
          "sha256": "47e4deecb8f18e755e1ac994f14c7b66792457207ba5a7db025995f140d5edf5"
        },
        {
          "path": "tests/protocol_tests.cpp",
          "sha256": "7a8ef0e13ccfe887d37bce17ec30663aff5467af4d5e3fee679bca04bea979b8"
        },
        {
          "path": "scripts/Install-MetaGLinkDirect.ps1",
          "sha256": "3b45b7023416a047163d7edabeba7413cfb2b9d7e073983f02eb0910284a7865"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "840ad4ffbe235c6458119df5bc59dc96012396525257d02cf0fc356866cb0eb6"
        },
        {
          "path": "tests/installer.Tests.ps1",
          "sha256": "99300ab966bda583e33c60bf464dbcdd876aec131606f25ce53d93313409b238"
        },
        {
          "path": "packaging/README.txt",
          "sha256": "c409052cefc9859ae65d41f8d617cca1f87cb576edc84dd744eb4fa2fe35e1e4"
        }
      ],
      "scope_sha256": null,
      "red": null,
      "reason": "The scope fingerprint tool behind earlier entries is not available in this session; inputs carry their own hashes (null = deleted file)."
    },
    {
      "id": "EV-OBS33-LOAD",
      "acceptance": [
        "AC-MODULE",
        "AC-OBS"
      ],
      "kind": "behavior",
      "status": "pass",
      "command": [
        "out/windows/mgl-module-tests.exe",
        "out/windows/meta-g-link-direct.dll",
        "<OBS 33.0.0-rc2>/bin/64bit"
      ],
      "method": "Official OBS 33.0.0-rc2 Windows zip (digest-verified): module ABI test passed; portable OBS 33 loaded the 0.2.0 plugin from plugins/meta-g-link-direct/ (new layout) and registered the camera source with no legacy warning. OBS 33.0.0-rc2 was then installed with the official signed installer: it loaded the plugin from the ProgramData flat layout, skipped the bin/64bit copy, listed the Wi-Fi adapters and streamed 720 x 1280 video at 30.0 fps with 48 kHz stereo audio (mixer peak -29.7 dBFS; render skips held at the 2 present before streaming). Final OBS 33.0 not yet released.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/AUDIO_AND_OBS33_VERIFICATION.md",
        "sha256": "cd54b464fd53b9aad34148a468b8cad98ae55d3b98a17e46a79b155fb414e124"
      },
      "inputs": [
        {
          "path": "CMakeLists.txt",
          "sha256": "b903d25825336f84b5a2f596ce6f3a9c86fd1e93a15e7139a313ec35dda77d11"
        },
        {
          "path": "include/mgl/stream.hpp",
          "sha256": "5cf40f97cac5c92a4197173585e6d421c63b709cf9b5c3468883f6ec38d99f65"
        },
        {
          "path": "src/protocol/stream.cpp",
          "sha256": "6514fd1d35dd9c344a3abeac53ea44590f00a725ca2e2aae721e08478655e576"
        },
        {
          "path": "include/mgl/media_session.hpp",
          "sha256": "c6a01be1339133961b2b20f6c5a2a0f21d574cad8d66466ec627b50e8b2f4397"
        },
        {
          "path": "src/windows/media_session.cpp",
          "sha256": "f09f551bf388fba131cd5db7c4c9c2efba455c2a5b14889a1e981117f1bb1207"
        },
        {
          "path": "include/mgl/wifi_station.hpp",
          "sha256": "ac6c3df9b17f8a7bc2fbbf92f918008bd7b2929d76dd2f71dfe28ec29d344636"
        },
        {
          "path": "src/windows/wifi_station.cpp",
          "sha256": "a3b0c56d87d04f9ab4afc33ad7986d1d861621b3250db7a36fc563e10f71dcb2"
        },
        {
          "path": "src/plugin/camera.h",
          "sha256": "354cca54429f5bf9acc282c3b4b3f78d51f26ef5d093b09ad76a1b2929bf6d9d"
        },
        {
          "path": "src/plugin/camera.cpp",
          "sha256": "aeb22cca3d7b2e9ab366360e3109ab9217240e9c92dd931cc4eba40ff2b2b786"
        },
        {
          "path": "src/plugin/source.c",
          "sha256": "de457fc283bbb1924b36cd530e6a04b8954501e4e0e9f8c7b74e393bbcaa8448"
        },
        {
          "path": "src/plugin/aac_decoder.hpp",
          "sha256": "fd2e73957c3b38db215cc996640cd89f20365092104ce6a790f321e357dc975b"
        },
        {
          "path": "src/plugin/aac_decoder.cpp",
          "sha256": "ae0c98f51187ec61df74f6575f666abd43479425f3ee6743b39cf3fa97ef9946"
        },
        {
          "path": "src/plugin/hevc_decoder.cpp",
          "sha256": "f99c0a6ce28a9e0bd196dca0c9fcb30293eb3a349bad48876a7780d4f86f54dd"
        },
        {
          "path": "src/plugin/ffmpeg_handles.hpp",
          "sha256": "c2e5c0ffd2dc1ac7ae0a6fe91093233afe845cc76e2039d5849c689879bb2d9d"
        },
        {
          "path": "data/locale/en-US.ini",
          "sha256": "47e4deecb8f18e755e1ac994f14c7b66792457207ba5a7db025995f140d5edf5"
        },
        {
          "path": "tests/protocol_tests.cpp",
          "sha256": "7a8ef0e13ccfe887d37bce17ec30663aff5467af4d5e3fee679bca04bea979b8"
        },
        {
          "path": "scripts/Install-MetaGLinkDirect.ps1",
          "sha256": "3b45b7023416a047163d7edabeba7413cfb2b9d7e073983f02eb0910284a7865"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "840ad4ffbe235c6458119df5bc59dc96012396525257d02cf0fc356866cb0eb6"
        },
        {
          "path": "tests/installer.Tests.ps1",
          "sha256": "99300ab966bda583e33c60bf464dbcdd876aec131606f25ce53d93313409b238"
        },
        {
          "path": "packaging/README.txt",
          "sha256": "c409052cefc9859ae65d41f8d617cca1f87cb576edc84dd744eb4fa2fe35e1e4"
        }
      ],
      "scope_sha256": null,
      "red": null,
      "reason": "The scope fingerprint tool behind earlier entries is not available in this session; inputs carry their own hashes (null = deleted file)."
    },
    {
      "id": "EV-INSTALLER-DRIVES",
      "acceptance": [
        "AC-OBS"
      ],
      "kind": "behavior",
      "status": "pass",
      "command": [
        "scripts/Invoke-Project.ps1",
        "test:installer"
      ],
      "method": "Installer takes ProgramData, Program Files and local application data from the Windows known-folder call and searches a running OBS and Steam library folders. Pester installer suite (4) and the installer-drive-letter red drill passed. With a scratch folder mapped to M: (subst), Windows PowerShell 5.1 ran the bundle from M:, installed into a portable OBS 33.0.0-rc2 at M:\\obs33 (flat layout), that OBS loaded the plugin, and discovery without -ObsPath found M:\\obs33 only through the running process. Relocated ProgramData or profile folders and a real Steam OBS not tested.",
      "environment": {
        "platform": "windows",
        "tools": {
          "python": "3.14.0",
          "cmake": "3.31.3",
          "llvm": "22.1.1",
          "powershell": "7.6.6",
          "gitleaks": "8.30.1",
          "node": "24.20.0",
          "npm": "11.19.0",
          "semgrep": "1.177.0",
          "actionlint": "1.7.12",
          "zizmor": "1.25.2",
          "osv_scanner": "2.6.0",
          "pester": "5.7.1",
          "psscriptanalyzer": "1.25.0",
          "jscpd": "5.4.1",
          "precommit": "4.5.1",
          "cppcheck": "2.22.0",
          "msvc": "19.44.35222.0"
        }
      },
      "exit_code": 0,
      "artifact": {
        "path": "reports/AUDIO_AND_OBS33_VERIFICATION.md",
        "sha256": "cd54b464fd53b9aad34148a468b8cad98ae55d3b98a17e46a79b155fb414e124"
      },
      "inputs": [
        {
          "path": "CMakeLists.txt",
          "sha256": "b903d25825336f84b5a2f596ce6f3a9c86fd1e93a15e7139a313ec35dda77d11"
        },
        {
          "path": "include/mgl/stream.hpp",
          "sha256": "5cf40f97cac5c92a4197173585e6d421c63b709cf9b5c3468883f6ec38d99f65"
        },
        {
          "path": "src/protocol/stream.cpp",
          "sha256": "6514fd1d35dd9c344a3abeac53ea44590f00a725ca2e2aae721e08478655e576"
        },
        {
          "path": "include/mgl/media_session.hpp",
          "sha256": "c6a01be1339133961b2b20f6c5a2a0f21d574cad8d66466ec627b50e8b2f4397"
        },
        {
          "path": "src/windows/media_session.cpp",
          "sha256": "f09f551bf388fba131cd5db7c4c9c2efba455c2a5b14889a1e981117f1bb1207"
        },
        {
          "path": "include/mgl/wifi_station.hpp",
          "sha256": "ac6c3df9b17f8a7bc2fbbf92f918008bd7b2929d76dd2f71dfe28ec29d344636"
        },
        {
          "path": "src/windows/wifi_station.cpp",
          "sha256": "a3b0c56d87d04f9ab4afc33ad7986d1d861621b3250db7a36fc563e10f71dcb2"
        },
        {
          "path": "src/plugin/camera.h",
          "sha256": "354cca54429f5bf9acc282c3b4b3f78d51f26ef5d093b09ad76a1b2929bf6d9d"
        },
        {
          "path": "src/plugin/camera.cpp",
          "sha256": "aeb22cca3d7b2e9ab366360e3109ab9217240e9c92dd931cc4eba40ff2b2b786"
        },
        {
          "path": "src/plugin/source.c",
          "sha256": "de457fc283bbb1924b36cd530e6a04b8954501e4e0e9f8c7b74e393bbcaa8448"
        },
        {
          "path": "src/plugin/aac_decoder.hpp",
          "sha256": "fd2e73957c3b38db215cc996640cd89f20365092104ce6a790f321e357dc975b"
        },
        {
          "path": "src/plugin/aac_decoder.cpp",
          "sha256": "ae0c98f51187ec61df74f6575f666abd43479425f3ee6743b39cf3fa97ef9946"
        },
        {
          "path": "src/plugin/hevc_decoder.cpp",
          "sha256": "f99c0a6ce28a9e0bd196dca0c9fcb30293eb3a349bad48876a7780d4f86f54dd"
        },
        {
          "path": "src/plugin/ffmpeg_handles.hpp",
          "sha256": "c2e5c0ffd2dc1ac7ae0a6fe91093233afe845cc76e2039d5849c689879bb2d9d"
        },
        {
          "path": "data/locale/en-US.ini",
          "sha256": "47e4deecb8f18e755e1ac994f14c7b66792457207ba5a7db025995f140d5edf5"
        },
        {
          "path": "tests/protocol_tests.cpp",
          "sha256": "7a8ef0e13ccfe887d37bce17ec30663aff5467af4d5e3fee679bca04bea979b8"
        },
        {
          "path": "scripts/Install-MetaGLinkDirect.ps1",
          "sha256": "3b45b7023416a047163d7edabeba7413cfb2b9d7e073983f02eb0910284a7865"
        },
        {
          "path": "scripts/Invoke-Project.ps1",
          "sha256": "840ad4ffbe235c6458119df5bc59dc96012396525257d02cf0fc356866cb0eb6"
        },
        {
          "path": "tests/installer.Tests.ps1",
          "sha256": "99300ab966bda583e33c60bf464dbcdd876aec131606f25ce53d93313409b238"
        },
        {
          "path": "packaging/README.txt",
          "sha256": "c409052cefc9859ae65d41f8d617cca1f87cb576edc84dd744eb4fa2fe35e1e4"
        }
      ],
      "scope_sha256": null,
      "red": null,
      "reason": "The scope fingerprint tool behind earlier entries is not available in this session; inputs carry their own hashes (null = deleted file)."
    }
  ],
  "checkpoint": {
    "scope_sha256": "a3529185a96542b04ea84569e420c8d57d7390efa0a8d6f482bdcb2de07bb871",
    "inputs": [
      {
        "path": ".clang-format",
        "sha256": "b8b235ad236526882c0fdc52d3a17194410d13a184ab572937de4fa2c231ecdd"
      },
      {
        "path": ".clang-tidy",
        "sha256": "c2f69f181a8cd59ba8bdc3fc46328b22c7c65705adf3daadd6e6ebba796a7464"
      },
      {
        "path": ".cppcheck-obs.cfg",
        "sha256": "43af454a083f4d7b44dcd22e48f191f8699f8eff272ded6fea1f9b7eb412364a"
      },
      {
        "path": ".gitattributes",
        "sha256": "7615f5cb3052a2fddfa34520ad43cd3bc9e3058dd913432bf602201e89a1b4d8"
      },
      {
        "path": ".github/dependabot.yml",
        "sha256": "52f52486fba5f32ef49c3ef1b216fac04888ad0847c64b57df89b28655fb82a3"
      },
      {
        "path": ".github/workflows/quality.yml",
        "sha256": "ce7867c76f0755790a8df94a77fb782a6fb6c52c2802fc332e2ef9958132a3ca"
      },
      {
        "path": ".gitignore",
        "sha256": "51afdd661c7a97f4fb20f0cbd4338ba95adc00b30b0a3d6d95c2190777e7a469"
      },
      {
        "path": ".gitleaks.toml",
        "sha256": "ce1b21983f8ff159aa5762beb7e973e1b2b2dfe8bd350cf39daa0ec2da2e711e"
      },
      {
        "path": ".jscpd.json",
        "sha256": "8aba253a1058ebdc4515d74367a37a0e53aac6451af06a760172c74100e1c4b6"
      },
      {
        "path": ".pre-commit-config.yaml",
        "sha256": "c2df117a2f7ab21bc8210e626bb67710515abe62fa76bc9011408dc15c5c188a"
      },
      {
        "path": ".semgrep.yml",
        "sha256": "c989195a0157c259c43150cb2edb54842800317827b5504004de5831110d6619"
      },
      {
        "path": "AGENTS.md",
        "sha256": "24745ad096dfa94b67d46bfca08acb8b9d3e1ff0e95e725658bc4b49c490a634"
      },
      {
        "path": "CHANGELOG.md",
        "sha256": "b0f21e34ea828690a7ff08b3dfba358dd81c430fb59a106686825462575acac3"
      },
      {
        "path": "CMakeLists.txt",
        "sha256": "d5a84bf8a9399d7e393cb8847f7480bef8a889de4e3b3fc8ce158fe051b17d15"
      },
      {
        "path": "CMakePresets.json",
        "sha256": "10ddf515194db5a77acbf0a7007c5a29120ea0725c023e1e17e6a47a47a655bb"
      },
      {
        "path": "LICENSE",
        "sha256": "8d56b405468aad11f87ab5763f901e276e08d9646ff5c8481b1762b6b789e9ed"
      },
      {
        "path": "PROJECT_BRIEF.md",
        "sha256": "220c1c763c639a16a83434cb5ef8ba7a74de69e60f8b34f6b6b35bc49ee84950"
      },
      {
        "path": "PSScriptAnalyzerSettings.psd1",
        "sha256": "427f99245961626682b7ed07663c63e67aeb9566d1bffe372aee05b98e32f2e0"
      },
      {
        "path": "README.md",
        "sha256": "247382f59836dec8e143e470b1b74a6318430e020bdc11b5cb39084cf103fc46"
      },
      {
        "path": "THIRD_PARTY_NOTICES.md",
        "sha256": "39a2887ee55df4114e82e5de7374444514f7b6bb561bfba4613df874f20a9d0d"
      },
      {
        "path": "data/locale/en-US.ini",
        "sha256": null
      },
      {
        "path": "dependencies.lock.json",
        "sha256": "d2a77c6df8d744ce9c52e723d127f0812a5c26749b8c251f83a9c0b01deb8b2e"
      },
      {
        "path": "include/mgl/address.hpp",
        "sha256": "9889fa8f7c51d4236dc5d3e00c46167952f2879478da0842e0335eca28ebc6ed"
      },
      {
        "path": "include/mgl/capabilities.hpp",
        "sha256": "7daa77cf5c2f1334d1843c853be556e79981cfcde164e1cd31b303efc60430cc"
      },
      {
        "path": "include/mgl/discovery.hpp",
        "sha256": "7a262e29abb007fadad655d5d172cd138d46485162dd651d26deb26ea2b74094"
      },
      {
        "path": "include/mgl/rfcomm.hpp",
        "sha256": "66d2fc76d1d8e2e74049c3e33304298e5b08cfcaa5b34da2bc5b80d551003483"
      },
      {
        "path": "include/mgl/sdu.hpp",
        "sha256": "bf120c3f0f14cf566c1a298bd15002d00656de7828772ee1a141834328668db6"
      },
      {
        "path": "include/mgl/session.hpp",
        "sha256": null
      },
      {
        "path": "include/mgl/stream.hpp",
        "sha256": null
      },
      {
        "path": "package-lock.json",
        "sha256": "4deb7e5248da52fcdb7385dc9c96f915f841e3509466222990bd7a751770dd16"
      },
      {
        "path": "package.json",
        "sha256": "1053cfdf62f78e96e8f488835b6abaf6e7accdab3f0d868b68793d6aeb29a1e6"
      },
      {
        "path": "scripts/Invoke-Project.ps1",
        "sha256": "3c46440439ddc920db9b4913f5224daa011345f46206c4ed3da72586b671e7b8"
      },
      {
        "path": "src/media/decoder.cpp",
        "sha256": null
      },
      {
        "path": "src/media/stream.cpp",
        "sha256": null
      },
      {
        "path": "src/plugin/glasses_source.cpp",
        "sha256": null
      },
      {
        "path": "src/plugin/module.c",
        "sha256": "35f8b9adde13bf628bd2218aa275fb10dd74807f78ccbb164efdca35352cc46c"
      },
      {
        "path": "src/probe/main.cpp",
        "sha256": "4d212ca8471cb367652754e1a04d16844001d9f1192d30ae910d0fdc2e4b8c18"
      },
      {
        "path": "src/probe/private_input.cpp",
        "sha256": "5c251d6eacf7d06803f085505aeb485f7627620ee989f13925a85fc1304d4e4c"
      },
      {
        "path": "src/probe/private_input.hpp",
        "sha256": "b90870ed2085b7aebe46726f517252c8fe2c168d15341b01eb16fdda0654cc43"
      },
      {
        "path": "src/protocol/address.cpp",
        "sha256": "c3004bc33bb615eee1e285a7666fb28bff020a22f412f366381be8aa065d5eaf"
      },
      {
        "path": "src/protocol/sdu.cpp",
        "sha256": "9e92d77135826f7460b3e74bf05dd24cbfbe67efd2f551eebb6f433d2bf29084"
      },
      {
        "path": "src/windows/apartment.hpp",
        "sha256": "c392d69883555a5da3111f7e5a2f916a0e36b812ad609494f8fac00cf0d3ef29"
      },
      {
        "path": "src/windows/capabilities.cpp",
        "sha256": "e25b5e87758b9d4dab91706663babd60fdc0e1059d36ad1a2ea9d59bdf6d156e"
      },
      {
        "path": "src/windows/classic.cpp",
        "sha256": "ece9d6024a38f9d81fab43bdba941173b47153d0b9bacab449bc8f1b2c4fea13"
      },
      {
        "path": "src/windows/discovery.cpp",
        "sha256": "5066920f2c8bb1fc0dd143857978b72668e3e41c18aedb7af8ba95f4145fcccf"
      },
      {
        "path": "src/windows/identity.cpp",
        "sha256": null
      },
      {
        "path": "src/windows/rfcomm.cpp",
        "sha256": "9da6dbd1321ba3be1d7746e65a56ea9fa02354eedfb385c36bbce0f11742ae20"
      },
      {
        "path": "src/windows/session.cpp",
        "sha256": null
      },
      {
        "path": "src/windows/wifi_group.cpp",
        "sha256": null
      },
      {
        "path": "tests/address_tests.cpp",
        "sha256": "e9e3311b0609a4eef7ab4fcfa3a55943ae20600fd07978606cdfadc65412e901"
      },
      {
        "path": "tests/live.Tests.ps1",
        "sha256": "efd413d3f5bfc27aa403fcfd99f61da5fc58271ba90527d57fb9914945bfd65f"
      },
      {
        "path": "tests/module_tests.cpp",
        "sha256": "02470464448f4f2c0ed552d47cc10988b895ea37177ea71b26905a5b05b4a20b"
      },
      {
        "path": "tests/probe.Tests.ps1",
        "sha256": "6e241cf9154c67044628d35fa5f3b0c6b92c9139dc666d6ed9adfab082ddc064"
      },
      {
        "path": "tests/probe_support.ps1",
        "sha256": "ad53ccd95cc106348768cb5889de2be8608401934064ec3c361466e66ed8c4a2"
      },
      {
        "path": "tests/runner.Tests.ps1",
        "sha256": "08675144ad7cddc1213d848507220228d74999dcceeaf9034fac811a3876f5b1"
      },
      {
        "path": "tests/sdu_tests.cpp",
        "sha256": "ecebacb95369954308e0431df4fd8fbfed85275b09e7b1b0be13556494df32fd"
      },
      {
        "path": "tests/session_tests.cpp",
        "sha256": null
      },
      {
        "path": "tests/source_tests.cpp",
        "sha256": null
      },
      {
        "path": "tests/stream_tests.cpp",
        "sha256": null
      }
    ],
    "environment": {
      "platform": "windows",
      "tools": {
        "python": "3.14.0",
        "cmake": "3.31.3",
        "llvm": "22.1.1",
        "powershell": "7.6.6",
        "gitleaks": "8.30.1",
        "node": "24.20.0",
        "npm": "11.19.0",
        "semgrep": "1.177.0",
        "actionlint": "1.7.12",
        "zizmor": "1.25.2",
        "osv_scanner": "2.6.0",
        "pester": "5.7.1",
        "psscriptanalyzer": "1.25.0",
        "jscpd": "5.4.1",
        "precommit": "4.5.1",
        "cppcheck": "2.22.0",
        "msvc": "19.44.35222.0"
      }
    },
    "verified_tasks": [
      "TASK-SETUP"
    ],
    "pending_operations": [],
    "next_action": "After 0.2.0: the final OBS 33.0 release once out, measure audio/video sync, reconnection after a real drop, quality changes, two Wi-Fi adapters, the accessibility pass, trust-signature verification, the battery reading and the full-charge endurance run.",
    "source_revision": "da0d500482158171550a452dda6b478304abf7c7"
  }
}
```

