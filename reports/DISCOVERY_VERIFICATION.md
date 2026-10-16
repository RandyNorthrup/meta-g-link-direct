# Direct discovery verification - 2026-10-15

The full `pwsh -NoProfile -File scripts/Invoke-Project.ps1 quality` command exited 0 after the new drill count was corrected. This verifies the configured software foundation and diagnostic contract. It does not establish a working glasses connection or OBS camera source.

## Current physical observations

`discovery-probe.json` contains the actual redacted native output; `discovery-observation.json` records the native exit, elapsed time and independent Windows adapter observation. The final scan returned in 27 ms because Windows aborted it, rather than completing its configured ten-second listening window.

| Observation | Result | Meaning |
|---|---|---|
| WLAN enumeration | 1 interface, error 0 | Previously absent adapter is now detected |
| WLAN association | 0 associated interfaces | Wi-Fi is disconnected; one Ethernet interface is up |
| Bluetooth radio | Present, enumeration error 0 | Radio existence does not establish BLE transport support |
| Passive BLE discovery | Error 9, `TransportNotSupported`; native exit 3 | Scan did not complete and received no packets |
| Recognizable glasses PnP name | No match | Heuristic cache check only; not proof that no bond exists or glasses are absent |
| Glasses LAN identity / authentication | Not verified | User reports shared LAN; no authenticated endpoint has been established |

Microsoft defines error 9 in [BluetoothError](https://learn.microsoft.com/en-us/uwp/api/windows.devices.bluetooth.bluetootherror). The [Visor radio protocol](https://github.com/macedonga/visor/wiki/Radios-and-links) negotiates the Wi-Fi endpoint after the Bluetooth session. Shared LAN does not establish that session. This result does not justify a hardware purchase, identify the precise driver cause, or rule out Classic RFCOMM.

## Delivered increment and tests

- Extended canonical `WirelessCapabilities` with Wi-Fi association count and the existing CLI with default/help/explicit discovery modes. New Windows discovery implementation uses the installed SDK public C++/WinRT projection; no new downloaded dependency.
- Passive scan has a ten-second listening window and two-second stop deadline, safe shared callback state, explicit native failures, counters only, and no pairing, ownership or network mutation. Meta company/service markers identify candidates only; no model/authentication claim.
- CTest: 2/2 suites, including 83 independent framing examples and real OBS-runtime module ABI checks. Pester: 3/3 runner tests plus 7/7 actual CLI tests. None skipped. Physical scan test verifies honest native failure reporting on this PC; it does not turn error 9 into successful discovery.
- Full configured format, compiler/static analysis, PowerShell/workflow checks, dead-code/duplication, secret/history/SAST/npm/OSV checks passed. Windows AddressSanitizer instrumented build and heap-overflow drill passed with the intended failure and restoration. New BLE success/callback paths were not exercised under ASan or race instrumentation.
- All 17 maintained drills returned baseline 0, intended red 1, restored 0. Independent inspection verified changed input hashes, exact restoration and matching diagnostics. New argument drill changes rejection into exit 0; real CLI assertions reject it. See `red-drills.json`, `red/*.txt` and `quality-discovery.txt`.
- All declared tool versions were independently re-observed; see `discovery-tools-observed.json` and normalized `discovery-context.json`.

Three line-scoped `misc-include-cleaner` annotations address LLVM 22 mapping of SDK generated type definitions to the included public projection; the real compiler and all other owned analysis remain enabled. Activation-factory/watcher null checks address the SDK activation boundary without suppressing the null-dereference analyzer. Initial lint/activation and stale-count attempts are retained as failures in the discovery initial logs. The stale full-set counter was raised from 16 to the actual 17; the complete aggregate was then rerun successfully.

Historical foundation logs and their original receipts are preserved under `foundation-red/` and `foundation-red-drills.json`. The earlier ZIP/`VERIFICATION.md` describe the earlier foundation revision; that ZIP has not been refreshed and contains no discovery CLI. No plugin was installed into OBS during this increment.

## Remaining work and next action

TASK-AUTH remains active and incomplete. Investigate the current Windows BLE transport failure and implement/test the documented Classic RFCOMM path, then protected identity and a reproducible authorized Meta setup flow. Any real ownership change must first explain phone coexistence and recovery. Existing Meta authentication authorization remains valid; no password, token, owner key or iPhone content was collected.

A successful scan/candidate path, callback stress/race testing, actual authenticated device information with phone off, latest stable firmware proof, Wi-Fi group-owner operation, real video/optional audio, OBS source/UI/accessibility, startup/reconnect/latency targets and the >=120-minute run remain open. Portable coverage/fuzz/Linux sanitizer and complete native component advisory audit remain deferred in the canonical plan. These limits are not waived by passing diagnostic tests.

Evidence storage correction: .gitattributes now disables text conversion under reports/ so native CRLF tool output and its SHA-256 survive Git round trips. Independently compared Git filtered versus raw blob hashes for every report file; all matched. This changes evidence storage only, after the software aggregate passed; source and executable inputs are unchanged. Fresh staged/history secret checks still apply.
