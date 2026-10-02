#include "ui_bridge.hpp"
#include "match_turbo.hpp"
#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h>
#include <deque>
#include <stdexcept>
#include <map>
#include <set>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <thread>

namespace {
bool match_turbo = false;
std::uint16_t original_modifiers(NSEventModifierFlags flags) {
    std::uint16_t value = 0;
    if (flags & NSEventModifierFlagCommand) value |= 0x100;
    if (flags & NSEventModifierFlagShift) value |= 0x200;
    if (flags & NSEventModifierFlagCapsLock) value |= 0x400;
    if (flags & NSEventModifierFlagOption) value |= 0x800;
    if (flags & NSEventModifierFlagControl) value |= 0x1000;
    return value;
}
std::uint8_t original_character(NSEvent* event) {
    // AppKit represents navigation keys as Unicode function characters;
    // the original EventRecord uses the MacRoman control-character codes.
    switch (event.keyCode) {
        case 51: return 8; case 117: return 127;
        case 36: return 13; case 76: return 3; case 53: return 27;
        case 48: return 9;
    }
    NSString* text = event.characters;
    if (!text.length) return 0;
    const auto first = [text characterAtIndex:0];
    switch (first) {
        case NSUpArrowFunctionKey: return 30; case NSDownArrowFunctionKey: return 31;
        case NSLeftArrowFunctionKey: return 28; case NSRightArrowFunctionKey: return 29;
        case NSHomeFunctionKey: return 1; case NSEndFunctionKey: return 4;
        case NSPageUpFunctionKey: return 11; case NSPageDownFunctionKey: return 12;
        case NSHelpFunctionKey: return 5;
    }
    if (first >= NSF1FunctionKey && first <= NSF35FunctionKey) return 16;
    NSData* bytes = [[text substringToIndex:1] dataUsingEncoding:NSMacOSRomanStringEncoding allowLossyConversion:NO];
    return bytes.length == 1 ? *static_cast<const std::uint8_t*>(bytes.bytes) : 0;
}
}

@interface CMNativeWindow : NSPanel
@property(nonatomic) BOOL acceptsActivation;
@end
@implementation CMNativeWindow
- (BOOL)canBecomeKeyWindow { return self.acceptsActivation; }
- (BOOL)canBecomeMainWindow { return self.acceptsActivation; }
@end

@interface CMNativeView : NSView {
@public
    std::deque<cm::aot::UIEvent>* pending;
    std::uint32_t token;
    bool paintQueued;
    double wheelRemainder;
}
@property(nonatomic, strong) NSImage* renderedImage;
@end
@implementation CMNativeView
- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent*)event { (void)event; return YES; }
- (void)queueInput:(NSEvent*)native kind:(cm::aot::UIEvent::Kind)kind {
    cm::aot::UIEvent event{kind, token};
    const auto point = [self.window convertPointToScreen:native.locationInWindow];
    const auto height = CGRectGetHeight(CGDisplayBounds(CGMainDisplayID()));
    event.x = std::int16_t(std::lround(point.x)); event.y = std::int16_t(std::lround(height - point.y));
    event.modifiers = original_modifiers(native.modifierFlags);
    if (kind != cm::aot::UIEvent::Kind::mouse_down && (NSEvent.pressedMouseButtons & 1) == 0)
        event.modifiers |= 0x80;
    if (self.window.isKeyWindow) event.modifiers |= 1;
    // Match Carbon's EventTimeToTicks rounding, retaining defined 32-bit wrap.
    event.when = std::uint32_t(std::uint64_t(std::floor(native.timestamp * 60 + 0.5)));
    if (kind == cm::aot::UIEvent::Kind::mouse_down || kind == cm::aot::UIEvent::Kind::mouse_up) {
        if (native.type == NSEventTypeLeftMouseDown || native.type == NSEventTypeLeftMouseUp) event.button = 1;
        else if (native.type == NSEventTypeRightMouseDown || native.type == NSEventTypeRightMouseUp) event.button = 2;
        else event.button = std::uint16_t(native.buttonNumber + 1);
    }
    if (kind == cm::aot::UIEvent::Kind::key_down || kind == cm::aot::UIEvent::Kind::key_up || kind == cm::aot::UIEvent::Kind::auto_key)
        event.message = (std::uint32_t(native.keyCode) << 8) | original_character(native);
    if (kind == cm::aot::UIEvent::Kind::wheel) {
        // deltaY is AppKit's line delta even for a precise trackpad. Retain
        // fractions between events instead of dropping small wheel movements.
        wheelRemainder += native.deltaY;
        event.wheel_delta = std::int32_t(std::trunc(wheelRemainder));
        wheelRemainder -= event.wheel_delta;
        if (!event.wheel_delta) return;
    }
    pending->push_back(std::move(event));
}
- (void)mouseDown:(NSEvent*)event { [self queueInput:event kind:cm::aot::UIEvent::Kind::mouse_down]; }
- (void)mouseUp:(NSEvent*)event { [self queueInput:event kind:cm::aot::UIEvent::Kind::mouse_up]; }
- (void)rightMouseDown:(NSEvent*)event { [self mouseDown:event]; }
- (void)rightMouseUp:(NSEvent*)event { [self mouseUp:event]; }
- (void)otherMouseDown:(NSEvent*)event { [self mouseDown:event]; }
- (void)otherMouseUp:(NSEvent*)event { [self mouseUp:event]; }
- (void)mouseMoved:(NSEvent*)event {
    [self queueInput:event kind:cm::aot::UIEvent::Kind::mouse_moved];
    pending->back().message = 0xfa000000;
}
- (void)keyDown:(NSEvent*)event {
    [self queueInput:event kind:event.isARepeat ? cm::aot::UIEvent::Kind::auto_key : cm::aot::UIEvent::Kind::key_down];
}
- (void)keyUp:(NSEvent*)event { [self queueInput:event kind:cm::aot::UIEvent::Kind::key_up]; }
- (void)scrollWheel:(NSEvent*)event { [self queueInput:event kind:cm::aot::UIEvent::Kind::wheel]; }
- (void)drawRect:(NSRect)dirty {
    (void)dirty;
    if (self.renderedImage) {
        [self.renderedImage drawInRect:self.bounds fromRect:NSZeroRect operation:NSCompositingOperationCopy
                              fraction:1 respectFlipped:YES hints:@{NSImageHintInterpolation:@(NSImageInterpolationNone)}];
    } else if (!paintQueued) {
        paintQueued = true;
        pending->push_back({cm::aot::UIEvent::Kind::update, token});
    }
}
@end

