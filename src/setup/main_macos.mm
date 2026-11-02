// SPDX-License-Identifier: AGPL-3.0-only
// Meta G Link Setup on macOS, the glasses setup window, with the same screens and exit codes as the
// Windows one (src/setup/main.cpp). It explains the change of owner, shows the Meta login in a
// WKWebView with a non-persistent data store (so nothing stays afterwards), recognises the end of
// the login by its redirect address, and runs the change of owner. Exit codes: 0 done, 3 the setup
// failed, 8 cancelled.
#include "mgl/provision.hpp"
#include "mgl/setup.hpp"

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

#include <cstdint>
#include <cstdlib>
#include <string>
#include <utility>

namespace {
constexpr int ExitDone = 0;
constexpr int ExitFailed = 3;
constexpr int ExitCancelled = 8;
constexpr CGFloat Margin = 20;
constexpr CGFloat Gap = 12;
constexpr CGFloat StartWidth = 640;
constexpr CGFloat StartHeight = 760;
constexpr CGFloat MinimumWidth = 480;
constexpr CGFloat MinimumHeight = 520;

enum class Screen : std::uint8_t { Intro, Login, Paste, Working, Done, Failed };

NSString* text(const std::string& value) { return [NSString stringWithUTF8String:value.c_str()]; }
} // namespace

@interface SetupController
    : NSObject <NSApplicationDelegate, NSWindowDelegate, WKNavigationDelegate, WKUIDelegate>
@end

@implementation SetupController {
    NSWindow* _window;
    NSStackView* _column;
    NSTextField* _heading;
    NSTextField* _body;
    NSButton* _browser;
    NSTextField* _addressLabel;
    NSTextField* _address;
    NSProgressIndicator* _progress;
    NSView* _loginArea;
    WKWebView* _view;
    NSButton* _primary;
    NSButton* _secondary;
    Screen _screen;
    int _exitCode;
    mgl::provision::Authorize _request;
}

