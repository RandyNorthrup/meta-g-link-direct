# Quality profiles, the setup window without WebView2, the accessibility pass, and the battery outputs (0.4.0)

Observed on 2026-10-20 on the development PC (Windows 11 Pro x64 build 26200, one 3440 × 1440 monitor at 100% scaling, OBS 33.0.0-rc2 with Randy's profile at a 30 fps canvas, obs-websocket 5.7.6, the same glasses and adapters as earlier reports). The quality runs used the 0.3.2 release; the setup-window checks used 0.3.2 and then the 0.4.0 build; the battery checks used the 0.4.0 build.

## Quality profiles in OBS

The test scene went live with the camera source at 1080 × 1920, then the quality was changed to 360 × 640 while live. Figures are the plugin's once-a-minute log lines.

| Profile | Time live | From the glasses | Shown in OBS | Presentation delay | Late a minute | Undecodable / skipped |
|---|---|---|---|---|---|---|
| 1080 × 1920 | 3 min 14 s | 30.0 fps | 29.9–30.1 fps | 198–295 ms | 1–3 | 0 / 0 |
| 360 × 640 | 3 min | 24.0 fps | 24.0 fps | 182–213 ms | 1 | 0 / 0 |

- 360 × 640 asks the glasses for 24 fps, so 24.0 fps is its full rate.
- The first frame came 12.5 s after the scene went live. The change to 360 × 640 took effect 1.6 s later.
- An earlier run of about a minute each gave the same rates (1080 × 1920: 30.0 fps, delay 205 ms; 360 × 640: 24.1 fps, delay 169 ms; change in 1.5 s).
- No stray messages were logged in either run, which adds 6 minutes of clean streaming to the 0.3.2 decoder fix's record.
- **Battery:** the glasses reported their battery every 10–20 seconds. It fell from 92% to 78% in about 3 minutes at 1080 × 1920, then from 78% to 69% in about 3 minutes at 360 × 640. Higher quality drains faster. Whether Randy wore the glasses was not recorded, and no stop on taking them off was seen.

## The setup window without WebView2

- **Method:** the setup window was started with `WEBVIEW2_BROWSER_EXECUTABLE_FOLDER` pointing at a folder that does not exist, so WebView2 cannot start. Nothing was uninstalled. No login was attempted, so the glasses' owner did not change.
- **Result:** Continue led to the browser screen: the heading *Log in to Meta in your browser*, the three steps, **Open the login in your browser**, the address box, Continue and Cancel.
- Cancel and Esc closed the window with exit code 8 (cancelled). No `MetaGLinkDirect-setup-*` browser folder was left in `%TEMP%`.

## Accessibility pass

The checks read the Windows UI Automation tree, which is what Narrator reads, and moved focus through it. Keystrokes were sent only to the setup window. The OBS dialog did not accept the foreground for injected keys, and a guard stopped the walk there after two keys, so its tab order is taken from the tree, which follows Qt's creation order.

### OBS properties

- Every control takes keyboard focus and has a name: the two lists, the three checkboxes and the three buttons, plus Defaults, OK and Cancel. Forget stays disabled until its confirmation is ticked.
- Each property's tooltip reaches UI Automation as the control's description.
- **OBS limitation:** the *Video quality* list is named "Video", and the *Wi-Fi adapter* list "Glasses connection (shared by all glasses sources)".
  - **Cause:** OBS's properties view (`shared/properties-view/properties-view.cpp`) adds each row with `QFormLayout::addRow(QWidget *, QWidget *)` and never calls `setBuddy`. Qt then names a list after the group box it sits in.
  - **Plugin-side attempt, reverted:** dropping the lists' long descriptions, so the label is a plain `QLabel`, left the names unchanged. The descriptions were restored.
  - **Upstream fix:** setting the label's buddy in OBS. That would fix every plugin.
- **480 px wide:** everything lays out, except that *Keep the glasses connected while OBS is open* was cut to "…while OBS is ope" next to its help icon, because Qt checkboxes do not wrap. 0.4.0 shortens it to **Keep connected while OBS is open**, which fits at 480 px. The forget confirmation fits with a few pixels to spare.
- **Forget, at Randy's request:** 0.4.0 moves it out of *Glasses setup* into a group of its own, *Forget the glasses*, with the confirmation tick under the button. Each group has its own result line. Checked in OBS: the new group shows the disabled button with the tick under it.

### Setup window

| Check | 0.3.2 | 0.4.0 |
|---|---|---|
| Roles and names on the first screen | Heading and body as text, Continue and Cancel as buttons | Same |
| Tab, Shift+Tab, Enter and Esc | Tab and Shift+Tab move between the buttons; Enter on Continue goes on; Esc cancels | Unchanged code path (`IsDialogMessage`) |
| Name of the browser screen's address box | The whole instructions paragraph | *Address after the login:*, from its own label |
| Tab order on the browser screen | Cancel, address box, browser button, Continue (box before the button above it) | Browser button, address box, Continue, Cancel |
| First focus on the browser screen | Continue | **Open the login in your browser** |
| Focus after switching to another window and back | The window itself (no visible focus) | The control that had it (tested on the address box) |
| Narrowest size | Not checked | 480 × 520: text wraps, nothing overlaps |

- **Size on smaller screens (code review):** 0.3.2 opened at 640 × 760 logical pixels from the cascade position. At 150% on a 1080p screen that is 1,140 physical pixels, taller than the screen, so Continue and Cancel fell below its bottom edge. 0.4.0 opens centred on its monitor, and cuts its opening and minimum sizes down to the monitor's free area. This PC's single monitor at 100% cannot show that case.
- **High contrast (code review):** the window and its static text use system colours (`COLOR_WINDOW`, `COLOR_WINDOWTEXT`). The buttons, edit box and progress bar are standard themed controls. Not tried with a contrast theme on, because that changes the whole desktop.
- **150–200% scaling (code review):** the process is per-monitor DPI aware (V2). Layout and fonts are scaled from 96-DPI values with `GetDpiForWindow` and `SystemParametersInfoForDpi`, and are rebuilt on `WM_DPICHANGED`. Not tried at those scales, because the display scale is a desktop-wide setting.
- **Not automated:** the setup window has no UI test. These checks were run by hand through UI Automation.

## Battery outputs

0.4.0 passes the battery on two ways (brief D25). The test started idle, then made the test scene live with a temporary OBS **Text (GDI+)** source reading `battery.txt`, then switched back to the previous scene. The temporary source was removed afterwards.

| Moment | `GetBattery` | `battery.txt` | `BatteryChanged` events |
|---|---|---|---|
| Idle, before | `{"available": false}` | empty | none |
| Live, about 50 s | `{"available": true, "charging": false, "percent": 66}` | `65%` (a report came in between) | 68%, 67%, 66%, 65% |
| Scene left | `{"available": false}` | empty | `{"available": false}` |

- The text source showed "66%" in OBS's screenshot of it.
- The plugin logged nothing about obs-websocket, so the vendor registered. It logs a line only when obs-websocket is missing or refuses the request.
- **Tests:** two new protocol tests cover the overlay text and the publisher: each change published once and in order, a report with only a new voltage not published, unknown published when the link goes down, and no call after the listener is removed. The new red drill `battery-change` makes the publisher forget what it published, and the publisher test must then fail.
- **Not tested:** a Stream Deck plugin reading the file, and a file reader holding the file open while the plugin replaces it (the plugin tries three times, 20 ms apart, then waits for the next change).

## Still open

- High contrast and 150–200% scaling on a real display, for both the properties and the setup window.
- A full keyboard walk of the OBS properties, with focus held in OBS.
- Narrator by ear, as opposed to the UI Automation tree it reads.
