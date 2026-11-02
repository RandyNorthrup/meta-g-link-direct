// SPDX-License-Identifier: AGPL-3.0-only
// The user's region on macOS, for mgl/platform.hpp.
#include "mgl/platform.hpp"

#import <Foundation/Foundation.h>

#include <optional>
#include <string>

namespace mgl::platform {
std::optional<std::string> userRegion() {
    @autoreleasepool {
        NSString* code = [NSLocale.currentLocale objectForKey:NSLocaleCountryCode];
        constexpr NSUInteger CodeLength = 2;
        if (code.length != CodeLength) {
            return std::nullopt;
        }
        return std::string(code.UTF8String);
    }
}
} // namespace mgl::platform
