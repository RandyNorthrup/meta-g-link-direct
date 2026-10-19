// SPDX-License-Identifier: AGPL-3.0-only
// The OBS asynchronous video source: it runs the direct glasses media pipeline on its own worker,
// decodes the HEVC frames and hands them to OBS with their timestamps.
#include "hevc_decoder.hpp"

#include "mgl/bytes.hpp"
#include "mgl/media_session.hpp"
#include "mgl/stream.hpp"

#include <obs-module.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <new>
#include <string>

namespace {
constexpr std::uint64_t NanosPerMicro = 1000;

// The glasses hold one session at a time, and OBS creates transient source instances (the add
// dialog's preview, studio-mode duplicates). Only one instance may drive the radio at once, so a
// process-wide guard hands the single capture to whichever instance OBS activates first.
std::mutex g_capture_mutex;
const void* g_capture_owner = nullptr;

// Selectable quality profiles, best first. 720x1280 is the confirmed target; lower profiles ease a
// struggling link. Labels come from the locale file.
struct QualityPreset {
    const char* key;
    mgl::stream::VideoConfig config;
};
constexpr std::array<QualityPreset, 4> kQualityPresets{{
    {"q720",
     {.width = 720, .height = 1280, .bitrate = 4'000'000, .frame_rate = 30, .iframe_interval = 1}},
    {"q1080",
     {.width = 1080, .height = 1920, .bitrate = 4'000'000, .frame_rate = 30, .iframe_interval = 1}},
    {"q504",
     {.width = 504, .height = 896, .bitrate = 1'500'000, .frame_rate = 30, .iframe_interval = 1}},
    {"q360",
     {.width = 360, .height = 640, .bitrate = 600'000, .frame_rate = 24, .iframe_interval = 1}},
}};

struct CameraSource {
    obs_source_t* source = nullptr;
    std::unique_ptr<mgl::plugin::HevcDecoder> decoder;
    std::unique_ptr<mgl::MediaSession> session;
    bool owns_capture = false;
    std::uint64_t received = 0;
    std::uint64_t decoded = 0;
    mgl::stream::VideoConfig config = kQualityPresets[0].config;
    std::atomic<mgl::MediaState> state{mgl::MediaState::Idle};
};

const char* stateText(mgl::MediaState state) {
    switch (state) {
    case mgl::MediaState::Idle:
        return "Status.Idle";
    case mgl::MediaState::Connecting:
        return "Status.Connecting";
    case mgl::MediaState::Streaming:
        return "Status.Streaming";
    case mgl::MediaState::Reconnecting:
        return "Status.Reconnecting";
    case mgl::MediaState::Failed:
        return "Status.Failed";
    case mgl::MediaState::Stopped:
        return "Status.Stopped";
    }
    return "Status.Idle";
}

void outputFrame(CameraSource* self, const mgl::plugin::DecodedFrame& decoded) {
    obs_source_frame frame{};
    frame.format = VIDEO_FORMAT_I420;
    frame.width = decoded.width;
    frame.height = decoded.height;
    frame.timestamp = decoded.pts_us * NanosPerMicro;
    frame.full_range = decoded.full_range;
    for (int plane = 0; plane < 3; ++plane) {
        // OBS copies the planes into its async cache during this call.
        frame.data[plane] = const_cast<std::uint8_t*>(
            decoded.plane[plane]); // NOLINT(cppcoreguidelines-pro-type-const-cast)
        frame.linesize[plane] = decoded.linesize[plane];
    }
    video_format_get_parameters(VIDEO_CS_601,
                                frame.full_range ? VIDEO_RANGE_FULL : VIDEO_RANGE_PARTIAL,
                                frame.color_matrix, frame.color_range_min, frame.color_range_max);
    if (self->decoded <= 1) {
        blog(LOG_INFO,
             "[meta-g-link-direct] output frame %ux%u linesize=%u,%u,%u full_range=%d src=%p",
             decoded.width, decoded.height, decoded.linesize[0], decoded.linesize[1],
             decoded.linesize[2], static_cast<int>(frame.full_range),
             static_cast<void*>(self->source));
    }
    obs_source_output_video(self->source, &frame);
}

const char* sourceGetName(void* /*type_data*/) { return obs_module_text("MetaGLinkCamera"); }

void* sourceCreate(obs_data_t* /*settings*/, obs_source_t* source) {
    auto* self = new (std::nothrow) CameraSource();
    if (self == nullptr) {
        return nullptr;
    }
    self->source = source;
    // Present frames as they arrive: the glasses' presentation clock is unrelated to OBS's, so the
    // default timestamp-buffered path would schedule them far in the future and show nothing.
    obs_source_set_async_unbuffered(source, true);
    // The capture does not start here: OBS makes transient instances at create time. It starts
    // when the source is activated in the program output (see sourceActivate).
    return self;
}

void startCapture(CameraSource* self) {
    {
        const std::scoped_lock lock(g_capture_mutex);
        if (g_capture_owner != nullptr) {
            blog(LOG_INFO, "[meta-g-link-direct] another instance holds the glasses; staying idle");
            return;
        }
        g_capture_owner = self;
        self->owns_capture = true;
    }
    self->decoder = std::make_unique<mgl::plugin::HevcDecoder>();
    (void)self->decoder->open();
    mgl::MediaCallbacks callbacks;
    callbacks.on_parameters = [self](mgl::ByteView params) {
        self->decoder->setParameters(params);
    };
    callbacks.on_frame = [self](std::uint64_t pts, mgl::ByteView annex, bool keyframe) {
        ++self->received;
        self->decoder->decode(pts, annex, keyframe,
                              [self](const mgl::plugin::DecodedFrame& decoded) {
                                  if (self->decoded == 0) {
                                      blog(LOG_INFO, "[meta-g-link-direct] receiving video %ux%u",
                                           decoded.width, decoded.height);
                                  }
                                  ++self->decoded;
                                  outputFrame(self, decoded);
                              });
    };
    callbacks.on_log = [](const std::string& message) {
        blog(LOG_INFO, "[meta-g-link-direct] %s", message.c_str());
    };
    callbacks.on_state = [self](mgl::MediaState state) { self->state.store(state); };
    self->session = std::make_unique<mgl::MediaSession>(std::move(callbacks), self->config);
    self->session->start();
}

void stopCapture(CameraSource* self) {
    if (self->session) {
        self->session->stop();
        self->session.reset();
    }
    if (self->owns_capture) {
        const std::scoped_lock lock(g_capture_mutex);
        self->owns_capture = false;
        if (g_capture_owner == self) {
            g_capture_owner = nullptr;
        }
    }
}

void sourceActivate(void* data) { startCapture(static_cast<CameraSource*>(data)); }
void sourceDeactivate(void* data) { stopCapture(static_cast<CameraSource*>(data)); }

void sourceGetDefaults(obs_data_t* settings) {
    obs_data_set_default_string(settings, "quality", kQualityPresets[0].key);
}

obs_properties_t* sourceGetProperties(void* data) {
    obs_properties_t* props = obs_properties_create();
    obs_property_t* quality = obs_properties_add_list(props, "quality", obs_module_text("Quality"),
                                                      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
    for (const auto& preset : kQualityPresets) {
        obs_property_list_add_string(quality, obs_module_text(preset.key), preset.key);
    }
    obs_property_set_long_description(quality, obs_module_text("Quality.Desc"));
    // A Narrator-readable status line showing the current connection state.
    const auto* self = static_cast<const CameraSource*>(data);
    const char* status_key = self != nullptr ? stateText(self->state.load()) : "Status.Idle";
    obs_properties_add_text(props, "status", obs_module_text(status_key), OBS_TEXT_INFO);
    obs_properties_add_text(props, "note", obs_module_text("SetupNote"), OBS_TEXT_INFO);
    return props;
}

void sourceUpdate(void* data, obs_data_t* settings) {
    auto* self = static_cast<CameraSource*>(data);
    const char* key = obs_data_get_string(settings, "quality");
    for (const auto& preset : kQualityPresets) {
        if (key != nullptr && std::string(key) == preset.key) {
            self->config = preset.config;
            break;
        }
    }
    // A running capture picks up the new quality on its next (re)start.
}

void sourceDestroy(void* data) {
    auto* self = static_cast<CameraSource*>(data);
    if (self == nullptr) {
        return;
    }
    stopCapture(self);
    delete self;
}
} // namespace

extern "C" void mgl_register_camera_source(void);

void mgl_register_camera_source(void) {
    static obs_source_info info{};
    info.id = "meta_g_link_camera";
    info.type = OBS_SOURCE_TYPE_INPUT;
    info.output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_DO_NOT_DUPLICATE;
    info.get_name = sourceGetName;
    info.create = sourceCreate;
    info.destroy = sourceDestroy;
    info.activate = sourceActivate;
    info.deactivate = sourceDeactivate;
    info.get_defaults = sourceGetDefaults;
    info.get_properties = sourceGetProperties;
    info.update = sourceUpdate;
    info.icon_type = OBS_ICON_TYPE_CAMERA;
    obs_register_source(&info);
}
