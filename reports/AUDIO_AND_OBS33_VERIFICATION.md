# Glasses audio, OBS 33 and plugin location (0.2.0)

Observed on 2026-10-19 on the development PC (Windows 11 Pro x64 build 26200, OBS 32.2.2 and later also OBS 33.0.0-rc2, the same glasses and adapters as [the Wi-Fi link report](WIFI_LINK_VERIFICATION.md)).

## Glasses audio

- With audio requested (ConfigureAudioStreamRequest enabled, AAC-LC 48 kHz stereo at 96 kbit/s), a protocol harness over the production `MediaSession` counted, in one 40 s session: 675 video access units, 1,061 audio access units, and **no audio CodecConfigurationUpdateRequest**. The only codec configuration was the 99-byte video one.
- The audio units arrived at about 47 per second (48,000 / 1,024 samples), with presentation times about 21–24 ms apart. Their first bytes (for example `21 19 8F FF FF FF …`) are a raw AAC channel-pair element, not ADTS. So the glasses send raw AAC-LC stereo frames and no AudioSpecificConfig.
- Visor's notes describe the AudioSpecificConfig arriving as a codec configuration; on this firmware it did not. The plugin therefore configures the decoder with the AudioSpecificConfig for the requested format (`11 90`: AAC-LC, 48 kHz index 3, two channels), built from the same constants as the request. A configuration the glasses do send replaces it.
- In OBS 32.2.2 with *Glasses microphone audio* on: `receiving audio 48000 Hz, 2 channel(s)` 22 s after the scene went live, just before the video; OBS's own `InputVolumeMeters` reported the source with a peak of −15 dBFS over 12 s; OBS skipped 0 render frames out of 2,973 while video and audio ran. With the setting off the source stays silent in the mixer.
- Not measured: audio/video sync offset, and audio over long runs.

## OBS 33

- OBS 33.0.0-rc2 (2026-10-16) reworks plugin loading. Third-party plugins have a new location, `%ProgramData%\obs-studio\plugins\<name>\<name>.dll` with `data\` beside it. Plugins in the old locations load but are tagged "Legacy" in the Plugin Manager. Per the release notes, plugins in the Program Files `obs-plugins` folder stop loading in OBS 34.0. A plugin found in the new location is preferred over a legacy copy. Sources: the 33.0.0-rc2 release notes, `libobs/obs-module.c` and `frontend/plugin-manager/InstalledPluginRow.cpp` at tag `33.0.0-rc2`, and obsproject.com/kb/legacy-plugin-locations.
- OBS 33 drops FFmpeg 6 and 7. Its Windows build ships `avcodec-62.dll` and `avutil-60.dll`, the same FFmpeg ABI this plugin links, as OBS 32.2.2 does.
- The official OBS 33.0.0-rc2 Windows zip (SHA-256 `33d5fab2…4127`, matching GitHub's published digest) was extracted to a scratch folder. `mgl-module-tests` against its runtime passed, as it does against 32.2.2. Run portable with the plugin in `plugins\meta-g-link-direct\` (the new layout) and beside the main OBS 32 (`--multi`), it logged `[meta-g-link-direct] 0.1.0 loaded; camera source registered` with no legacy warning (an interim build), and again `[meta-g-link-direct] 0.2.0 loaded; camera source registered` with the final 0.2.0 code.
- OBS 33.0.0-rc2 was then installed on the development PC with the official installer (digest-verified, Authenticode-signed by OBS Project, LLC) into `C:\Program Files\obs-studio`, beside the existing OBS 32, after backing up the OBS profile. This project's installer found it through the registry and installed both ProgramData layouts. Started with the user's profile, OBS 33 logged `[meta-g-link-direct] 0.2.0 loaded; camera source registered` from the flat layout and `Skipping legacy plugin … bin/64bit … already loaded from …`. In OBS 33, over its WebSocket:
  - the Wi-Fi adapter list showed Automatic and the USB adapter;
  - with glasses audio on, audio (48 kHz stereo) and 720 × 1280 video started 16 s after the scene went live;
  - for about 60 s it ran at 30.0 fps with about 2.2% OBS CPU and flat memory;
  - render skips held at 2 (present before streaming) and were 2 of 3,552 after the stop;
  - the mixer peaked at −29.7 dBFS;
  - a source screenshot showed a live first-person frame (deleted after the check);
  - "Camera stopped" came on the scene switch and no Wi-Fi profile was left behind.
- Not done: the final OBS 33.0 release, which is not out yet.

## Plugin location

- The installer now writes `%ProgramData%\obs-studio\plugins\meta-g-link-direct` (`C:\ProgramData\…` on this PC) with the plugin in both layouts: flat `meta-g-link-direct.dll` for OBS 33 and later, and `bin\64bit\meta-g-link-direct.dll` for OBS 32. OBS 33 skips the second copy, as its log shows. A portable OBS gets its version's layout in its own folder. The installer removes this plugin's copies elsewhere. An earlier draft installed only the installed version's layout, but that breaks whichever OBS is not targeted when both are on one PC, and it would need a reinstall after every OBS upgrade.
- On the development PC (OBS 32.2.2, per-user install), the real install moved the plugin from the old `obs-plugins\64bit` folder to the ProgramData `bin\64bit` layout, removed the old copy and its data folder, and OBS loaded it from there. Dry runs chose the flat layout for the portable OBS 33 and removed only the setup tool when OBS could not be found.
- Other drives: the installer takes ProgramData, Program Files and the local application data folder from Windows, and also looks for a running OBS and every Steam library. With the scratch folder mapped to `M:` (`subst`) and the bundle run from `M:` under Windows PowerShell 5.1:
  - `-ObsPath M:\obs33` chose the portable layout `M:\obs33\plugins\meta-g-link-direct`, and the copy step put the DLL and locale there;
  - that OBS 33.0.0-rc2, started from `M:` beside the user's OBS, logged `[meta-g-link-direct] 0.2.0 loaded; camera source registered`;
  - with no `-ObsPath`, the installer listed `C:\Program Files\obs-studio`, `M:\obs33` (found only through the running process) and the per-user OBS 32, and used the first;
  - the test OBS was then closed by its process id (its window did not close within 15 s), the user's OBS kept running, and `M:` was removed.
  Not tested: ProgramData or the user profile actually relocated to another drive, and a real Steam OBS; the Steam library parsing is covered by a unit test.
- Defects found and fixed while testing:
  - Under `-WhatIf`, loading the CIM module printed its alias definitions as "What if" actions. The process lookup now runs without `-WhatIf`; it changes nothing.
  - A `-WhatIf` run left `.mgl-write-test` probe files in the OBS and ProgramData folders. They were deleted, the probe no longer runs during `-WhatIf`, and it removes its file directly.
  - Under Windows PowerShell 5.1 an empty permission result became `$null`, and a real install failed safely, with exit 1 and nothing changed. The result is now always an array.
