# Shared link, microphone source, keep-connected and setup window (0.3.0)

Observed on 2026-10-19 on the development PC (Windows 11 Pro x64 build 26200, the same glasses and adapters as [the Wi-Fi link report](WIFI_LINK_VERIFICATION.md)). The OBS runs used OBS 33.0.0-rc2: a portable copy started beside the user's OBS with `--multi` (13:01–13:05) and the installed OBS with the user's profile (14:22–14:27, then 15:12–15:16 with the release build). The first two runs used the 0.3.0 code before its version number was raised, so the plugin logged "0.2.0 loaded; camera and microphone sources registered". Timings come from the OBS WebSocket request time and the plugin's log lines.

## Stream changes inside one session

A protocol harness over the production `MediaSession`, with temporary instrumentation that was removed afterwards, changed the stream five times in one media session. Each change was ControlStream STOP, the new ConfigureVideo and ConfigureAudio requests, then ControlStream START; every request was answered.

| Step | Request | About 6 s of stream after the start |
|---|---|---|
| A | 720 × 1280, no audio | 170 frames, first 353 ms after the start, 16.7 kB per frame, 0 audio units |
| B | 504 × 896 | 169 frames, first after 305 ms, 6.3 kB per frame |
| C | 504 × 896 with audio | 177 frames, first after 307 ms, 288 audio units |
| D | video disabled (`enabled` 0), audio on | 0 frames, 369 audio units |
| E | 720 × 1280, audio off | 170 frames, first after 305 ms |

The session then streamed on and stopped cleanly (1,805 frames in total, no reconnect). In an earlier run that requested audio without configuring video, the glasses streamed audio alone (347 units in 20 s); the session of that time closed it because no video came. The demand-driven session expects video only when a live source wants it.

While only audio streamed, Randy looked at the glasses: the camera's privacy light was **off**. The glasses power the camera down in this mode.

## Battery report

The glasses push svc28 device-status messages (types 2005–2044). Their DeviceState field 1 is the battery: charge state, percentage, temperature in thousandths of a degree Celsius and voltage in millivolts. The first decoded report read "Glasses battery: 70%, not charging (3.90 V, 31.2 C)" while the type 2 SystemHealthEvent level read 71, which identifies that level as the battery percentage.

- On a fresh charge, the type 2 level held at 71 for the first minute of audio-only streaming.
- In the installed-OBS run the battery went from 57% (14:24:08) to 49% (14:27:28) across a cold start, video, connected idle, microphone-only streaming and an open setup window. The modes were mixed, so this gives no drain rate for any one of them.
- Not measured: drain per mode, and a full charge.

## OBS 33.0.0-rc2, portable

| Action | Result |
|---|---|
| Camera scene live, keep-connected off | Video 19.1 s after the scene switch (authentication alone took 9.3 s in this run) |
| Keep-connected on, scene switched away | "Connected to the glasses …, camera off" 0.8 s later |
| Camera scene live again | Video 0.94 s after the switch |
| Quality 720 × 1280 → 504 × 896 | "receiving video 504x896" 1.4 s after the request |
| Glasses audio on | "receiving audio 48000 Hz, 2 channel(s)" 1.0 s after the request |
| Quality back to 720 × 1280 | "receiving video 720x1280" 1.4 s after the request |
| Microphone-only scene live | Audio alone 1.5 s after the switch; OBS's `InputVolumeMeters` reported the microphone source with a peak of −12.9 dBFS over 10 s; no video requested |

None of these changes reconnected.

## OBS 33.0.0-rc2, installed, with the user's profile

- OBS loaded the plugin from the ProgramData flat layout and skipped the `bin\64bit` copy.
- A cold start: authenticated after 2.5 s, the glasses' network up after 4.9 s, joined after 9.5 s, media session ready after 10.6 s, video after 12.9 s.
- With keep-connected on, the link reached "Connected to the glasses after 14.8 s, camera off". A microphone source going live then streamed audio 0.5 s after the scene switch, with the mixer peaking at −26.7 dBFS and the camera off.
- Turning keep-connected on in one source copied it to all five glasses sources in the profile and to the plugin's `link.json`; turning it off disconnected the glasses ("No glasses source needs the glasses now; disconnecting.").
- The properties showed the Video, Audio, Glasses connection and Glasses setup groups ([screenshot](../assets/obs-properties.png)) and the battery line ("Glasses battery: 57%, not charging (3.73 V, 30.2 C)" in the log).
- *Set up glasses…* opened the setup window; the link paused ("The glasses setup window is open. The glasses link waits until it closes."); Cancel closed it with exit code 8 and the link reconnected.
- Randy confirmed that *Tools → Ray-Ban Meta Glasses (Direct)* opens the properties.
- Defect found and fixed: a start from the connected, camera-off state logged "Video is flowing after 17.2 s", the time since the connection began, instead of its own time. Such a start now reports the time since it began (verified with the release build, below).

## Release build, installed from the zip

- `package` passed (every gate, 126 protocol cases, 31 Pester cases, 22 red drills) and built `meta-g-link-direct-0.3.0-windows-x64.zip`; the extracted zip matched its `.sha256` file.
- OBS was closed and `Install.cmd` from the extracted zip ran in its own console: it found OBS 33 in Program Files, installed both ProgramData layouts and both setup programs without an elevation prompt, and on **n** finished without changing the glasses' owner ([screenshot](../assets/installer.png), from a second run with `-ObsPath` because this PC also has a per-user OBS 32, which the first run listed). OBS 33.0.0-rc2 then logged "0.3.0 loaded; camera and microphone sources registered" and skipped the `bin\64bit` copy.
- With keep-connected on, the glasses connected in 13.5 s and waited with the camera off (battery 98%). A camera scene going live then logged "Video is flowing 1.3 s after the change"; the first frame came 1.4 s after the scene switch. Keep-connected off disconnected the glasses, and the user's program scene was restored.
- A first attempt found the glasses asleep: the Bluetooth connection was refused after about 46 s, the link scheduled a retry, and because nothing needed the glasses any more by then it went idle without retrying.

## Setup window

- The WebView2 SDK 1.0.2903.40 (NuGet, SHA-256 pinned) builds the window with its static loader; the window needs only the WebView2 Runtime that Windows 11 includes.
- The window rendered with standard controls ([screenshot](../assets/setup-window.png)). After Continue, Meta's login loaded inside the window. Randy logged in; the window recognised the final redirect, took the code from it, changed the owner and showed Done. `meta-g-link-probe.exe --link` then reported `{"stage":"ready","trust_tx_ok":true,"trust_rx_ok":true,"ready":true}` with the new owner key.
- What led to the window: in the console flow, a typed confirmation was rejected for its letter case, the console went behind the browser when the login opened, and one pasted login code had already been used, so Meta answered HTTP 500. Randy judged the flow too complicated.
- Not tested: the browser fallback without WebView2, and the keyboard, Narrator, high-contrast and scaling checks.

## Trust signature

After the AirShield handshake the glasses push an svc36 message (type 4096) with a 32-byte field 1 (the size of a SHA-256 digest), a 512-byte field 2 (the size of an RSA-4096 signature) and field 3 set to 1. Their device certificate carries an RSA public key. A check written for a P-256 r||s signature therefore could not pass and was removed before release. The Meta app performs this check in native code; which key and which bytes it verifies is not known yet, so the gap stays open.
