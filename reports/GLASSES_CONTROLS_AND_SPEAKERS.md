# Glasses controls, wear sensor and speakers (after 0.3.0)

Observed on 2026-10-19 on the development PC (Windows 11 Pro x64 build 26200, the same glasses and adapters as [the Wi-Fi link report](WIFI_LINK_VERIFICATION.md)), with the installed 0.3.0 setup tool's 60-second stream check (`meta-g-link-probe.exe --stream`, its status lines timestamped as they arrived) and OBS 33.0.0-rc2 with the 0.3.0 plugin. Randy wore the glasses and performed each action. No plugin code was changed. The logs record message types and sizes only, never payloads.

## Which controls reach this PC

Every run streamed the full minute (1,801–1,808 frames). Besides the messages every run carries (stream start-up types 2014, 2018 and 2009; health and battery reports; media message types 23 and 25, which also arrived with no action, so they are routine), the actions gave:

| Action | What this PC saw |
|---|---|
| Single tap, double tap, swipe forward and back on the touchpad | Nothing on the media link; no media or volume key in Windows (a low-level keyboard hook was listening) |
| Touch and hold on the touchpad | Nothing on the media link; the glasses spoke a hint about a long press, apparently because Meta AI needs the phone app |
| Long press of the capture button | svc28 type 2032 (47 bytes), in both runs that included it; the glasses also spoke; the stream continued |
| Short press of the capture button alone | No type 2032; one svc **0** type 0 message (256 bytes) arrived about 30 s after the stream began and is unexplained |
| Glasses taken off for about 15 s, then put back on | Nothing distinct; the stream continued |

So the touchpad is handled inside the glasses with no phone connected, and the long press of the capture button is the only control seen on the link so far (one message type, not yet confirmed in a run with that action alone). The glasses' wear sensor sends no change on the media link that the stream check logs; the stream does not stop when the glasses come off. The Bluetooth control link was not logged. Button and wear work is parked.

## Speakers

Windows lists the glasses as *Headphones (RB Meta 00TM)* (stereo Bluetooth audio), *Headset (RB Meta 00TM Hands-Free)* and a media-control device.

- With Windows' output on the glasses and a YouTube video playing, the video kept pausing while the glasses camera streamed and played normally once the stream ended. The Ethernet connection kept internet access throughout, so the glasses were sending pause commands while their camera ran. The stream check still got 1,803 frames.
- A plain player (`ffplay`, a 440 Hz tone for 35 s) on the same output played continuously through the camera stream, by Randy's ear, and the stream check got 1,805 frames.
- In OBS, the profile's monitoring device was set to *Headphones (RB Meta 00TM)* after a backup of the profile, and OBS logged it at start-up. A temporary beep source set to *Monitor Only* in a scene with a glasses camera source played steadily in the glasses, not from the PC speakers, from before the camera started through about 30 s of streaming (video 15.9 s after the scene went live). OBS reported 30 fps and 0 of 2,689 render frames skipped at about 2% CPU. The beep source was then removed and the program scene restored.
- Not measured: the Bluetooth audio delay, and battery use with the speakers playing.
