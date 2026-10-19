// SPDX-License-Identifier: AGPL-3.0-only
// The shared glasses link settings; see link_settings.h.
#include "link_settings.h"

#include "glasses_link.h"
#include "source_ids.h"

#include <obs-module.h>
#include <util/bmem.h>
#include <util/platform.h>

#include <string.h>

#define MGL_LINK_SETTINGS_FILE "link.json"
// More glasses sources than this in one OBS is not a real setup; the rest keep their own copy
// until their properties are next opened.
#define MGL_MAX_MIRRORED_SOURCES 32

static obs_data_t* shared = NULL;
// False until the shared values have been saved once, so that an upgrade can adopt the values a
// source still holds from a version that kept them per source.
static bool shared_saved = false;

static void apply_to_link(void) {
    mglLinkSetWifiAdapter(obs_data_get_string(shared, MGL_SETTING_WIFI_ADAPTER));
    mglLinkSetKeepReady(obs_data_get_bool(shared, MGL_SETTING_KEEP_READY));
}

static void save_shared(void) {
    char* directory = obs_module_config_path("");
    char* path = obs_module_config_path(MGL_LINK_SETTINGS_FILE);
    if (directory == NULL || path == NULL) {
        blog(LOG_WARNING, "[meta-g-link-direct] No config folder for the shared glasses settings.");
    } else if (os_mkdirs(directory) == MKDIR_ERROR) {
        blog(LOG_WARNING, "[meta-g-link-direct] Could not create the plugin config folder.");
    } else if (!obs_data_save_json_safe(shared, path, "tmp", "bak")) {
        blog(LOG_WARNING, "[meta-g-link-direct] Could not save the shared glasses settings.");
    } else {
        shared_saved = true;
    }
    bfree(path);
    bfree(directory);
}

void mgl_link_settings_load(void) {
    char* path = obs_module_config_path(MGL_LINK_SETTINGS_FILE);
    obs_data_t* loaded = NULL;
    if (path != NULL && os_file_exists(path)) {
        loaded = obs_data_create_from_json_file_safe(path, "bak");
        if (loaded == NULL) {
            blog(LOG_WARNING, "[meta-g-link-direct] The shared glasses settings could not be "
                              "read; using the defaults.");
        }
    }
    bfree(path);
    shared_saved = loaded != NULL;
    shared = loaded != NULL ? loaded : obs_data_create();
    obs_data_set_default_string(shared, MGL_SETTING_WIFI_ADAPTER, "");
    obs_data_set_default_bool(shared, MGL_SETTING_KEEP_READY, false);
    apply_to_link();
}

void mgl_link_settings_free(void) {
    obs_data_release(shared);
    shared = NULL;
}

static bool same_values(obs_data_t* left, obs_data_t* right) {
    return strcmp(obs_data_get_string(left, MGL_SETTING_WIFI_ADAPTER),
                  obs_data_get_string(right, MGL_SETTING_WIFI_ADAPTER)) == 0 &&
           obs_data_get_bool(left, MGL_SETTING_KEEP_READY) ==
               obs_data_get_bool(right, MGL_SETTING_KEEP_READY);
}

static void copy_values(obs_data_t* target, obs_data_t* origin) {
    obs_data_set_string(target, MGL_SETTING_WIFI_ADAPTER,
                        obs_data_get_string(origin, MGL_SETTING_WIFI_ADAPTER));
    obs_data_set_bool(target, MGL_SETTING_KEEP_READY,
                      obs_data_get_bool(origin, MGL_SETTING_KEEP_READY));
}

void mgl_link_settings_adopt(obs_data_t* source_settings) {
    if (shared == NULL) {
        return;
    }
    if (!shared_saved) {
        copy_values(shared, source_settings);
        save_shared();
        apply_to_link();
        return;
    }
    copy_values(source_settings, shared);
}

bool mgl_is_glasses_source(const obs_source_t* source) {
    const char* id = obs_source_get_unversioned_id(source);
    return id != NULL &&
           (strcmp(id, MGL_CAMERA_SOURCE_ID) == 0 || strcmp(id, MGL_MICROPHONE_SOURCE_ID) == 0);
}

struct mirror_targets {
    obs_source_t* origin;
    obs_source_t* found[MGL_MAX_MIRRORED_SOURCES];
    size_t count;
};

static bool collect_glasses_source(void* data, obs_source_t* source) {
    struct mirror_targets* targets = data;
    if (source != targets->origin && mgl_is_glasses_source(source) &&
        targets->count < MGL_MAX_MIRRORED_SOURCES) {
        obs_source_t* held = obs_source_get_ref(source);
        if (held != NULL) {
            targets->found[targets->count++] = held;
        }
    }
    return true;
}

void mgl_link_settings_update(obs_source_t* origin, obs_data_t* source_settings) {
    if (shared == NULL || same_values(shared, source_settings)) {
        return;
    }
    copy_values(shared, source_settings);
    save_shared();
    apply_to_link();
    // Copy into the other glasses sources after the enumeration, not inside it: an update runs
    // their callbacks, which then find the values equal and stop.
    struct mirror_targets targets = {.origin = origin, .count = 0};
    obs_enum_sources(collect_glasses_source, &targets);
    obs_data_t* patch = obs_data_create();
    copy_values(patch, shared);
    for (size_t index = 0; index < targets.count; ++index) {
        obs_source_update(targets.found[index], patch);
        obs_source_release(targets.found[index]);
    }
    obs_data_release(patch);
}
