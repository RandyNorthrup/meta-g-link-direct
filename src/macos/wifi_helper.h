// SPDX-License-Identifier: AGPL-3.0-only
// The Wi-Fi role of Meta G Link Helper; see wifi_helper.mm.
#ifndef MGL_MACOS_WIFI_HELPER_H
#define MGL_MACOS_WIFI_HELPER_H

#import <Foundation/Foundation.h>

@interface WifiHelper : NSObject
// client is the socket to the caller, or NoDescriptor to only settle the Location question.
// finished runs on the main queue when the role is over.
- (instancetype)initWithClient:(int)client finished:(dispatch_block_t)finished;
- (void)start;
@end

#endif
