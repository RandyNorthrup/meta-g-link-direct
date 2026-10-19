# Meta G Link Direct

Native Windows OBS plugin that shows the Ray-Ban Meta Gen 2 glasses camera in OBS over a direct local wireless link, with no phone in the loop.

**Current status: working on real hardware.** The "Ray-Ban Meta Glasses (Direct)" source shows the live 720×1280 HEVC camera at about 30 fps in OBS 32.2.2, after a one-time Meta setup that makes the PC the glasses' owner. The glasses host their own 2.4 GHz Wi-Fi network for the PC, which streamed without a drop in every run so far (longest 180 s; stop and restart verified). This is proven on one PC, one Bluetooth adapter, one Wi-Fi adapter and one pair of glasses. Still open: the glasses-hosted network inside OBS itself (proven so far with the same media session from the command line), reconnection after a real link drop (implemented, not yet exercised on hardware), the other quality presets (only 720×1280 is proven), a Narrator and keyboard pass over the source properties, latency measurement, latest-firmware certification, and the 120-minute run.

Setup moves the glasses' owner key from the Meta AI phone app to this PC. The phone app stops working with the glasses until you remove and re-add them in Meta AI.

The confirmed product contract is in [PROJECT_BRIEF.md](PROJECT_BRIEF.md). [PLAN.md](PLAN.md) owns implementation milestones, proof, and deferred gates. The intended product receives local video directly into OBS, with adjustable quality and optional glasses audio disabled by default. Meta authentication is permitted for setup if required; the phone must be off during the eventual local-streaming acceptance tests.

## Install and set up

Needs Windows 11 x64, OBS Studio 32.x, a Bluetooth adapter with Classic and BLE, a Wi-Fi adapter that can join a 2.4 GHz WPA2 network, and the Visual C++ x64 runtime (the OBS installer provides it). No router, hotspot or network setting changes.

While the camera runs, the PC's Wi-Fi adapter is connected to the glasses' own network, which has no internet. A PC that reaches the internet over Ethernet (or a second Wi-Fi adapter) stays online; a PC whose only connection is that Wi-Fi adapter is offline until the camera stops. Windows returns the adapter to its usual networks afterwards.

`scripts/Install-MetaGLinkDirect.ps1` installs from a setup bundle folder containing `plugin/meta-g-link-direct.dll`, `plugin/locale/en-US.ini` and `tools/meta-g-link-probe.exe`. It runs under Windows PowerShell 5.1. In order, it:

1. finds OBS and checks its FFmpeg (`avcodec-62`, `avutil-60`) and the runtime;
2. waits for OBS to be closed (it never closes OBS);
3. copies the plugin, elevating only that copy when OBS is under Program Files;
4. copies the setup tool to `%LOCALAPPDATA%\MetaGLinkDirect\bin`;
5. imports an optional `meta-tokens.txt`;
6. offers the one-time glasses setup.

`-WhatIf` lists every action without changing anything. `-Uninstall` removes the plugin and tool but keeps the sealed identity. So far it has been verified with a `-WhatIf` run on the development PC; an install on a fresh PC has not been run yet.

The setup tool has the Meta AI app's two client tokens built in. They identify the app, not a person, and are already public. An imported `meta-tokens.txt` (one `HW|<app id>|<secret>` line and one `AR|<app id>|<secret>` line) overrides them if Meta rotates them. `.gitignore` excludes that file.

The setup steps the installer runs, if you want them by hand:

```powershell
meta-g-link-probe.exe --auth-url     # prints the Meta login address; log in, then copy the final address
'<code from that address>' | meta-g-link-probe.exe --provision   # ownership change; seals the owner key
meta-g-link-probe.exe --link         # the authenticated Bluetooth session reaches ready
meta-g-link-probe.exe --stream       # the full camera path for 60 s; reports frame counts
```

