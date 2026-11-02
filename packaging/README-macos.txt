Meta G Link Direct: Ray-Ban Meta (Gen 2) glasses camera and microphones in OBS
Studio on macOS
Full guide: https://github.com/RandyNorthrup/meta-g-link-direct#readme

BEFORE YOU START
- Setup makes this Mac the glasses' owner. The Meta AI phone app can't use the
  glasses until you remove and add them again in Meta AI.
- Turn off your phone's Bluetooth, and disconnect the glasses from any other
  computer: they refuse a new connection while their Bluetooth is full.
- While the glasses are connected, the Mac's Wi-Fi is on their network and has no
  internet. Use Ethernet if you stream online.
- These builds are not notarized. macOS 13.3 or newer and OBS.app in /Applications
  are required.

1. PAIR THE GLASSES WITH THIS MAC (once)
   Pairing mode: fold both arms, then hold the capture button about 12 seconds,
   until the light inside the frame blinks blue. Then connect "RB Meta ...." in
   System Settings > Bluetooth.

2. INSTALL
   Close OBS, then run:
       ./install.sh
   Meta G Link Helper then opens and macOS asks two questions, once each:
   - Bluetooth, to reach the glasses.
   - Location, because macOS shows a Wi-Fi network's name only to apps allowed
     Location, and the helper needs the name to join the glasses' network. It never
     reads where you are.
   Please allow both. To change an answer later, use System Settings > Privacy &
   Security > Bluetooth or Location Services.

3. MAKE THIS MAC THE OWNER (once)
   Run ./install.sh --setup, or use "Set up glasses..." in the glasses source's
   properties in OBS. Log in to Meta in that window; it finishes by itself. Cancel
   changes nothing.
   The first time OBS connects afterwards, macOS asks for your password so OBS can
   read the glasses' key. Enter it and choose Always Allow. OBS waits until you
   answer.

USE
In OBS, add the "Ray-Ban Meta Glasses (Direct)" source to a scene and make the
scene live. The first frame takes about 15 seconds. For the glasses' microphones
without the camera, add "Ray-Ban Meta Glasses Microphone (Direct)" instead. Tools >
Ray-Ban Meta Glasses (Direct) opens the settings.

While the glasses are connected, their battery level is in
~/Library/Application Support/obs-studio/plugin_config/meta-g-link-direct/battery.txt,
and obs-websocket clients can ask the vendor "meta-g-link-direct" for it
(GetBattery, event BatteryChanged).

REMOVE
Run ./install.sh --uninstall. It keeps the glasses' stored identity. To forget it
as well, use "Forget these glasses" in the source's properties first.
