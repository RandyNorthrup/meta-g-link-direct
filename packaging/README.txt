Meta G Link Direct: Ray-Ban Meta (Gen 2) glasses camera in OBS Studio on Windows
Full guide: https://github.com/RandyNorthrup/meta-g-link-direct#readme

BEFORE YOU START
- Setup makes this PC the glasses' owner. The Meta AI phone app can't use the
  glasses until you remove and add them again in Meta AI.
- Turn off your phone's Bluetooth.
- While the camera runs, the PC's Wi-Fi adapter is connected to the glasses and
  has no internet. Use Ethernet if you stream online.

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
   Answer "y" when the installer offers setup and type YES. Log in to Meta in
   the browser, then paste the final address from the browser's address bar
   into the installer window. To do this later, close OBS and run
   .\Install.cmd -Setup in a terminal in this folder (it reinstalls, then
   goes straight to the setup).

USE
In OBS, add the "Ray-Ban Meta Glasses (Direct)" source to a scene and make the
scene live. The first frame takes about 20 seconds.

REMOVE
Double-click Uninstall.cmd. It keeps the glasses' stored identity; to forget it
as well, delete the folder %LOCALAPPDATA%\MetaGLinkDirect afterwards.
