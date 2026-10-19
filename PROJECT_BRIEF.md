# Meta G Link Direct — project brief

| Field | Value |
|---|---|
| Status | Confirmed contract; first public pre-release (0.1.0) |
| Product owner | Randy |
| Technical work | Coding agent under Randy's direction |
| Created / last updated | 2026-10-15 / 2026-10-19 |
| Decision authority | Randy for scope, acceptance, releases and support |

This brief owns the product contract. [`PLAN.md`](PLAN.md) owns delivery tasks, evidence and open gates. [`README.md`](README.md) describes what is implemented and verified.

## Product

A native OBS Studio source plugin for Windows that shows the camera of Ray-Ban Meta Gen 2 glasses live in OBS, directly over local wireless links: no phone, no relay, no screen mirroring and no virtual webcam. Video quality must be good and adjustable. Glasses audio is optional and off by default; users normally record audio from their own OBS audio device.

Meta is contacted only during the one-time setup that makes the PC the glasses' owner. Streaming afterwards is local: Bluetooth for control, and a Wi-Fi network that the glasses host for the PC (see D15). The setup moves ownership away from the Meta AI phone app; users must be told this before it happens.

## Users and distribution

- Built for Randy first, then published for other users as pre-release builds on GitHub (`RandyNorthrup/meta-g-link-direct`, D17).
- Each release ships one zip built by the `package` task: a double-click installer, the plugin, the setup tool, the license and notices, plus a SHA-256 file. The binaries are unsigned, so Windows SmartScreen may warn.
- The installer never closes or restarts OBS and copies only this plugin's files. Uninstall removes those files; Forget deletes the stored identity.
- Support is best effort through GitHub issues. There is no SLA, telemetry or automatic update check.

## Targets

| Area | Target | Status (see PLAN.md) |
|---|---|---|
| Video | 720 × 1280 portrait at a nominal 30 fps, plus at least one lower profile | 720 × 1280 at 30 fps proven; lower profiles untested |
| Endurance | One full glasses battery charge with no drop or reconnect, recorded in measured minutes (D16) | Open; longest run so far 3 minutes |
| Latency | 95th percentile glass-to-OBS ≤ 500 ms | Not measured |
| Startup | First frame ≤ 15 s after a ready connection | About 2 s from a ready media session; about 18 s from the scene going live, including the Wi-Fi setup |
| Reconnect | ≤ 30 s after the glasses become available again | Implemented, not exercised on hardware |
| Stability | No growing media queue or memory; no OBS stall | Two-minute OBS run: flat memory, no render frames skipped by the source |
| Audio | Optional glasses audio, off by default | Not implemented |

No 1080p, latency, battery or uptime claim is made without measurement.

## Supported environment

- Windows 11 x64 and OBS Studio 32.x (verified on Windows 11 Pro build 26200 and OBS 32.2.2).
- A Bluetooth adapter with both Bluetooth LE and Classic (the glasses pair as a dual-mode device; the session runs over Classic RFCOMM) and a Wi-Fi adapter that can join a 2.4 GHz WPA2 network.
- Ray-Ban Meta Gen 2 glasses. The firmware build used for verification is not yet recorded. Other generations are untested. Firmware downgrades are not a support strategy.
- One glasses connection per OBS instance.

## Experience and accessibility

- Native OBS source properties: a quality list, a Wi-Fi adapter list (Automatic or a named adapter), a status line and a setup note. English only.
- Keyboard access, Narrator labels, visible focus, light/dark/high-contrast themes, properties usable from 480 logical pixels wide and at 100–200% display scaling (D08). Not yet verified.
- Errors are actionable and the OBS log names each connection step.
- No logos or brand assets of Meta, Ray-Ban, Apple or OBS. Product names identify compatibility only and imply no endorsement or affiliation.

## Data, security and privacy

- The owner key and any imported Meta app tokens are sealed with Windows DPAPI for the current user under `%LOCALAPPDATA%\MetaGLinkDirect`. Forget deletes them.
- Wi-Fi credentials for the glasses' network are random per session and live only in memory and in a temporary Windows profile that is deleted when the camera stops.
- The only credentials in the repository are the Meta AI app's two public client tokens (AGENTS.md revision 3). No user tokens, owner keys, passwords, Wi-Fi passphrases, raw captures or raw media may enter the repository, logs or diagnostics.
- No telemetry, update checks or plugin-managed recordings. OBS recording stays under the user's control. Logs carry status lines only.
- Known gap: the PC does not yet verify the glasses' trust signature.

## Decision ledger

