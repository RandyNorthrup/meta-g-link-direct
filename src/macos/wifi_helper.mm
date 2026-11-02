// SPDX-License-Identifier: AGPL-3.0-only
// The Wi-Fi role of Meta G Link Helper. macOS gives a network's name only to an app the user
// allowed Location, and CoreWLAN cannot join a network without its name. OBS declares no such use,
// and a plugin inherits that, so this app holds the permission and joins for the caller.
// networksetup is not an option: it saves the passphrase in the System keychain and then waits for
// someone to answer a password dialog.
//
// The session lasts as long as the caller's socket, so a caller that crashes still returns the
// adapter to its usual networks. macOS remembers every network it joins: leaving takes this one off
// the known networks, but its keychain item stays because only an administrator can delete it. That
// item holds a passphrase good for one finished session, and wifiNetworkName keeps it to one item.
#import "wifi_helper.h"

#include "helper_protocol.hpp"
#include "helper_shared.hpp"

#import <CoreLocation/CoreLocation.h>
#import <CoreWLAN/CoreWLAN.h>

#include <unistd.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <thread>
#include <utility>

namespace {
using namespace std::chrono_literals;
namespace helper = mgl::helper;

// A scan soon after another scan, the system's own included, reports "busy" for about ten seconds
// and then takes three to six, so one join can need most of this.
constexpr auto JoinBudget = 30s;
constexpr auto RetryPause = 1500ms;

struct JoinRequest {
    std::string adapter;
    std::string ssid;
    std::string passphrase;
};

std::optional<std::string> readLine(int client) {
    constexpr std::size_t LongestLine = 128; // a WPA2 passphrase has at most 63 characters
    std::string line;
    while (line.size() < LongestLine) {
        char byte = 0;
        const auto got = read(client, &byte, 1);
        if (got < 0 && errno == EINTR) {
            continue;
        }
        if (got <= 0) {
            return std::nullopt;
        }
        if (byte == '\n') {
            return line;
        }
        line.push_back(byte);
    }
    return std::nullopt;
}

std::optional<JoinRequest> readRequest(int client) {
    auto adapter = readLine(client);
    auto ssid = adapter ? readLine(client) : std::nullopt;
    auto passphrase = ssid ? readLine(client) : std::nullopt;
    if (!passphrase) {
        return std::nullopt;
    }
    return JoinRequest{.adapter = std::move(*adapter),
                       .ssid = std::move(*ssid),
                       .passphrase = std::move(*passphrase)};
}

NSString* text(const std::string& value) { return [NSString stringWithUTF8String:value.c_str()]; }

// The network by its name, checked against the result: a scan that reports "busy" returns an older,
// unfiltered list.
CWNetwork* findNetwork(CWInterface* wifi, NSString* ssid, NSError** error) {
    NSSet<CWNetwork*>* found = [wifi scanForNetworksWithName:ssid error:error];
    if (*error != nil) {
        return nil;
    }
    for (CWNetwork* network in found) {
        if ([network.ssid isEqualToString:ssid]) {
            return network;
        }
    }
    return nil;
}

std::pair<const char*, int> join(CWInterface* wifi, NSString* ssid, NSString* passphrase) {
    const auto deadline = std::chrono::steady_clock::now() + JoinBudget;
    bool visible = false;
    int last_error = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        NSError* error = nil;
        // A fresh look every time: after a refused join macOS goes back to its usual network, and
        // an older scan result then keeps failing.
        CWNetwork* network = findNetwork(wifi, ssid, &error);
        if (network != nil) {
            visible = true;
            if ([wifi associateToNetwork:network password:passphrase error:&error]) {
                return {helper::StatusOk, 0};
            }
        }
        if (error != nil) {
            last_error = static_cast<int>(error.code);
        }
        std::this_thread::sleep_for(RetryPause);
    }
    return {visible ? helper::StatusConnectFailed : helper::StatusNotVisible, last_error};
}

