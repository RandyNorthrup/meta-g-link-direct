// SPDX-License-Identifier: AGPL-3.0-only
#include <obs-module.h>

OBS_DECLARE_MODULE()

MODULE_EXPORT const char* obs_module_name(void) { return "Meta G Link Direct"; }

MODULE_EXPORT const char* obs_module_description(void) {
    return "Direct Gen 2 glasses development foundation. Camera source is not implemented yet.";
}

MODULE_EXPORT const char* obs_module_author(void) { return "Meta G Link Direct contributors"; }

bool obs_module_load(void) {
    blog(LOG_INFO, "[meta-g-link-direct] Foundation %s loaded; camera source pending", MGL_VERSION);
    return true;
}
