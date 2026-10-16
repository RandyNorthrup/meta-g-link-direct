# Meta G Link Direct — project brief

| Field | Value |
|---|---|
| Status | Confirmed — product contract accepted by Randy on 2026-10-15; implementation and hardware evidence pending |
| Product owner | Randy |
| Technical work | Coding agent under Randy's direction; Randy owns personal-prototype maintenance, security, support, and releases |
| Created / last updated | 2026-10-15 |
| Decision authority | Randy for product scope, acceptance, and the confirmed personal-prototype ownership below |

## Executive contract

The requested product is a native OBS Studio source plugin for Windows that receives live camera video and optionally glasses audio directly from Ray-Ban Meta glasses. Randy confirmed **Gen 2**, correcting the Gen 1 assumption in the supplied research. The intended receiving path runs inside OBS without a phone bridge, streaming relay, screen mirroring application, or virtual webcam driver. Initial use is personal, with possible public distribution later. Video quality must be good and adjustable. Randy normally uses another audio device, so glasses audio is optional and disabled by default.

The first usable OBS release must prove **at least 120 minutes of continuous streaming** on the current stable glasses firmware. Battery, thermal, radio, and firmware limits may prevent this; the acceptance target must not be silently lowered. Accepted video targets are a measured 720 × 1280 portrait stream at a nominal 30 frames/second at the highest required profile, at least one lower supported quality profile, and no growing media queue. Accepted glass-to-OBS latency target is a 95th percentile of at most 500 ms. These are confirmed requirements, not observed device capabilities.

Local connections are preferred. Randy subsequently allowed Meta authentication when necessary, relaxing the earlier strict offline-setup requirement. The intended design uses Meta only for required authentication/ownership setup, then receives video and optional audio directly over local wireless links with the phone powered off. A requirement for a Meta service on every streaming session must be surfaced as a new decision rather than silently added.

The glasses are already configured with Meta AI on iPhone. Randy authorized investigation of a one-time local import of existing pairing/ownership data without exposing credentials, if feasible. No verified iPhone export route was found. This investigation is optional now that Meta authentication is allowed; it must not become an unnecessary prerequisite or cause phone data collection without a concrete need.

The first hardware milestone must prove an authenticated connection and a decoded camera frame on Windows with the phone powered off. A small diagnostic program may establish this before the connection library is embedded into the plugin. This sequence is confirmed; later implementation details remain subject to direct technical evidence.

Success requires real hardware evidence. Neither the availability of Windows wireless APIs nor Visor's Android implementation demonstrates a working Windows connection. Firmware compatibility, initial ownership setup, reconnection, measured media characteristics, and stable OBS operation remain unverified.

## People and delivery constraints

- Confirmed requester and product decision maker: Randy.
- Primary audience: Randy's personal use first. Public distribution may be considered later; commercial distribution is not selected.
- Proposed ownership for this personal prototype: Randy accepts product, privacy, security, release, rollback, support, and retirement decisions; the coding agent performs authorized engineering and records evidence. There is no external support SLA or on-call service. Public distribution requires a new readiness review.
- No deadline has been supplied. Proposed delivery is milestone-driven with no spending, certificate purchase, new public service, or public publication. Use existing hardware and tools or project-local development dependencies. Revisit schedule, funded maintenance, and any purchase before expanding these boundaries.
- Applicable constraints already provided: use the caveman skill for chat; maintain documentation; make all UI accessible and responsive; report next steps and blockers.

## Scope and journeys

The native OBS plugin, direct Windows connection, adjustable video quality, and optional glasses audio are confirmed project goals. The following complete journey is proposed for final contract confirmation:

1. Discover glasses and provide clear pairing and authentication status.
2. Complete ownership setup and retain the necessary local identity securely.
3. Start a direct live video session, adjust its supported quality settings, and expose it as an OBS source. Allow glasses audio to be enabled optionally; the separately configured OBS audio device remains Randy's normal audio path.
4. Stop, restart, and recover from a wireless disconnect or glasses restart.
5. Forget the local identity and remove stored credentials through a documented path.

