// SPDX-License-Identifier: AGPL-3.0-only
// The C boundary between the OBS glue (C17: module.c, source.c, link_settings.c) and the glasses
// link core (glasses_link.cpp, C++20). OBS headers stay on the C side: they use the MSVC _udiv128
// intrinsic, which clang cannot parse (LLVM issue 59168), and the C++ side must stay checkable by
// clang-tidy.
//
// The glasses accept one session at a time, so the process holds one shared link. Every glasses
// source in OBS registers with it and says what it shows; the link streams what the live sources
// show, changes the stream in place when that changes, and decodes each medium once for all of
// them.
#ifndef MGL_PLUGIN_GLASSES_LINK_H
#define MGL_PLUGIN_GLASSES_LINK_H

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
struct MglVideoFrame {
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

// A block of decoded glasses audio as 32-bit float planar samples, one plane per channel. The
// planes are valid only for the duration of the callback.
struct MglAudioBlock {
    const float* planes[8]; // NOLINT(modernize-avoid-c-arrays) — C ABI; OBS takes up to 8 planes
    uint32_t channels;
    uint32_t frames; // samples per channel
    uint32_t sample_rate;
    uint64_t timestamp_ns;
};

struct MglSource;

// Called once from module load, before any source exists. on_log may be NULL.
void mglLinkInit(void (*on_log)(const char* message));
// Stops the link and waits up to timeout_ms for it to stop the stream and leave the glasses'
// Wi-Fi network. Call once from module unload. False if time ran out.
bool mglLinkShutdown(uint32_t timeout_ms);

// Registers an OBS source with the link. on_frame or on_audio may be NULL for a medium the source
// never shows. Both run on the link's worker thread with context passed back unchanged. NULL if the
// source cannot be allocated.
struct MglSource* mglSourceCreate(void (*on_frame)(void* context, const struct MglVideoFrame* frame),
                                  void (*on_audio)(void* context, const struct MglAudioBlock* audio),
                                  void* context);
// Unregisters and frees the source. Once it returns no callback reaches the source. NULL is
// ignored.
void mglSourceDestroy(struct MglSource* source);
// Whether the source is live in OBS's program output. Returns at once; the link follows within a
// fraction of a second.
void mglSourceSetLive(struct MglSource* source, bool live);
// What the source shows while it is live: the glasses' video, their microphones, or both.
void mglSourceSetMedia(struct MglSource* source, bool video, bool audio);
// Selects a quality preset by locale key; NULL or unknown keys are ignored. The source that went
// live first among those showing video sets the stream's profile.
void mglSourceSetQuality(struct MglSource* source, const char* key);
// Locale key of the quality preset at index, or NULL past the last one. Index 0 is the default.
const char* mglQualityKey(size_t index);

// Settings shared by every glasses source. The Wi-Fi adapter is an MglWifiAdapter id; NULL or ""
// selects the first adapter Windows lists, and an id that is no longer present makes the next
// connection fail with a log line instead of using another adapter. With keep_ready the link
// connects while any glasses source exists and stays connected with the camera off.
void mglLinkSetWifiAdapter(const char* id);
void mglLinkSetKeepReady(bool keep_ready);
// Locale key describing the link's current state.
const char* mglLinkStatusKey(void);
// The glasses' battery as they last reported it over the current link: the percentage, or -1 when
// no report has arrived or the link is down. charging may be NULL.
int mglLinkBattery(bool* charging);

// One Wi-Fi adapter that can join the glasses' network. Both strings are NUL-terminated UTF-8;
// a description too long for name is cut at a character boundary.
struct MglWifiAdapter {
    char id[64];    // NOLINT(modernize-avoid-c-arrays) — C ABI; the Windows interface GUID
    char name[256]; // NOLINT(modernize-avoid-c-arrays) — C ABI; the adapter's description
};
// Copies up to capacity of the Wi-Fi adapters Windows lists into adapters, in Windows' order, and
// stores how many were copied in count. False (count 0) if the list could not be built; an
// unavailable WLAN service is not a failure and simply lists none.
bool mglWifiAdapterList(struct MglWifiAdapter* adapters, size_t capacity, size_t* count);

// Opens the setup tool in its own console window. The link releases the glasses while it runs and
// reconnects, with whatever identity the setup left, once the window closes. Returns the locale key
// of the outcome: Setup.Started, Setup.Running (a setup window is already open), Setup.ToolMissing
// or Setup.Failed.
const char* mglSetupStart(void);
// Deletes this PC's stored glasses identity and imported tokens after releasing the glasses. The
// glasses keep this PC's key as owner until they are set up somewhere else. True when it is gone.
bool mglForgetGlasses(void);

#ifdef __cplusplus
}
#endif
#endif
