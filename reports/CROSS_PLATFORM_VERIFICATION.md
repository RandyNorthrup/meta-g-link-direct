# Cross-platform verification

What was run on 2026-11-01 for version 0.5.0, on which machine, and what came out. [`CERTIFICATION.md`](CERTIFICATION.md) summarises this per platform and lists the gaps. One pair of Ray-Ban Meta Gen 2 glasses was used throughout; their Bluetooth device information reports firmware 20.3.6.

## Machines

| Machine | System | OBS | Radios | Role |
|---|---|---|---|---|
| Windows PC | Windows 11 Pro, build 26200 | 33.0.1 | USB Bluetooth and USB Wi-Fi adapters, Ethernet uplink | Glasses and OBS |
| Linux laptop | Ubuntu 26.04.1, kernel 7.0, KDE Plasma on Wayland | 33.0.0 (OBS project archive) and 33.0.1 (Flatpak) | Built-in Bluetooth and Wi-Fi, Ethernet | Glasses and OBS |
| Mac mini 2018 | macOS 15.7.4, Intel, System Integrity Protection on | 33.0.1 (Intel) | Built-in Bluetooth and Wi-Fi, Ethernet | Glasses and OBS |
| MacBook Pro 2018 | macOS 15.7.9, Intel, System Integrity Protection off, AMFI boot argument set | 33.0.1 | | Builds only: with that boot argument macOS never shows a permission question for an app that isn't Apple's (`Policy disallows prompt`), so it can't test the helper |
| WSL on the Windows PC | Arch Linux, GCC 16, Clang 23, FFmpeg 9 (libavcodec 63), Qt 6.12 | 33.0.1 | None | Build against the newest libraries; no radios |

## Results on the glasses

Each row is the installed plugin in OBS, driven through obs-websocket: the scene goes live, the quality changes, the glasses' microphones go on, and the scene leaves. Frames were checked by eye and then deleted. "Video after" is from the scene going live to the first frame.

| Platform | OBS | Video after | Frame rate | Quality change to first frame | Glasses audio peak | Battery outputs | Afterwards |
|---|---|---|---|---|---|---|---|
| Windows 0.5.0 | 33.0.1 | 11.8 s | 30 fps | changed to 504 × 896 and back; time not read | not repeated in this run (see [the audio report](AUDIO_AND_OBS33_VERIFICATION.md)) | request and log | glasses disconnected |
| Ubuntu native, tree before the version change | 33.0.0 | 15.5 s, 15.7 s | 30 fps | 1.2 s | −37.2 dBFS (quiet room) | request and log | back on the home network, 0 profiles left |
| Ubuntu Flatpak, tree before the version change | 33.0.1 | 15.7 s | 30 fps | 1.1 s | −9.5 dBFS | request and log | back on the home network, 0 profiles left |
| macOS, tree before the version change | 33.0.1 | 14.6–23.2 s, two at 31–36 s | 29.9 fps from the glasses, 29.9 shown, 0 undecodable, 18 late over 60 s | 2.5–5.4 s | −11.3 dBFS | file, request and log | back on the home network in about 1 s |
| macOS 0.5.0 installed bundle | 33.0.1 | 15.8 s, 31.0 s | | | | | back on the home network, 1 keychain item, 0 known networks |

A three-minute run on macOS showed OBS at 30 fps with 0 render frames skipped, OBS using about 40% of one core, and memory flat at about 600 MB (601 to 605 MB over 2 minutes).

Linux Bluetooth: the handshake reached `keys_derived`, the change of owner returned `ok`, and `--link` reached `ready` with trust status 4096, the same as Windows. The glasses' owner key was sealed in the desktop's Secret Service. The Flatpak's change of owner sealed it in its own `default.keyring` (mode 600).

### Battery

From the glasses' own reports, streaming 720 × 1280: 62% to 45% in 3 min 33 s on macOS (with quality and audio changes), and 33% to 25% in about 2 minutes on Linux. That is 4–5% a minute, so about 20–25 minutes from a full charge, shorter than the earlier unconfirmed 35–40 minutes. No full-charge run has been made.

## macOS Wi-Fi: what was tried and why

The glasses' network is joined by Meta G Link Helper, not by OBS.

