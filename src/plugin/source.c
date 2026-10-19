// SPDX-License-Identifier: AGPL-3.0-only
// The OBS sources for the glasses: the camera (video, optionally with the glasses' microphones) and
// the microphone on its own. This file holds only the OBS callbacks, the properties and the media
// hand-off; the shared glasses link is the C++ core behind glasses_link.h.
#include "glasses_link.h"
#include "link_settings.h"
#include "source_ids.h"

#include <media-io/audio-io.h>
#include <media-io/video-io.h>
#include <obs-module.h>
#include <util/bmem.h>

#include <stdio.h>

#define MGL_SETTING_QUALITY "quality"
#define MGL_SETTING_GLASSES_AUDIO "glasses_audio"
#define MGL_SETTING_FORGET_CONFIRM "forget_confirm"

// The adapters shown in the Wi-Fi adapter list; PCs rarely have more than one or two.
#define MGL_MAX_WIFI_ADAPTERS 8

struct glasses_source {
    obs_source_t* source;
    struct MglSource* link;
    bool camera; // the camera source; otherwise the microphone source
};

// Runs on the link's worker thread for each decoded picture.
static void output_frame(void* context, const struct MglVideoFrame* picture) {
    obs_source_t* source = context;
    struct obs_source_frame frame = {0};
    frame.format = VIDEO_FORMAT_I420;
    frame.width = picture->width;
    frame.height = picture->height;
    frame.timestamp = picture->timestamp_ns;
    frame.full_range = picture->full_range;
    // obs_source_frame takes mutable plane pointers but only reads them: OBS copies the planes
    // into its async cache during obs_source_output_video.
    frame.data[0] = (uint8_t*)picture->plane_y;
    frame.data[1] = (uint8_t*)picture->plane_u;
    frame.data[2] = (uint8_t*)picture->plane_v;
    frame.linesize[0] = picture->stride_y;
    frame.linesize[1] = picture->stride_u;
    frame.linesize[2] = picture->stride_v;
    video_format_get_parameters(VIDEO_CS_601,
                                frame.full_range ? VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL,
                                frame.color_matrix, frame.color_range_min, frame.color_range_max);
    obs_source_output_video(source, &frame);
}

// Runs on the link's worker thread for each block of decoded glasses audio (mono or stereo).
static void output_audio(void* context, const struct MglAudioBlock* sound) {
    obs_source_t* source = context;
    struct obs_source_audio audio = {0};
    for (uint32_t plane = 0; plane < sound->channels && plane < MAX_AV_PLANES; ++plane) {
        audio.data[plane] = (const uint8_t*)sound->planes[plane];
    }
    audio.frames = sound->frames;
    audio.speakers = sound->channels == 1 ? SPEAKERS_MONO : SPEAKERS_STEREO;
    audio.format = AUDIO_FORMAT_FLOAT_PLANAR;
    audio.samples_per_sec = sound->sample_rate;
    audio.timestamp = sound->timestamp_ns;
    obs_source_output_audio(source, &audio);
}

static void log_line(const char* message) { blog(LOG_INFO, "[meta-g-link-direct] %s", message); }

static const char* camera_get_name(void* type_data) {
    (void)type_data;
    return obs_module_text("MetaGLinkCamera");
}

static const char* microphone_get_name(void* type_data) {
    (void)type_data;
    return obs_module_text("MetaGLinkMicrophone");
}

static void apply_source_settings(struct glasses_source* glasses, obs_data_t* settings) {
    if (glasses->camera) {
        mglSourceSetQuality(glasses->link, obs_data_get_string(settings, MGL_SETTING_QUALITY));
        mglSourceSetMedia(glasses->link, true,
                          obs_data_get_bool(settings, MGL_SETTING_GLASSES_AUDIO));
    } else {
        mglSourceSetMedia(glasses->link, false, true);
    }
}

static void* glasses_create(obs_data_t* settings, obs_source_t* source, bool camera) {
    struct glasses_source* glasses = bzalloc(sizeof(struct glasses_source));
    glasses->source = source;
    glasses->camera = camera;
    glasses->link = mglSourceCreate(camera ? output_frame : NULL, output_audio, source);
    if (glasses->link == NULL) {
        bfree(glasses);
        return NULL;
    }
    // A stored tick must not leave Forget armed when the properties next open.
    obs_data_set_bool(settings, MGL_SETTING_FORGET_CONFIRM, false);
    mgl_link_settings_adopt(settings);
    apply_source_settings(glasses, settings);
    if (camera) {
        // Present frames as they arrive: the glasses' presentation clock is unrelated to OBS's,
        // so the default timestamp-buffered path would schedule them far in the future.
        obs_source_set_async_unbuffered(source, true);
    }
    // Nothing streams yet: OBS makes transient instances at create time. A source starts the
    // stream when it goes live in the program output (see glasses_activate).
    return glasses;
}

static void* camera_create(obs_data_t* settings, obs_source_t* source) {
    return glasses_create(settings, source, true);
}

