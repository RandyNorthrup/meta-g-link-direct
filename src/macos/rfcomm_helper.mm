// SPDX-License-Identifier: AGPL-3.0-only
// Meta G Link Helper: the app that holds what macOS gives only to an app the user allowed, and that
// declares it in its own Info.plist. OBS declares neither Bluetooth nor Location, and a plugin
// inherits that. So this small app, started through LaunchServices so that it answers for itself,
// has two roles, each its own process (helper_protocol.hpp). The Bluetooth role, in this file,
// opens the Meta RFCOMM service and relays its bytes over a Unix socket to
// rfcomm_helper_client.cpp; it sees only AirShield's encrypted frames, and keys stay in the caller.
// The Wi-Fi role is in wifi_helper.mm. Opened by hand, with no role, the helper only settles both
// permission questions.
//
// The helper exits with its connection.
#include "helper_protocol.hpp"
#include "helper_shared.hpp"

#import "wifi_helper.h"

#import <Cocoa/Cocoa.h>
#import <CoreBluetooth/CoreBluetooth.h>
#import <IOBluetooth/IOBluetooth.h>

#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <string>

namespace {
constexpr std::array<std::uint8_t, 16> MetaService{
    0x8B, 0x0D, 0x26, 0x87, 0x42, 0xA4, 0x44, 0xCB, 0x94, 0x36, 0xFB, 0xA3, 0xB9, 0xB9, 0x6D, 0xE2,
};
constexpr NSTimeInterval SdpQueryWait = 8.0;
constexpr NSTimeInterval OpenWait = 12.0;
constexpr std::size_t ReadChunk = 4096;

using mgl::helper::connectToClient;
using mgl::helper::NoDescriptor;
using mgl::helper::writeAll;

NSString* socketPathFor(const char* role) {
    NSArray<NSString*>* arguments = NSProcessInfo.processInfo.arguments;
    const NSUInteger flag = [arguments indexOfObject:[NSString stringWithUTF8String:role]];
    return flag != NSNotFound && flag + 1 < arguments.count ? arguments[flag + 1] : nil;
}
} // namespace

@interface HelperApp
    : NSObject <NSApplicationDelegate, CBCentralManagerDelegate, IOBluetoothRFCOMMChannelDelegate>
@property(strong) CBCentralManager* central;
@property(strong) IOBluetoothRFCOMMChannel* channel;
@property(strong) dispatch_source_t reader;
@property(strong) IOBluetoothSDPUUID* service;
@property(strong) IOBluetoothDevice* glasses;
@property(strong) NSWindow* window;
@property(strong) WifiHelper* wifi;
@property BOOL started;
@property BOOL askOnly;
@property int client;
@end

@implementation HelperApp
- (void)finishWith:(const char*)status detail:(int)detail {
    if (self.client != NoDescriptor) {
        const std::string line = mgl::helper::statusLine(status, detail);
        (void)writeAll(self.client, line.data(), line.size());
    }
    [NSApp terminate:nil];
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    (void)notification;
    if (NSString* path = socketPathFor(mgl::helper::WifiRole); path != nil) {
        [self startWifiRole:connectToClient(path.UTF8String)];
        return;
    }
    NSString* path = socketPathFor(mgl::helper::BluetoothRole);
    self.client = path != nil ? connectToClient(path.UTF8String) : NoDescriptor;
    self.askOnly = self.client == NoDescriptor;
    // Creating the manager asks the user the first time; its state update says when that is
    // settled.
    if ([CBManager authorization] == CBManagerAuthorizationNotDetermined) {
        self.window = mgl::helper::showWhyWeAsk(
            @"Meta G Link Direct reaches your glasses over Bluetooth. Please allow Bluetooth for "
            @"this helper when macOS asks.");
    }
    self.central = [[CBCentralManager alloc] initWithDelegate:self queue:nil];
}

- (void)startWifiRole:(int)client {
    self.wifi = [[WifiHelper alloc] initWithClient:client
                                          finished:^{
                                            [NSApp terminate:nil];
                                          }];
    [self.wifi start];
}

- (void)centralManagerDidUpdateState:(CBCentralManager*)central {
    (void)central;
    const auto authorization = [CBManager authorization];
    if (authorization == CBManagerAuthorizationNotDetermined || self.started) {
        return;
    }
    self.started = YES;
    [self.window orderOut:nil];
    if (self.askOnly) {
        [self startWifiRole:NoDescriptor];
        return;
    }
    if (authorization != CBManagerAuthorizationAllowedAlways) {
        [self finishWith:mgl::helper::StatusNoPermission detail:0];
        return;
    }
    [self findGlasses];
}