@interface CMNativeAppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate> {
@public
    std::deque<cm::aot::UIEvent>* pending;
    std::set<std::filesystem::path>* hostArguments;
    std::map<std::uint32_t, CMNativeWindow*>* gameWindows;
    bool launching;
}
- (void)originalMenuAction:(NSMenuItem*)sender;
- (void)originalQuitAction:(NSMenuItem*)sender;
- (void)validationClick:(NSMenuItem*)sender;
- (void)validationMove:(NSMenuItem*)sender;
- (void)toggleMatchTurbo:(NSMenuItem*)sender;
@end
@implementation CMNativeAppDelegate
- (void)validationPosition:(BOOL)click {
    // A native accessibility action remains usable when a UI driver's pixel
    // click mapping is lost after a long main-thread simulation. This explicit
    // test-device dialog still delivers ordinary input to the original engine.
    CMNativeWindow* target = nil;
    for (NSWindow* candidate in NSApp.orderedWindows) {
        if ([candidate isKindOfClass:CMNativeWindow.class] &&
            ((CMNativeWindow*)candidate).acceptsActivation) {
            target = (CMNativeWindow*)candidate;
            break;
        }
    }
    if (!target) {
        for (const auto& [token, candidate] : *gameWindows) {
            (void)token;
            if (!candidate.acceptsActivation) continue;
            if (target) { NSBeep(); return; } // Do not guess between hidden game windows.
            target = candidate;
        }
    }
    if (!target) { NSBeep(); return; }
    NSAlert* dialog = [NSAlert new];
    dialog.messageText = click ? @"Validation click" : @"Validation move";
    dialog.informativeText = click
        ? @"Enter a position in the game image, measured from its top-left corner. This sends one unmodified left click to the original game."
        : @"Enter a position in the game image, measured from its top-left corner. This sends one mouse movement to the original game.";
    [dialog addButtonWithTitle:click ? @"Send click" : @"Send move"];
    [dialog addButtonWithTitle:@"Cancel"];
    NSView* fields = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 260, 64)];
    NSTextField* x = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 34, 260, 24)];
    NSTextField* y = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 260, 24)];
    x.placeholderString = @"X coordinate"; x.accessibilityLabel = @"X coordinate";
    y.placeholderString = @"Y coordinate"; y.accessibilityLabel = @"Y coordinate";
    [fields addSubview:x]; [fields addSubview:y];
    dialog.accessoryView = fields;
    dialog.window.initialFirstResponder = x;
    if ([dialog runModal] != NSAlertFirstButtonReturn) return;
    NSInteger horizontal = 0, vertical = 0;
    NSScanner* sx = [NSScanner scannerWithString:x.stringValue];
    NSScanner* sy = [NSScanner scannerWithString:y.stringValue];
    CMNativeView* view = (CMNativeView*)target.contentView;
    if (![sx scanInteger:&horizontal] || !sx.atEnd || ![sy scanInteger:&vertical] || !sy.atEnd ||
        horizontal < 0 || vertical < 0 || horizontal >= view.bounds.size.width || vertical >= view.bounds.size.height) {
        NSBeep(); return;
    }
    const auto point = [view convertPoint:NSMakePoint(horizontal, vertical) toView:nil];
    const auto timestamp = NSProcessInfo.processInfo.systemUptime;
    if (!click) {
        NSEvent* event = [NSEvent mouseEventWithType:NSEventTypeMouseMoved
            location:point modifierFlags:0 timestamp:timestamp
            windowNumber:target.windowNumber context:nil eventNumber:0 clickCount:0 pressure:0];
        [view mouseMoved:event];
        return;
    }
    for (unsigned released = 0; released < 2; ++released) {
        NSEvent* event = [NSEvent mouseEventWithType:released ? NSEventTypeLeftMouseUp : NSEventTypeLeftMouseDown
            location:point modifierFlags:0 timestamp:timestamp + released * 0.08
            windowNumber:target.windowNumber context:nil eventNumber:0 clickCount:1 pressure:released ? 0 : 1];
        [view queueInput:event kind:released ? cm::aot::UIEvent::Kind::mouse_up : cm::aot::UIEvent::Kind::mouse_down];
    }
}
- (void)validationClick:(NSMenuItem*)sender { (void)sender; [self validationPosition:YES]; }
- (void)validationMove:(NSMenuItem*)sender { (void)sender; [self validationPosition:NO]; }
- (void)toggleMatchTurbo:(NSMenuItem*)sender {
    cm::aot::set_match_turbo_enabled(!cm::aot::match_turbo_enabled());
    sender.state = cm::aot::match_turbo_enabled() ? NSControlStateValueOn : NSControlStateValueOff;
}
- (void)originalMenuAction:(NSMenuItem*)sender {
    cm::aot::UIEvent event{cm::aot::UIEvent::Kind::menu};
    event.message = std::uint32_t(sender.tag);
    pending->push_back(std::move(event));
}
- (void)originalQuitAction:(NSMenuItem*)sender {
    (void)sender;
    pending->push_back({cm::aot::UIEvent::Kind::quit});
}
- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    (void)notification;
    launching = false;
    pending->push_back({cm::aot::UIEvent::Kind::launch});
}
- (void)applicationDidBecomeActive:(NSNotification*)notification {
    (void)notification;
    cm::aot::UIEvent event{cm::aot::UIEvent::Kind::suspend_resume};
    event.message = 0x01000001;
    pending->push_back(event);
}
- (void)applicationDidResignActive:(NSNotification*)notification {
    (void)notification;
    cm::aot::UIEvent event{cm::aot::UIEvent::Kind::suspend_resume};
    event.message = 0x01000000;
    pending->push_back(event);
}
- (void)windowDidBecomeKey:(NSNotification*)notification {
    NSWindow* window = notification.object;
    CMNativeView* view = (CMNativeView*)window.contentView;
    cm::aot::UIEvent event{cm::aot::UIEvent::Kind::activate, view->token};
    event.modifiers = 1;
    pending->push_back(event);
}
- (void)windowDidResignKey:(NSNotification*)notification {
    NSWindow* window = notification.object;
    CMNativeView* view = (CMNativeView*)window.contentView;
    pending->push_back({cm::aot::UIEvent::Kind::activate, view->token});
}
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)application {
    (void)application;
    pending->push_back({cm::aot::UIEvent::Kind::quit});
    // Let the original game perform its save/quit checks before it exits.
    return NSTerminateCancel;
}
- (void)application:(NSApplication*)application openFiles:(NSArray<NSString*>*)files {
    cm::aot::UIEvent event{cm::aot::UIEvent::Kind::open_files};
    for (NSString* path in files) {
        const auto file = std::filesystem::absolute(path.fileSystemRepresentation).lexically_normal();
        // The native host consumes its CLI arguments as segment/game paths.
        // AppKit also reports those arguments as documents during launch.
        // Preserve genuine Finder requests and all later open-file events.
        if (!launching || !hostArguments->contains(file)) event.files.push_back(file);
    }
    if (!event.files.empty()) pending->push_back(std::move(event));
    [application replyToOpenOrPrint:NSApplicationDelegateReplySuccess];
}
@end