static void* microphone_create(obs_data_t* settings, obs_source_t* source) {
    return glasses_create(settings, source, false);
}

static void glasses_destroy(void* data) {
    struct glasses_source* glasses = data;
    mglSourceDestroy(glasses->link);
    bfree(glasses);
}

// cppcheck-suppress constParameterCallback ; obs_source_info fixes this signature as (void *data).
static void glasses_activate(void* data) {
    const struct glasses_source* glasses = data;
    mglSourceSetLive(glasses->link, true);
}

// cppcheck-suppress constParameterCallback ; obs_source_info fixes this signature as (void *data).
static void glasses_deactivate(void* data) {
    const struct glasses_source* glasses = data;
    mglSourceSetLive(glasses->link, false);
}

static void glasses_update(void* data, obs_data_t* settings) {
    struct glasses_source* glasses = data;
    // Applied at once: a live stream changes in place in about a second.
    apply_source_settings(glasses, settings);
    mgl_link_settings_update(glasses->source, settings);
}

static void shared_defaults(obs_data_t* settings) {
    obs_data_set_default_string(settings, MGL_SETTING_WIFI_ADAPTER, "");
    obs_data_set_default_bool(settings, MGL_SETTING_KEEP_READY, false);
    obs_data_set_default_bool(settings, MGL_SETTING_FORGET_CONFIRM, false);
}

static void camera_get_defaults(obs_data_t* settings) {
    obs_data_set_default_string(settings, MGL_SETTING_QUALITY, mglQualityKey(0));
    obs_data_set_default_bool(settings, MGL_SETTING_GLASSES_AUDIO, false);
    shared_defaults(settings);
}