Pair the glasses in Windows Bluetooth settings first: fold the arms, then hold the capture button about 12 seconds until the light blinks blue. Turn off Bluetooth on the phone. In OBS, add the "Ray-Ban Meta Glasses (Direct)" source to a scene. It connects when its scene goes live and disconnects when the scene leaves the program output. The source properties offer a quality list and a live status line.

## What exists

- C++20 frame-prefix library ported from the pinned AGPL-3.0 Visor implementation. It distinguishes incomplete headers, valid frame lengths, and invalid types without allocating or receiving camera data.
- The direct camera path, proven on hardware: AirShield handshake and owner trust over Bluetooth RFCOMM, the Meta change-owner setup, a 2.4 GHz / 20 MHz Wi-Fi network the glasses host (svc10 `CreateSoftApNetwork`) that the PC joins as an ordinary client, a second AirShield session over TCP, the stream configuration, HEVC access-unit reassembly and FFmpeg decode to I420. The session and stream protocol is ported from the pinned AGPL-3.0 Visor implementation; the glasses-hosted network follows the Meta AI app's protocol classes.
- Why the glasses host the network: with the glasses as a Wi-Fi client, video stopped 8–54 s into every run on every access point tried (Windows Mobile Hotspot pinned to 2.4 GHz, a dual-band home router), whatever the bitrate, adapter channel-width setting or audio setting. On their own 2.4 GHz / 20 MHz network, the same stream ran about 180 s (5,370 frames) without a stall. Details: [the Wi-Fi link report](reports/WIFI_LINK_VERIFICATION.md).
- The OBS source. OBS's video thread starts and stops it, and neither call waits on the radio: a stopped capture is cut off from OBS at once and tears the link down on a background thread. Module unload waits for that teardown. One source drives the glasses at a time; another waits its turn and shows "Waiting". The worker reconnects after a drop with a 2–15 s backoff that resets after video flows. It catches every exception at the thread boundary so a failure cannot take OBS down, and reports "Streaming" only once frames arrive.
- Each session gives the glasses' network a fresh `mgl-` name and a 20-symbol passphrase from the system CSPRNG, and uses the Windows region as the network's country code. The PC joins with a temporary Windows Wi-Fi profile that is deleted when the camera stops. If the PC cannot join the glasses' network or open the media connection, the plugin closes the network over Bluetooth before retrying. Once the PC has joined, the glasses leave Bluetooth, and after a stream they close the network themselves within about 15 seconds (measured).
- Native Windows WLAN/Bluetooth probe. Default reports radio presence, Wi-Fi association and controller feature support separately. Explicit `--discover` performs a bounded active BLE scan using Visor's Meta company/service markers. `--discover-classic` performs Classic inquiry and looks up Windows-registered paired Meta RFCOMM services by UUID. Counts/errors contain no names, addresses or payloads. Candidates and Windows bonds do not establish glasses authentication.
- Explicit `--connect-classic` selects only one Windows-registered Meta service with a paired peer, attempts an encrypted/authenticated Bluetooth socket connection with deadlines, closes it and sends no application frames. Multiple services are refused. It never pairs or silently falls back from BLE. Successful connection behavior still needs actual glasses proof.
- Explicit uncached per-peer Meta SDP lookup (`--discover-classic-live`), a separate paired socket check (`--connect-classic-live`), and an opt-in native Windows pairing picker (`--pair-classic`). Pairing never runs implicitly.
- Direct Winsock probes accept a trustworthy glasses Bluetooth address privately on stdin, without a Windows bond prerequisite: `--direct-classic-uuid` resolves the Meta service; `--direct-classic-channel7` explicitly uses the publicly observed Gen 2 control channel. Each waits for one bounded frame, sends no application messages and never switches routes silently. A matching hello prefix is a candidate, not authentication.
- C17 OBS glue (`module.c`, `source.c`) compiled against the real OBS 32.2.2 SDK, with the C++20 camera core behind the small C interface in `src/plugin/camera.h`. An isolated loader test checks the DLL against the installed OBS runtime, its exports and its metadata. It is not a test of an OBS scene or of streaming.
- Four CTest suites cover 83 framing/truncation/type cases, 20 private-address format/order/boundary cases, 68 protocol cases (SDU phases, protobuf, DataX, encoding, the glasses-hosted network messages) and the OBS module ABI. Three Pester runner tests, 20 CLI contract cases and one separate live SDP test cover diagnostics, private input, report failures and actual query execution. These software checks do not establish a glasses connection.
- Native build, formatting, static analysis, dead-code/duplication checks, secret/SAST/dependency scanning, and maintained defect/restoration drills.

