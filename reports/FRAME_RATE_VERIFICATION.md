# Frame rate: 30 fps from the glasses, about 13 fps in OBS (fixed in 0.3.1)

Observed on 2026-10-20 on the development PC (Windows 11 Pro x64 build 26200, the same glasses and adapters as [the Wi-Fi link report](WIFI_LINK_VERIFICATION.md), OBS 33.0.0-rc2 with Randy's profile at a 30 fps canvas). Randy reported that every quality setting showed about 13–15 fps in OBS, although the plugin asks the glasses for 30.

## What the glasses send

A temporary build of the setup tool's 60-second stream check (`meta-g-link-probe.exe --stream`) recorded, for every video unit, its capture time (the 8-byte prefix), its arrival time and the number of pictures in it (HEVC NAL units with `first_slice_segment_in_pic_flag`). The instrumentation was removed afterwards.

- **The glasses encode a true 30 fps.** In a typical run, 1,804 units arrived over 60.09 s of capture time, and all 1,803 gaps between capture times were 33 ms. Every unit held exactly one picture (the first an IDR, then trailing pictures), and 61 keyframes arrived, one per second as requested.
- **They deliver the pictures in bursts.**
  - Over the same run, 1,139 of the 1,803 gaps between arrivals were under 5 ms, and 277 were over 120 ms.
  - A typical sequence of arrival gaps, in milliseconds: `0 1 0 160 0 1 2 159 1 2 1 1 132 1 3 2 145 …`.
  - So four or five pictures arrive at once, then nothing for 110–160 ms.
- In one run the link was worse: a 1.8 s stall and five capture gaps over 120 ms, so about 200 frames never arrived (26.7 fps by capture time). That is lost data, separate from the bursts.

## Why OBS showed about 13 fps

- The plugin handed each picture to OBS as soon as it was decoded, in OBS's *unbuffered* async mode.
- In that mode OBS keeps only the newest queued picture on each rendered frame and discards the rest (`ready_async_frame` in `libobs/obs-source.c`, OBS 32.2.2).
- The single-threaded decoder spread each burst over a few milliseconds, so about two pictures of every burst survived. At six to seven bursts a second, that gives about 13 fps on screen.

## Where the bursts come from

| Test | Bursts |
|---|---|
| Ping the glasses every 10 ms during the stream (590 replies in 30 s) | Unchanged. Pings took a median 18 ms, a 90th percentile of 43 ms and at most 164 ms, with 3.6% lost, on a link a metre long |
| Windows power plan: Wireless Adapter power saving | Already *Maximum Performance*; the USB adapter exposes no power-saving property |
| Ask for 1.2 Mbps instead of 4 Mbps | Unchanged, and even cleaner: 1,379 gaps under 5 ms and 372 over 120 ms |
| SessionSettings `allowFrameCapture` 0 (the Meta app never sets it) | Unchanged |
| Windows WLAN media streaming mode (`wlan_intf_opcode_media_streaming_mode`, accepted with status 0) | Unchanged |
| `SetWifiProperties` with only the country code, so `enableWakelockTimeout` is off (acknowledged with status 0) | Unchanged |

- The experiments ran cumulatively, not one at a time. Each restored source file kept its old modification time, so the build did not recompile it, and each run also carried the earlier changes. This was found afterwards, the files were rebuilt from the committed sources, and the binaries were checked for leftover experiment strings. The result stands: no change, alone or combined, affected the bursts.
- The ping replies come from the glasses' network stack, not their video pipeline, and they were as irregular as the video. The burst period also does not depend on the bitrate. So the timing comes from the glasses' radio on this link, most likely a power-saving duty cycle on the network they host.

## What the Meta AI app does

Two read-only research passes over the decompiled Meta AI Android app, from the session's earlier jadx output plus a re-decompile with `--show-bad-code`:

- **The video request matches ours** in the fields that matter.
  - Our `eisMode` 1 means stabilization *disabled*.
  - The frame-rate fields are the same (frame rate 5, capture frame rate 8), and media batching is 0.
  - Meta's highest built-in bitrate is 1.2 Mbps at 720 × 1280 and 30 fps; its default is 24 fps at 400 kbps. We ask for 4 Mbps.
  - Meta never sets `allowFrameCapture`.
- **The app paces its own player.**
  - It rebases the capture time onto the phone's clock once, at the first frame.
  - Before each decode it sleeps so that frames are at least 30 ms apart.
  - It releases each decoded frame for display at its capture time.
- **It has no switch for the glasses' radio.**
  - None of the service-10 link messages carries a power, latency or throughput mode.
  - Its only Wi-Fi setting, `SetWifiProperties`, is sent once at Bluetooth connect with the country code and an optional wakelock-timeout flag.
  - When the phone joins a glasses-hosted network, it makes a plain client join and a plain TCP connection, like this PC.

## The fix

The plugin now presents each picture at its own capture cadence, the way the Meta app's player does:

- **`mgl::playout::PlayoutClock`** (`src/protocol/playout.cpp`, portable and unit-tested):
  - It maps capture times onto the receiver's clock: the earliest arrival over the last 10 s, plus a delay.
  - The delay is the worst lateness over that window plus 15 ms, kept between 20 and 500 ms.
  - The delay rises at once when a unit comes later than the delay covers, and it shrinks by at most 1% of elapsed time.
  - Audio and video share one clock: the earliest arrival-minus-capture value of each medium matched within 1 ms.
- **`mgl::plugin::VideoPacer`** (`src/plugin/video_pacer.cpp`):
  - It decodes each picture on its own thread 15 ms before it is due.
  - It hands the picture to OBS stamped with its presentation time on OBS's clock.
  - OBS's default buffered mode then shows each picture on its frame.
  - Audio is stamped on the same clock as it decodes.
- **Errors and reporting:**
  - Decoder rejections, which were silently ignored before, are counted and logged.
  - Once a minute the log reports the frame rate received and shown, the delay, late units and undecodable or skipped pictures.

## Verification in OBS

| | 0.3.0 | 0.3.1 |
|---|---|---|
| Distinct glasses pictures per second in a 20 s OBS recording of the glasses scene (30 fps canvas) | **12.8** (242 in 18.97 s) | **29.6** (577 of 582 frames in 19.5 s) |

- **How the pictures were counted:** each recording was cropped to the glasses' 720 × 1080 area. `mpdecimate` then counted distinct pictures; OBS repeats the last picture on frames that have no new one.
- **During both runs** Randy wore the glasses and looked around.
- **The recordings were deleted after counting.**
- **The 0.3.1 log line after 60 s:** "Video over the last 60 s: 30.0 fps from the glasses, 30.0 fps shown, presentation delay 188 ms, 2 late, 0 undecodable, 0 skipped."
- **Audio still plays:** OBS's audio meter showed the camera source with glasses audio on peaking at −11.0 dBFS, and a microphone-only source peaking at −12.3 dBFS. Both were measured on Randy's voice.
- **Release build, installed from the zip:** a 30.8 s OBS recording held 920 distinct pictures in 922 frames (29.8 per second). The log reported "30.0 fps from the glasses, 29.9 fps shown, presentation delay 452 ms, 9 late, 0 undecodable, 0 skipped". The link was rougher in this run, so the delay grew to cover it and no frame was lost. That recording, cropped to the glasses' picture with the audio removed, is the README clip (`assets/glasses-30fps.mp4`, and 10 s of it as `assets/glasses-demo.webp`, AGENTS.md revision 7). Every screen in it was checked at full resolution; no text is legible.
- **Cost:** about 0.2 s of added latency, the size of one burst gap; more on an uneven link.
- **Not measured:** glass-to-OBS latency and audio/video sync. Both media now share one clock, which should improve sync, but it has not been measured.
