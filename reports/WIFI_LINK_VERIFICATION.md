# Wi-Fi link investigation

Observed on 2026-10-18 on the development PC: Windows 11 Pro x64 build 26200, one USB Wi-Fi adapter (Realtek chipset, the PC's only Wi-Fi radio), the PC online over Ethernet, and one pair of Ray-Ban Meta Gen 2 glasses worn throughout with the phone's Bluetooth off. The glasses' firmware build was not recorded. Every stream used 720×1280 HEVC at 30 fps and 4 Mbit/s with glasses audio off unless stated. Raw logs stayed in the session scratchpad and are not committed; they contain network addresses.

## The problem

With the glasses joining an access point (svc10 `JoinAccessPoint`), video stopped 8–54 s into every run:

- Windows Mobile Hotspot pinned to 2.4 GHz: drops in every run.
- Windows Wi-Fi Direct legacy access point: the glasses refused to join (status 2 on every try).
- The legacy hosted network: would not start on this driver.
- A dual-band home router, which the glasses joined on 5 GHz at 80 MHz: drops in every run (two observed at 21.9 s and 39.0 s).

Changes that made no difference on the hotspot: bitrate (360p at 600 kbit/s dropped as fast as 720p at 4 Mbit/s), forcing the adapter to 20 MHz / VHT off / 802.11b-g / bandwidth setting 0, LDPC and STBC off, glasses audio on or off, keeping the Bluetooth session open or closing it, adapter power management off, and a svc100 keepalive every 2 s. Every adapter setting was restored to its original value afterwards.

## Ping during the drop (inconclusive)

During two home-router runs a continuous ping went to the glasses' Wi-Fi address. Only the last lines of the ping output were kept, without timestamps. In the second run the glasses answered 20 consecutive pings at 2–25 ms, then stopped answering. Working back from the four-second timeouts, the replies ended a few seconds before the session declared the stall, and the session declares a stall 8 s after the last keepalive answer. In the first run the replies ended at about the time video stopped. So the glasses lost reachability somewhere between the moment video stopped and a few seconds later, before the session tore down. No `ControlStreamStopped` arrived and the TCP connection was not closed.

This neither confirms nor rules out the Wi-Fi firmware hang in Visor's notes, which ties the hang to the access point's real channel width. The true channel width of the Windows hotspot on this adapter was never confirmed, and the glasses-hosted network below runs at 20 MHz. An early reading of this data in the session, that the glasses stayed reachable for about 20 s after video stopped, was wrong.

## The glasses can host their own network

The Meta AI Android app's protocol classes (`com.oculus.atc`, decompiled locally with jadx; nothing from the app is copied into this repository) define svc10 message types that Visor's notes do not use:

| Message | Type |
| --- | --- |
| CreateSoftApNetwork | 0x1000 |
| SoftApNetworkInfo (reply) | 0x1001 |
| DestroySoftApNetwork | 0x1002 |
| DestroySoftApNetworkResult | 0x1003 |
| JoinAccessPoint | 0x1010 |

`CreateSoftApNetwork` fields: 3 band (1 = 2.4 GHz, 2 = 5 GHz), 4 channel, 5 hidden, 6 country code, 7 UUID (16 bytes), 8 credentials {1 SSID, 2 password}, 9 bandwidth (1 = 20 MHz, 2 = 40 MHz, 3 = 80 MHz). `SoftApNetworkInfo`: 1 status (1 success, 2 internal error, 3 start failed, 4 IP failed), 2 repeated address {3 address text}, 3 credentials, 4 port, 5 channel. `DestroySoftApNetwork` has no fields.

## Results with the glasses hosting 2.4 GHz / 20 MHz on channel 6

| Run | Path | Result |
| --- | --- | --- |
| 1 | Standalone harness | 5,370 frames in about 179 s of video, no stall, first frame 21 s after start |
| 2 | Production `MediaSession` | 3,127 frames in a 120 s session (about 104 s of video), no stall, first frame 15.9 s after start |
| 3–4 | Production `MediaSession`, two 30 s sessions back to back | Both streamed; the second started cleanly after the first stopped; no Wi-Fi profile left behind |
| 5 | Close over the Wi-Fi link after a stream | Sent, never acknowledged; the network was still visible on an immediate scan |
| 6 | Network lifetime after a stream | Still visible on a fresh scan 8 s after the session ended, gone at 14 s |
| 7 | Close over Bluetooth before any client joined | Acknowledged (`0x1003`); gone on an immediate fresh scan |

The glasses close the Bluetooth session once the PC has joined their network, so after a stream no close can be sent over Bluetooth.

## Resulting design

The PC no longer hosts any network. `MediaSession` asks the glasses to host their own 2.4 GHz / 20 MHz network with fresh random credentials and the Windows region as country code, joins it with a temporary Native Wi-Fi profile, and opens the media session over TCP. If the PC cannot join or cannot open the media connection, it closes the glasses' network over Bluetooth. After a stream the glasses close it on their own. The Mobile Hotspot and Wi-Fi Direct owners and the `JoinAccessPoint` path were removed.

Consequence: while the camera runs, the PC's Wi-Fi adapter is on the glasses' network, which has no internet. A PC online only through that adapter is offline until the camera stops.

## Not yet verified

- The new link inside OBS (the OBS source runs the same `MediaSession`).
- The rewritten DHCP detection in `wifi_station.cpp` (moved from parsing addresses to the IP Helper address table after static analysis) on hardware.
- Reconnection after a real drop, longer runs, the 120-minute acceptance run, latency, thermal behaviour and battery cost.
- Other adapters and drivers, 5 GHz or wider channels on the glasses' network, and the current firmware build.
