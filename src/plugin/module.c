// SPDX-License-Identifier: AGPL-3.0-only
#include "camera.h"

#include <obs-module.h>

// How long unload waits for stopped cameras to stop the stream and leave the glasses' Wi-Fi
// network before OBS unloads this module's code.
#define MGL_TEARDOWN_TIMEOUT_MS 10000u

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("meta-g-link-direct", "en-US")

// Defined in source.c; registers the direct Gen 2 glasses camera source.
extern void mgl_register_camera_source(void);

MODULE_EXPORT const char* obs_module_name(void) { return "Meta G Link Direct"; }

MODULE_EXPORT const char* obs_module_description(void) {
    return "Direct Ray-Ban Meta Gen 2 camera source over a local wireless link.";
}

MODULE_EXPORT const char* obs_module_author(void) { return "Meta G Link Direct contributors"; }

bool obs_module_load(void) {
    mgl_register_camera_source();
    blog(LOG_INFO, "[meta-g-link-direct] %s loaded; camera source registered", MGL_VERSION);
    return true;
}

void obs_module_unload(void) {
    if (!mglCameraShutdown(MGL_TEARDOWN_TIMEOUT_MS)) {
        blog(LOG_WARNING, "[meta-g-link-direct] glasses teardown did not finish before unload");
    }
}