- (NSTextField*)labelWithFont:(NSFont*)font {
    NSTextField* label = [NSTextField wrappingLabelWithString:@""];
    label.font = font;
    label.selectable = YES;
    return label;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    (void)notification;
    _exitCode = ExitCancelled;
    const NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                    NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
    _window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, StartWidth, StartHeight)
                                          styleMask:style
                                            backing:NSBackingStoreBuffered
                                              defer:NO];
    _window.title = @"Set up Ray-Ban Meta glasses";
    _window.contentMinSize = NSMakeSize(MinimumWidth, MinimumHeight);
    _window.delegate = self;

    _heading = [self labelWithFont:[NSFont boldSystemFontOfSize:NSFont.systemFontSize * 1.35]];
    _body = [self labelWithFont:[NSFont systemFontOfSize:NSFont.systemFontSize]];
    _browser = [NSButton buttonWithTitle:@"Open the login in your browser"
                                  target:self
                                  action:@selector(openBrowser:)];
    _addressLabel = [NSTextField labelWithString:@"Address after the login:"];
    _address = [NSTextField textFieldWithString:@""];
    _address.accessibilityLabel = @"Address after the login";
    _progress = [[NSProgressIndicator alloc] init];
    _progress.style = NSProgressIndicatorStyleBar;
    _progress.indeterminate = YES;
    _loginArea = [[NSView alloc] init];
    _primary = [NSButton buttonWithTitle:@"Continue" target:self action:@selector(primary:)];
    _primary.keyEquivalent = @"\r";
    _secondary = [NSButton buttonWithTitle:@"Cancel" target:self action:@selector(secondary:)];
    _secondary.keyEquivalent = @"\033";

    NSStackView* buttons = [NSStackView stackViewWithViews:@[ _secondary, _primary ]];
    buttons.spacing = Gap;
    NSStackView* row = [NSStackView stackViewWithViews:@[ [NSView new], buttons ]];
    row.distribution = NSStackViewDistributionFill;

    _column = [NSStackView stackViewWithViews:@[
        _heading, _body, _browser, _addressLabel, _address, _progress, _loginArea, row
    ]];
    _column.orientation = NSUserInterfaceLayoutOrientationVertical;
    _column.alignment = NSLayoutAttributeLeading;
    _column.spacing = Gap;
    _column.edgeInsets = NSEdgeInsetsMake(Margin, Margin, Margin, Margin);
    _column.translatesAutoresizingMaskIntoConstraints = NO;
    [_window.contentView addSubview:_column];
    NSView* content = _window.contentView;
    [NSLayoutConstraint activateConstraints:@[
        [_column.topAnchor constraintEqualToAnchor:content.topAnchor],
        [_column.bottomAnchor constraintEqualToAnchor:content.bottomAnchor],
        [_column.leadingAnchor constraintEqualToAnchor:content.leadingAnchor],
        [_column.trailingAnchor constraintEqualToAnchor:content.trailingAnchor],
    ]];
    const CGFloat inner = -2 * Margin;
    for (NSView* wide in @[ _heading, _body, _address, _progress, _loginArea, row ]) {
        [wide.widthAnchor constraintEqualToAnchor:_column.widthAnchor constant:inner].active = YES;
    }
    [_loginArea setContentHuggingPriority:NSLayoutPriorityDefaultLow
                           forOrientation:NSLayoutConstraintOrientationVertical];

    // Tab follows the screen from top to bottom.
    _window.autorecalculatesKeyViewLoop = YES;
    [self showScreen:Screen::Intro detail:""];
    [_window center];
    [_window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (void)setPrimary:(NSString*)primary secondary:(NSString*)secondary {
    _primary.hidden = primary == nil;
    _secondary.hidden = secondary == nil;
    if (primary != nil) {
        _primary.title = primary;
    }
    if (secondary != nil) {
        _secondary.title = secondary;
    }
}

- (void)showScreen:(Screen)screen detail:(const std::string&)detail {
    _screen = screen;
    const BOOL paste = screen == Screen::Paste;
    _browser.hidden = !paste;
    _addressLabel.hidden = !paste;
    _address.hidden = !paste;
    _progress.hidden = screen != Screen::Working;
    _loginArea.hidden = screen != Screen::Login;
    if (screen == Screen::Working) {
        [_progress startAnimation:nil];
    } else {
        [_progress stopAnimation:nil];
    }
    switch (screen) {
    case Screen::Intro:
        _heading.stringValue = @"Set up your Ray-Ban Meta glasses";
        _body.stringValue =
            @"This makes THIS computer the owner of your glasses. The Meta AI phone app, or "
            @"another "
            @"computer, loses them until they are removed and added again there.\n\n"
            @"Before you continue:\n"
            @"•  Pair the glasses with this Mac in System Settings > Bluetooth.\n"
            @"•  Turn off Bluetooth on your phone.\n"
            @"•  Keep the glasses charged and out of the case.\n\n"
            @"Next you log in to Meta in this window. Nothing needs to be copied. macOS may ask to "
            @"allow Bluetooth and Location for Meta G Link Helper; please allow both.";
        [self setPrimary:@"Continue" secondary:@"Cancel"];
        break;
    case Screen::Login:
        _heading.stringValue = @"Log in to Meta";
        _body.stringValue =
            @"Log in below with your Meta account. This window carries on by itself "
            @"when the login is done.";
        [self setPrimary:nil secondary:@"Cancel"];
        break;
    case Screen::Paste:
        _heading.stringValue = @"Log in to Meta in your browser";
        _body.stringValue = @"1.  Select Open the login in your browser, and log in.\n"
                            @"2.  When the login has finished, copy the whole address from the "
                            @"browser's address bar.\n"
                            @"3.  Paste it in the box below and select Continue.";
        [self setPrimary:@"Continue" secondary:@"Cancel"];
        break;
    case Screen::Working:
        _heading.stringValue = @"Setting up the glasses";
        _body.stringValue =
            @"Changing the glasses' owner to this computer. This can take a minute; "
            @"keep the glasses nearby.";
        [self setPrimary:nil secondary:nil];
        break;
    case Screen::Done:
        _heading.stringValue = @"Done";
        _body.stringValue =
            @"This computer now owns the glasses. In OBS they connect when a glasses source goes "
            @"live.";
        [self setPrimary:@"Close" secondary:nil];
        break;
    case Screen::Failed:
        _heading.stringValue = @"The setup did not finish";
        _body.stringValue = text(detail);
        [self setPrimary:@"Try again" secondary:@"Close"];
        break;
    }
    [_window recalculateKeyViewLoop];
    [_window makeFirstResponder:paste ? _browser : (_primary.hidden ? _secondary : _primary)];
}

- (void)openBrowser:(id)sender {
    (void)sender;
    [NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:text(_request.url)]];
}