## Tested development environment

Windows 11 Pro x64 build 26200; OBS 32.2.2; Visual Studio 2022/MSVC 19.44.35222.0; Windows SDK 10.0.26100.0; CMake 3.31.3; Visual Studio's Ninja; LLVM 22.1.1; PowerShell 7.6.6; Python 3.14.0; Node 24.20.0/npm 11.19.0; Pester 5.7.1; PSScriptAnalyzer 1.25.0; Gitleaks 8.30.1; Semgrep 1.177.0; actionlint 1.7.12; zizmor 1.25.2; OSV-Scanner 2.6.0; and pre-commit.

`dependencies.lock.json` pins native source/tool archives and records observed tools. `package-lock.json` locks the copy-detection development tool. No JavaScript runtime is part of the plugin.

Bootstrap downloads verified OBS source/dependencies, builds the development SDK, extracts Cppcheck 2.22.0 into `.tools/`, and restores the locked development package. It does not globally install those tools or install a plugin into OBS. Other listed development tools/modules must already be available; missing checks fail explicitly.

## Development commands

Run from the repository root with PowerShell 7:

```powershell
pwsh -NoProfile -File scripts/Invoke-Project.ps1 bootstrap
pwsh -NoProfile -File scripts/Invoke-Project.ps1 configure
pwsh -NoProfile -File scripts/Invoke-Project.ps1 build
pwsh -NoProfile -File scripts/Invoke-Project.ps1 test
pwsh -NoProfile -File scripts/Invoke-Project.ps1 probe
pwsh -NoProfile -File scripts/Invoke-Project.ps1 discover
pwsh -NoProfile -File scripts/Invoke-Project.ps1 discover:classic
pwsh -NoProfile -File scripts/Invoke-Project.ps1 discover:classic:live
# Explicit socket check, after glasses are paired with Windows:
pwsh -NoProfile -File scripts/Invoke-Project.ps1 connect:classic
```

The runner configures the Visual Studio environment only when required and reuses an existing matching environment. Owned builds use two parallel jobs. Generated dependencies, build files and local tools remain under ignored project directories.

After building, run `out/windows/meta-g-link-probe.exe --help` for native exit codes. No arguments performs no BLE scan. `--discover` listens for up to ten seconds plus a two-second stop deadline; early native failure returns immediately. Counts are advertisement packets, not unique devices. Exit 6 means a completed scan found no Meta marker; exit 3 means native discovery failed. The PowerShell runner converts any nonzero native result to a task failure; use the executable directly when the exact native exit code matters. Tests include a real local scan, so physical driver failure is checked for honest reporting, not treated as a successful scan.

`--discover-classic` requests four Windows inquiry units (5.12 seconds, not a hard whole-command deadline) and a registered-service lookup. Each async service lookup/open/socket connect has a five-second deadline and explicit cancellation. `--connect-classic` skips inquiry so it does not compete with socket connection. Exit 6 also means no registered Meta RFCOMM service; exit 7 means multiple such services were refused. Native Windows inquiry can include cached peers, so a device count is not proof of current reachability. Registered-service lookup is not a complete uncached SDP scan of all nearby devices. Automated tests exercise discovery only; socket connection is an explicit hardware check.

