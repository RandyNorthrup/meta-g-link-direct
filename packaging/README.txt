Meta G Link Direct: Ray-Ban Meta (Gen 2) glasses camera and microphones in OBS
Studio on Windows
Full guide: https://github.com/RandyNorthrup/meta-g-link-direct#readme

BEFORE YOU START
- Setup makes this PC the glasses' owner. The Meta AI phone app can't use the
  glasses until you remove and add them again in Meta AI.
- Turn off your phone's Bluetooth.
- While the glasses are connected, the PC's Wi-Fi adapter is on their network
  and has no internet. Use Ethernet if you stream online.

1. PAIR THE GLASSES WITH THIS PC (once)
   a. Turn off Bluetooth on your phone.
   b. Take the glasses out of the case; make sure they are charged and on.
   c. On the PC: Settings > Bluetooth & devices > Add device > Bluetooth.
   d. Pairing mode: fold both arms, then hold the capture button (the one you
      press to take a photo) about 12 seconds, until the light inside the frame
      blinks blue.
   e. Choose "RB Meta ...." in the Windows list and accept any prompt.
   f. Check that "RB Meta ...." is listed under Bluetooth & devices.
   Not listed? Your Bluetooth adapter must support Bluetooth LE. Put the
   glasses in the case for a few seconds and try pairing mode again.

2. INSTALL
   a. Close OBS.
   b. Double-click Install.cmd. If SmartScreen warns, choose
      More info > Run anyway (the files are not code-signed).

3. MAKE THIS PC THE OWNER (once)
   Answer "y" when the installer offers setup. The setup window opens: select
   Continue, log in to Meta in that window, and it finishes by itself.
   Nothing needs to be copied. Cancel changes nothing. To do this later, use
   "Set up glasses..." in the glasses source's properties in OBS.

USE
In OBS, add the "Ray-Ban Meta Glasses (Direct)" source to a scene and make the
scene live. The first frame takes about 15 seconds. For the glasses'
microphones without the camera, add "Ray-Ban Meta Glasses Microphone (Direct)"
instead. Tools > Ray-Ban Meta Glasses (Direct) opens the settings: video
quality, the glasses' microphones (off by default), the Wi-Fi adapter, and
"Keep connected while OBS is open", which makes a source start in
about a second but uses the glasses' battery and keeps the Wi-Fi adapter
offline until OBS closes.

While the glasses are connected, their battery level is in
%APPDATA%\obs-studio\plugin_config\meta-g-link-direct\battery.txt for an OBS
Text (GDI+) source with "Read from file", and obs-websocket clients can ask
the vendor "meta-g-link-direct" for it (GetBattery, event BatteryChanged).

Works with OBS Studio 32 and 33, on any drive. The plugin is installed to
%ProgramData%\obs-studio\plugins\meta-g-link-direct (usually C:\ProgramData),
in the layouts both versions read, so upgrading OBS needs no reinstall. If the
installer picks the wrong OBS or finds none, run it from a terminal with the
OBS folder: .\Install.cmd -ObsPath "D:\Apps\obs-studio"

REMOVE
Double-click Uninstall.cmd. It keeps the glasses' stored identity. To forget it
as well, use "Forget these glasses" in the source's properties first, or delete
the folder %LOCALAPPDATA%\MetaGLinkDirect afterwards.