The first hardware proof must distinguish authentication/ownership setup from normal streaming. Prefer local identity import only if a reproducible route is found; otherwise investigate Meta-authenticated setup that provisions a PC-owned identity. After setup, prove new streaming sessions and reconnection with the phone off and the plugin's internet path unavailable. A factory-reset initialization path is not required by the current answers. Bluetooth pairing alone must not be confused with possession of the application owner key.

Visor's ownership flow changes the glasses' stored owner key. Coexistence with the Meta AI phone app and a return-to-phone workflow need explicit testing and clear user-facing explanation before any real ownership change. No ownership change has been attempted.

Proposed non-goals are mobile app development, assistant/LLM features, gallery import, firmware modification, cloud hosting, public distribution, other glasses generations, other operating systems, custom audio-device routing, and factory-reset initialization without Meta. Firmware downgrading is not an acceptable support strategy for this scope.

There are no existing requirement/task identities or migration obligations in this workspace. Once the brief is confirmed, `PLAN.md` will be the canonical delivery and verification ledger.

## Experience, brand, and accessibility

- Proposed working name: Meta G Link Direct, derived from the workspace name. Public brand/trademark review is deferred until public distribution is proposed; no affiliation claim or third-party logo is planned.
- Existing visual assets: none found in the initial workspace scan. No logos, fonts, artwork, or brand permissions have been supplied.
- Proposed UI: native OBS source properties with clear status, labeled controls, actionable errors, and OBS theme integration. Use existing OBS controls; no new logo, artwork, web UI, or design system is needed.
- Accessibility and responsive behavior are required by the requester. Proposed acceptance covers complete keyboard access, visible focus, Narrator labels/status, light/dark/high contrast behavior, minimum 4.5:1 ordinary-text contrast and 3:1 control/focus contrast where the plugin controls colors, and usable layouts at 100%, 150%, and 200% Windows display scaling.
- Verify properties dialogs from 480 logical pixels wide through the available desktop width, with wrapping or scrolling rather than clipped controls. Use English initially; other languages and RTL support are outside the first personal prototype. No animation is planned. Randy is the proposed visual acceptance owner.

## Product shape and supported environments

- Confirmed shape: native OBS Studio input source plugin on Windows, with direct wireless communication to Gen 2 glasses.
- Selected foundation: C++20 protocol/platform library, C17 OBS ABI entry point, CMake/Ninja, and MSVC 19.44.35222.0 with Windows SDK 10.0.26100.0. LLVM 22.1.1 supplies formatting and C++ static analysis. The C entry point uses MSVC native analysis because an OBS header requires `_udiv128`, unsupported by clang-cl. Exact archives and observed tools are recorded in `dependencies.lock.json`; `PLAN.md` records build and verification decisions.
- Confirmed deployment machine: this PC. Current inventory reports Windows 11 Pro x64, version `10.0.26200`, build `26200`, and OBS Studio `32.2.2`. Broader supported versions remain open.
- A working-status Bluetooth radio is present. Controller feature inspection succeeds and reports LE unsupported, Classic supported, HCI/LMP version fields 4; this is consistent with passive BLE error 9 (`TransportNotSupported`). After Randy connected Wi-Fi, Windows enumerated one WLAN interface, currently unassociated, with Ethernet up. Classic inquiry succeeds, but no registered Meta RFCOMM service is present. Randy reports glasses do not appear in Windows pairing and no other Bluetooth controller is available. The documented LE initial bonding path is unavailable on this controller. Randy requires continued use of the existing adapter; Classic-only endpoint identification/direct probing is now the investigation path. Uncached queries found no Meta service with one timeout, the native picker was canceled because glasses were absent, and the paired headphones are separate. Actual authenticated operation and group-owner capability remain unproved. No adapter-model interview or purchase is authorized by this observation. Current redacted facts belong to `reports/CLASSIC_VERIFICATION.md`; the inventory below remains historical.
- Confirmed firmware support target: the latest stable firmware available for Randy's Gen 2 glasses at verification time, with no intentional requirement to downgrade. Record the exact installed build automatically if the direct protocol exposes it, and bind acceptance evidence to that build. Compatibility with later updates requires revalidation; the current target is a requirement, not a completed compatibility claim.
- Current companion setup: official Meta AI app on iPhone. Android-only export methods do not establish an import path for this setup.
- Proposed scale: one glasses connection on one local OBS instance; additional devices or concurrent sessions require confirmation.
- Browsers, cloud regions, currencies, hosted tenants, and server databases: N/A to the proposed local plugin; revisit if scope changes.

