# Controller and Classic verification - 2026-10-15

The full `pwsh -NoProfile -File scripts/Invoke-Project.ps1 quality` command exited 0. This establishes configured software checks for the controller/Classic diagnostic increment, not authenticated glasses access or an OBS camera source.

## Hardware observations and blocker

Actual final output is in `classic-observation-final.json`. Native controller information comes from the installed SDK's `IOCTL_BTH_GET_LOCAL_INFO` and LMP feature definitions; no names or addresses are exported.

| Observation | Final result |
|---|---|
| Controller feature query | Error 0, valid result |
| LE / Classic supported | false / true |
| HCI / LMP version fields | 4 / 4 |
| Wi-Fi | One interface, zero associated; Ethernet remains the existing PC network path |
| Classic inquiry | Completed, error 0, two device reports, one Windows bond, 5211 ms |
| Windows-registered Meta RFCOMM services | 0 |
| Explicit connection command | Native exit 6 in 37 ms; no socket connection attempted |
| User's pairing attempt | Glasses do not appear in Windows |
| Other Bluetooth controller available | User reports none |

The valid LE feature result explains why the documented BLE discovery route is unavailable on this controller. [Visor's protocol notes](https://github.com/macedonga/visor/wiki/Radios-and-links) describe initial LE discovery/dual bonding and a Classic fallback that needs a bond. That documented initial path is blocked on current hardware. A Windows-compatible controller supporting LE and Classic is the next hardware prerequisite; no purchase, driver replacement, firmware downgrade or reset was performed or authorized by this investigation.

Classic inquiry can include cached peers. Registered-service lookup is not a complete uncached SDP scan and does not prove that no Meta service exists on an undiscovered remote peer. Neither inquiry counts nor Windows bonding establish application authentication. No account/ownership changes, iPhone-content access or Meta credentials were used.

## Delivered code and verification

- Extended the existing radio probe with valid/unknown controller feature state. Optional feature-query failure does not silently mean LE unsupported.
- Added `--discover-classic` for explicit Classic inquiry and registered Meta UUID lookup, and separate `--connect-classic` that requires one paired service, requests Bluetooth encryption/authentication, attempts a socket open with deadlines, explicitly closes it and sends no application frames. There is no BLE-to-Classic automatic fallback or guessed RFCOMM channel.
- Extracted the existing MTA lifetime into one private Windows header with actual BLE and Classic consumers. New `classic.cpp` has the distinct Classic diagnostic responsibility. No new downloaded dependency or fake OBS source.
- CTest: 2/2 suites, including 83 framing examples and real OBS-runtime module metadata/ABI checks. Pester: 3/3 runner and 10/10 real CLI tests. None skipped. Tests exercise honest current hardware outcomes, not successful glasses pairing.
- All configured formatting, compiler/static analysis, PowerShell/workflow, dead-code/duplication, secret/history/SAST/npm/OSV checks passed. Windows AddressSanitizer build and intentional heap-overflow/restoration drill passed. Real BLE/callback and RFCOMM connected/canceled/error paths were not sanitizer/race-certified.
- All 18 maintained drills returned baseline 0, intended red 1, restored 0, with independent hash/diagnostic checks. The new connection-claim mutation makes discovery falsely report connected; the CLI assertion rejects it. See `quality-classic.txt`, `red-drills.json` and `red/*.txt`.
- Nine Classic and three existing BLE type declarations retain line-scoped `misc-include-cleaner` annotations for LLVM 22 mapping of SDK generated types to public projection headers. Real public SDK headers and all other owned analysis remain enabled. Explicit activation-factory/result guards address null handling without suppressing the analyzer. Initial lint findings are retained in `classic-lint-initial.txt`.
- All declared tools were independently re-observed and unchanged in `classic-context.json`. Source and receipt-artifact bytes are checked against staged Git before committing; raw report bytes use the existing `reports/** -text` rule.

Five-second deadlines apply to each asynchronous lookup/open/connect. Timeout requests cancellation; socket close releases pending connection. Classic inquiry requests 5.12 seconds from Windows, with driver/operation overhead possible. It is not a hard whole-command deadline. Connection success, pairing-consent behavior, timeout/cancellation with a real Meta peer, multiple-service selection and return-to-phone behavior remain unproved.

Earlier discovery red logs are archived in `discovery-red/` with `discovery-red-drills.json`; original foundation logs remain in `foundation-red/`. The foundation ZIP remains historical and has not been refreshed or installed into OBS.

## Resume

TASK-SETUP software proof is refreshed; TASK-AUTH hardware acceptance remains blocked and its authenticated session/identity/Meta setup implementation is incomplete. When a suitable controller is available, rerun capability/discovery checks, establish Windows pairing without removing Meta ownership, then exercise actual service lookup and explicit socket connection. If a Windows bond exists but registered Meta lookup remains empty, investigate the direct uncached RFCOMM service API before treating absence as incompatibility.

The [manufacturer pairing instructions](https://www.ray-ban.com/australia/c/frequently-asked-questions-ray-ban-meta-smart-glasses) describe placing glasses in the case and holding the rear button until blue pairing indication. Do not factory-reset or change account to compensate for a controller limitation. An ordinary Bluetooth bond can interrupt the phone link; reconnect through iPhone Bluetooth/Meta AI using the existing account, and stop if recovery demands reset/account change. This recovery path has not been exercised here. Real ownership changes still require a concrete coexistence/recovery explanation first.

Authenticated device info with phone off and after glasses restart, actual latest stable firmware, real video/optional audio, Wi-Fi group-owner operation, quality/latency/start/reconnect targets, accessible responsive OBS UI and >=120-minute continuous streaming remain open. Native component advisory audit and portable coverage/fuzz/Linux/race instrumentation remain deferred. No full-project closure or working camera claim is made.
