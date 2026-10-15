# Foundation verification — 2026-10-15

The configured Windows software checks passed through the actual `package` task, which runs the complete `quality` aggregate before staging the development ZIP. See `quality-package.txt`, `red-drills.json`, each `red/*.txt` log, and the source-bound receipts in `PLAN.md`.

## Results and limits

| Check | Result | Scope |
|---|---|---|
| Native configure/build | Pass | Windows x64, MSVC C17/C++20, OBS SDK 32.2.2 |
| CTest | Pass, 2/2 | 83 framing/truncation/type cases and real-runtime module ABI/metadata loading |
| Pester | Pass, 3/3; none skipped | Disposable source formatting, WhatIf preservation, native failure propagation |
| Windows AddressSanitizer | Pass, including failing heap-overflow canary and restored green | Instrumented owned test/module build; external OBS runtime is not sanitizer-certified |
| Format and static analysis | Pass | Owned C/C++ formatting, C++ clang-tidy, MSVC C analysis, PSScriptAnalyzer, actionlint, pedantic zizmor |
| Dead code and duplication | Pass | Configured Cppcheck models/owned-source scope and jscpd; scoped analysis decisions are recorded in the plan |
| Secrets | Pass | Directory scan, staged pre-commit scan and reachable initial Git history; randomized base32 AWS canary was rejected |
| SAST | Pass | Three configured C/C++ rules, six source targets; unsafe-copy drill was rejected |
| npm / OSV | Pass | Nine locked npm development packages; both rejected a temporary vulnerable lodash lock entry without installing it |
| Maintained red drills | Pass, 16/16 | Every drill returned baseline 0, intended nonzero 1, restored 0, with exact byte/hash restoration |
| Native input integrity | Pass | Four independently rechecked pinned archives; see `native-input-integrity.json` |
| ZIP inspection | Pass | CRC, exactly four expected files, x64 PE machine type, packaged DLL identical to tested DLL; see `package-manifest.json` |

The 16 drills cover parser length, module metadata, formatting, owned-header lint, compiler type checking, unused code, script analysis, workflow security, secret detection, unsafe copying, duplication, ASan heap overflow, npm audit, OSV, WhatIf guarding and native exit propagation. Original failed canary/harness attempts are retained separately. They do not count as passing proof.

## Development artifact

- Archive: `dist/foundation-0.1.0.zip`
- SHA-256: `5ca04a9a7a8afe3fc7a213366b8a80d0ca2293404602fd4ec2e471ef16e0736f`
- Product source revision: `6959a50efcbc758757a716507080bac7066482a2` (subsequent reconciliation changes are documentation/evidence only).
- The archive contains the development metadata DLL, README, AGPL license and third-party notices. It is unsigned and has no camera source. It has not been installed into the user's OBS profile.

## Open verification

- Discovery, authenticated direct session, authorized Meta ownership/application-token flow, phone coexistence/recovery, real camera frames and optional audio are not implemented or proved. Owner: technical implementation; next: M1 direct connection prototype.
- Actual latest stable Gen 2 firmware compatibility, quality selection, end-to-end latency, reconnect/startup targets, OBS source behavior, accessible responsive controls, and the minimum 120-minute run remain required. Owner: technical implementation and Randy for physical acceptance; due before their hardware/OBS milestones.
- Windows currently reports no WLAN interface and Bluetooth present. The available adapter must be connected and its actual group-owner capability tested before a direct media run. No adapter-model interview or driver/network change was performed.
- Portable coverage/floor, fuzzing and Linux/hosted sanitizer execution remain open before the session/media parser expansion is certified. No coverage percentage, Linux UBSan/TSan/Valgrind pass, or hosted CI pass is claimed. Owner: technical implementation; next: supported portable-core execution before M1/M2 certification.
- The npm dependency audit does not establish an audit of native OBS, FFmpeg or other SDK components. Owner: technical/security work; next: native component inventory and advisory assessment before accepting authentication/media dependencies.

This is foundation software evidence. It is not certification of a functioning glasses plugin, OBS scene, hardware compatibility, long-session reliability or public release.
