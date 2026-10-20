# Latency, audio/video sync, reconnection, and two bugs they exposed (0.3.2)

Observed on 2026-10-20 on the development PC (Windows 11 Pro x64 build 26200, OBS 33.0.0-rc2 with Randy's profile at a 30 fps canvas, the same glasses and adapters as earlier reports), with the 0.3.1 release and then the 0.3.2 fixes. Randy wore the glasses for every run.

## Glass-to-OBS latency: about 0.4 s

- **Setup:** a browser page drew a counter in hundredths of a second on the monitor. A temporary OBS scene showed a window capture of that page on the left and the glasses' picture, unscaled, on the right. Randy looked at the monitor, and OBS recorded both.
- **Method:** in each recorded frame, the page's own value (from the window capture) was compared with the value the glasses saw on the monitor. The glasses' camera exposure often caught two refreshes at once; those are read as the midpoint.

| Window capture | Glasses' view | Difference |
|---|---|---|
| 81.80 | ~81.44 | 0.36 s |
| 84.30 | ~83.92 | 0.38 s |
| 86.84 | ~86.44 | 0.40 s |
| 89.34 | 88.89 | 0.45 s |
| 91.86 | ~91.44 | 0.42 s |

- **Result:** about 0.4 s, within the brief's 500 ms target in all five readings.
- **Where it comes from:**
  - The playout delay was 295 ms at the end of that run (from the plugin's log), so the glasses, the radio and decoding add roughly 0.1 s.
  - The window capture lags the page by a frame or so, which makes these figures slightly low rather than high.
- **Not done:** a 95th-percentile figure over a long run.

## Audio/video sync: within one frame

- **Setup:** with the glasses' microphones on, Randy clapped in front of the glasses. Each clap's sound onset (a 5–10 ms rise in the recording's audio) was compared with the frame where the hands met.
- **First clap:** sound at 1.450 s, contact at 1.433–1.467 s.
- **Second clap:** sound at 4.740 s, contact at 4.733–4.767 s.
- **Result:** the two are within one frame (about ±33 ms), so no Sync Offset is needed. Both media run on the playout clock, which presents them by the same capture times.

## Reconnecting

- **Bluetooth after power-on:**
  - After a power-on, Windows showed the glasses as disconnected: Classic and LE disconnected, audio endpoints unavailable.
  - Without anyone pressing Connect in Windows, the setup tool's `--link` (the plugin's Bluetooth path) reached a ready, trusted session in 2 s.
  - Windows then showed the Classic link and the glasses' audio endpoints as connected.
  - So the plugin connects the glasses itself; earlier in the day a connection attempt was refused while the glasses were asleep.
- **Wi-Fi drop:**
  - `netsh wlan disconnect` on the glasses' network in the middle of a stream does not make a clean drop: video kept arriving for about 19 s afterwards (966 frames in a run where about 384 came before the command).
  - When the media session did end, the plugin logged "The media link to the glasses dropped".
  - In OBS, video returned 21.5 s after that, through a full reconnect (Bluetooth, the glasses' network, the media session). That is within the brief's 30 s target.
  - A clean out-of-range drop is still to be tested.

## Bug 1: camera sources from 0.1.0 never recorded glasses audio

- **What happened:** the first measurement recording had a silent audio track although the plugin logged "receiving audio 48000 Hz, 2 channel(s)". The source "MGL Test Camera" had every audio track switched off. Two sources created recently had all six on.
- **Cause, in OBS 32.2.2:**
  - `obs_source_get_audio_mixers` returns 0 for a source whose type has no `OBS_SOURCE_AUDIO`, and OBS saves that as `"mixers": 0`.
  - `obs_load_source_type` restores the saved value.
  - The camera source had no audio in 0.1.0. So every camera source created then loads with no tracks: glasses audio moves the mixer meter but reaches no recording or stream. Earlier audio checks read OBS's meters, which show the source either way, so they missed it.
- **Fix:** the camera source now has a `load` callback, which OBS calls after it has restored the tracks and private settings. A camera source that has never been checked and has every track off gets OBS's default tracks 1–6, with a log line. A private setting marks it as checked, and new sources are marked at creation, so later choices are kept.
- **Verified:** OBS logged "Turned on the audio tracks of 'MGL Test Camera': it was saved by 0.1.0, before the source had audio, with every track off." The source then reported all six tracks on, and the other two sources were unchanged. The second recording carried the glasses' audio (peak −1.8 dBFS).
- **Not covered by tests:** this runs inside OBS's source loading, which has no automated test here.

## Bug 2: a frame ending inside a message header garbled the stream

- **What happened:** near the end of the second recording, the log filled with messages of nonsense services and types (for example "service 0 type 52430 (8843 bytes)"). The recording dropped from 30 to 22.4 distinct pictures per second, and then to 4.5. A message from the glasses between them (the battery report) still parsed, so the encrypted frames were intact.
- **Cause:**
  - The DataX decoder handled a message whose payload continues into the next encrypted frame, but not a frame that ends inside a message's header.
  - With one to three header bytes left at a frame's end, it dropped them; with the tags cut, it would close the link. Either way it then read the next frame from a wrong position until something happened to realign.
  - Visor's splitter at the revision this project ported made the same assumption. Visor's current `DataXConnection.splitMessages` already treats the frames' data as one stream, with a carry, wherever a frame cuts a message, header included.
- **Fix:**
  - The decoder strips each frame's pad once (AirShield always pads 1–16 bytes, as `FrameCrypto::seal` does), keeps the start of a header that a frame ends in, and joins it to the next frame.
  - It still fails a header whose tags are malformed when the whole message is present.
- **Tests:**
  - A new protocol test packs six messages (with and without tags, empty and long payloads) into frames split at every byte position, 637 sizes, and checks that each split decodes exactly like the unsplit stream.
  - Before the fix, 194 of the 637 sizes failed.
  - A new red drill (`datax-frame-boundary`) removes the carried header and must make that test fail.
- **Real glasses:** a 60 s stream check afterwards authenticated over Bluetooth, set up the network and received 1,753 frames with no stray messages. The bug's rate depends on where the glasses' frames end, so a long run without stray messages is the remaining proof.

## Proof videos

- **Without audio:** the README clip, from the 0.3.1 release recording.
- **With the glasses' microphones:** taken from the second recording, 0–28 s, before the garbled stretch: 720 × 1080 at 30 fps, 29.7–30 distinct pictures per second, claps included. It is kept outside the repository for Randy to review, because AGENTS.md revision 7 allows README footage only with the audio removed.
- The measurement recordings and the temporary OBS scene were deleted.
