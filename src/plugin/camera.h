// SPDX-License-Identifier: AGPL-3.0-only
// The C boundary between the OBS glue (source.c, C17) and the camera core (camera.cpp, C++20).
// OBS headers stay on the C side: they use the MSVC _udiv128 intrinsic, which clang cannot parse
// (LLVM issue 59168), and the C++ side must stay checkable by clang-tidy.
#ifndef MGL_PLUGIN_CAMERA_H
#define MGL_PLUGIN_CAMERA_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
extern "C" {
#else
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#endif

// One decoded I420 picture. The planes are valid only for the duration of the callback.
struct MglCameraFrame {
    const uint8_t* plane_y;
    const uint8_t* plane_u;
    const uint8_t* plane_v;
    uint32_t stride_y;
    uint32_t stride_u;
    uint32_t stride_v;
    uint32_t width;
    uint32_t height;
    uint64_t timestamp_ns;
    bool full_range;
};

struct MglCamera;

// Returns NULL if the camera cannot be allocated. Both callbacks run on the camera's worker
// thread; context is passed back to on_frame unchanged.
struct MglCamera* mglCameraCreate(void (*on_frame)(void* context, const struct MglCameraFrame* frame),
                                  void (*on_log)(const char* message), void* context);
// Stops any capture and frees the camera. NULL is ignored. Returns without waiting for the radio:
// the link is torn down on a background thread (see mglCameraShutdown).
void mglCameraDestroy(struct MglCamera* camera);
// Starts the capture. If another camera in this process holds the glasses, this one waits for
// them (status Waiting) instead of competing. Returns at once.
void mglCameraStart(struct MglCamera* camera);
// Stops the capture. Returns at once, so OBS's video thread never waits on the radio; frames stop
// reaching OBS immediately and the link is torn down on a background thread.
void mglCameraStop(struct MglCamera* camera);
// Waits up to timeout_ms for stopped cameras to finish tearing the link down (stop the stream,
// leave the glasses' Wi-Fi network). Call once from module unload. False if time ran out.
bool mglCameraShutdown(uint32_t timeout_ms);

// Locale key of the quality preset at index, or NULL past the last one. Index 0 is the default.
const char* mglCameraQualityKey(size_t index);
// Selects a quality preset by locale key; NULL or unknown keys are ignored. A running capture
// picks up the new preset on its next start.
void mglCameraSetQuality(struct MglCamera* camera, const char* key);
// Locale key describing the current connection state. NULL reports the idle state.
const char* mglCameraStatusKey(const struct MglCamera* camera);

#ifdef __cplusplus
}
#endif
#endif