- (void)primary:(id)sender {
    (void)sender;
    switch (_screen) {
    case Screen::Intro:
    case Screen::Failed:
        [self startLogin];
        break;
    case Screen::Paste:
        [self useAddress:std::string(_address.stringValue.UTF8String)];
        break;
    case Screen::Done:
        _exitCode = ExitDone;
        [NSApp terminate:nil];
        break;
    case Screen::Login:
    case Screen::Working:
        break;
    }
}

- (void)secondary:(id)sender {
    (void)sender;
    if (_screen == Screen::Working) {
        return;
    }
    if (_screen == Screen::Failed) {
        _exitCode = ExitFailed;
    } else if (_screen == Screen::Done) {
        _exitCode = ExitDone;
    } else {
        _exitCode = ExitCancelled;
    }
    [NSApp terminate:nil];
}

- (void)startLogin {
    _request = mgl::provision::authorizeRequest();
    [self showScreen:Screen::Login detail:""];
    WKWebViewConfiguration* configuration = [WKWebViewConfiguration new];
    configuration.websiteDataStore = WKWebsiteDataStore.nonPersistentDataStore;
    _view = [[WKWebView alloc] initWithFrame:_loginArea.bounds configuration:configuration];
    _view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    _view.navigationDelegate = self;
    _view.UIDelegate = self;
    [_loginArea addSubview:_view];
    [_view loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:text(_request.url)]]];
    [_window makeFirstResponder:_view];
}

// The login ends by sending the browser to the redirect address; it carries the code.
- (void)webView:(WKWebView*)webView
    decidePolicyForNavigationAction:(WKNavigationAction*)action
                    decisionHandler:(void (^)(WKNavigationActionPolicy))handler {
    (void)webView;
    const std::string address(action.request.URL.absoluteString.UTF8String);
    if (address.starts_with(mgl::provision::LoginRedirectUri)) {
        handler(WKNavigationActionPolicyCancel);
        [_view removeFromSuperview];
        _view = nil;
        [self useAddress:address];
        return;
    }
    handler(WKNavigationActionPolicyAllow);
}

// Pages that open a new window stay in this one.
- (WKWebView*)webView:(WKWebView*)webView
    createWebViewWithConfiguration:(WKWebViewConfiguration*)configuration
               forNavigationAction:(WKNavigationAction*)action
                    windowFeatures:(WKWindowFeatures*)features {
    (void)configuration;
    (void)features;
    [webView loadRequest:action.request];
    return nil;
}

- (void)fail:(const std::string&)problem {
    _exitCode = ExitFailed;
    [self showScreen:Screen::Failed detail:problem];
}

// Checks a finished login's address and starts the change of owner with its code.
- (void)useAddress:(const std::string&)address {
    const auto login = mgl::setup::readLoginAddress(address, _request.state);
    if (!login.code) {
        [self fail:login.problem];
        return;
    }
    [self showScreen:Screen::Working detail:""];
    const std::string auth_code = *login.code;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
      const auto outcome = mgl::setup::changeOwner(auth_code);
      const bool ok = outcome.ok;
      const std::string problem = outcome.problem;
      dispatch_async(dispatch_get_main_queue(), ^{
        if (ok) {
            self->_exitCode = ExitDone;
            [self showScreen:Screen::Done detail:""];
        } else {
            [self fail:problem];
        }
      });
    });
}

// A change of owner must not be cut off half way.
- (BOOL)windowShouldClose:(NSWindow*)sender {
    (void)sender;
    if (_screen == Screen::Working) {
        NSBeep();
        return NO;
    }
    [NSApp terminate:nil];
    return YES;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    (void)sender;
    return YES;
}

- (void)applicationWillTerminate:(NSNotification*)notification {
    (void)notification;
    std::exit(_exitCode);
}
@end

int main() {
    @autoreleasepool {
        NSApplication* application = NSApplication.sharedApplication;
        SetupController* controller = [SetupController new];
        application.delegate = controller;
        [application setActivationPolicy:NSApplicationActivationPolicyRegular];
        [application run];
    }
    return ExitFailed;
}
