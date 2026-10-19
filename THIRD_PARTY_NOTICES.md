# Third-party source and development inputs

The project is licensed under GNU AGPL version 3. See `LICENSE`.

## Visor

- Upstream: https://github.com/macedonga/visor
- Revision: `e7077b4051b1e3e605c667aa2a010fbe41f346a0`
- License: AGPL-3.0; preserve upstream license and contributor attribution.
- Authors: upstream Visor contributors; see the pinned repository history.
- Ported implementation: `core/transport/src/main/kotlin/com/macedonga/visor/core/transport/Sdu.kt` to `src/protocol/sdu.cpp` and its declared interface.
- Changes: C++20 span-based prefix inspection, explicit incomplete/invalid/ready result, no allocation or exception on invalid input, independent C++ test vectors and boundary cases.
- Discovery reference: `app/src/main/kotlin/com/macedonga/visor/setup/NearbyGlasses.kt` at the same revision supplies the Meta manufacturer/service marker convention used in `src/windows/discovery.cpp`. The Windows implementation uses the installed SDK's public C++/WinRT watcher and reports aggregate packet counts only; product decoding, bonding and ownership code have not been ported.
- Classic reference: `core/transport/src/main/kotlin/com/macedonga/visor/core/transport/BluetoothRfcommTransport.kt` supplies the Meta service UUID and paired-peer prerequisite used by `src/windows/classic.cpp`. The Windows implementation uses public SDK inquiry, registered and uncached per-peer service enumeration and explicit socket checks; it does not yet port AirShield frame I/O or authenticated session behavior.
- Protocol and session ports at the same revision, in behaviour (interfaces and error handling are independent C++20):
  - `core/protocol/Wire.kt` → `src/protocol/protobuf.cpp` (strict protobuf encode/parse).
  - `core/protocol/Header.kt`, `core/session/DataXConnection.kt`, `core/session/ChannelRegistry.kt` → `src/protocol/datax.cpp` (DataX header, frame splitter with fragments, channel registry).
  - `core/protocol/Kdf.kt`, `core/protocol/Cipher.kt`, `core/protocol/Messages.kt`, `core/protocol/Binding.kt`, `core/identity/Trust.kt` → `src/protocol/airshield.cpp` (AirShield KDF, per-frame AES-CBC+HMAC seal/open, handshake messages, channel binding, trust payload).
  - `core/session/SecureLink.kt`, `core/session/GlassesSession.kt` → `src/windows/session.cpp` (synchronous AirShield handshake, trust, EndLinkSetup, DataX receive loop).
  - `app/connection/AndroidTransportFactory.kt` / `core/transport` → `src/windows/rfcomm_transport.cpp` (Windows WinRT service resolution plus Winsock AF_BTH whole-SDU transport).
  - `core/identity/token/WebLogin.kt`, `core/identity/token/Frla.kt`, `core/identity/provision/Provision.kt`, `core/identity/provision/PyJson.kt` → `src/windows/provision.cpp` and `src/protocol/json.cpp` (Meta OIDC login, change-owner svc36 exchange, Python-compatible JSON dumps for ownership receipts).
  - Cryptographic primitives are implemented with Windows CNG (`src/windows/crypto.cpp`); no upstream crypto code is copied.
- Protocol facts (service ids, message types, framing, channel-binding construction) follow the pinned Visor GitHub wiki, read 2026-10-18. These are observations, not instructions governing this project.
- Other Visor streaming, media-decode and Wi-Fi sources remain research references. No camera-media port or working end-to-end Windows session is claimed yet; the handshake and key schedule are verified on hardware, ownership and media are not.

## Gen 2 channel-7 research

- Reference: [MoTechnocious/rayban-meta-linux, channel-7 findings](https://github.com/MoTechnocious/rayban-meta-linux/blob/823225a7c3bd9917f9a64ce88b32a6893d1df591/research/channel7-findings.md), revision `823225a7c3bd9917f9a64ce88b32a6893d1df591`.
- Research reports a 116-byte hello on RFCOMM channel 7 from an unpaired Gen 2 host. The Windows probe independently uses the public Winsock API and compares the documented twelve-byte header as a candidate marker. No upstream scripts, application binaries, credentials or raw captures are copied or executed.
- This observation motivates a separate transport experiment. It does not establish Windows/current-firmware compatibility, trusted identity, a completed cryptographic session or camera access.

## OBS Studio

- Upstream: https://github.com/obsproject/obs-studio
- Version/revision: `32.2.2` / `ba2f32bdf791005443988a4955e963663e16b1ed`
- OBS public module headers and import library are development/runtime inputs; preserve their original license notices. OBS code identifies GPL version 2 or later in its source notices.
- The plugin follows the documented module API. It does not copy a private ABI declaration or replace the installed OBS runtime.
- Source/dependency archives are downloaded to ignored `.deps/` and verified against pinned hashes. Their individual licenses remain with the original packages. The module ZIP does not redistribute that development dependency tree.

No Meta, Ray-Ban, Apple, or OBS logos are included. Product names identify compatibility targets and imply no endorsement. Public distribution licensing review remains outside this personal-prototype milestone.