// Takes the network off the known networks, so macOS neither lists it nor tries to rejoin it.
// Bounded, because a Mac set to ask for an administrator here would otherwise hold up the leaving.
void forgetNetwork(NSString* adapter, NSString* ssid) {
    constexpr auto ForgetWait = 5s;
    constexpr auto PollInterval = 100ms;
    NSTask* task = [NSTask new];
    task.executableURL = [NSURL fileURLWithPath:@"/usr/sbin/networksetup"];
    task.arguments = @[ @"-removepreferredwirelessnetwork", adapter, ssid ];
    task.standardOutput = NSFileHandle.fileHandleWithNullDevice;
    task.standardError = NSFileHandle.fileHandleWithNullDevice;
    if (![task launchAndReturnError:nil]) {
        return;
    }
    const auto deadline = std::chrono::steady_clock::now() + ForgetWait;
    while (task.running && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(PollInterval);
    }
    if (task.running) {
        [task terminate];
    }
}

// Restarting the adapter is what returns it to its usual networks. One that already moved on to
// another network stays there. Nothing is left to report to: the caller has gone, and the system
// shows the adapter's state.
void leave(CWInterface* wifi, NSString* ssid) {
    NSString* current = wifi.ssid;
    const BOOL moved_on = current != nil && ![current isEqualToString:ssid];
    if (current != nil && !moved_on) {
        [wifi disassociate];
    }
    forgetNetwork(wifi.interfaceName, ssid);
    if (!moved_on) {
        (void)[wifi setPower:NO error:nil];
        (void)[wifi setPower:YES error:nil];
    }
}

void awaitCallerEnd(int client) {
    char byte = 0;
    for (;;) {
        const auto got = read(client, &byte, 1);
        if (got == 0 || (got < 0 && errno != EINTR)) {
            return;
        }
    }
}
} // namespace

@interface WifiHelper () <CLLocationManagerDelegate>
@property(strong) CLLocationManager* location;
@property(strong) NSWindow* window;
@property(copy) dispatch_block_t finished;
@property int client;
@property BOOL asked;
@property BOOL settled;
@end

@implementation WifiHelper
- (instancetype)initWithClient:(int)client finished:(dispatch_block_t)finished {
    self = [super init];
    if (self != nil) {
        self.client = client;
        self.finished = finished;
    }
    return self;
}

- (void)start {
    // Its first update reports the current answer.
    self.location = [CLLocationManager new];
    self.location.delegate = self;
}

- (void)locationManagerDidChangeAuthorization:(CLLocationManager*)manager {
    const CLAuthorizationStatus status = manager.authorizationStatus;
    if (status == kCLAuthorizationStatusNotDetermined) {
        if (!self.asked) {
            self.asked = YES;
            self.window = helper::showWhyWeAsk(
                @"macOS shows the name of a Wi-Fi network only to apps allowed Location. Meta G "
                @"Link "
                @"Direct needs the name to join your glasses' network, and never reads where you "
                @"are. Please allow it when macOS asks.");
            [manager requestWhenInUseAuthorization];
        }
        return;
    }
    if (self.settled) {
        return;
    }
    self.settled = YES;
    [self.window orderOut:nil];
    if (self.client == helper::NoDescriptor) {
        self.finished();
        return;
    }
    if (status == kCLAuthorizationStatusDenied || status == kCLAuthorizationStatusRestricted) {
        [self answer:helper::StatusNoPermission detail:0];
        self.finished();
        return;
    }
    // CoreWLAN's calls block for seconds; the main queue keeps answering the system meanwhile.
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
      [self runSession];
    });
}

- (void)answer:(const char*)status detail:(int)detail {
    const std::string line = helper::statusLine(status, detail);
    (void)helper::writeAll(self.client, line.data(), line.size());
}

- (void)runSession {
    @autoreleasepool {
        const auto request = readRequest(self.client);
        if (!request) {
            [self answer:helper::StatusBadRequest detail:0];
        } else {
            CWInterface* wifi =
                [CWWiFiClient.sharedWiFiClient interfaceWithName:text(request->adapter)];
            NSString* ssid = text(request->ssid);
            const auto [status, detail] = join(wifi, ssid, text(request->passphrase));
            [self answer:status detail:detail];
            if (status == helper::StatusOk) {
                awaitCallerEnd(self.client);
            }
            leave(wifi, ssid);
        }
    }
    close(self.client);
    dispatch_async(dispatch_get_main_queue(), self.finished);
}
@end
