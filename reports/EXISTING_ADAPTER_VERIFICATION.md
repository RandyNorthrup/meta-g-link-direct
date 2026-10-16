# Existing-adapter Classic investigation

Observed during the 2026-10-15 local session (some host UTC timestamps fall on October 16). This supersedes the replacement-controller recommendation in the historical Classic report. It does not supersede the authenticated-camera acceptance requirements.

## Implemented increment

Continue with Randy's existing Classic-capable controller. The documented BLE discovery/bonding route fails because this controller reports LE unsupported. That observation alone cannot establish that unpaired Classic transport is impossible. The implementation now provides:

- Longer Classic inquiry and uncached per-peer Meta SDP with separate requested/completed/error/limit counters.
- An explicit native Windows pairing picker. It never runs from default probes or automated tests.
- Separate Winsock address-targeted probes for the Meta service UUID and explicitly selected channel 7. They require no Windows-bond check, send no application frames, receive at most one 4096-byte frame, close resources, and never silently switch routes.
- Private local address input with console echo disabled and restored; exact format/order validation rejects zero, broadcast, embedded NUL, trailing text and oversized input before a network call.
- Counters/status only in output. A process-local MGL_PROBE_REPORT path explicitly requests redacted JSON export; write failure remains nonzero. No identifiers, raw frames or credentials are saved by the probe.

The WinRT query/socket path keeps its existing paired/encrypted socket contract. The separate Winsock route tests whether a lower-level unpaired transport can work; it does not claim application authentication.

## Actual hardware observations

[Initial uncached result](live-sdp-initial.json) and [controlled result](live-sdp-controlled.json) each resolved two Classic peers and issued two uncached Meta service queries. Both found zero Meta services and one query timed out (HRESULT 0x800705B4), so discovery was incomplete. For the controlled result, Randy confirmed iPhone Bluetooth off in Settings and glasses powered/awake outside the case. These results preceded the longer 20.48-second inquiry setting.

[Native picker outcome](live-pairing-outcome.json): Randy reported glasses absent and canceled; native pairing_error was 1223, with no new bond or socket. The process completion was observed, so no pairing operation remains pending. Randy also confirmed the existing paired headphones are separate from the glasses. No peer address is guessed or inferred from those headphones.

The [final longer-inquiry observation](existing-adapter-live-observation.json) took 29,793 ms and returned native exit 3: two peers resolved, two uncached queries, zero Meta services, one timeout and no socket. Phone/glasses physical setup was not freshly reconfirmed for this last observation. The newer live-query software test exercised the longer inquiry and actual query counters. Its passing assertion checks query execution and honest error/exit reporting, not discovery of glasses. No positive direct Winsock connection/hello was attempted: the glasses' trustworthy Classic Bluetooth address remains unidentified. The Meta AI Bluetooth-address availability question is unanswered at this checkpoint.

## Protocol reference and limits

[Public Gen 2 channel-7 research](https://github.com/MoTechnocious/rayban-meta-linux/blob/823225a7c3bd9917f9a64ce88b32a6893d1df591/research/channel7-findings.md) reports a 116-byte server hello from an unpaired Linux host. Its twelve-byte header is only a candidate marker in this probe. No upstream scripts, credentials, app binaries or raw captures were copied/executed. This is evidence for trying a targeted Windows experiment, not proof for this controller, latest firmware, trusted identity or camera access.

Native inquiry and platform service resolution can add synchronous delay. Async query/socket waits have explicit deadlines; a hard whole-process deadline is not claimed. Only the test harness terminates its own child on deadline. Hidden interactive input/Narrator, positive/ambiguous service discovery, actual connect/hello and cleanup failure paths still require suitable direct observations.

## Verification state

Focused checks passed: native build; strict C++/PowerShell/workflow analysis; Cppcheck and zero clone threshold; three CTest suites (83 framing cases, 20 address cases, OBS ABI); three runner tests; twenty CLI cases; one separate actual live-SDP test. The initial newline-comparison fixture mismatch and analyzer findings remain in the initial logs; the report assertion compares JSON text independent of the Windows stdout newline translation. No test expectation was changed to claim transport success.

The [full configured quality aggregate](existing-adapter-quality.txt) exited 0: build, formatting, strict analysis, dead-code/copy checks, configured security scans, three CTest suites, twenty-four Pester cases and all twenty defect/restoration drills. The ASan drill rejected the intended heap overflow and its restored build passed. [Independent restoration audit](existing-adapter-restoration-check.json) checked all twenty current file hashes, intended diagnostics and baseline/red/restored exits. Source-bound PLAN receipts keep this software proof separate from deferred glasses/OBS acceptance. Raw native reports retain exact CRLF/spacing bytes under the declared report policy; owned-source whitespace checks exclude those immutable output artifacts.

## Blocker and next action

Identify the glasses' actual Classic Bluetooth address, preferably through a specifically labeled field in Meta AI, then enter it privately at the local probe prompt. Do not paste it into chat or arguments. Test the UUID route first and the channel-7 route separately if needed. No phone-content importer or new hardware requirement is introduced.

Protected application identity, Meta setup if required, authenticated device information, exact current firmware, phone-off/restarted sessions, Wi-Fi negotiation, real adjustable camera/audio, OBS source UI/accessibility and the 120-minute run remain open. Current DLL is metadata-only and is not installed as a functional glasses source. No driver/network reconfiguration, factory reset, ownership transfer, publication or refreshed ZIP occurred.
