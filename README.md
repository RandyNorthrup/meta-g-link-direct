# Meta G Link Direct

Native Windows OBS plugin project for a direct Ray-Ban Meta Gen 2 connection.

**Current status: development foundation only. Camera capture, glasses authentication, live media, source controls, current-firmware compatibility, and the 120-minute streaming requirement are not implemented or verified. The module does not register a camera source.**

The confirmed product contract is in [PROJECT_BRIEF.md](PROJECT_BRIEF.md). [PLAN.md](PLAN.md) owns implementation milestones, proof, and deferred gates. The intended product receives local video directly into OBS, with adjustable quality and optional glasses audio disabled by default. Meta authentication is permitted for setup if required; the phone must be off during the eventual local-streaming acceptance tests.

## What exists

- C++20 frame-prefix library ported from the pinned AGPL-3.0 Visor implementation. It distinguishes incomplete headers, valid frame lengths, and invalid types without allocating or receiving camera data.
- Native Windows WLAN/Bluetooth probe. Default reports radio presence, Wi-Fi association and controller feature support separately. Explicit `--discover` performs a bounded passive BLE scan using Visor's Meta company/service markers. `--discover-classic` performs Classic inquiry and looks up Windows-registered paired Meta RFCOMM services by UUID. Counts/errors contain no names, addresses or payloads. Candidates and Windows bonds do not establish glasses authentication.
- Explicit `--connect-classic` selects only one Windows-registered Meta service with a paired peer, attempts an encrypted/authenticated Bluetooth socket connection with deadlines, closes it and sends no application frames. Multiple services are refused. It never pairs or silently falls back from BLE. Successful connection behavior still needs actual glasses proof.
- Explicit uncached per-peer Meta SDP lookup (`--discover-classic-live`), a separate paired socket check (`--connect-classic-live`), and an opt-in native Windows pairing picker (`--pair-classic`). Pairing never runs implicitly.
- Direct Winsock probes accept a trustworthy glasses Bluetooth address privately on stdin, without a Windows bond prerequisite: `--direct-classic-uuid` resolves the Meta service; `--direct-classic-channel7` explicitly uses the publicly observed Gen 2 control channel. Each waits for one bounded frame, sends no application messages and never switches routes silently. A matching hello prefix is a candidate, not authentication.
- C17 OBS module foundation compiled against the real OBS 32.2.2 SDK. An isolated loader test checks the DLL against the installed OBS runtime, its exports, and truthful development metadata. This is not a test of an OBS scene or streaming source.
- Three CTest suites cover 83 framing/truncation/type cases, 20 private-address format/order/boundary cases and OBS module ABI. Three Pester runner tests, 20 CLI contract cases and one separate live SDP test cover diagnostics, private input, report failures and actual query execution. These software checks do not establish a glasses connection.
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

OBS is discovered at its verified per-user installation. For another installation location, set the optional process environment variable `MGL_OBS_RUNTIME` to that installation's `bin/64bit` directory before configuring. The directory must contain `obs.dll`; it is a runtime path, not a credential. There is no `.env` loader or implemented Meta authentication configuration yet.

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

- `include/mgl/`, `src/protocol/`: portable framing contract and implementation.
- `src/windows/`: local controller capabilities, BLE/Classic diagnostics and a shared private MTA lifetime; later authenticated transport and identity code belongs here.
- `src/probe/`: diagnostic CLI.
- `src/plugin/`: native OBS module entry point; no camera source yet.
- `tests/`: independent native examples and disposable-resource runner checks.
- `scripts/Invoke-Project.ps1`: canonical task and defect-drill runner.
- `reports/`: actual observations and source-bound proof linked by the plan.

MSVC compiles native C/C++ and analyzes the C entry point. LLVM performs C++ analysis with the real compilation database. The C entry point avoids an unsupported clang-cl intrinsic in OBS headers; it does not replace OBS declarations with a private ABI.

Cppcheck uses the same database with MSVC external-include spelling normalized, explicit compiler builtins, standard/Windows models and six real dynamic OBS entry points. Missing-system-header notices from that model-based analysis and third-party dependency headers are excluded explicitly; quoted owned headers remain checked, and the real compiler/LLVM gates validate system includes. See the plan for these scoped analysis decisions and their required drills.

## Privacy and future credentials

Current code enumerates local radios, optionally scans BLE/Classic or queries SDP, offers explicit Windows pairing and bounded socket/hello checks, and exposes module metadata. Peer addresses exist transiently in memory for native API calls. Direct probes use hidden local input or caller-controlled stdin; reports retain counters and errors, never names, addresses, payloads or owner credentials. Only an explicitly requested direct-probe report is saved. It has no Meta login, stored owner identity, media recording, telemetry, automatic update check, or phone data importer. The accepted future policy requires protected local identity, explicit Forget, transient media buffers, requested redacted diagnostics, and OBS-controlled recording. Never put passwords, owner keys, tokens, Wi-Fi passphrases, raw captures, or private diagnostic bundles into this repository.

## Troubleshooting and next milestone

Randy requires continued work with the existing adapter. It reports Classic supported and LE unsupported (HCI/LMP version fields 4). BLE discovery fails with `TransportNotSupported`; that rules out the documented BLE route on this controller, not every possible Classic-only route. Earlier registered lookup found no Meta service. Actual uncached queries over two resolved peers also found none; one query timed out, so this was incomplete discovery. The Windows picker was canceled because glasses were absent; no new bond or socket was created. The existing paired headphones are separate from the glasses. See the current source-bound report in `PLAN.md`; [earlier Classic verification](reports/CLASSIC_VERIFICATION.md) remains historical.

[Public Gen 2 Linux research](https://github.com/MoTechnocious/rayban-meta-linux/blob/823225a7c3bd9917f9a64ce88b32a6893d1df591/research/channel7-findings.md) reports an unpaired RFCOMM channel-7 server hello. This supports trying a targeted Windows transport probe; it does not prove this adapter, current firmware, application authentication or streaming. The glasses' trustworthy Classic address has not been identified. If Meta AI exposes a specifically labeled Bluetooth address, use private local input above; no phone-content importer or address guessing is implemented.

Same LAN does not supply a camera endpoint. The [Visor protocol](https://github.com/macedonga/visor/wiki/Radios-and-links) negotiates a Wi-Fi endpoint through authenticated Bluetooth control. Actual Windows group-owner operation is still untested. Next: identify the glasses' Classic address and run the explicit UUID/channel-7 probes, then implement and prove protected application authentication, Meta setup if needed, and a real frame with the phone off. Meta AI on iPhone remains the companion; no supported local owner-key export has been established. Ownership/recovery behavior must be explained before any actual ownership change.

Do not infer current-firmware compatibility, 720p/30fps, latency, accessible source UI, or two-hour stability from the foundation tests. Those remain explicit hardware/OBS gates.

## License

AGPL-3.0. Preserve [LICENSE](LICENSE) and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). This is an independent personal prototype; no public release, signing identity, hosted service or endorsement is claimed.