- **`networksetup -setairportnetwork` is not usable.** It joined the network, then blocked: it saves the passphrase in the System keychain, and its own read of that item waits on a `SecurityAgent` password dialog. A sample of the stuck process showed `_CWSetWiFiKeychainItem_10_9` → `SecItemCopyMatching`. The media session waited behind it for 18 minutes, and OBS would have hung on stop.
- **CoreWLAN without Location is not enough.** `scanForNetworksWithName:` finds the network without the Location permission, but with `ssid` and `ssidData` nil, and `associateToNetwork:` then fails every time with `-3900`.
- **A scan reports "busy" for about 10 seconds after another scan** (measured: 20 tries at 0.5 s intervals, three rounds). While busy it returns an older, unfiltered list, so the helper checks each result's name. A scan made just after the glasses report their network can miss it, which accounts for the 31–36 s starts: the first scan took 4.2 s and missed the network, the next was held until 15 s and took 8.9 s, and the association itself took 0.5 s.
- **The helper app holds Location, like Bluetooth.** With Location allowed, the scan returns the name and the association works. The passphrase goes to the helper over its private socket, not on a command line. If the caller dies, the socket closes and the helper still leaves the network.
- **macOS remembers every network it joins.** After 13 sessions with random names, 13 entries sat in the known networks and in the System keychain. A plain user can take a network off the known networks (`-removepreferredwirelessnetwork`, no prompt) but can't delete the keychain item (`CWKeychainDeleteWiFiPassword` returns `-61`, no prompt). So on macOS the network keeps one name per computer (`wifi-network-name` in the storage folder), the passphrase stays random per session, and the helper removes the known-network entry when it leaves. After later sessions: 0 known networks, 1 keychain item. That item holds the passphrase of a finished session.
- **Disassociating from a home network sets its `TemporarilyDisabled` flag**, and auto-join then skips it. A test tool caused this on the home network; the plugin only disassociates from the glasses' network, and auto-join returned to the home network in about 1 second in every later session.

### The failed runs before charging

Between 15:58 and 16:09, when the battery was low (level not recorded), the plugin joined the network but the glasses mostly didn't answer: TCP SYNs to port 20203 went unanswered (`error 60`), `ping` and `nc` failed as well, and two attempts stopped at stages 2 and 3. The Wi-Fi driver reported a strong signal (RSSI −40), AWDL off and almost no Bluetooth airtime, and no other scans ran in those windows. After charging, 16 of the next 17 starts reached video on the first try. Fast and slow joins both worked, which refutes a timing theory proposed during the test. The cause is **not proven**; a low battery is the leading suspect. The one later failure was macOS refusing a join (`-3906`), then going back to the home network; the plugin recovered on its next attempt. The helper now scans again after a refused join; that exact failure hasn't recurred since, so the change is unproven.

### Permissions after an update

Ad-hoc signed builds are a new app to macOS each time. An old Bluetooth answer makes macOS ask again with no window in front, so the question stays hidden and the helper waits. The installer now resets this app's Bluetooth answer first (`tccutil reset BluetoothAlways`), and the helper then asks in the open. Verified on the Mac mini: without the reset the question stayed hidden for over 4 minutes; with it, the question appeared and was answered in 20 s.

A keychain item likewise doesn't trust a rebuilt program. OBS.app is on the item's list and read it with no prompt in every run. A rebuilt `meta-g-link-probe` asks for the keychain password on each read until someone chooses Always Allow, which lasts for that build only.

## Linux: where the identity lives

Inside a Flatpak, when the desktop offers the Secret portal, libsecret keeps secrets in the app's own file, encrypted with a key from that portal, and `SECRET_BACKEND=service` doesn't change that (checked with a small test program built in the Flatpak's SDK). So the Flatpak has its own identity, and setting up one of native OBS and the Flatpak takes the glasses from the other. The installer's `org.freedesktop.secrets` permission serves desktops without the portal.

In the test session the keyring's prompt window couldn't appear, because the session's activation environment held no display; this was fixed for that session with `dbus-update-activation-environment`. A locked keyring after an automatic login is normal and needs the user's password.

## Setup windows

The GTK window (Linux) was opened and captured: heading, explanation, *Continue* and *Cancel* in place. The AppKit window (macOS) was opened and read from the system's window list: 640 × 788, centred on a 1280 × 1024 display. Neither was walked through a full login on these platforms; the change of owner they call was run through `--provision`, with the code from a real Meta login piped in. A plain `screencapture` without the Screen Recording permission omits other apps' windows, so it can't show this.

## Builds and packages

| Target | Build and tests | Package |
|---|---|---|
| Windows | MSVC, `/W4 /WX`; 7 of 7 CTest suites; Pester 5, 21 and 4 | `package` gate; see [`CERTIFICATION.md`](CERTIFICATION.md) |
| Ubuntu 26.04, native | GCC, warnings as errors; 6 of 6 CTest suites; 0 mbedTLS symbols exported | `linux-x86_64` bundle: checksum, `--dry-run`, install and `--uninstall` run; Flatpak permissions added and taken back |
| Flatpak SDK 26.08 | GCC 16, warnings as errors; 6 of 6 CTest suites; 0 mbedTLS symbols exported; links the runtime's FFmpeg 62 and OBS | Same bundle |
| macOS, Intel | Xcode 26.3; 6 of 6 CTest suites | `macos-x86_64` bundle: checksum and install run on the Mac mini |

Apple Silicon has a CI job only, against the official OBS Apple build; no glasses have been tried on it.

The red drills `buffered-partial-read` and `login-state` were run: green, intended failure, exact restoration, green.
