// SPDX-License-Identifier: AGPL-3.0-only
// What the roles inside Meta G Link Helper share: the socket to the caller and the window that
// explains a permission question.
#ifndef MGL_MACOS_HELPER_SHARED_HPP
#define MGL_MACOS_HELPER_SHARED_HPP

#import <Cocoa/Cocoa.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace mgl::helper {
inline constexpr int NoDescriptor = -1;

inline int connectToClient(const std::string& path) {
    const int descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
    if (descriptor < 0) {
        return NoDescriptor;
    }
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    if (path.size() >= sizeof(address.sun_path)) {
        close(descriptor);
        return NoDescriptor;
    }
    std::memcpy(address.sun_path, path.c_str(), path.size() + 1);
    if (connect(descriptor, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
        close(descriptor);
        return NoDescriptor;
    }
    return descriptor;
}

inline bool writeAll(int descriptor, const void* data, std::size_t size) {
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    while (size > 0) {
        const auto wrote = write(descriptor, bytes, size);
        if (wrote < 0 && errno == EINTR) {
            continue;
        }
        if (wrote <= 0) {
            return false;
        }
        bytes += wrote;
        size -= static_cast<std::size_t>(wrote);
    }
    return true;
}

// macOS shows a permission question for an app in the foreground, so the helper steps forward only
// when it has one to ask, and says why.
inline NSWindow* showWhyWeAsk(NSString* reason) {
    constexpr CGFloat Width = 460;
    constexpr CGFloat Height = 90;
    constexpr CGFloat Margin = 20;
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    NSWindow* window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, Width, Height)
                                                   styleMask:NSWindowStyleMaskTitled
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    window.title = @"Meta G Link Helper";
    window.releasedWhenClosed = NO;
    NSTextField* label = [NSTextField wrappingLabelWithString:reason];
    label.frame = NSMakeRect(Margin, Margin, Width - (2 * Margin), Height - (2 * Margin));
    [window.contentView addSubview:label];
    [window center];
    [window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    return window;
}
} // namespace mgl::helper
#endif