Live lookup requests a 20.48-second Classic inquiry, then a twenty-second query budget over at most eight unique inquiry peers using `GetRfcommServicesForIdAsync(..., Uncached)`. Errors, limits and incomplete discovery remain visible; zero services after a failed query is not a completed negative result. Native inquiry itself is synchronous, so its requested duration is not a hard whole-process deadline. `test:live` is separate from the ordinary test loop; full `quality` runs it and the skipped-query red drill. It contacts inquiry peers for service discovery without pairing or opening an application socket.

`pair:classic` opens the Windows native picker and waits for selection or Cancel. Choose only your glasses; cancel if absent. It may create a Windows Bluetooth bond and interrupt the phone link, but performs no Meta login or ownership transfer. Do not reset glasses or remove the existing Meta account. Return through iPhone Bluetooth/Meta AI using the same account afterward; recovery is not yet proved by this project.

For a known glasses address, run the executable directly in a local console:

```powershell
out/windows/meta-g-link-probe.exe --direct-classic-uuid
# A separate explicit test of the documented control channel:
out/windows/meta-g-link-probe.exe --direct-classic-channel7
```

Enter the address only at the hidden local prompt. Supported forms: twelve hexadecimal digits, or six octets separated consistently by colons or dashes. Zero, broadcast, malformed and oversized inputs fail before connecting. Do not paste the address into chat, arguments, scripts or Git. Each direct probe uses nonblocking connect/receive waits of five seconds each, closes its socket, retains no raw frame, and reports counts/status only. Native service resolution can add platform-controlled delay. Exit 5 rejects input; exit 3 reports transport/probe failure. Exit 0 means a complete transport frame was received, not authenticated glasses or camera success. An optional process-local `MGL_PROBE_REPORT` path explicitly requests the same redacted JSON in a file; write failure returns exit 4. The console prompt is plain text and keyboard usable; Narrator/manual hidden-input behavior remains unverified.

Controller capability fields are valid only when `bluetooth_features_valid` is true; otherwise inspect `bluetooth_features_error`. A failed optional feature query does not imply LE is unsupported or invalidate successful radio enumeration.

OBS is discovered at its verified per-user installation. For another installation location, set the optional process environment variable `MGL_OBS_RUNTIME` to that installation's `bin/64bit` directory before configuring. The directory must contain `obs.dll`; it is a runtime path, not a credential. There is no `.env` loader. Configure from the runner (or with `MGL_OBS_RUNTIME` set): a plain `cmake` re-configure without it registers the module-ABI test with an empty runtime path, and that test then fails.

Opt-in diagnostics: `MGL_HTTP_DEBUG` traces the Meta setup requests by URL, field names, reply status and reply keys, never values. `--stream` runs the same media session as the OBS source and writes its status lines to stderr.

Focused checks:

```powershell
pwsh -NoProfile -File scripts/Invoke-Project.ps1 format:check
pwsh -NoProfile -File scripts/Invoke-Project.ps1 lint
pwsh -NoProfile -File scripts/Invoke-Project.ps1 deadcode
pwsh -NoProfile -File scripts/Invoke-Project.ps1 test:asan
```

`format` applies formatting. `typecheck` uses the same compiler gate as `build`. Separate `security:secrets`, `security:sast`, `security:npm`, and `security:osv` tasks expose the individual scanners; `security:audit` also scans reachable Git history. A full-history scan requires an actual commit; no history is not proof of a clean history.

Windows AddressSanitizer runs in a separate build. Portable ASan/UBSan commands are configured in the GitHub workflow but hosted/Linux execution is not established by a local Windows pass. The detailed verification report lists scope and remaining gaps, including native dependency auditing, coverage, and hardware acceptance.

## Structure and ownership