| ID | Decision | State | Choice | Owner | Date |
|---|---|---|---|---|---|
| D01 | Product shape | Confirmed | Native OBS source plugin with a direct glasses connection | Randy | 2026-10-15 |
| D02 | Glasses generation | Confirmed | Ray-Ban Meta Gen 2 | Randy | 2026-10-15 |
| D03 | Audience and priorities | Revised | Randy first, other users through public pre-releases; good adjustable video; glasses audio optional; endurance per D16 | Randy | 2026-10-19 |
| D04 | Setup and credentials | Confirmed, proven | Meta authentication allowed for the one-time ownership setup; local streaming afterwards | Randy | 2026-10-15 |
| D05 | Supported machine | Confirmed | Windows 11 x64, OBS 32.x; latest stable glasses firmware, build to be recorded | Randy | 2026-10-15 |
| D06 | Source reuse and license | Confirmed | Visor-derived AGPL-3.0 source with attribution | Randy | 2026-10-15 |
| D07 | Performance targets | Revised | The targets table above; the 120-minute minimum is replaced by D16 | Randy | 2026-10-19 |
| D08 | UI and accessibility | Confirmed, unverified | OBS-native English controls, keyboard and Narrator, 480 px width, 100–200% scaling, no custom assets in the UI | Randy | 2026-10-15 |
| D09 | Privacy and identity | Confirmed | DPAPI-sealed local identity; no telemetry, update checks or media retention; Forget deletes the identity | Randy | 2026-10-15 |
| D10 | Distribution | Revised | Public GitHub pre-releases: an unsigned zip with a double-click installer and a SHA-256 file; manual updates; no automatic OBS restart | Randy | 2026-10-19 |
| D11 | Delivery constraints | Revised | Public repository `RandyNorthrup/meta-g-link-direct`; push, tag and release only on Randy's request; no SLA, deadline or spending | Randy | 2026-10-19 |
| D12 | Hosted services | N/A | No hosted product service | Randy | 2026-10-15 |
| D13 | Identity source | Closed | Meta-authenticated setup replaced the optional iPhone import research; no iPhone data was accessed | Randy | 2026-10-15 |
| D14 | Adapters | Closed | The PC's original Classic-only controller could not pair the glasses. Proven with a TP-Link USB adapter (Bluetooth LE and Classic) and one USB Wi-Fi adapter | Randy | 2026-10-18 |
| D15 | Wi-Fi link role | Confirmed, proven | No router, hotspot or network changes and no phone. The glasses host a 2.4 GHz / 20 MHz network (`CreateSoftApNetwork`) and the PC joins it as a client. While streaming, the PC's Wi-Fi adapter has no internet | Randy | 2026-10-18 |
| D16 | Endurance target | Confirmed | One full glasses battery charge with no drop or reconnect, recorded in measured minutes. Replaces the 120-minute minimum because streaming appears to drain the battery in roughly 35–40 minutes (unconfirmed) | Randy | 2026-10-19 |
| D17 | Public release | Confirmed | Publish the repository and a 0.1.0 pre-release for other users | Randy | 2026-10-19 |

## Research references

Upstream Visor was ported at commit [`e7077b4051b1e3e605c667aa2a010fbe41f346a0`](https://github.com/macedonga/visor/commit/e7077b4051b1e3e605c667aa2a010fbe41f346a0) (AGPL-3.0); see [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) for the ported files.

- [Visor README](https://github.com/macedonga/visor/blob/e7077b4051b1e3e605c667aa2a010fbe41f346a0/README.md): Gen 2 testing on firmware `72387580077200100`; Meta login and ownership setup; one glasses session at a time.
- [LiveStream.kt](https://github.com/macedonga/visor/blob/e7077b4051b1e3e605c667aa2a010fbe41f346a0/core/media/src/main/kotlin/com/macedonga/visor/core/media/LiveStream.kt): stream start-up, codec configuration and frame reassembly; its 1080p option is exploratory.
- [Wifi.kt](https://github.com/macedonga/visor/blob/e7077b4051b1e3e605c667aa2a010fbe41f346a0/core/media/src/main/kotlin/com/macedonga/visor/core/media/Wifi.kt): the phone hosts a group and the glasses join it with `JoinAccessPoint`. On this project's hardware every join-an-access-point path stopped video within 8–54 s; the Meta AI app's `com.oculus.atc` classes also define `CreateSoftApNetwork`, which this project uses instead ([report](reports/WIFI_LINK_VERIFICATION.md)).
- [Visor wiki: identity and trust](https://github.com/macedonga/visor/wiki/Identity-and-trust), [Meta login and ownership](https://github.com/macedonga/visor/wiki/Meta-login-and-ownership): connection authentication uses the owner key held by the glasses; no Meta call after setup.
- [OBS source API](https://docs.obsproject.com/reference-sources): asynchronous video sources.
