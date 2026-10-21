// SPDX-License-Identifier: AGPL-3.0-only
// The glasses' battery file and obs-websocket vendor; see battery_status.h.
#include "battery_status.h"

#include "glasses_link.h"

#include <obs-frontend-api.h>
#include <obs-module.h>
#include <util/bmem.h>
#include <util/platform.h>

// obs-websocket-api.h, from the pinned OBS sources, uses assert and strlen without including them.
#include <assert.h>
#include <string.h>

#include <obs-websocket-api.h>

#define MGL_BATTERY_FILE "battery.txt"
#define MGL_WEBSOCKET_VENDOR "meta-g-link-direct"
#define MGL_BATTERY_REQUEST "GetBattery"
#define MGL_BATTERY_EVENT "BatteryChanged"
// A reader that has the file open can make one replacement fail; it is tried again this often,
// this far apart, before the change is given up until the next one.
#define MGL_BATTERY_WRITE_ATTEMPTS 3
#define MGL_BATTERY_WRITE_PAUSE_MS 20

static char* battery_path = NULL;
static obs_websocket_vendor vendor = NULL;
static bool following = false;
// One warning per run of failed writes, not one per battery report.
static bool write_failure_reported = false;

// Replaces the file in one step, so a reader sees the old text or the new, never a part.
static void write_battery_file(const char* text) {
    if (battery_path == NULL) {
        return;
    }
    for (int attempt = 0; attempt < MGL_BATTERY_WRITE_ATTEMPTS; ++attempt) {
        if (os_quick_write_utf8_file_safe(battery_path, text, strlen(text), false, "tmp", NULL)) {
            write_failure_reported = false;
            return;
        }
        os_sleep_ms(MGL_BATTERY_WRITE_PAUSE_MS);
    }
    if (!write_failure_reported) {
        write_failure_reported = true;
        blog(LOG_WARNING,
             "[meta-g-link-direct] Could not write %s in the plugin config folder; "
             "it is tried again at the next battery change.",
             MGL_BATTERY_FILE);
    }
}

// The data of GetBattery and BatteryChanged: {"available": false} without a link or before the
// glasses' first report, otherwise {"available": true, "percent": 87, "charging": false}.
static void fill_battery(obs_data_t* data, bool known, uint32_t percent, bool charging) {
    obs_data_set_bool(data, "available", known);
    if (known) {
        obs_data_set_int(data, "percent", (long long)percent);
        obs_data_set_bool(data, "charging", charging);
    }
}

// The link's battery listener, on the link's threads, one call at a time.
static void battery_changed(void* context, const struct MglBattery* battery) {
    (void)context;
    write_battery_file(battery->text);
    if (vendor != NULL) {
        obs_data_t* event = obs_data_create();
        fill_battery(event, battery->known, battery->percent, battery->charging);
        obs_websocket_vendor_emit_event(vendor, MGL_BATTERY_EVENT, event);
        obs_data_release(event);
    }
}

// obs-websocket's request thread.
static void get_battery(obs_data_t* request, obs_data_t* response, void* data) {
    (void)request;
    (void)data;
    bool charging = false;
    const int percent = mglLinkBattery(&charging);
    fill_battery(response, percent >= 0, percent >= 0 ? (uint32_t)percent : 0u, charging);
}

// Stops the battery calls and the vendor request while obs-websocket is still loaded: OBS may
// unload it before this module.
static void stop_following(void) {
    if (!following) {
        return;
    }
    following = false;
    mglLinkSetBatteryListener(NULL, NULL); // waits for a call in progress
    if (vendor != NULL) {
        obs_websocket_vendor_unregister_request(vendor, MGL_BATTERY_REQUEST);
        vendor = NULL;
    }
    write_battery_file(""); // the link is going down
}

static void frontend_event(enum obs_frontend_event event, void* data) {
    (void)data;
    if (event == OBS_FRONTEND_EVENT_EXIT) {
        stop_following();
    }
}

void mgl_battery_status_load(void) {
    char* directory = obs_module_config_path("");
    battery_path = obs_module_config_path(MGL_BATTERY_FILE);
    if (directory == NULL || battery_path == NULL || os_mkdirs(directory) == MKDIR_ERROR) {
        blog(LOG_WARNING, "[meta-g-link-direct] No plugin config folder for %s.", MGL_BATTERY_FILE);
        bfree(battery_path);
        battery_path = NULL;
    }
    bfree(directory);
    write_battery_file(""); // unknown until the glasses report over a link
    obs_frontend_add_event_callback(frontend_event, NULL);
}

// No glasses source, and so no link, exists before every module has loaded, so the battery calls
// start here, after the vendor is set, and never race its assignment.
void mgl_battery_status_post_load(void) {
    vendor = obs_websocket_register_vendor(MGL_WEBSOCKET_VENDOR);
    if (vendor == NULL) {
        blog(LOG_INFO,
             "[meta-g-link-direct] obs-websocket is not available, so its clients "
             "cannot ask for the glasses' battery; %s still has it.",
             MGL_BATTERY_FILE);
    } else if (!obs_websocket_vendor_register_request(vendor, MGL_BATTERY_REQUEST, get_battery,
                                                      NULL)) {
        blog(LOG_WARNING, "[meta-g-link-direct] obs-websocket refused the %s request.",
             MGL_BATTERY_REQUEST);
    }
    following = true;
    mglLinkSetBatteryListener(battery_changed, NULL);
}

void mgl_battery_status_free(void) {
    obs_frontend_remove_event_callback(frontend_event, NULL);
    if (following) {
        // OBS's exit event did not come, so obs-websocket may already be unloaded: the vendor is
        // left alone and only the battery calls and the file are stopped.
        following = false;
        mglLinkSetBatteryListener(NULL, NULL);
        vendor = NULL;
        write_battery_file("");
    }
    bfree(battery_path);
    battery_path = NULL;
}
