// SPDX-License-Identifier: AGPL-3.0-only
// The glasses' battery outside the source properties, for overlays and stream controllers:
// - battery.txt in this plugin's OBS config folder holds "87%", "87% charging", or nothing while
//   the battery is unknown, for an OBS text source (Read from file) or any tool that shows a file;
// - obs-websocket clients can ask for it with the vendor request GetBattery and are sent the vendor
//   event BatteryChanged, both from the vendor meta-g-link-direct.
#ifndef MGL_PLUGIN_BATTERY_STATUS_H
#define MGL_PLUGIN_BATTERY_STATUS_H

// Empties battery.txt. Call once from module load, after mglLinkInit.
void mgl_battery_status_load(void);
// Registers the obs-websocket vendor and its request, then starts following the battery.
// obs-websocket accepts vendors only once every module has loaded, so call once from
// obs_module_post_load. Without obs-websocket the file still works.
void mgl_battery_status_post_load(void);
// Stops following the battery, empties battery.txt and frees what load allocated. Call once from
// module unload, before the link shuts down.
void mgl_battery_status_free(void);

#endif
