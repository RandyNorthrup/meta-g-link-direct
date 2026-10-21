# Reports

Evidence referenced by the ledger in [`PLAN.md`](../PLAN.md). Ledger entries bind many of these files by SHA-256, so their bytes are kept exactly as produced (including native CRLF output). Each historical report describes the project as it was on its date; the current state is in the [README](../README.md) and the plan.

## Current

| File | Contents |
|---|---|
| [`WIFI_LINK_VERIFICATION.md`](WIFI_LINK_VERIFICATION.md) | Why the glasses host the Wi-Fi network, every hardware run on that link, and the OBS run |
| [`AUDIO_AND_OBS33_VERIFICATION.md`](AUDIO_AND_OBS33_VERIFICATION.md) | Glasses audio (raw AAC, no AudioSpecificConfig), OBS 33 installed and streaming, the ProgramData plugin location and installing on other drives |
| [`SHARED_LINK_AND_SETUP_VERIFICATION.md`](SHARED_LINK_AND_SETUP_VERIFICATION.md) | Stream changes inside one session, audio only with the camera off, the battery report, the shared link and keep-connected in OBS 33, the setup window, and the trust-signature finding |
| [`LATENCY_AND_SYNC_VERIFICATION.md`](LATENCY_AND_SYNC_VERIFICATION.md) | Glass-to-OBS latency (about 0.4 s), audio/video sync (within a frame), reconnecting after power-on and after a Wi-Fi drop, and the two 0.3.2 fixes: audio tracks of 0.1.0 camera sources, and messages split inside their header |
| [`ACCESSIBILITY_AND_QUALITY_VERIFICATION.md`](ACCESSIBILITY_AND_QUALITY_VERIFICATION.md) | The 1080 × 1920 and 360 × 640 profiles in OBS with battery drain, the setup window without WebView2, the keyboard and screen-reader pass with the 0.4.0 fixes, and the battery file and obs-websocket vendor |
| [`FRAME_RATE_VERIFICATION.md`](FRAME_RATE_VERIFICATION.md) | Why OBS showed about 13 fps of the glasses' 30: burst delivery from the glasses' radio, what the Meta app does, the playout fix, and 12.8 → 29.6 distinct pictures per second in OBS |
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