## Distribution, signing, and updates

Confirmed delivery for the personal prototype: unsigned local plugin ZIP, manual installation and updates, and no public release yet. Randy approved Visor-derived source under AGPL-3.0 with attribution. Preserve upstream notices and record reused paths and commits; public distribution licensing review remains a later release dependency.

Proposed installation lifecycle: stage a versioned ZIP with SHA-256 checksums; install or replace only this plugin's files when OBS is closed; keep a last-known-good local artifact for rollback; remove only this plugin's files on uninstall. Do not restart an active OBS session automatically. No signing identities, certificate custody, store approval, automatic updater, or notarization are applicable to this unsigned Windows-only prototype. Public release would reopen those decisions.

Trace artifacts to the exact source revision and dependency versions. A separate signed provenance service and published SBOM are deferred to any public-release review; dependency/source inventory and license attribution remain required now.

Confirmed privacy: protected local pairing identity; no telemetry, automatic update checks, or plugin-managed media recordings; redacted diagnostics only on request; Forget deletes the plugin's stored identity. OBS recording remains under the user's control. Proposed diagnostics are memory-only during ordinary operation, with an explicit local export containing no keys, tokens, wireless passphrases, raw media, or raw protocol dumps. Randy controls deletion of any deliberately exported diagnostic file.

## Data, authentication, security, and privacy

Data classes include Meta authentication material during authorized setup, the local glasses ownership identity, Bluetooth identifiers, temporary wireless network credentials, transient video/audio buffers, and requested redacted diagnostics. Proposed handling: encrypt persistent owner identity using Windows user-scoped protection; retain it until Forget; keep temporary access tokens, passphrases, and media in memory for the minimum required session lifetime; do not retain a Meta password; redact OBS log output. Confirm any additional persistence before introducing it.

No automatic identity backup is planned. If the identity is lost, provide an explicit re-authentication/re-provisioning recovery path, with its effect on phone coexistence explained. Forget removes plugin identity data, not unrelated system Bluetooth bonds or the phone app's data. No private material may enter project files, CI, artifacts, or diagnostic exports.

The proposed trust boundaries are glasses ↔ Windows wireless stack ↔ embedded connection library ↔ OBS, plus Meta services during required authentication/ownership setup or an optional one-time local identity import. Direct local media remains the goal. Live media processing and credential handling require explicit boundaries; OBS recording behavior must remain understandable to the user.

Visor documents Meta login and an ownership receipt during its normal setup. Meta authentication is now permitted, so a desktop port of that flow is a candidate. Its source builds require Meta application client tokens absent from the repository; whether there is a reproducible authorized way to supply the necessary application identity remains an implementation prerequisite to investigate. No credential purchase, account ownership, or new application registration is assumed. Credentials must not be requested in chat or written to source, logs, or project documentation.

The inspected `Trust.kt` proves possession of an owner key over the glasses link without an HTTP request. `Bundle.kt` represents imported owner identities, including a private key. This supports investigation of an offline client given an accepted key; it does not prove access to the key held by the official Meta AI app, nor a successful Windows session. The current companion is Meta AI on iPhone. No verified owner-key export path for that application was found in the inspected upstream code or documentation. Copying Bluetooth pairing information or a sealed identity file alone is not established as sufficient.