namespace cm::aot {
bool match_turbo_enabled() { return match_turbo; }
void set_match_turbo_enabled(bool enabled) { match_turbo = enabled; }
std::uint16_t legacy_event_mask(const UIEvent& event) {
    switch (event.kind) {
        case UIEvent::Kind::launch: case UIEvent::Kind::quit: case UIEvent::Kind::open_files: return 0x0400;
        case UIEvent::Kind::mouse_down: case UIEvent::Kind::menu: return 2;
        case UIEvent::Kind::mouse_up: return 4;
        case UIEvent::Kind::key_down: return 8;
        case UIEvent::Kind::key_up: return 16;
        case UIEvent::Kind::auto_key: return 32;
        case UIEvent::Kind::update: return 64;
        case UIEvent::Kind::activate: return 256;
        case UIEvent::Kind::suspend_resume: case UIEvent::Kind::mouse_moved: return 0x8000;
        case UIEvent::Kind::wheel: return 0; // Delivered through the Carbon handler, not EventRecord.
    }
}
namespace {
std::uint16_t native_event_mask(NSEvent* event) {
    switch (event.type) {
        case NSEventTypeLeftMouseDown: case NSEventTypeRightMouseDown: case NSEventTypeOtherMouseDown: return 2;
        case NSEventTypeLeftMouseUp: case NSEventTypeRightMouseUp: case NSEventTypeOtherMouseUp: return 4;
        case NSEventTypeKeyDown: return event.isARepeat ? 32 : 8;
        case NSEventTypeKeyUp: return 16;
        case NSEventTypeMouseMoved: return 0x8000;
        default: return 0;
    }
}
NSString* menu_text(const std::string& bytes) {
    NSString* value = [[NSString alloc] initWithBytes:bytes.data() length:bytes.size() encoding:NSMacOSRomanStringEncoding];
    if (!value) throw std::runtime_error("Invalid original menu text");
    return value;
}
constexpr std::uint32_t quit_command = 0x71756974;
}
std::vector<LegacyRect> NativeUI::desktop_rectangles() const {
    @autoreleasepool {
        // AppKit supplies the current usable desktop, accounting for menu bars
        // and the Dock. Convert Cocoa's upward Y axis to original global space.
        const auto height = CGRectGetHeight(CGDisplayBounds(CGMainDisplayID()));
        std::vector<LegacyRect> result;
        for (NSScreen* screen in NSScreen.screens) {
            const auto area = screen.visibleFrame;
            const std::array<double, 4> values{height - NSMaxY(area), NSMinX(area), height - NSMinY(area), NSMaxX(area)};
            LegacyRect rectangle;
            for (unsigned i = 0; i < 4; ++i) {
                if (values[i] < INT16_MIN || values[i] >= INT16_MAX)
                    throw std::runtime_error("Native desktop exceeds original region coordinate limits");
                rectangle[i] = std::int16_t(std::lround(values[i]));
            }
            result.push_back(rectangle);
        }
        if (result.empty()) throw std::runtime_error("Native desktop has no connected screens");
        return result;
    }
}
struct NativeUI::Impl {
    std::deque<UIEvent> pending;
    std::set<std::filesystem::path> host_arguments;
    CMNativeAppDelegate* delegate;
    bool finished_launch = false;
    // Only the isolated validation app opts in. Some UI drivers deliver an
    // AppKit event without moving the physical desktop pointer. Use that event's
    // position for subsequent legacy GetMouse queries in this test mode.
    bool event_pointer = [] {
        const auto value = std::getenv("CM_AOT_VALIDATION_EVENT_POINTER");
        return value && std::strcmp(value, "1") == 0;
    }();
    std::optional<NativePointerState> delivered_pointer;
    std::chrono::steady_clock::time_point validation_release_after{};
    struct Menu { NSMenu* native; std::int16_t id; bool enabled; };
    std::map<std::uint32_t, Menu> menus;
    std::map<std::uint32_t, CMNativeWindow*> windows;
    std::map<std::uint32_t, NSCursor*> cursors;
    std::map<std::uint32_t, EventRef> events;
    std::vector<std::uint32_t> menu_bar;
    // Commands and original menu selection numbers are distinct values.
    std::map<std::pair<std::uint32_t, std::uint16_t>, std::uint32_t> commands;
    void append(std::uint32_t token, const MenuItem& item) {
        auto& menu = menus.at(token);
        const auto index = std::uint16_t(menu.native.numberOfItems + 1);
        NSMenuItem* native;
        if (item.text == "-") native = [NSMenuItem separatorItem];
        else {
            if (item.key == 0x1b) throw std::runtime_error("Hierarchical menu resources require a submenu bridge");
            NSString* key = item.key ? [menu_text(std::string(1, char(item.key))) lowercaseString] : @"";
            native = [[NSMenuItem alloc] initWithTitle:menu_text(item.text)
                action:@selector(originalMenuAction:) keyEquivalent:key];
            native.target = delegate;
            native.enabled = item.enabled;
            native.state = item.mark ? NSControlStateValueOn : NSControlStateValueOff;
            native.tag = (std::uint32_t(std::uint16_t(menu.id)) << 16) | index;
            if (item.key == 'Q' || item.key == 'q') commands[{token, index}] = quit_command;
            if (item.style) {
                NSFontTraitMask traits = 0;
                if (item.style & 1) traits |= NSBoldFontMask;
                if (item.style & 2) traits |= NSItalicFontMask;
                NSFont* font = [[NSFontManager sharedFontManager] convertFont:[NSFont menuFontOfSize:0] toHaveTrait:traits];
                NSMutableDictionary* attributes = [@{NSFontAttributeName:font} mutableCopy];
                if (item.style & 4) attributes[NSUnderlineStyleAttributeName] = @(NSUnderlineStyleSingle);
                native.attributedTitle = [[NSAttributedString alloc] initWithString:native.title attributes:attributes];
            }
        }
        [menu.native addItem:native];
    }
    Impl() {
      @autoreleasepool {
        if (![NSThread isMainThread]) throw std::runtime_error("Native UI requires the macOS main thread");
        [NSApplication sharedApplication];
        delegate = [CMNativeAppDelegate new];
        delegate->pending = &pending;
        delegate->gameWindows = &windows;
        NSArray<NSString*>* arguments = NSProcessInfo.processInfo.arguments;
        for (NSUInteger i = 1; i < arguments.count; ++i)
            host_arguments.insert(std::filesystem::absolute(arguments[i].fileSystemRepresentation).lexically_normal());
        delegate->hostArguments = &host_arguments;
        delegate->launching = true;
        [NSApp setDelegate:delegate];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
      }
    }
    ~Impl() {
        for (const auto& [token, event] : events) { (void)token; ReleaseEvent(event); }
        for (const auto& [token, window] : windows) { (void)token; [window orderOut:nil]; [window close]; }
        if (NSApp.delegate == delegate) [NSApp setDelegate:nil];
    }
};
NativeUI::NativeUI() : impl_(std::make_unique<Impl>()) {}
NativeUI::~NativeUI() = default;
void NativeUI::create_window(std::uint32_t token, std::uint32_t kind, std::uint32_t attributes, const LegacyRect& bounds) {
    @autoreleasepool {
        if (impl_->windows.contains(token)) throw std::runtime_error("Duplicate native window token");
        if (kind != 13 || (attributes & ~0x03220000u))
            throw std::runtime_error("Original window class or attributes require an additional AppKit mapping");
        if (bounds[2] <= bounds[0] || bounds[3] <= bounds[1]) throw std::runtime_error("Invalid original window bounds");
        const auto height = CGRectGetHeight(CGDisplayBounds(CGMainDisplayID()));
        const NSRect rect = NSMakeRect(bounds[1], height - bounds[2], bounds[3] - bounds[1], bounds[2] - bounds[0]);
        const bool no_activate = (attributes & 0x00020000u) != 0;
        const auto style = NSWindowStyleMaskBorderless | (no_activate ? NSWindowStyleMaskNonactivatingPanel : 0);
        CMNativeWindow* window = [[CMNativeWindow alloc] initWithContentRect:rect styleMask:style
            backing:NSBackingStoreBuffered defer:NO];
        window.acceptsActivation = !no_activate;
        window.floatingPanel = NO;
        window.releasedWhenClosed = NO;
        window.hasShadow = (attributes & 0x00200000u) == 0;
        window.hidesOnDeactivate = (attributes & 0x01000000u) != 0;
        window.becomesKeyOnlyIfNeeded = no_activate;
        window.acceptsMouseMovedEvents = YES;
        window.backgroundColor = NSColor.clearColor;
        window.opaque = NO;
        CMNativeView* view = [[CMNativeView alloc] initWithFrame:NSMakeRect(0, 0, rect.size.width, rect.size.height)];
        view->pending = &impl_->pending; view->token = token; view->paintQueued = false;
        window.contentView = view;
        [window makeFirstResponder:view];
        window.delegate = impl_->delegate;
        // Carbon accepts the supplied global bounds, including the original
        // background window's intentional extension beyond desktop edges.
        [window setFrame:rect display:NO];
        impl_->windows.emplace(token, window);
    }
}
void NativeUI::dispose_window(std::uint32_t token) {
    auto window = impl_->windows.at(token);
    [window orderOut:nil]; [window close];
    impl_->windows.erase(token);
    std::erase_if(impl_->pending, [token](const auto& event) { return event.window == token; });
}
void NativeUI::show_window(std::uint32_t token, bool activate) {
    auto window = impl_->windows.at(token);
    if (activate && window.acceptsActivation) [window makeKeyAndOrderFront:nil];
    else [window orderFront:nil];
    CMNativeView* view = (CMNativeView*)window.contentView;
    view.needsDisplay = YES;
    // Original ShowWindow invalidates a newly exposed window even when Quartz
    // defers its physical display (for example with no active display). Queue
    // that invalidation now; drawRect coalesces with the same pending request.
    if (!view->paintQueued && !view.renderedImage) {
        view->paintQueued = true;
        impl_->pending.push_back({UIEvent::Kind::update, token});
    }
    [window displayIfNeeded];
}
LegacyRect NativeUI::window_bounds(std::uint32_t token, bool content) const {
    const auto window = impl_->windows.at(token);
    const auto rect = content ? [window contentRectForFrameRect:window.frame] : window.frame;
    const auto height = CGRectGetHeight(CGDisplayBounds(CGMainDisplayID()));
    const std::array<double, 4> values{height - NSMaxY(rect), NSMinX(rect), height - NSMinY(rect), NSMaxX(rect)};
    LegacyRect result;
    for (unsigned i = 0; i < 4; ++i) {
        if (values[i] < INT16_MIN || values[i] > INT16_MAX) throw std::runtime_error("Native window exceeds original coordinate limits");
        result[i] = std::int16_t(std::lround(values[i]));
    }
    return result;
}
std::int32_t NativeUI::window_number(std::uint32_t token) const { return impl_->windows.at(token).windowNumber; }
NativeWindowHit NativeUI::find_window(std::int16_t x, std::int16_t y) const {
    @autoreleasepool {
        const auto main = CGDisplayBounds(CGMainDisplayID());
        // Classic FindWindow gives the main menu bar precedence over windows.
        const auto menu_height = NSApp.mainMenu.menuBarHeight;
        if (NSMenu.menuBarVisible && menu_height > 0 && x >= CGRectGetMinX(main) && x < CGRectGetMaxX(main) &&
            y >= 0 && y < menu_height) return {1, 0};
        // Ask AppKit for front-to-back order, including non-key plain windows.
        // Only class-13 borderless windows are supported by create_window, so
        // every point in their half-open content rectangle is inContent.
        for (NSNumber* number in [NSWindow windowNumbersWithOptions:0]) {
            for (const auto& [token, owned] : impl_->windows) {
                if (number.integerValue != owned.windowNumber || !owned.visible || owned.miniaturized) continue;
                const auto bounds = window_bounds(token, true);
                if (x >= bounds[1] && x < bounds[3] && y >= bounds[0] && y < bounds[2]) return {3, token};
            }
        }
        return {0, 0};
    }
}
void NativeUI::set_window_title(std::uint32_t token, const std::string& title) {
    @autoreleasepool { impl_->windows.at(token).title = [[NSString alloc] initWithUTF8String:title.c_str()]; }
}
void NativeUI::present_window(std::uint32_t token, std::uint32_t width, std::uint32_t height,
                              std::uint32_t row_bytes, const std::vector<std::uint8_t>& argb) {
    @autoreleasepool {
        if (!width || !height || row_bytes < std::uint64_t(width) * 4 || argb.size() != std::uint64_t(row_bytes) * height)
            throw std::runtime_error("Invalid native window pixel buffer");
        const auto data = CFDataCreate(nullptr, argb.data(), argb.size());
        const auto provider = CGDataProviderCreateWithCFData(data);
        const auto colors = CGColorSpaceCreateDeviceRGB();
        const auto image = CGImageCreate(width, height, 8, 32, row_bytes, colors,
            CGBitmapInfo(kCGImageAlphaNoneSkipFirst) | CGBitmapInfo(kCGBitmapByteOrder32Big), provider, nullptr, false, kCGRenderingIntentDefault);
        CGColorSpaceRelease(colors); CGDataProviderRelease(provider); CFRelease(data);
        if (!image) throw std::runtime_error("Native Quartz rejected the original window pixel buffer");
        CMNativeView* view = (CMNativeView*)impl_->windows.at(token).contentView;
        view.renderedImage = [[NSImage alloc] initWithCGImage:image size:NSMakeSize(width, height)];
        CGImageRelease(image);
        view.needsDisplay = YES;
        [view displayIfNeeded];
    }
}
std::vector<std::filesystem::path> NativeUI::save_window_images(const std::filesystem::path& directory) const {
    @autoreleasepool {
        std::filesystem::create_directories(directory);
        std::vector<std::filesystem::path> paths;
        for (const auto& [token, window] : impl_->windows) {
            CMNativeView* view = (CMNativeView*)window.contentView;
            if (!view.renderedImage) continue;
            const auto image = [view.renderedImage CGImageForProposedRect:nullptr context:nil hints:nil];
            NSBitmapImageRep* bitmap = [[NSBitmapImageRep alloc] initWithCGImage:image];
            NSData* png = [bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
            const auto path = std::filesystem::absolute(directory / ("window-" + std::to_string(token) + ".png"));
            NSError* error = nil;
            if (![png writeToFile:[NSString stringWithUTF8String:path.c_str()] options:NSDataWritingAtomic error:&error])
                throw std::runtime_error("Cannot save native window image: " + std::string(error.localizedDescription.UTF8String));
            paths.push_back(path);
        }
        return paths;
    }
}
std::vector<NativeDisplay> NativeUI::displays() const {
    std::uint32_t count = 0;
    std::vector<CGDirectDisplayID> ids(8);
    while (true) {
        // GetMainDevice must still describe a connected main screen while the
        // displays are asleep. The active list can legitimately be empty then.
        const auto error = CGGetOnlineDisplayList(ids.size(), ids.data(), &count);
        if (error != kCGErrorSuccess) throw std::runtime_error("Cannot enumerate native displays: " + std::to_string(error));
        if (!count) throw std::runtime_error("No connected native display is available");
        if (count < ids.size()) break;
        if (ids.size() >= 1024) throw std::runtime_error("Native display count exceeds supported bounds");
        ids.resize(ids.size() * 2);
    }
    std::vector<NativeDisplay> result;
    const auto main = CGMainDisplayID();
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto rect = CGDisplayBounds(ids[i]);
        const std::array<double, 4> values{CGRectGetMinY(rect), CGRectGetMinX(rect), CGRectGetMaxY(rect), CGRectGetMaxX(rect)};
        NativeDisplay display{ids[i], {}, ids[i] == main, CGDisplayIsActive(ids[i]) != 0};
        for (unsigned field = 0; field < 4; ++field) {
            if (!std::isfinite(values[field]) || values[field] < INT16_MIN || values[field] > INT16_MAX)
                throw std::runtime_error("Native display exceeds the original QuickDraw coordinate range");
            display.bounds[field] = std::int16_t(std::lround(values[field]));
        }
        result.push_back(display);
    }
    return result;
}
void NativeUI::initialize_cursor() { @autoreleasepool { [[NSCursor arrowCursor] set]; } }
void NativeUI::create_cursor(std::uint32_t token, const ColorCursor& cursor) {
    @autoreleasepool {
        const auto data = CFDataCreate(nullptr, cursor.rgba.data(), cursor.rgba.size());
        const auto provider = CGDataProviderCreateWithCFData(data);
        const auto colors = CGColorSpaceCreateDeviceRGB();
        const auto image = CGImageCreate(16, 16, 8, 32, 64, colors, kCGImageAlphaLast, provider, nullptr, false, kCGRenderingIntentDefault);
        CGColorSpaceRelease(colors); CGDataProviderRelease(provider); CFRelease(data);
        if (!image) throw std::runtime_error("Quartz rejected the original color cursor image");
        NSImage* bitmap = [[NSImage alloc] initWithCGImage:image size:NSMakeSize(16, 16)];
        CGImageRelease(image);
        impl_->cursors.insert_or_assign(token, [[NSCursor alloc] initWithImage:bitmap hotSpot:NSMakePoint(cursor.x, cursor.y)]);
    }
}
void NativeUI::set_cursor(std::uint32_t token) { [impl_->cursors.at(token) set]; }
void NativeUI::dispose_cursor(std::uint32_t token) {
    const auto found = impl_->cursors.find(token);
    if (found == impl_->cursors.end()) throw std::runtime_error("Invalid or double native cursor disposal");
    if (NSCursor.currentCursor == found->second) [[NSCursor arrowCursor] set];
    impl_->cursors.erase(found);
}
NativePointerState NativeUI::pointer_state() const {
    if (impl_->event_pointer && impl_->delivered_pointer) return *impl_->delivered_pointer;
    const auto point = NSEvent.mouseLocation;
    const auto height = CGRectGetHeight(CGDisplayBounds(CGMainDisplayID()));
    const auto x = std::lround(point.x), y = std::lround(height - point.y);
    if (x < INT16_MIN || x > INT16_MAX || y < INT16_MIN || y > INT16_MAX)
        throw std::runtime_error("Native mouse position exceeds original Point coordinates");
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    const bool down = ::Button();
#pragma clang diagnostic pop
    return {std::int16_t(x), std::int16_t(y), std::uint16_t(::GetCurrentKeyModifiers() | (down ? 0 : 0x80)), down};
}
std::array<std::uint8_t, 16> NativeUI::key_map() const {
    std::array<std::uint8_t, 16> result{};
    if (NSApp.isActive) {
        for (unsigned key = 0; key < 128; ++key)
            if (CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, key))
                result[key / 8] |= 1u << (key % 8);
    }
    return result;
}
bool NativeUI::poll(UIEvent& event, double timeout_seconds) {
    @autoreleasepool {
        if (!impl_->finished_launch) {
            impl_->finished_launch = true;
            [NSApp finishLaunching];
        }
        NSDate* deadline = [NSDate dateWithTimeIntervalSinceNow:timeout_seconds];
        while (impl_->pending.empty()) {
            NSEvent* native = [NSApp nextEventMatchingMask:NSEventMaskAny
                untilDate:deadline
                inMode:NSDefaultRunLoopMode dequeue:YES];
            if (native) {
                [NSApp sendEvent:native];
                [NSApp updateWindows];
            }
            if (!native) break;
        }
        if (impl_->pending.empty()) return false;
        if (impl_->event_pointer && impl_->pending.front().kind == UIEvent::Kind::mouse_down) {
            const auto& click = impl_->pending.front();
            if (!impl_->delivered_pointer || impl_->delivered_pointer->x != click.x || impl_->delivered_pointer->y != click.y) {
                // A test device moves before pressing. Return the original
                // mouse-moved OS event first, leaving the click in queue. The
                // game's next GetMouse (before WaitNextEvent) then sees the new
                // coordinates before it handles the press. Merely peeking the
                // AppKit queue races with clicks arriving inside WaitNextEvent.
                impl_->delivered_pointer = NativePointerState{click.x, click.y,
                    std::uint16_t(click.modifiers | 0x80), false};
                event = click;
                event.kind = UIEvent::Kind::mouse_moved;
                event.message = 0xfa000000;
                event.button = 0;
                event.modifiers |= 0x80;
                return true;
            }
        }
        if (impl_->event_pointer && impl_->pending.front().kind == UIEvent::Kind::mouse_up) {
            // UI drivers may inject press/release one millisecond apart. The
            // game's polling loop can consume both before its next UI update.
            // Model an 80 ms test gesture, without changing simulation clocks,
            // normal input, or queue order. Never wait beyond the caller's wait.
            const auto remaining = impl_->validation_release_after - std::chrono::steady_clock::now();
            if (remaining > std::chrono::steady_clock::duration::zero()) {
                if (timeout_seconds > 0)
                    std::this_thread::sleep_for(std::min(std::chrono::duration<double>(remaining),
                                                        std::chrono::duration<double>(timeout_seconds)));
                if (std::chrono::steady_clock::now() < impl_->validation_release_after) return false;
            }
        }
        event = std::move(impl_->pending.front());
        impl_->pending.pop_front();
        if (impl_->event_pointer && (event.kind == UIEvent::Kind::mouse_down ||
                                    event.kind == UIEvent::Kind::mouse_up ||
                                    event.kind == UIEvent::Kind::mouse_moved)) {
            impl_->delivered_pointer = NativePointerState{event.x, event.y, event.modifiers,
                event.kind == UIEvent::Kind::mouse_down && event.button == 1};
            if (event.kind == UIEvent::Kind::mouse_down)
                impl_->validation_release_after = std::chrono::steady_clock::now() + std::chrono::milliseconds(80);
        }
        return true;
    }
}
void NativeUI::begin_event(std::uint32_t token, std::uint32_t event_class, std::uint32_t kind, const UIEvent& input) {
    EventRef event = nullptr;
    auto error = CreateEvent(nullptr, event_class, kind, GetCurrentEventTime(), 0, &event);
    if (error) throw std::runtime_error("Cannot create native Carbon event: " + std::to_string(error));
    const UInt32 modifiers = input.modifiers & 0x1f00;
    error = SetEventParameter(event, kEventParamKeyModifiers, typeUInt32, sizeof(modifiers), &modifiers);
    if (!error && input.button) error = SetEventParameter(event, kEventParamMouseButton, typeMouseButton, sizeof(input.button), &input.button);
    if (!error && input.kind == UIEvent::Kind::wheel)
        error = SetEventParameter(event, kEventParamMouseWheelDelta, typeSInt32, sizeof(input.wheel_delta), &input.wheel_delta);
    if (error) { ReleaseEvent(event); throw std::runtime_error("Cannot set native event parameter: " + std::to_string(error)); }
    impl_->events.emplace(token, event);
}
void NativeUI::end_event(std::uint32_t token) {
    ReleaseEvent(impl_->events.at(token)); impl_->events.erase(token);
}
NativeEventParameter NativeUI::event_parameter(std::uint32_t token, std::uint32_t name,
                                               std::uint32_t type, std::uint32_t size, bool data) {
    NativeEventParameter result;
    std::array<std::uint8_t, 8> native{};
    ByteCount actual_size = UINT32_MAX;
    result.error = GetEventParameter(impl_->events.at(token), name, type, &result.type,
        data ? std::min<std::uint32_t>(size, native.size()) : size, &actual_size, data ? native.data() : nullptr);
    if (actual_size > UINT32_MAX) throw std::runtime_error("Native event parameter exceeds the original size range");
    result.size = std::uint32_t(actual_size);
    if (result.error || !data) return result;
    // Only scalar input parameters are registered above. Let the native event
    // manager validate sizes, missing parameters, and coercions before encoding.
    std::uint32_t value = 0;
    if (result.size == 2 && result.type == typeMouseButton) {
        std::uint16_t button; std::memcpy(&button, native.data(), 2); value = button;
    } else if (result.size == 4 && (result.type == typeUInt32 || result.type == typeSInt32))
        std::memcpy(&value, native.data(), 4);
    else throw std::runtime_error("Native event parameter type needs an original ABI mapping");
    result.bytes.resize(result.size);
    for (unsigned i = 0; i < result.size; ++i) result.bytes[result.size - 1 - i] = std::uint8_t(value >> (i * 8));
    return result;
}
bool NativeUI::take_window_paint(UIEvent& event) {
    const auto found = std::find_if(impl_->pending.begin(), impl_->pending.end(), [](const auto& item) {
        return item.kind == UIEvent::Kind::update;
    });
    if (found == impl_->pending.end()) return false;
    event = std::move(*found); impl_->pending.erase(found);
    return true;
}
void NativeUI::flush_events(std::uint16_t which, std::uint16_t stop) {
    @autoreleasepool {
        for (auto it = impl_->pending.begin(); it != impl_->pending.end();) {
            const auto mask = legacy_event_mask(*it);
            if (mask & stop) return;
            if (mask & which) it = impl_->pending.erase(it);
            else ++it;
        }
        NSMutableArray<NSEvent*>* preserved = [NSMutableArray array];
        bool finished = false;
        for (unsigned count = 0; count < 10000; ++count) {
            NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:NSDate.distantPast
                                                 inMode:NSDefaultRunLoopMode dequeue:YES];
            if (!event) { finished = true; break; }
            const auto mask = native_event_mask(event);
            if (mask & stop) { [NSApp postEvent:event atStart:YES]; finished = true; break; }
            if (!(mask & which)) [preserved addObject:event];
        }
        for (NSEvent* event in preserved.reverseObjectEnumerator) [NSApp postEvent:event atStart:YES];
        if (!finished) throw std::runtime_error("Native event flush exceeded its bounded queue snapshot");
    }
}
void NativeUI::create_menu(std::uint32_t token, const MenuDefinition& menu) {
    @autoreleasepool {
        if (impl_->menus.contains(token)) throw std::runtime_error("Duplicate native menu token");
        const bool application = menu.title == std::string(1, char(0x14));
        NSMenu* native = [[NSMenu alloc] initWithTitle:application ? @"CM 01/02" : menu_text(menu.title)];
        native.autoenablesItems = NO;
        impl_->menus.emplace(token, Impl::Menu{native, menu.id, menu.enabled});
        for (const auto& item : menu.items) impl_->append(token, item);
        if (application) {
            // Carbon supplies an application Quit command in addition to an
            // old MENU resource's File/Quit. The game removes that duplicate.
            NSMenuItem* quit = [[NSMenuItem alloc] initWithTitle:@"Quit CM 01/02"
                action:@selector(originalQuitAction:) keyEquivalent:@"q"];
            quit.target = impl_->delegate;
            [native addItem:quit];
            impl_->commands[{token, std::uint16_t(native.numberOfItems)}] = quit_command;
        }
    }
}
void NativeUI::set_menu_bar(const std::vector<std::uint32_t>& menus) {
    @autoreleasepool {
        NSMenu* bar = [[NSMenu alloc] initWithTitle:@""];
        bar.autoenablesItems = NO;
        for (auto token : menus) {
            const auto& menu = impl_->menus.at(token);
            NSMenuItem* top = [[NSMenuItem alloc] initWithTitle:menu.native.title action:nil keyEquivalent:@""];
            top.enabled = menu.enabled;
            top.submenu = menu.native;
            [bar addItem:top];
        }
        NSMenuItem* match = [[NSMenuItem alloc] initWithTitle:@"Match" action:nil keyEquivalent:@""];
        NSMenu* match_menu = [[NSMenu alloc] initWithTitle:@"Match"];
        NSMenuItem* turbo = [[NSMenuItem alloc] initWithTitle:@"Turbo"
            action:@selector(toggleMatchTurbo:) keyEquivalent:@"t"];
        turbo.keyEquivalentModifierMask = NSEventModifierFlagCommand;
        turbo.target = impl_->delegate;
        turbo.state = match_turbo_enabled() ? NSControlStateValueOn : NSControlStateValueOff;
        [match_menu addItem:turbo];
        match.submenu = match_menu;
        [bar addItem:match];
        if (impl_->event_pointer) {
            NSMenuItem* top = [[NSMenuItem alloc] initWithTitle:@"Validation" action:nil keyEquivalent:@""];
            NSMenu* menu = [[NSMenu alloc] initWithTitle:@"Validation"];
            NSMenuItem* click = [[NSMenuItem alloc] initWithTitle:@"Click at position…"
                action:@selector(validationClick:) keyEquivalent:@""];
            click.target = impl_->delegate;
            [menu addItem:click];
            NSMenuItem* move = [[NSMenuItem alloc] initWithTitle:@"Move to position…"
                action:@selector(validationMove:) keyEquivalent:@""];
            move.target = impl_->delegate;
            [menu addItem:move];
            top.submenu = menu;
            [bar addItem:top];
        }
        impl_->menu_bar = menus;
        [NSApp setMainMenu:bar];
    }
}
std::uint32_t NativeUI::menu_key(std::uint8_t key) const {
    @autoreleasepool {
        if (!key) return 0;
        NSString* equivalent = [menu_text(std::string(1, char(key))) lowercaseString];
        for (const auto token : impl_->menu_bar) {
            const auto& menu = impl_->menus.at(token);
            if (!menu.enabled) continue;
            for (NSMenuItem* item in menu.native.itemArray) {
                if (item.enabled && !item.separatorItem &&
                    [item.keyEquivalent isEqualToString:equivalent])
                    return std::uint32_t(item.tag);
            }
        }
        return 0;
    }
}
std::optional<std::pair<std::uint32_t, std::uint16_t>> NativeUI::find_menu_command(
    std::uint32_t menu, std::uint32_t command, std::uint32_t index) const {
    if (!index) return {};
    const auto targets = menu ? std::vector<std::uint32_t>{menu} : impl_->menu_bar;
    for (auto token : targets) {
        const auto count = impl_->menus.at(token).native.numberOfItems;
        for (std::uint16_t item = 1; item <= count; ++item) {
            const auto found = impl_->commands.find({token, item});
            if (found != impl_->commands.end() && found->second == command && --index == 0)
                return std::pair{token, item};
        }
    }
    return {};
}
void NativeUI::delete_menu_item(std::uint32_t menu, std::uint16_t item) {
    @autoreleasepool {
        auto& entry = impl_->menus.at(menu);
        if (!item || item > entry.native.numberOfItems) return;
        [entry.native removeItemAtIndex:item - 1];
        std::map<std::pair<std::uint32_t, std::uint16_t>, std::uint32_t> commands;
        for (const auto& [key, command] : impl_->commands) {
            if (key.first != menu || key.second < item) commands[key] = command;
            else if (key.second > item) commands[{menu, std::uint16_t(key.second - 1)}] = command;
        }
        impl_->commands = std::move(commands);
        for (NSInteger index = 0; index < entry.native.numberOfItems; ++index) {
            NSMenuItem* native = [entry.native itemAtIndex:index];
            if (native.action == @selector(originalMenuAction:))
                native.tag = (std::uint32_t(std::uint16_t(entry.id)) << 16) | (index + 1);
        }
    }
}
void NativeUI::append_menu_item(std::uint32_t menu, const MenuItem& item) {
    @autoreleasepool { impl_->append(menu, item); }
}
std::uint16_t NativeUI::count_menu_items(std::uint32_t menu) const {
    return impl_->menus.at(menu).native.numberOfItems;
}
std::string NativeUI::menu_item_text(std::uint32_t menu, std::uint16_t item) const {
    @autoreleasepool {
        NSMenu* native = impl_->menus.at(menu).native;
        if (!item || item > native.numberOfItems) throw std::runtime_error("Invalid menu item index");
        NSMenuItem* entry = [native itemAtIndex:item - 1];
        if (entry.isSeparatorItem) return "-";
        NSData* data = [entry.title dataUsingEncoding:NSMacOSRomanStringEncoding];
        if (!data) throw std::runtime_error("Native menu text cannot be represented in the original encoding");
        return {static_cast<const char*>(data.bytes), data.length};
    }
}
void NativeUI::redraw_menu_bar() { @autoreleasepool { [NSApp.mainMenu update]; } }
void NativeUI::set_menu_item_text(std::uint32_t menu, std::uint16_t item, const std::string& text) {
    @autoreleasepool {
        NSMenu* native = impl_->menus.at(menu).native;
        if (!item || item > native.numberOfItems) throw std::runtime_error("Invalid menu item text index");
        NSMenuItem* entry = [native itemAtIndex:item - 1];
        if (entry.isSeparatorItem) {
            if (text != "-") throw std::runtime_error("Changing a menu separator into an item requires rebuilding its action");
            return;
        }
        entry.title = menu_text(text);
        if (entry.attributedTitle.length)
            entry.attributedTitle = [[NSAttributedString alloc] initWithString:entry.title
                attributes:[entry.attributedTitle attributesAtIndex:0 effectiveRange:nullptr]];
    }
}
}
