# Third-party notices

Meta G Link Direct is licensed under the GNU Affero General Public License version 3. See [`LICENSE`](LICENSE).

## Visor

- Upstream: https://github.com/macedonga/visor
- Revision: `e7077b4051b1e3e605c667aa2a010fbe41f346a0`
- License: AGPL-3.0. Upstream license and contributor attribution are preserved; authors are the Visor contributors in the pinned repository history.
- Ported in behaviour to independent C++20 (interfaces, error handling and tests are this project's own):
  - `core/transport/.../Sdu.kt` → `src/protocol/sdu.cpp` (frame-prefix inspection).
  - `core/protocol/Wire.kt` → `src/protocol/protobuf.cpp` (strict protobuf encode and parse).
  - `core/protocol/Header.kt`, `core/session/DataXConnection.kt`, `core/session/ChannelRegistry.kt` → `src/protocol/datax.cpp` (DataX header, fragmenting frame splitter, channel registry).
  - `core/protocol/Kdf.kt`, `Cipher.kt`, `Messages.kt`, `Binding.kt`, `core/identity/Trust.kt` → `src/protocol/airshield.cpp` (AirShield key schedule, per-frame AES-CBC + HMAC, handshake messages, channel binding, trust payload).
  - `core/session/SecureLink.kt`, `core/session/GlassesSession.kt` → `src/windows/session.cpp` (handshake, trust, EndLinkSetup, DataX receive loop).
  - `app/connection/AndroidTransportFactory.kt` and `core/transport` → `src/windows/rfcomm_transport.cpp` (RFCOMM whole-SDU transport over WinRT service resolution and Winsock).
  - `core/identity/token/WebLogin.kt`, `Frla.kt`, `core/identity/provision/Provision.kt`, `PyJson.kt` → `src/windows/provision.cpp`, `src/protocol/json.cpp` (Meta login, svc36 change of owner, Python-compatible JSON for ownership receipts).
  - `core/media/StreamProto.kt`, `LiveStream.kt`, `Canary.kt` → `src/protocol/stream.cpp`, `src/protocol/hevc.cpp`, `src/windows/media_session.cpp` (stream configuration and control messages including the audio configuration, HEVC and AAC access-unit reassembly with the presentation-time prefix, codec-configuration routing, stream bring-up order, svc100 keepalive). The fallback AudioSpecificConfig is this project's own: on Gen 2 the glasses sent no audio configuration.
  - `app/setup/NearbyGlasses.kt` and `core/transport/BluetoothRfcommTransport.kt` → `src/windows/discovery.cpp`, `src/windows/classic.cpp` (the Meta manufacturer and service markers and the RFCOMM service UUID used by the diagnostics).
- Protocol facts (service ids, message types, framing, channel binding) also follow the Visor GitHub wiki, read 2026-10-15 to 2026-10-18.
- Not ported: Visor's Wi-Fi group (`Wifi.kt`). This project uses a glasses-hosted network instead (below).

## Meta AI app protocol definitions

The glasses-hosted network messages (svc10 `CreateSoftApNetwork` 0x1000, `SoftApNetworkInfo` 0x1001, `DestroySoftApNetwork` 0x1002 and their field numbers and enums) were read from the protocol classes (`com.oculus.atc`) of the Meta AI Android app, decompiled locally for interoperability. No Meta code, binaries or assets are included; `src/protocol/stream.cpp` implements the messages independently.

The setup tool contains the Meta AI app's two client tokens (see `AGENTS.md` revision 3). They identify the Meta AI app and ship in every copy of it; they were already public, for example in [callbacked/kinesis](https://github.com/callbacked/kinesis).

## Gen 2 channel-7 research

[MoTechnocious/rayban-meta-linux, channel-7 findings](https://github.com/MoTechnocious/rayban-meta-linux/blob/823225a7c3bd9917f9a64ce88b32a6893d1df591/research/channel7-findings.md) at revision `823225a7c3bd9917f9a64ce88b32a6893d1df591` reports a 116-byte hello on RFCOMM channel 7 from an unpaired Gen 2 host. The `--direct-classic-channel7` diagnostic compares the documented twelve-byte header as a candidate marker. No upstream scripts, binaries, credentials or captures are copied or executed.

## OBS Studio

- Upstream: https://github.com/obsproject/obs-studio, version `32.2.2`, revision `ba2f32bdf791005443988a4955e963663e16b1ed`.
- The plugin builds against OBS's public module and frontend API headers and import libraries (`libobs` and `obs-frontend-api`, GPL-2.0-or-later in OBS's source notices) and follows the documented module API. No private ABI declaration is copied.
- The battery request and event use obs-websocket's header-only plugin API, `plugins/obs-websocket/lib/obs-websocket-api.h` from the same pinned OBS sources (obs-websocket, copyright Stephane Lepin and Kyle Manning, GPL-2.0-or-later). It is included from `.deps/` at build time, not copied into this repository; its inline functions are compiled into the plugin.
- Source and dependency archives are downloaded to the ignored `.deps/` and verified against pinned hashes. Their licenses stay with the original packages. Release zips do not redistribute them.

OBS Studio 33.0.0-rc2 (the official Windows zip, SHA-256 `33d5fab2e23ffbcd0e32a4a33ee11c26ebe7b99276a2e550445b27feb75a4127`) was used only as a test runtime for loading the plugin. Nothing from it is copied into this repository or the release zip.

## FFmpeg

The plugin decodes video and audio with the FFmpeg libraries `avcodec` and `avutil` that ship with OBS Studio (LGPL-2.1-or-later as built by OBS). It links their import libraries from the OBS dependency package and loads the DLLs from the user's OBS installation. Release zips do not include FFmpeg.

## Microsoft Windows SDK and C++/WinRT

Windows APIs (WinRT Bluetooth, Native Wi-Fi, IP Helper, CNG, DPAPI, WinHTTP, Winsock, Common Controls) are used through the Windows SDK headers and system libraries. Nothing from the SDK is redistributed.

## Microsoft Edge WebView2 SDK

- Package: `Microsoft.Web.WebView2` version `1.0.2903.40` from NuGet, pinned with its SHA-256 in `dependencies.lock.json` and downloaded by `bootstrap` into the ignored `.deps/`.
- The setup window (`meta-g-link-setup.exe`) is built with the SDK's headers and links its static loader (`WebView2LoaderStatic.lib`), so the release contains the loader inside that program and no separate WebView2 file. The WebView2 Runtime itself is part of Windows and is not redistributed. The package's other tools, which its `NOTICE.txt` covers, are not used in the release.
- License of the SDK:

```text
Copyright (C) Microsoft Corporation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

   * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
   * The name of Microsoft Corporation, or the names of its contributors
may not be used to endorse or promote products derived from this
software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

## Trademarks

No Meta, Ray-Ban, EssilorLuxottica, Apple or OBS logos are included. The README's screenshots and clip are this project's own (see `AGENTS.md` revision 7) and were cropped to leave out application logos. Product names identify compatibility only and imply no affiliation or endorsement.
