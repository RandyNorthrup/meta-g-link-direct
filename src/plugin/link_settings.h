// SPDX-License-Identifier: AGPL-3.0-only
// The settings every glasses source shares: the Wi-Fi adapter and whether the link stays ready.
// They live once, in this plugin's OBS config folder (link.json), and every glasses source shows
// them in its properties. A change made in one source's properties is saved, applied to the link
// and copied into the other glasses sources.
#ifndef MGL_PLUGIN_LINK_SETTINGS_H
#define MGL_PLUGIN_LINK_SETTINGS_H

#include <obs-module.h>

#define MGL_SETTING_WIFI_ADAPTER "wifi_adapter"
#define MGL_SETTING_KEEP_READY "keep_ready"

// Reads link.json and applies it to the link. Call once from module load, after mglLinkInit.
void mgl_link_settings_load(void);
// Frees the in-memory copy. Call once from module unload.
void mgl_link_settings_free(void);
// From a source's create callback: the source shows the shared values. The first source after an
// upgrade from a version that kept these per source hands its values to the shared settings.
void mgl_link_settings_adopt(obs_data_t* source_settings);
// From a source's update callback: a shared value that differs was changed by the user in origin's
// properties; it is saved, applied and copied into the other glasses sources.
void mgl_link_settings_update(obs_source_t* origin, obs_data_t* source_settings);
// True for this plugin's camera and microphone source ids.
bool mgl_is_glasses_source(const obs_source_t* source);

#endif
