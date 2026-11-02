Meta G Link Direct: Ray-Ban Meta (Gen 2) glasses camera and microphones in OBS
Studio on Linux
Full guide: https://github.com/RandyNorthrup/meta-g-link-direct#readme

BEFORE YOU START
- Setup makes this computer the glasses' owner. The Meta AI phone app can't use
  the glasses until you remove and add them again in Meta AI.
- Turn off your phone's Bluetooth, and disconnect the glasses from any other
  computer: they refuse a new connection while their Bluetooth is full.
- While the glasses are connected, the computer's Wi-Fi adapter is on their
  network and has no internet. Use Ethernet or a second adapter if you stream
  online.
- This needs BlueZ and NetworkManager, and a Secret Service keyring such as GNOME
  Keyring or KWallet for the glasses' identity.

1. PAIR THE GLASSES WITH THIS COMPUTER (once)
   Pairing mode: fold both arms, then hold the capture button about 12 seconds,
   until the light inside the frame blinks blue. Then pair "RB Meta ...." in your
   desktop's Bluetooth settings, or run:
       bluetoothctl
       scan on
       pair <address>
       trust <address>

2. INSTALL
   Close OBS, then run:
       ./install.sh
   It installs into each OBS it finds: the native one and the OBS Flatpak. The
   Flatpak build works on any distribution. The native build was made on Ubuntu
   26.04 and needs that FFmpeg and OBS; if it doesn't fit, the installer says so,
   and the README's "Build from source" gives you one that does.
   For the Flatpak, the installer also allows the Bluetooth, BlueZ, NetworkManager
   and Secret Service access the plugin needs.

3. MAKE THIS COMPUTER THE OWNER (once)
   Run ./install.sh --setup, or use "Set up glasses..." in the glasses source's
   properties in OBS. Log in to Meta in that window; it finishes by itself. Cancel
   changes nothing.
   The OBS Flatpak keeps its own identity, apart from native OBS: if you use both,
   the one you set up last owns the glasses, and the other needs its setup again.

USE
In OBS, add the "Ray-Ban Meta Glasses (Direct)" source to a scene and make the
scene live. The first frame takes about 15 seconds. For the glasses' microphones
without the camera, add "Ray-Ban Meta Glasses Microphone (Direct)" instead. Tools >
Ray-Ban Meta Glasses (Direct) opens the settings.

While the glasses are connected, their battery level is in
~/.config/obs-studio/plugin_config/meta-g-link-direct/battery.txt (for the Flatpak,
~/.var/app/com.obsproject.Studio/config/obs-studio/...), and obs-websocket clients
can ask the vendor "meta-g-link-direct" for it (GetBattery, event BatteryChanged).

REMOVE
Run ./install.sh --uninstall. It keeps the glasses' stored identity. To forget it
as well, use "Forget these glasses" in the source's properties first.
