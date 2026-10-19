// SPDX-License-Identifier: AGPL-3.0-only
// The OBS asynchronous video source for the direct glasses camera. This file holds only the OBS
// callbacks and the frame hand-off; the camera itself is the C++ core behind camera.h.
#include "camera.h"

#include <media-io/video-io.h>
#include <obs-module.h>

static void log_line(const char* message) { blog(LOG_INFO, "[meta-g-link-direct] %s", message); }

// Runs on the camera worker thread for each decoded picture.
static void output_frame(void* context, const struct MglCameraFrame* picture) {
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

static const char* source_get_name(void* type_data) {
    (void)type_data;
    return obs_module_text("MetaGLinkCamera");
}

static void* source_create(obs_data_t* settings, obs_source_t* source) {
    struct MglCamera* camera = mglCameraCreate(output_frame, log_line, source);
    if (camera == NULL) {
        return NULL;
    }
    mglCameraSetQuality(camera, obs_data_get_string(settings, "quality"));
    // Present frames as they arrive: the glasses' presentation clock is unrelated to OBS's, so the
    // default timestamp-buffered path would schedule them far in the future and show nothing.
    obs_source_set_async_unbuffered(source, true);
    // The capture does not start here: OBS makes transient instances at create time. It starts
    // when the source is activated in the program output (see source_activate).
    return camera;
}

static void source_destroy(void* data) { mglCameraDestroy(data); }

static void source_activate(void* data) { mglCameraStart(data); }

static void source_deactivate(void* data) { mglCameraStop(data); }

static void source_get_defaults(obs_data_t* settings) {
    obs_data_set_default_string(settings, "quality", mglCameraQualityKey(0));
}

// cppcheck-suppress constParameterCallback ; obs_source_info fixes this signature as (void *data).
static obs_properties_t* source_get_properties(void* data) {
    obs_properties_t* props = obs_properties_create();
    obs_property_t* quality = obs_properties_add_list(props, "quality", obs_module_text("Quality"),
                                                      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    for (size_t index = 0; mglCameraQualityKey(index) != NULL; ++index) {
        const char* key = mglCameraQualityKey(index);
        obs_property_list_add_string(quality, obs_module_text(key), key);
    }
    obs_property_set_long_description(quality, obs_module_text("Quality.Desc"));
    // A Narrator-readable status line showing the current connection state.
    obs_properties_add_text(props, "status", obs_module_text(mglCameraStatusKey(data)),
                            OBS_TEXT_INFO);
    obs_properties_add_text(props, "note", obs_module_text("SetupNote"), OBS_TEXT_INFO);
    return props;
}

static void source_update(void* data, obs_data_t* settings) {
    // A running capture picks up the new quality on its next start.
    mglCameraSetQuality(data, obs_data_get_string(settings, "quality"));
}

// Called once from obs_module_load in module.c.
void mgl_register_camera_source(void) {
    static struct obs_source_info info = {
        .id = "meta_g_link_camera",
        .type = OBS_SOURCE_TYPE_INPUT,
        .output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_DO_NOT_DUPLICATE,
        .get_name = source_get_name,
        .create = source_create,
        .destroy = source_destroy,
        .activate = source_activate,
        .deactivate = source_deactivate,
        .get_defaults = source_get_defaults,
        .get_properties = source_get_properties,
        .update = source_update,
        .icon_type = OBS_ICON_TYPE_CAMERA,
    };
    obs_register_source(&info);
}
