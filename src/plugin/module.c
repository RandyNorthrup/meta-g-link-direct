// SPDX-License-Identifier: AGPL-3.0-only
#include "battery_status.h"
#include "glasses_link.h"
#include "link_settings.h"
#include "source_ids.h"

#include <obs-frontend-api.h>
#include <obs-module.h>

// How long unload waits for the link to stop the stream and leave the glasses' Wi-Fi network
// before OBS unloads this module's code.
#define MGL_TEARDOWN_TIMEOUT_MS 10000u

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("meta-g-link-direct", "en-US")

// Defined in source.c; registers the glasses camera and microphone sources.
extern void mgl_register_sources(void);

MODULE_EXPORT const char* obs_module_name(void) { return "Meta G Link Direct"; }

MODULE_EXPORT const char* obs_module_description(void) {
    return "Direct Ray-Ban Meta Gen 2 camera source and microphone source over a local wireless "
           "link.";
}

MODULE_EXPORT const char* obs_module_author(void) { return "Meta G Link Direct contributors"; }

static void log_line(const char* message) { blog(LOG_INFO, "[meta-g-link-direct] %s", message); }

static bool find_glasses_source(void* data, obs_source_t* source) {
    obs_source_t** found = data;
    if (mgl_is_glasses_source(source)) {
        *found = obs_source_get_ref(source);
        return *found == NULL; // keep looking only if that source was going away
    }
    return true;
}

// Tools > Ray-Ban Meta Glasses (Direct): opens the first glasses source's properties, where every
// glasses setting lives. Without one, a camera source is added to the current scene first.
static void open_glasses_settings(void* data) {
    (void)data;
    obs_source_t* glasses = NULL;
    obs_enum_sources(find_glasses_source, &glasses);
    if (glasses == NULL) {
        obs_source_t* scene_source = obs_frontend_get_current_scene();
        obs_scene_t* scene = obs_scene_from_source(scene_source);
        if (scene == NULL) {
            blog(LOG_WARNING, "[meta-g-link-direct] No current scene to add the glasses to.");
            obs_source_release(scene_source);
            return;
        }
        glasses =
            obs_source_create(MGL_CAMERA_SOURCE_ID, obs_module_text("MetaGLinkCamera"), NULL, NULL);
        if (glasses != NULL) {
            obs_scene_add(scene, glasses);
        }
        obs_source_release(scene_source);
    }
    if (glasses != NULL) {
        obs_frontend_open_source_properties(glasses);
        obs_source_release(glasses);
    }
}

bool obs_module_load(void) {
    mglLinkInit(log_line);
    mgl_link_settings_load();
    mgl_battery_status_load();
    mgl_register_sources();
    obs_frontend_add_tools_menu_item(obs_module_text("ToolsMenu"), open_glasses_settings, NULL);
    blog(LOG_INFO, "[meta-g-link-direct] %s loaded; camera and microphone sources registered",
         MGL_VERSION);
    return true;
}

void obs_module_post_load(void) { mgl_battery_status_post_load(); }

void obs_module_unload(void) {
    mgl_battery_status_free();
    if (!mglLinkShutdown(MGL_TEARDOWN_TIMEOUT_MS)) {
        blog(LOG_WARNING, "[meta-g-link-direct] glasses teardown did not finish before unload");
    }
    mgl_link_settings_free();
}
