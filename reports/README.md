# Reports

Evidence referenced by the ledger in [`PLAN.md`](../PLAN.md). Ledger entries bind many of these files by SHA-256, so their bytes are kept exactly as produced (including native CRLF output). Each historical report describes the project as it was on its date; the current state is in the [README](../README.md) and the plan.

## Current

| File | Contents |
|---|---|
| [`WIFI_LINK_VERIFICATION.md`](WIFI_LINK_VERIFICATION.md) | Why the glasses host the Wi-Fi network, every hardware run on that link, and the OBS run |
| [`AUDIO_AND_OBS33_VERIFICATION.md`](AUDIO_AND_OBS33_VERIFICATION.md) | Glasses audio (raw AAC, no AudioSpecificConfig), OBS 33 installed and streaming, the ProgramData plugin location and installing on other drives |
| [`SHARED_LINK_AND_SETUP_VERIFICATION.md`](SHARED_LINK_AND_SETUP_VERIFICATION.md) | Stream changes inside one session, audio only with the camera off, the battery report, the shared link and keep-connected in OBS 33, the setup window, and the trust-signature finding |
| [`GLASSES_CONTROLS_AND_SPEAKERS.md`](GLASSES_CONTROLS_AND_SPEAKERS.md) | Which glasses controls reach this PC, the wear sensor, and OBS audio monitoring through the glasses' speakers |
| [`wifi-link-gates.txt`](wifi-link-gates.txt) | Gate results for the release gate (`package`) |
| [`red/softap-port.txt`](red/softap-port.txt) | Recorded red drill for the `SoftApNetworkInfo` port field |

## Historical milestones

| Milestone | Summary | Evidence files |
|---|---|---|
| Foundation (2026-10-15) | Build, quality gates, frame-prefix parser, OBS module ABI | [`VERIFICATION.md`](VERIFICATION.md), `foundation-red/`, `red/`, `red-*.txt`, `lint-*.txt`, `cppcheck-*.txt`, `snapshot-*.json` |
| BLE discovery (2026-10-15) | Passive BLE scan on the PC's original Classic-only controller | [`DISCOVERY_VERIFICATION.md`](DISCOVERY_VERIFICATION.md), `discovery-*`, `discovery-red/` |
| Classic investigation (2026-10-15) | Classic inquiry and RFCOMM service lookup before the glasses were paired | [`CLASSIC_VERIFICATION.md`](CLASSIC_VERIFICATION.md), `classic-*`, `classic-red/` |
| Existing-adapter probes (2026-10-15) | Uncached SDP, the pairing picker and direct RFCOMM probes on the original controller | [`EXISTING_ADAPTER_VERIFICATION.md`](EXISTING_ADAPTER_VERIFICATION.md), `existing-adapter-*`, `live-*`, `direct-rfcomm-*` |

The glasses were later paired through a Bluetooth adapter with LE support, and the authenticated session, ownership change, media and OBS source were proven from there; see the [CHANGELOG](../CHANGELOG.md).
