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

A three-minute run on macOS showed OBS at 30 fps with 0 render frames skipped in its last stretch, OBS using about 40% of one core, and memory flat at about 600 MB (601 to 605 MB over 2 minutes). That run also held two reconnects, found later in the log (see [When the glasses stop answering](#when-the-glasses-stop-answering)); the later run on the released bundle had none.

Linux Bluetooth: the handshake reached `keys_derived`, the change of owner returned `ok`, and `--link` reached `ready` with trust status 4096, the same as Windows. The glasses' owner key was sealed in the desktop's Secret Service. The Flatpak's change of owner sealed it in its own `default.keyring` (mode 600).

### Battery

From the glasses' own reports, streaming 720 × 1280: 62% to 45% in 3 min 33 s on macOS (with quality and audio changes), and 33% to 25% in about 2 minutes on Linux. A later run with a fuller battery fell from 88% to 77% in 3 min 23 s. That is 3–5% a minute, so about 20–30 minutes from a full charge, shorter than the earlier unconfirmed 35–40 minutes. No full-charge run has been made.

### The microphone source

A throwaway scene held only the existing microphone source, so the camera stayed off: no video unit arrived in any run, and the camera source stayed inactive. The glasses delivered 48 kHz stereo AAC.

| Platform | OBS | Audio after | Length | Drops | Peak level |
|---|---|---|---|---|---|
| Windows | 33.0.1 | 13.5, 13.2 and 11.6 s | 2.7, 6.6 and 7.5 min | 1, recovered in 28 s (first run) | −16.2 to −54.7 dBFS |
| Linux, native | 33.0.0 | 14.7 s | 2.9 min | 0 | −57.0 dBFS (nobody speaking) |
| Linux, Flatpak | 33.0.1 | 15.6 s, then 14.7 s after the drop | 2.9 min | 1, recovered in 16 s | −39.0 dBFS |
| macOS, Intel | 33.0.1 | 14.3 s after an answer on the keychain | 3.5 min | 0 | −8.4 dBFS |

**Battery with only the microphones:** about 1.5% a minute on Windows (69% to 60% in 6 min 10 s; 37% to 28% in 6 min), about a third of video, so more than an hour from a full charge. It read faster at low charge on other machines: 58% to 55% in 72 s on Linux and 26% to 19% in 2 min 31 s on macOS. No full-charge run has been made.

### The released files

After publishing, the release's own files were downloaded, checked against their SHA-256 files, installed with their installers, and run with a charged battery (each owner change used a real Meta login with its code piped in).

| Platform | OBS | Video after | Frame rate | Quality change | Audio peak | Battery | Afterwards |
|---|---|---|---|---|---|---|---|
| Windows zip | 33.0.1 | 16.5 s, 16.3 s | 30 fps | 1.2 s | −45.4 dBFS (quiet room) | request and log | glasses disconnected |
| Linux bundle, native | 33.0.0 | 17.2 s | 30 fps | 1.2 s | −40.0 dBFS (quiet room) | request and log | home network, 0 profiles left |
| Linux bundle, Flatpak | 33.0.1 | 15.9 s | 30 fps, 0 render frames skipped of 2,738 | 1.1 s | −31.8 dBFS | request and log | home network, 0 profiles left |
| macOS bundle, Intel | 33.0.1 | 30.1, 14.7, 15.5, 36.2 and 16.0 s | 30.0 fps from the glasses, 30.1 shown, 1 late, 0 undecodable over 60 s; 1 render frame skipped of 8,547 | 1.2–1.3 s | −30.8 dBFS | file, request and log | home network, 1 keychain item, 0 known networks |

On macOS the battery fell from 88% to 77% in 3 min 23 s of video, with OBS at about 40% of one core and memory flat at about 550 MB.

The setup window opened from the Flatpak too (640 × 760 in the X11 window list). A full login through the Linux and macOS setup windows still hasn't been done; the owner changes went through `--provision`.

## macOS Wi-Fi: what was tried and why

The glasses' network is joined by Meta G Link Helper, not by OBS.

- **`networksetup -setairportnetwork` is not usable.** It joined the network, then blocked: it saves the passphrase in the System keychain, and its own read of that item waits on a `SecurityAgent` password dialog. A sample of the stuck process showed `_CWSetWiFiKeychainItem_10_9` → `SecItemCopyMatching`. The media session waited behind it for 18 minutes, and OBS would have hung on stop.
- **CoreWLAN without Location is not enough.** `scanForNetworksWithName:` finds the network without the Location permission, but with `ssid` and `ssidData` nil, and `associateToNetwork:` then fails every time with `-3900`.
- **A scan reports "busy" for about 10 seconds after another scan** (measured: 20 tries at 0.5 s intervals, three rounds). While busy it returns an older, unfiltered list, so the helper checks each result's name. A scan made just after the glasses report their network can miss it, which accounts for the 31–36 s starts: the first scan took 4.2 s and missed the network, the next was held until 15 s and took 8.9 s, and the association itself took 0.5 s.
- **The helper app holds Location, like Bluetooth.** With Location allowed, the scan returns the name and the association works. The passphrase goes to the helper over its private socket, not on a command line. If the caller dies, the socket closes and the helper still leaves the network.
- **macOS remembers every network it joins.** After 13 sessions with random names, 13 entries sat in the known networks and in the System keychain. A plain user can take a network off the known networks (`-removepreferredwirelessnetwork`, no prompt) but can't delete the keychain item (`CWKeychainDeleteWiFiPassword` returns `-61`, no prompt). So on macOS the network keeps one name per computer (`wifi-network-name` in the storage folder), the passphrase stays random per session, and the helper removes the known-network entry when it leaves. After later sessions: 0 known networks, 1 keychain item. That item holds the passphrase of a finished session.
- **Disassociating from a home network sets its `TemporarilyDisabled` flag**, and auto-join then skips it. A test tool caused this on the home network; the plugin only disassociates from the glasses' network, and auto-join returned to the home network in about 1 second in every later session.

### When the glasses stop answering

Two kinds of failure showed up, and neither is explained yet.

**Minutes when the glasses barely answer.** Twice on the Mac, at 15:58–16:09 and 21:16–21:20, the plugin joined the network but the glasses mostly didn't respond: TCP SYNs to port 20203 went unanswered (`error 60`, sometimes 64 or 65), `ping` and `nc` failed as well, and some attempts stopped part-way through the handshake. In the first cluster the Wi-Fi driver reported a strong signal (RSSI −40), AWDL off and almost no Bluetooth airtime, and no other scans ran. Each cluster cleared by itself. These theories were checked and don't hold:

- *Join timing*: fast and slow joins both worked and both failed.
- *The glasses' channel*: the glasses choose channel 1, 6 or 11 though the plugin asks for 6; channels 1 and 11 each show both outcomes.
- *Low charge*: the first cluster cleared after charging, but the second began at about 45%, while Windows streamed normally at 17% and 28%.

Between the clusters, 16 of 17 starts reached media first try. The one exception was macOS refusing a join (`-3906`) and going back to the home network; the plugin recovered on its next attempt. The helper now scans again after a refused join; that failure hasn't recurred since, so the change is unproven.

**Sessions ended by "The glasses stopped answering over Wi-Fi."** This ended sessions on Windows (2 min 8 s into a microphone run), on the Flatpak (79 s into one) and on the Mac (twice in video runs at about 17:07–17:09, and 21 s into a microphone attempt during the second cluster). Each recovered by itself in 16–28 s. Until 0.5.1, the session ended whenever the glasses' keepalive answers stopped for 8 s, even if media was still arriving. Whether media was still arriving in those runs wasn't recorded. Since 0.5.1 a streaming session goes by its media and logs a pause in the answers, so the next one shows which it was. Runs since then, of 7.5 minutes on Windows and 3.5 on the Mac, had no pause and no drop, so the fix has not yet been seen working.

### Permissions after an update

Ad-hoc signed builds are a new app to macOS each time. An old Bluetooth answer makes macOS ask again with no window in front, so the question stays hidden and the helper waits. The installer now resets this app's Bluetooth answer first (`tccutil reset BluetoothAlways`), and the helper then asks in the open. Verified on the Mac mini: without the reset the question stayed hidden for over 4 minutes; with it, the question appeared and was answered in 20 s.

A keychain item likewise doesn't trust another program, even one on its access list. The tool that creates the identity lists OBS.app on the item, yet after each owner change macOS asked once for OBS (`displaying keychain prompt for /Applications/OBS.app`), and the media session waited until someone answered. This was first read as "OBS never prompts", because the prompt was answered at the Mac while tests checked for it before or after. Always Allow settles it until the next setup. A rebuilt `meta-g-link-probe` asks in the same way, once per build.

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

The `platforms.yml` workflow passed on all four jobs: Ubuntu 26.04 native, the newest OBS Flatpak, Intel macOS and Apple Silicon macOS. The Apple Silicon job built arm64 binaries against the official OBS 33.0.1 Apple build and passed 6 of 6 suites; its bundle is the one released. No glasses have been tried on Apple Silicon. On Arch Linux with OBS 33.0.1, FFmpeg 9 and GCC 16, the build and 6 of 6 suites also passed, with no radios to go further.

The red drills `buffered-partial-read` and `login-state` were run: green, intended failure, exact restoration, green.