// The paired device that offers the Meta service, from its cached SDP record. A device without one
// is asked only if its name says it may be the glasses, so unrelated devices are not woken.
- (void)findGlasses {
    self.service = [IOBluetoothSDPUUID uuidWithBytes:MetaService.data() length:MetaService.size()];
    NSMutableArray<IOBluetoothDevice*>* offering = [NSMutableArray array];
    NSMutableArray<IOBluetoothDevice*>* unknown = [NSMutableArray array];
    for (IOBluetoothDevice* device in IOBluetoothDevice.pairedDevices) {
        if ([device getServiceRecordForUUID:self.service] != nil) {
            [offering addObject:device];
        } else if ([device.name containsString:@"Meta"]) {
            [unknown addObject:device];
        }
    }
    if (offering.count > 1) {
        [self finishWith:mgl::helper::StatusAmbiguous detail:0];
    } else if (offering.count == 1) {
        self.glasses = offering.firstObject;
        [self openChannel];
    } else if (unknown.count == 0) {
        [self finishWith:mgl::helper::StatusNoMetaService detail:0];
    } else if (unknown.count > 1) {
        [self finishWith:mgl::helper::StatusAmbiguous detail:0];
    } else {
        self.glasses = unknown.firstObject;
        const IOReturn started = [self.glasses performSDPQuery:self uuids:@[ self.service ]];
        if (started != kIOReturnSuccess) {
            [self finishWith:mgl::helper::StatusConnectFailed detail:static_cast<int>(started)];
            return;
        }
        [self performSelector:@selector(sdpTimedOut) withObject:nil afterDelay:SdpQueryWait];
    }
}

- (void)sdpQueryComplete:(IOBluetoothDevice*)device status:(IOReturn)status {
    (void)device;
    [NSObject cancelPreviousPerformRequestsWithTarget:self
                                             selector:@selector(sdpTimedOut)
                                               object:nil];
    if (status != kIOReturnSuccess || [self.glasses getServiceRecordForUUID:self.service] == nil) {
        [self finishWith:mgl::helper::StatusNoMetaService detail:static_cast<int>(status)];
        return;
    }
    [self openChannel];
}

- (void)sdpTimedOut {
    [self finishWith:mgl::helper::StatusConnectFailed detail:ETIMEDOUT];
}

- (void)openChannel {
    IOBluetoothSDPServiceRecord* record = [self.glasses getServiceRecordForUUID:self.service];
    BluetoothRFCOMMChannelID channel_id = 0;
    const IOReturn found = [record getRFCOMMChannelID:&channel_id];
    if (found != kIOReturnSuccess) {
        [self finishWith:mgl::helper::StatusConnectFailed detail:static_cast<int>(found)];
        return;
    }
    IOBluetoothRFCOMMChannel* opening = nil;
    const IOReturn started = [self.glasses openRFCOMMChannelAsync:&opening
                                                    withChannelID:channel_id
                                                         delegate:self];
    if (started != kIOReturnSuccess) {
        [self finishWith:mgl::helper::StatusConnectFailed detail:static_cast<int>(started)];
        return;
    }
    self.channel = opening;
    [self performSelector:@selector(openTimedOut) withObject:nil afterDelay:OpenWait];
}

- (void)openTimedOut {
    [self finishWith:mgl::helper::StatusConnectFailed detail:ETIMEDOUT];
}

- (void)rfcommChannelOpenComplete:(IOBluetoothRFCOMMChannel*)channel status:(IOReturn)status {
    [NSObject cancelPreviousPerformRequestsWithTarget:self
                                             selector:@selector(openTimedOut)
                                               object:nil];
    if (status != kIOReturnSuccess) {
        [self finishWith:mgl::helper::StatusConnectFailed detail:static_cast<int>(status)];
        return;
    }
    self.channel = channel;
    const std::string line = mgl::helper::statusLine(mgl::helper::StatusOk, 0);
    if (!writeAll(self.client, line.data(), line.size())) {
        [channel closeChannel];
        [NSApp terminate:nil];
        return;
    }
    [self startRelayingFromClient];
}

// Bytes from the glasses go straight to the caller.
- (void)rfcommChannelData:(IOBluetoothRFCOMMChannel*)channel
                     data:(void*)data
                   length:(size_t)length {
    if (!writeAll(self.client, data, length)) {
        [channel closeChannel];
        [NSApp terminate:nil];
    }
}

- (void)rfcommChannelClosed:(IOBluetoothRFCOMMChannel*)channel {
    (void)channel;
    [NSApp terminate:nil];
}

// Bytes from the caller go to the glasses in pieces the channel accepts; its end ends the helper.
- (void)startRelayingFromClient {
    self.reader =
        dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, static_cast<uintptr_t>(self.client), 0,
                               dispatch_get_main_queue());
    __weak HelperApp* weak = self;
    dispatch_source_set_event_handler(self.reader, ^{
      HelperApp* strong = weak;
      std::array<std::uint8_t, ReadChunk> chunk{};
      const auto got = read(strong.client, chunk.data(), chunk.size());
      if (got <= 0) {
          [strong.channel closeChannel];
          [NSApp terminate:nil];
          return;
      }
      const std::size_t mtu = strong.channel.getMTU;
      for (std::size_t sent = 0; sent < static_cast<std::size_t>(got);) {
          const std::size_t step = std::min(static_cast<std::size_t>(got) - sent, mtu);
          if ([strong.channel writeSync:chunk.data() + sent
                                 length:static_cast<UInt16>(step)] != kIOReturnSuccess) {
              [strong.channel closeChannel];
              [NSApp terminate:nil];
              return;
          }
          sent += step;
      }
    });
    dispatch_resume(self.reader);
}
@end

int main() {
    // A vanished caller is reported by write's result, and the Wi-Fi role still has to leave.
    (void)std::signal(SIGPIPE, SIG_IGN);
    @autoreleasepool {
        NSApplication* application = NSApplication.sharedApplication;
        HelperApp* delegate = [HelperApp new];
        application.delegate = delegate;
        [application setActivationPolicy:NSApplicationActivationPolicyAccessory];
        [application run];
    }
    return 0;
}