- `include/mgl/`, `src/protocol/`: portable protocol contract and implementation (SDU, protobuf, DataX, AirShield framing, stream messages, HEVC, JSON, encodings).
- `src/windows/`: CNG crypto, DPAPI identity and token storage, Meta setup, WinHTTP, RFCOMM and TCP transports, the Wi-Fi client connection to the glasses' network, the media session, and the radio diagnostics.
- `src/probe/`: setup and diagnostic CLI.
- `src/plugin/`: OBS glue in C17 (`module.c`, `source.c`), the C++20 camera core (`camera.cpp`) and the FFmpeg HEVC decoder.
- `tests/`: independent native examples and disposable-resource runner checks.
- `scripts/Invoke-Project.ps1`: canonical task and defect-drill runner. `scripts/Install-MetaGLinkDirect.ps1`: end-user installer.
- `reports/`: actual observations and source-bound proof linked by the plan.

MSVC compiles native C/C++ and analyzes the C files with `/analyze`. LLVM performs C++ analysis with the real compilation database. Only the C files include OBS headers, because those headers use an MSVC intrinsic that clang cannot parse; nothing replaces OBS declarations with a private ABI.

Cppcheck uses the same database with MSVC external-include spelling normalized, explicit compiler builtins, standard/Windows models and six real dynamic OBS entry points. Missing-system-header notices from that model-based analysis and third-party dependency headers are excluded explicitly; quoted owned headers remain checked, and the real compiler/LLVM gates validate system includes. See the plan for these scoped analysis decisions and their required drills.

## Privacy and credentials

The owner key and any imported Meta tokens are sealed with Windows DPAPI for the current Windows account, in `%LOCALAPPDATA%\MetaGLinkDirect` (`identity.bin`, `tokens.bin`). Nothing prints or logs them. Delete that folder to forget the glasses identity.

Meta is contacted only during setup: the OIDC login, the AR login and the two pairing calls. Streaming makes no Meta or cloud call; video travels over the glasses' own Wi-Fi network to this PC and into OBS. The plugin has no telemetry and no update checks, and it records nothing; recording stays under OBS's control. Logs carry status lines only, never payloads, addresses or credentials.

The only credentials in this repository are the Meta AI app's two public client tokens (see `AGENTS.md` revision 3). Never add passwords, owner keys, user tokens, Wi-Fi passphrases, raw captures or private diagnostic bundles.

## Troubleshooting

The source's properties show its live status. The OBS log (Help > Log Files) has each connection step under `[meta-g-link-direct]`.

- **Stuck on Connecting or Reconnecting:** check that the glasses are on, charged and in range, and that phone Bluetooth is off. The source retries on its own with a 2–15 s backoff.
- **"No stored glasses identity" or "Could not connect … set up for this PC":** run the setup. It also fails this way if another PC or the phone took ownership since.
- **"This PC could not join the glasses' Wi-Fi network":** check that Wi-Fi is on and airplane mode is off. "No Wi-Fi adapter is available" means Windows sees no Wi-Fi adapter at all.
- **"The glasses did not start their Wi-Fi network":** the glasses' Wi-Fi may still be busy with an earlier session. The source retries; if it keeps failing, power-cycle the glasses.
- **The internet drops while the camera runs:** the PC's Wi-Fi adapter is on the glasses' network, which has no internet. Use Ethernet or a second Wi-Fi adapter for an online stream.
- **Waiting:** another Ray-Ban Meta source is using the glasses. It takes over when the other source leaves the program output.
- **After OBS crashed mid-stream:** the glasses close their network on their own within about 15 seconds. Delete any leftover `mgl-` network in Windows Wi-Fi settings if one remains.

Hardware history from before the BLE adapter is in [earlier Classic verification](reports/CLASSIC_VERIFICATION.md) and `PLAN.md`.

Do not infer latest-firmware compatibility, latency, accessibility or two-hour stability from these tests. Those remain explicit hardware and OBS gates.

## License

AGPL-3.0. Preserve [LICENSE](LICENSE) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). This is an independent personal prototype; no public release, signing identity, hosted service or endorsement is claimed.