static void add_wifi_adapter_list(obs_properties_t* props) {
    obs_property_t* list =
        obs_properties_add_list(props, MGL_SETTING_WIFI_ADAPTER, obs_module_text("WifiAdapter"),
                                OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    obs_property_list_add_string(list, obs_module_text("WifiAdapter.Auto"), "");
    struct MglWifiAdapter adapters[MGL_MAX_WIFI_ADAPTERS];
    size_t count = 0;
    if (!mglWifiAdapterList(adapters, MGL_MAX_WIFI_ADAPTERS, &count)) {
        blog(LOG_WARNING, "[meta-g-link-direct] Could not list the Wi-Fi adapters.");
    }
    for (size_t index = 0; index < count; ++index) {
        obs_property_list_add_string(list, adapters[index].name, adapters[index].id);
    }
    obs_property_set_long_description(list, obs_module_text("WifiAdapter.Desc"));
}

// Shows the outcome of a setup button in the result line, for sighted and Narrator users alike.
static void show_result(obs_properties_t* props, const char* key) {
    obs_property_t* result = obs_properties_get(props, "setup_result");
    obs_property_set_description(result, obs_module_text(key));
    obs_property_set_visible(result, true);
}

static bool setup_clicked(obs_properties_t* props, obs_property_t* property, void* data) {
    (void)property;
    (void)data;
    show_result(props, mglSetupStart());
    return true;
}

// cppcheck-suppress constParameterCallback ; obs_property_clicked_t fixes this as (void *data).
static bool forget_clicked(obs_properties_t* props, obs_property_t* property, void* data) {
    (void)property;
    const struct glasses_source* glasses = data;
    obs_data_t* settings = obs_source_get_settings(glasses->source);
    const bool confirmed = obs_data_get_bool(settings, MGL_SETTING_FORGET_CONFIRM);
    obs_data_release(settings);
    if (!confirmed) {
        show_result(props, "Forget.NeedsConfirm");
        return true;
    }
    show_result(props, mglForgetGlasses() ? "Forget.Done" : "Forget.Failed");
    obs_data_t* patch = obs_data_create();
    obs_data_set_bool(patch, MGL_SETTING_FORGET_CONFIRM, false);
    obs_source_update(glasses->source, patch);
    obs_data_release(patch);
    return true;
}

// Forget works only while its confirmation is ticked; OBS properties cannot show a dialog.
static bool forget_confirm_changed(void* data, obs_properties_t* props, obs_property_t* property,
                                   obs_data_t* settings) {
    (void)data;
    (void)property;
    obs_property_set_enabled(obs_properties_get(props, "forget"),
                             obs_data_get_bool(settings, MGL_SETTING_FORGET_CONFIRM));
    return true;
}

// The shared connection settings and the setup controls, the same in every glasses source.
static void add_link_groups(obs_properties_t* props, struct glasses_source* glasses) {
    obs_properties_t* connection = obs_properties_create();
    add_wifi_adapter_list(connection);
    obs_property_t* keep =
        obs_properties_add_bool(connection, MGL_SETTING_KEEP_READY, obs_module_text("KeepReady"));
    obs_property_set_long_description(keep, obs_module_text("KeepReady.Desc"));
    // Narrator-readable status lines: the link's state and, while connected, the glasses' battery,
    // both as they were when the properties opened.
    obs_properties_add_text(connection, "status", obs_module_text(mglLinkStatusKey()),
                            OBS_TEXT_INFO);
    bool charging = false;
    const int battery = mglLinkBattery(&charging);
    if (battery >= 0) {
        char line[128];
        snprintf(line, sizeof(line), "%s %d%%%s", obs_module_text("Battery"), battery,
                 charging ? obs_module_text("Battery.Charging") : "");
        obs_properties_add_text(connection, "battery", line, OBS_TEXT_INFO);
    }
    obs_properties_add_group(props, "connection_group", obs_module_text("Group.Connection"),
                             OBS_GROUP_NORMAL, connection);

    obs_properties_t* setup = obs_properties_create();
    obs_properties_add_text(setup, "note", obs_module_text("SetupNote"), OBS_TEXT_INFO);
    obs_property_t* start = obs_properties_add_button2(setup, "setup", obs_module_text("Setup"),
                                                       setup_clicked, glasses);
    obs_property_set_long_description(start, obs_module_text("Setup.Desc"));
    obs_property_t* confirm = obs_properties_add_bool(setup, MGL_SETTING_FORGET_CONFIRM,
                                                      obs_module_text("Forget.Confirm"));
    obs_property_set_modified_callback2(confirm, forget_confirm_changed, glasses);
    obs_property_t* forget = obs_properties_add_button2(setup, "forget", obs_module_text("Forget"),
                                                        forget_clicked, glasses);
    obs_property_set_long_description(forget, obs_module_text("Forget.Desc"));
    obs_property_set_enabled(forget, false);
    obs_property_t* result = obs_properties_add_text(setup, "setup_result", "", OBS_TEXT_INFO);
    obs_property_set_visible(result, false);
    obs_properties_add_group(props, "setup_group", obs_module_text("Group.Setup"), OBS_GROUP_NORMAL,
                             setup);
}

static obs_properties_t* camera_get_properties(void* data) {
    obs_properties_t* props = obs_properties_create();

    obs_properties_t* video = obs_properties_create();
    obs_property_t* quality =
        obs_properties_add_list(video, MGL_SETTING_QUALITY, obs_module_text("Quality"),
                                OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    for (size_t index = 0; mglQualityKey(index) != NULL; ++index) {
        const char* key = mglQualityKey(index);
        obs_property_list_add_string(quality, obs_module_text(key), key);
    }
    obs_property_set_long_description(quality, obs_module_text("Quality.Desc"));
    obs_properties_add_group(props, "video_group", obs_module_text("Group.Video"), OBS_GROUP_NORMAL,
                             video);

    obs_properties_t* audio = obs_properties_create();
    obs_property_t* glasses_audio =
        obs_properties_add_bool(audio, MGL_SETTING_GLASSES_AUDIO, obs_module_text("GlassesAudio"));
    obs_property_set_long_description(glasses_audio, obs_module_text("GlassesAudio.Desc"));
    obs_properties_add_text(audio, "audio_hint", obs_module_text("GlassesAudio.Hint"),
                            OBS_TEXT_INFO);
    obs_properties_add_group(props, "audio_group", obs_module_text("Group.Audio"), OBS_GROUP_NORMAL,
                             audio);

    add_link_groups(props, data);
    return props;
}

static obs_properties_t* microphone_get_properties(void* data) {
    obs_properties_t* props = obs_properties_create();
    obs_properties_add_text(props, "microphone_note", obs_module_text("Microphone.Note"),
                            OBS_TEXT_INFO);
    add_link_groups(props, data);
    return props;
}

// Called once from obs_module_load in module.c.
void mgl_register_sources(void) {
    static struct obs_source_info camera = {
        .id = MGL_CAMERA_SOURCE_ID,
        .type = OBS_SOURCE_TYPE_INPUT,
        // OBS_SOURCE_AUDIO puts the camera in the audio mixer; it stays silent unless glasses audio
        // is turned on in its properties.
        .output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE,
        .get_name = camera_get_name,
        .create = camera_create,
        .destroy = glasses_destroy,
        .activate = glasses_activate,
        .deactivate = glasses_deactivate,
        .get_defaults = camera_get_defaults,
        .get_properties = camera_get_properties,
        .update = glasses_update,
        .icon_type = OBS_ICON_TYPE_CAMERA,
    };
    static struct obs_source_info microphone = {
        .id = MGL_MICROPHONE_SOURCE_ID,
        .type = OBS_SOURCE_TYPE_INPUT,
        .output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE,
        .get_name = microphone_get_name,
        .create = microphone_create,
        .destroy = glasses_destroy,
        .activate = glasses_activate,
        .deactivate = glasses_deactivate,
        .get_defaults = shared_defaults,
        .get_properties = microphone_get_properties,
        .update = glasses_update,
        .icon_type = OBS_ICON_TYPE_AUDIO_INPUT,
    };
    obs_register_source(&camera);
    obs_register_source(&microphone);
}