Apple documents application access controls and device-specific protections for iOS Keychain data. This is a reason not to assume an ordinary backup or USB connection yields an exportable owner key. The inspected sources do not establish where Meta AI for iPhone stores this particular key or its protection class. No categorical claim that export is impossible is justified by this research. No iPhone backup, device contents, or credentials have been accessed.

Visor is marked AGPL-3.0, and Randy approved Visor-derived AGPL-3.0 project source with attribution. Qualified public-distribution review is deferred until such a release is requested. This brief records the chosen scope and does not provide legal certification.

## Existing material and research evidence

The initial stack scan succeeded on 2026-10-15. The workspace contained no files, source code, assets, project configuration, or Git repository. There is no canonical implementation to extend here. The supplied research is an input, not proof of hardware support.

Upstream Visor was inspected at commit [`e7077b4051b1e3e605c667aa2a010fbe41f346a0`](https://github.com/macedonga/visor/commit/e7077b4051b1e3e605c667aa2a010fbe41f346a0). References below are pinned where possible:

- [README and requirements](https://github.com/macedonga/visor/blob/e7077b4051b1e3e605c667aa2a010fbe41f346a0/README.md): author reports Gen 2 testing on firmware `72387580077200100`; other glasses are untested. Setup uses Meta login/ownership. Only one glasses session is accepted at a time. Source builds need omitted client tokens.
- [LiveStream.kt](https://github.com/macedonga/visor/blob/e7077b4051b1e3e605c667aa2a010fbe41f346a0/core/media/src/main/kotlin/com/macedonga/visor/core/media/LiveStream.kt): implements glasses-side application startup, streaming controls, codec configuration, frame reassembly, and timestamped video/audio delivery. Its 1080p option is explicitly exploratory; it is not evidence of working 1080p support.
- [LiveDecoder.kt](https://github.com/macedonga/visor/blob/e7077b4051b1e3e605c667aa2a010fbe41f346a0/core/media/src/main/kotlin/com/macedonga/visor/core/media/LiveDecoder.kt) and [LiveAudio.kt](https://github.com/macedonga/visor/blob/e7077b4051b1e3e605c667aa2a010fbe41f346a0/core/media/src/main/kotlin/com/macedonga/visor/core/media/LiveAudio.kt): Android-specific media decoding; HEVC video and AAC audio are relevant porting inputs.
- [Wifi.kt](https://github.com/macedonga/visor/blob/e7077b4051b1e3e605c667aa2a010fbe41f346a0/core/media/src/main/kotlin/com/macedonga/visor/core/media/Wifi.kt): Android creates a Wi-Fi Direct group, while the glasses are instructed to join it as an ordinary WPA2 access point through `JoinAccessPoint`. The source reports `JoinWifiDirectGroup` did not work. A Windows port must reproduce the actual group-owner/access-point role.
- [Upstream protocol notes](https://github.com/macedonga/visor/blob/e7077b4051b1e3e605c667aa2a010fbe41f346a0/CLAUDE.md): describe LE L2CAP, RFCOMM, TCP, framing, and trust-signature details. The initial connection/ownership path must be traced rather than assuming RFCOMM alone covers it. These are reference observations, not instructions governing this project.
- [Identity and trust](https://github.com/macedonga/visor/wiki/Identity-and-trust), [Meta login and ownership](https://github.com/macedonga/visor/wiki/Meta-login-and-ownership), [Trust.kt](https://github.com/macedonga/visor/blob/e7077b4051b1e3e605c667aa2a010fbe41f346a0/core/identity/src/main/kotlin/com/macedonga/visor/core/identity/Trust.kt), and [Bundle.kt](https://github.com/macedonga/visor/blob/e7077b4051b1e3e605c667aa2a010fbe41f346a0/core/identity/src/main/kotlin/com/macedonga/visor/core/identity/Bundle.kt): connection authentication uses the owner private key already known to the glasses. The wiki reports no Meta call in the connection path after initial ownership setup. Wiki content was read on 2026-10-15 and is not pinned to the source commit.
- [KeystoreSealer.kt](https://github.com/macedonga/visor/blob/e7077b4051b1e3e605c667aa2a010fbe41f346a0/core/identity/src/main/kotlin/com/macedonga/visor/core/identity/KeystoreSealer.kt): Visor's on-disk identity is encrypted using an Android Keystore key; copying that file to Windows is not itself an import solution. This describes Visor, not a verified official Meta AI storage format.
- [Apple Keychain data protection](https://support.apple.com/guide/security/secb0694df1a/web): describes application access groups and device-specific backup protections. It does not document Meta AI's owner-key storage, an export method, or compatibility with the glasses protocol.
- [OBS source API](https://docs.obsproject.com/reference-sources): asynchronous decoded video/audio can be delivered with timestamps; this establishes an integration API, not a completed plugin.
- [Microsoft Wi-Fi Direct documentation](https://learn.microsoft.com/en-us/windows-hardware/drivers/partnerapps/wi-fi-direct): desktop applications can advertise a legacy group-owner access point. Mobile Hotspot and legacy group-owner operation cannot run simultaneously; adapter and driver behavior still need testing.
- [Microsoft Bluetooth RFCOMM documentation](https://learn.microsoft.com/en-us/windows/uwp/devices-sensors/send-or-receive-files-with-rfcomm): Windows exposes RFCOMM APIs. This does not establish complete compatibility with the glasses bootstrap and ownership exchange.

The initial research snapshot preceded implementation. The current foundation includes a small Visor-derived frame-prefix port, native radio diagnostics, an OBS module with metadata only, tests, build inputs, and quality tooling. The module does not register a camera source. No glasses pairing/ownership changes, iPhone data access, credential acquisition, or networking changes have been performed. Current proof and deferred gates belong to `PLAN.md` and its referenced reports.

### Initial read-only development-machine inventory

Initial observations on 2026-10-15, before the Wi-Fi adapter was connected. This table is historical; current radio/scan observations are in `reports/DISCOVERY_VERIFICATION.md`. These observations do not establish broad platform support:

| Item | Observed result | Implication |
|---|---|---|
| Operating system | Windows 11 Pro x64, version `10.0.26200`, build `26200` | Initial target machine identified |
| OBS | `32.2.2`, installed under `%LOCALAPPDATA%\Programs\obs-studio` | Initial plugin compatibility target identified |
| Physical network interfaces | Two Intel I225-V Ethernet interfaces; one connected | Does not provide the proposed direct wireless access-point role |
| WLAN | `netsh wlan show drivers`: `There is no wireless interface on the system.`; WLAN AutoConfig service running | No usable WLAN interface currently enumerated; not proof of absent or incompatible adapter hardware |
| Bluetooth | `Generic Bluetooth Radio`, `BTHUSB`, status `OK`, device error code `0`; Bluetooth service running | Radio present; glasses discovery/RFCOMM interoperability untested |
| Build tools | CMake, Clang, clang-tidy, clang-format, and Git found | Availability only; versions and compatibility not yet checked |
| Other tools | Ninja found later in Visual Studio's CMake tools; cppcheck and gcovr not found in initial inventory | Reuse installed Ninja; explicitly track any unavailable gate |
| Supplied adapter | Randy reports a Wi-Fi adapter is available | Model, connection, drivers, and group-owner capability still unknown |

## Quality, reliability, and evidence

Proposed milestone order:

1. Verify local prerequisites and a reproducible authentication/ownership setup path. Prefer a local import if feasible; Meta authentication is allowed when needed. Treat iPhone identity export as optional research, not a blocking requirement.
2. Prove a direct authenticated connection and read device information.
3. Decode a real camera frame with the phone off and no internet access; measure actual resolution, frame rate, latency, resource use, thermal behavior, and duration. Prove quality adjustment within device-supported limits. Verify optional glasses audio independently of the normal external OBS audio device.
4. Prove cold start and reconnection after glasses restart with the phone still off.
5. Integrate the demonstrated path into OBS and verify source lifecycle, audio/video synchronization, error recovery, accessible controls, and responsive layouts.

The 120-minute minimum session requirement is confirmed. Proposed numeric quality/latency targets appear in the executive contract. Measure decoder/transport CPU and working set on this PC before choosing resource ceilings; fail acceptance for an unbounded queue or sustained memory growth. First-frame startup within 15 seconds after a ready connection and reconnection within 30 seconds after glasses become available are proposed initial targets. No 1080p, frame-rate, latency, uptime, or battery claim is approved without measurement.

Tests will need deterministic protocol/parser fixtures, malformed/truncated input coverage, transport cancellation and timeout checks, lifecycle tests, integration checks, real hardware runs, and OBS verification. Repeatable green → intended red → restored green drills are required for implemented behavior and configured gates. Mocked transport results cannot satisfy direct hardware acceptance.

Requirements readiness, automated test results, real glasses evidence, and OBS acceptance remain separate statuses. Foundation checks now exist; consult current source-bound receipts in `PLAN.md`. A successful DLL loader test does not establish an OBS scene, camera stream, current-firmware support, accessible source UI, or the 120-minute requirement.

## Release pipeline, operations, and lifecycle

Proposed source/release model: local Git on `main`, with no remote or publication until separately requested. Keep CI-compatible checks in the repository; GitHub Actions definitions may be prepared but must not be presented as hosted passing CI until actually run. Use semantic pre-release versions, real changelog entries, SHA-pinned actions, read-only workflow permissions, dependency and secret scans, compiler/static-analysis gates, and repeatable red drills. Network credentials and real glasses remain outside hosted CI; real hardware acceptance runs locally.

Randy is the proposed release and rollback authority. Keep the current and last-known-good ZIP with checksums; manually remove/replace the plugin if it causes OBS instability. Log status and actionable failures without private data. Export diagnostics only at the user's request. Personal support and incident decisions belong to Randy; no external messaging, alert service, availability SLA, cloud backups, or funded public maintenance is part of this scope. On retirement, uninstall this plugin and Forget its local identity; document any necessary return-to-phone setup. Do not collect another application's private files without a concrete, authorized import path.

For the proposed local plugin, managed cloud availability, database migrations, hosted on-call rotations, and disaster-recovery infrastructure are N/A unless the service model changes. Local identity loss and recovery still require a decision.

## Decision ledger

| ID | Decision | State | Choice or question | Owner | Due | Blocking impact |
|---|---|---|---|---|---|---|
| D01 | Product shape | Confirmed | Begin project setup for a native OBS plugin with a direct glasses connection | Randy | 2026-10-15 | None |
| D02 | Glasses generation | Confirmed | Gen 2; supersedes Gen 1 in supplied research | Randy | 2026-10-15 | Firmware and actual compatibility still unverified |
| D03 | Audience and success priorities | Confirmed | Personal use first; possible public release later. Good adjustable video; external audio normally used, glasses audio optional. At least 120 minutes continuous streaming | Randy | 2026-10-15 | Real media and full-duration acceptance remain unverified |
| D04 | Initial setup and credential path | Confirmed preference / Open verification | Local connections preferred; Meta authentication allowed when necessary. Prefer local streaming after setup. This supersedes the earlier strict offline-setup constraint | Randy / technical investigation | Revised 2026-10-15; setup proof before hardware acceptance, date TBD | Reproducible desktop authentication still needs proof; iPhone key export no longer blocks setup |
| D05 | Supported machine and versions | Confirmed target / Open verification | This PC: Windows 11 Pro x64 build 26200, OBS 32.2.2. Latest stable Gen 2 firmware required; exact build to be recorded. Wi-Fi enumerated; Bluetooth controller reports LE unsupported and Classic supported. Do not request adapter model again | Randy / technical verification | Target confirmed 2026-10-15; hardware validation before connection proof, date TBD | No identified glasses Classic endpoint or authenticated session; existing-adapter direct/current-firmware compatibility unproved |
| D06 | Source reuse and license | Confirmed | Visor-derived AGPL-3.0 project source with attribution | Randy | 2026-10-15 | Public distribution review remains deferred until requested |
| D07 | Scope and performance | Confirmed targets / Open verification | 120-minute minimum; adjustable video; optional glasses audio off by default; 720×1280 nominal 30fps and lower profile; p95 ≤500ms, startup ≤15s, reconnect ≤30s | Randy / technical verification | Confirmed 2026-10-15; verify at media/OBS milestones, date TBD | Actual hardware gates remain open |
| D08 | UI and branding | Confirmed | OBS-native English controls; existing themes; keyboard/Narrator; 480 logical-pixel dialog width and 100–200% scaling; no custom assets | Randy / technical verification | Confirmed 2026-10-15; verify at OBS milestone, date TBD | UI implementation and visual tests remain open |
| D09 | Privacy and local identity | Confirmed | Protected user-scoped local identity; no telemetry/update checks/media retention; memory-only ordinary diagnostics, explicit redacted export; Forget deletes plugin identity; re-provision if identity is lost | Randy / technical implementation | 2026-10-15 | Implementation/security proof remains open |
| D10 | Distribution and trust | Confirmed | Unsigned local ZIP, manual installs/updates, no public release; checksums, exact-source traceability, versioned rollback, no automatic OBS restart | Randy | 2026-10-15 | Public-release review deferred; current artifact proof remains required |
| D11 | Ownership and delivery constraints | Confirmed | Randy owns personal release/privacy/security/support/retirement decisions; no SLA/deadline/new spending; local Git, CI-compatible gates, no remote | Randy | 2026-10-15 | Hosted CI execution and public scope remain deferred |
| D12 | Hosted service infrastructure | N/A | No hosted product service in the proposed receiving path; hosted tenancy, database migrations, cloud on-call, browsers, SEO, and store signing do not apply | Randy to confirm scope | Before brief confirmation; calendar date TBD | Revisit if scope changes |
| D13 | Local identity source | Confirmed source / Optional research | Meta AI app on iPhone. Local import research allowed, but no verified export route found. Meta authentication is now permitted instead | Technical investigation | Source and revised constraint confirmed 2026-10-15 | Does not block project setup; must not be presented as a working import feature |
| D14 | Existing-adapter constraint | Confirmed constraint / Open verification | Use the available adapter. Wi-Fi is enumerated; controller supports Classic but reports no LE. No replacement-controller requirement or spending. Investigate uncached SDP and a trustworthy private-address direct RFCOMM probe; do not guess identities or require firmware downgrade | Randy / technical verification | Before direct media hardware test; calendar date TBD | Glasses Classic address, direct application authentication and Wi-Fi/media operation remain unproved |

## Readiness confirmation

- [x] Workspace scanned successfully; no existing implementation or assets found.
- [x] Research sources inspected; Gen 2 correction recorded.
- [x] Audience, scope, non-goals, success measures, and supported environments confirmed.
- [x] Authentication policy, secret handling, upstream reuse, and licensing choices resolved; real authentication remains a technical proof obligation.
- [x] Brand, accessibility, responsive, and localization targets confirmed.
- [x] Distribution, signing, release, privacy, operations, and lifecycle decisions confirmed or explicitly deferred with appropriate boundaries.
- [x] Foundation stack/dependency compatibility verified by native build and isolated runtime loading; later transport/media dependencies still require validation.
- [x] Owner reviewed and confirmed the completed brief.

**Confirmation:** Randy accepted the complete scaffolding contract on 2026-10-15, including proposed quality/latency/reconnection targets, accessibility defaults, ownership, privacy, distribution, and the 120-minute minimum streaming requirement. This confirms requirements, not implemented or tested functionality. Future firmware compatibility and real hardware acceptance remain verification work.

The remaining rows marked assumed/proposed above describe defaults explicitly accepted in this confirmation. Deferred public-release decisions do not block the personal prototype. Technical unknowns are tracked as verification obligations in `PLAN.md`; they must not be reinterpreted as product approval gaps.
