#include "ui_bridge.hpp"
#include "native_drawing.hpp"
#include "match_turbo.hpp"
#include "match_speed.hpp"
#include "match_debugging.hpp"
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
#include <functional>

namespace {
bool match_turbo = false;
bool native_speed_enabled = false;
std::uint16_t selected_match_speed = cm::aot::turbo_match_speed;
std::filesystem::path match_speed_preference;
bool debugging_available = false, debugging_enabled = false;
std::filesystem::path debugging_preference;
bool repeated_key_event(NSEvent* event) {
    // isARepeat is undefined for mouse/accessibility menu events and AppKit
    // raises NSInternalInconsistencyException if queried for a non-key event.
    return (event.type == NSEventTypeKeyDown || event.type == NSEventTypeKeyUp) && event.isARepeat;
}
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
    cm::aot::LegacyRect logicalBounds;
    CGImageRef sourceImage;
    CGImageRef nativeBackground;
    std::shared_ptr<const cm::aot::NativeDrawing> nativeDrawing;
    std::function<cm::aot::PresentationPoint(cm::aot::PresentationPoint)> inversePoint;
}
@property(nonatomic, strong) NSImage* renderedImage;
@end
@implementation CMNativeView
- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent*)event { (void)event; return YES; }
- (void)queueInput:(NSEvent*)native kind:(cm::aot::UIEvent::Kind)kind {
    cm::aot::UIEvent event{kind, token};
    const auto host = [self.window convertPointToScreen:native.locationInWindow];
    const auto point = inversePoint({host.x, CGRectGetHeight(CGDisplayBounds(CGMainDisplayID())) - host.y});
    // One inverse for window events and continuous polling. Positions outside
    // the scaled game area stay outside the original panel.
    event.x = std::int16_t(std::clamp(std::floor(point.x), double(INT16_MIN), double(INT16_MAX)));
    event.y = std::int16_t(std::clamp(std::floor(point.y), double(INT16_MIN), double(INT16_MAX)));
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
- (void)mouseDragged:(NSEvent*)event { [self mouseMoved:event]; }
- (void)rightMouseDragged:(NSEvent*)event { [self mouseMoved:event]; }
- (void)otherMouseDragged:(NSEvent*)event { [self mouseMoved:event]; }
- (void)dealloc {
    if (sourceImage) CGImageRelease(sourceImage);
    if (nativeBackground) CGImageRelease(nativeBackground);
}
- (void)keyDown:(NSEvent*)event {
    // AppKit handles enabled command shortcuts first. Swallow disabled/repeated
    // command equivalents here instead of leaking them into bitmap text fields
    // or the original Command-S sound toggle.
    if ((event.modifierFlags & NSEventModifierFlagCommand) &&
        (event.keyCode == 1 || event.keyCode == 31 || event.keyCode == 36 || event.keyCode == 43 || event.keyCode == 33)) {
        NSBeep(); return;
    }
    [self queueInput:event kind:event.isARepeat ? cm::aot::UIEvent::Kind::auto_key : cm::aot::UIEvent::Kind::key_down];
}
- (void)keyUp:(NSEvent*)event { [self queueInput:event kind:cm::aot::UIEvent::Kind::key_up]; }
- (void)scrollWheel:(NSEvent*)event { [self queueInput:event kind:cm::aot::UIEvent::Kind::wheel]; }
- (void)drawRect:(NSRect)dirty {
    (void)dirty;
    if (self.renderedImage) {
        // Draw original pixels directly into AppKit's backing context. Do not
        // rasterize a resized NSImage or multiply the user scale by Retina.
        CGContextRef context = NSGraphicsContext.currentContext.CGContext;
        CGContextSaveGState(context);
        CGContextSetInterpolationQuality(context, kCGInterpolationNone);
        CGContextSetBlendMode(context, kCGBlendModeCopy);
        CGContextTranslateCTM(context, 0, self.bounds.size.height);
        CGContextScaleCTM(context, 1, -1);
        CGContextDrawImage(context, NSRectToCGRect(self.bounds), nativeBackground ? nativeBackground : sourceImage);
        CGContextRestoreGState(context);
        if (nativeDrawing) {
            CGContextSaveGState(context);
            CGContextScaleCTM(context, self.bounds.size.width / nativeDrawing->width,
                             self.bounds.size.height / nativeDrawing->height);
            cm::aot::draw_native_shapes(context, *nativeDrawing);
            CGContextRestoreGState(context);
        }
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
    std::function<void()> refreshPresentation;
    std::function<void(NSInteger)> selectScale;
    std::function<void(cm::aot::DirectCommand)> selectCommand;
}
- (void)originalMenuAction:(NSMenuItem*)sender;
- (void)interfaceScaleAction:(NSMenuItem*)sender;
- (void)directCommandAction:(NSMenuItem*)sender;
- (void)originalQuitAction:(NSMenuItem*)sender;
- (void)validationClick:(NSMenuItem*)sender;
- (void)validationMove:(NSMenuItem*)sender;
@end
@implementation CMNativeAppDelegate
- (void)interfaceScaleAction:(NSMenuItem*)sender { selectScale(sender.tag); }
- (void)directCommandAction:(NSMenuItem*)sender {
    if (sender.enabled && !repeated_key_event(NSApp.currentEvent)) selectCommand(cm::aot::DirectCommand(sender.tag));
}
- (void)applicationDidChangeScreenParameters:(NSNotification*)notification {
    (void)notification; if (refreshPresentation) refreshPresentation();
}
- (void)windowDidChangeScreen:(NSNotification*)notification {
    (void)notification; if (refreshPresentation) refreshPresentation();
}
- (void)windowDidChangeBackingProperties:(NSNotification*)notification {
    (void)notification; if (refreshPresentation) refreshPresentation();
}
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
        horizontal < 0 || vertical < 0 || horizontal >= view->logicalBounds[3] - view->logicalBounds[1] || vertical >= view->logicalBounds[2] - view->logicalBounds[0]) {
        NSBeep(); return;
    }
    const auto point = [view convertPoint:NSMakePoint(
        horizontal * view.bounds.size.width / (view->logicalBounds[3] - view->logicalBounds[1]),
        vertical * view.bounds.size.height / (view->logicalBounds[2] - view->logicalBounds[0])) toView:nil];
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
- (void)originalMenuAction:(NSMenuItem*)sender {
    if (!sender.enabled || repeated_key_event(NSApp.currentEvent)) return;
    if (sender.tag == 0x00020001 && selectCommand) { selectCommand(cm::aot::DirectCommand::save_game); return; }
    if (std::any_of(pending->begin(), pending->end(), [](const auto& event) {
        return event.kind == cm::aot::UIEvent::Kind::menu;
    })) { NSBeep(); return; }
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
bool native_match_speed_enabled() { return native_speed_enabled; }
bool native_debugging_available() { return debugging_available; }
bool native_debugging_enabled() { return debugging_available && debugging_enabled; }
void initialize_native_debugging(bool enabled) { debugging_available = true; debugging_enabled = enabled; }
void select_native_debugging(bool enabled) {
    if (!debugging_available || enabled == debugging_enabled) return;
    if (!debugging_preference.empty()) {
        @autoreleasepool {
            NSData* data = [NSPropertyListSerialization dataWithPropertyList:@{@"Version": @1, @"Debugging": @(enabled)}
                format:NSPropertyListXMLFormat_v1_0 options:0 error:nil];
            std::error_code error;
            std::filesystem::create_directories(debugging_preference.parent_path(), error);
            if (error || !data || ![data writeToFile:[NSString stringWithUTF8String:debugging_preference.c_str()]
                options:NSDataWritingAtomic error:nil]) throw std::runtime_error("Cannot save Debugging preference");
        }
    }
    debugging_enabled = enabled;
}
std::uint16_t native_match_speed() { return selected_match_speed; }
void initialize_native_match_speed(std::uint16_t value) {
    native_speed_enabled = true;
    selected_match_speed = valid_match_speed(value) ? value : turbo_match_speed;
    match_turbo = true;
}
void select_native_match_speed(std::uint16_t value) {
    if (!native_speed_enabled || !valid_match_speed(value) || value == selected_match_speed) return;
    selected_match_speed = value;
    if (match_speed_preference.empty()) return;
    @autoreleasepool {
        NSDictionary* preference = @{@"Version": @1, @"MatchSpeed": @(value)};
        NSData* data = [NSPropertyListSerialization dataWithPropertyList:preference
            format:NSPropertyListXMLFormat_v1_0 options:0 error:nil];
        std::error_code error;
        std::filesystem::create_directories(match_speed_preference.parent_path(), error);
        if (error || !data || ![data writeToFile:[NSString stringWithUTF8String:match_speed_preference.c_str()]
                options:NSDataWritingAtomic error:nil])
            std::fprintf(stderr, "Match speed preference could not be saved; the current choice remains usable.\n");
    }
}
std::uint16_t legacy_event_mask(const UIEvent& event) {
    switch (event.kind) {
        case UIEvent::Kind::launch: case UIEvent::Kind::quit: case UIEvent::Kind::open_files: return 0x0400;
        case UIEvent::Kind::mouse_down: case UIEvent::Kind::menu: case UIEvent::Kind::command: return 2;
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
    std::set<unsigned short> host_command_keyups;
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
    // A working game polls events and the pointer thousands of times a
    // second. Each AppKit pump and pointer read is a window-server round trip,
    // costlier than the game's own work between polls; see poll/pointer_state.
    std::optional<NativePointerState> pointer_reading;
    std::chrono::steady_clock::time_point pointer_read_at{}, pumped_at{};
    // The game edited its own menus: refresh the command items even when the
    // command context is unchanged (refresh_commands re-asserts File > Save).
    bool menus_edited = true;
    struct Menu { NSMenu* native; std::int16_t id; bool enabled; };
    std::map<std::uint32_t, Menu> menus;
    std::map<std::uint32_t, CMNativeWindow*> windows;
    std::map<std::uint32_t, LegacyRect> logical_bounds;
    std::uint32_t primary_window = 0;
    InterfaceScale requested_scale = InterfaceScale::original;
    std::optional<PresentationLayout> layout;
    std::filesystem::path preference;
    NSMenu* scale_menu = nil;
    NSMenu* direct_menu = nil;
    CommandContext command_context;
    std::string command_status = "Unavailable while starting";
    void refresh_commands() {
        for (NSMenuItem* item in direct_menu.itemArray) {
            if (item.tag >= 0 && item.tag < NSInteger(direct_command_count))
                item.enabled = command_context.ready && command_context.actions[item.tag] != 0;
            else if (item.tag == 101) { item.enabled = NO; }
            else if (item.tag == 100) { item.title = [NSString stringWithUTF8String:command_status.c_str()]; item.enabled = NO; }
        }
        for (const auto& [token, menu] : menus) {
            (void)token;
            if (menu.id == 2 && menu.native.numberOfItems > 0) {
                NSMenuItem* save = [menu.native itemAtIndex:0];
                save.identifier = @"save_game";
                save.enabled = command_context.ready && command_context.actions[std::size_t(DirectCommand::save_game)] != 0;
            }
        }
    }
    bool applying_layout = false;
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
    static PresentationRect rectangle(const LegacyRect& r) {
        return {double(r[1]), double(r[0]), double(r[3] - r[1]), double(r[2] - r[0])};
    }
    bool background(std::uint32_t token) const {
        if (!primary_window || token == primary_window || windows.at(token).acceptsActivation) return false;
        const auto a = rectangle(logical_bounds.at(token)), b = rectangle(logical_bounds.at(primary_window));
        return a.x <= b.x && a.y <= b.y && a.x + a.width >= b.x + b.width &&
            a.y + a.height >= b.y + b.height && (a.width > b.width || a.height > b.height);
    }
    PresentationPoint logical_point(PresentationPoint host) const {
        if (layout && !(host.y >= 0 && host.y < NSApp.mainMenu.menuBarHeight))
            return layout->to_logical(host);
        return host;
    }
    PresentationRect scene() const {
        auto r = rectangle(logical_bounds.at(primary_window));
        for (const auto& [token, bounds] : logical_bounds) {
            if (background(token)) continue;
            const auto b = rectangle(bounds);
            const auto right = std::max(r.x + r.width, b.x + b.width);
            const auto bottom = std::max(r.y + r.height, b.y + b.height);
            r.x = std::min(r.x, b.x); r.y = std::min(r.y, b.y);
            r.width = right - r.x; r.height = bottom - r.y;
        }
        return r;
    }
    NSScreen* screen() const {
        NSScreen* selected = primary_window ? windows.at(primary_window).screen : nil;
        if (selected) return selected;
        return NSScreen.mainScreen ? NSScreen.mainScreen : NSScreen.screens.firstObject;
    }
    PresentationRect available() const {
        const auto area = screen().visibleFrame;
        const auto height = CGRectGetHeight(CGDisplayBounds(CGMainDisplayID()));
        return {area.origin.x, height - NSMaxY(area), area.size.width, area.size.height};
    }
    void refresh_presentation() {
        if (applying_layout || !primary_window || !screen()) return;
        applying_layout = true;
        layout = presentation_layout(requested_scale, scene(), available(), screen().backingScaleFactor);
        const auto height = CGRectGetHeight(CGDisplayBounds(CGMainDisplayID()));
        for (const auto& [token, window] : windows) {
            if (background(token)) {
                // Retain the original desktop's logical window and pixels for
                // the engine, without covering the macOS desktop around the game.
                [window orderOut:nil];
                continue;
            }
            const auto host = layout->to_host(rectangle(logical_bounds.at(token)));
            [window setFrame:NSMakeRect(host.x, height - host.y - host.height, host.width, host.height) display:NO];
            window.contentView.needsDisplay = YES;
        }
        for (NSMenuItem* item in scale_menu.itemArray) {
            const auto choice = interface_scales[item.tag];
            item.enabled = scale_fits(choice, scene(), available());
            item.state = choice == layout->effective ? NSControlStateValueOn : NSControlStateValueOff;
        }
        applying_layout = false;
    }
    bool choose_scale(InterfaceScale choice) {
        if (primary_window && !scale_fits(choice, scene(), available())) return false;
        requested_scale = choice;
        refresh_presentation();
        if (!preference.empty()) {
            // Tiny host-only preference, never a game resource or save payload.
            NSDictionary* value = @{ @"Version": @1,
                @"InterfaceScale": [NSString stringWithUTF8String:std::string(scale_name(choice)).c_str()] };
            NSError* error = nil;
            NSData* data = [NSPropertyListSerialization dataWithPropertyList:value format:NSPropertyListXMLFormat_v1_0 options:0 error:&error];
            std::error_code code;
            std::filesystem::create_directories(preference.parent_path(), code);
            if (code || !data || ![data writeToFile:[NSString stringWithUTF8String:preference.c_str()] options:NSDataWritingAtomic error:&error])
                std::fprintf(stderr, "Presentation preference could not be saved; current scale remains usable.\n");
        }
        return true;
    }
    explicit Impl(std::filesystem::path path) : preference(std::move(path)) {
      @autoreleasepool {
        if (![NSThread isMainThread]) throw std::runtime_error("Native UI requires the macOS main thread");
        [NSApplication sharedApplication];
        // Presentation preferences are separate from original resources/saves.
        // Existing installations without this preference start in Turbo.
        initialize_native_match_speed();
        initialize_native_debugging();
        debugging_preference = preference.empty() ? std::filesystem::path{} : preference.parent_path() / "Debugging.plist";
        if (!debugging_preference.empty()) {
            std::error_code error;
            const auto size = std::filesystem::file_size(debugging_preference, error);
            if (!error && size <= 4096) {
                NSData* data = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:debugging_preference.c_str()]];
                id value = data ? [NSPropertyListSerialization propertyListWithData:data
                    options:NSPropertyListImmutable format:nullptr error:nil] : nil;
                if ([value isKindOfClass:NSDictionary.class] && [value[@"Version"] isEqual:@1] &&
                    [value[@"Debugging"] isKindOfClass:NSNumber.class] && [value[@"Debugging"] isEqual:@YES])
                    debugging_enabled = true;
            }
        }
        match_speed_preference = preference.empty() ? std::filesystem::path{} :
            preference.parent_path() / "MatchSpeed.plist";
        if (!match_speed_preference.empty()) {
            std::error_code error;
            const auto size = std::filesystem::file_size(match_speed_preference, error);
            if (!error && size <= 4096) {
                NSData* data = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:match_speed_preference.c_str()]];
                id value = data ? [NSPropertyListSerialization propertyListWithData:data
                    options:NSPropertyListImmutable format:nullptr error:nil] : nil;
                if ([value isKindOfClass:NSDictionary.class] && [value[@"Version"] isEqual:@1] &&
                    [value[@"MatchSpeed"] isKindOfClass:NSNumber.class]) {
                    const auto speed = [value[@"MatchSpeed"] integerValue];
                    if (speed >= 0 && speed <= UINT16_MAX && valid_match_speed(std::uint16_t(speed)))
                        selected_match_speed = std::uint16_t(speed);
                }
            }
        }
        delegate = [CMNativeAppDelegate new];
        delegate->pending = &pending;
        delegate->gameWindows = &windows;
        NSArray<NSString*>* arguments = NSProcessInfo.processInfo.arguments;
        for (NSUInteger i = 1; i < arguments.count; ++i)
            host_arguments.insert(std::filesystem::absolute(arguments[i].fileSystemRepresentation).lexically_normal());
        delegate->hostArguments = &host_arguments;
        delegate->launching = true;
        delegate->refreshPresentation = [this] { refresh_presentation(); };
        delegate->selectScale = [this](NSInteger index) {
            if (index < 0 || index >= 5 || !choose_scale(interface_scales[index])) NSBeep();
        };
        delegate->selectCommand = [this](DirectCommand command) {
            const auto index = std::size_t(command);
            if (index >= direct_command_count || !command_context.ready || !command_context.actions[index]) { NSBeep(); return; }
            UIEvent event{UIEvent::Kind::command};
            event.message = std::uint32_t(command);
            event.command_context = command_context.identity;
            pending.push_back(event);
            command_context = {};
            command_status = "Queued for the original game";
            refresh_commands();
        };
        if (!preference.empty()) {
            std::error_code error;
            const auto size = std::filesystem::file_size(preference, error);
            if (!error && size <= 4096) {
                NSData* data = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:preference.c_str()]];
                id value = data ? [NSPropertyListSerialization propertyListWithData:data options:NSPropertyListImmutable format:nullptr error:nil] : nil;
                if ([value isKindOfClass:NSDictionary.class] && [value[@"Version"] isEqual:@1] &&
                    [value[@"InterfaceScale"] isKindOfClass:NSString.class]) {
                    if (auto choice = parse_scale([value[@"InterfaceScale"] UTF8String])) requested_scale = *choice;
                }
            }
        }
        [NSApp setDelegate:delegate];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
      }
    }
    ~Impl() {
        delegate->refreshPresentation = {}; delegate->selectScale = {}; delegate->selectCommand = {};
        for (const auto& [token, event] : events) { (void)token; ReleaseEvent(event); }
        for (const auto& [token, window] : windows) { (void)token; [window orderOut:nil]; [window close]; }
        if (NSApp.delegate == delegate) [NSApp setDelegate:nil];
    }
};
NativeUI::NativeUI(std::filesystem::path preference) : impl_(std::make_unique<Impl>(std::move(preference))) {}
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
        view->logicalBounds = bounds;
        view->inversePoint = [owner = impl_.get()](PresentationPoint point) { return owner->logical_point(point); };
        window.contentView = view;
        [window makeFirstResponder:view];
        window.delegate = impl_->delegate;
        // Carbon accepts the supplied global bounds, including the original
        // background window's intentional extension beyond desktop edges.
        [window setFrame:rect display:NO];
        impl_->windows.emplace(token, window);
        impl_->logical_bounds.emplace(token, bounds);
        // The original engine creates one activating 800x600 interface, plus a
        // nonactivating desktop and possible overlays. Keep that logical model.
        if (!impl_->primary_window && !no_activate && bounds[3] - bounds[1] == 800 && bounds[2] - bounds[0] == 600)
            impl_->primary_window = token;
        impl_->refresh_presentation();
    }
}
void NativeUI::dispose_window(std::uint32_t token) {
    auto window = impl_->windows.at(token);
    [window orderOut:nil]; [window close];
    impl_->windows.erase(token);
    impl_->logical_bounds.erase(token);
    if (impl_->primary_window == token) { impl_->primary_window = 0; impl_->layout.reset(); }
    impl_->refresh_presentation();
    std::erase_if(impl_->pending, [token](const auto& event) { return event.window == token; });
}
void NativeUI::show_window(std::uint32_t token, bool activate) {
    auto window = impl_->windows.at(token);
    if (impl_->background(token)) [window orderOut:nil];
    else if (activate && window.acceptsActivation) [window makeKeyAndOrderFront:nil];
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
    (void)content; // Supported class-13 windows are borderless.
    return impl_->logical_bounds.at(token);
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
                              std::uint32_t row_bytes, const std::vector<std::uint8_t>& argb,
                              std::shared_ptr<const NativeDrawing> native) {
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
        if (view->sourceImage) CGImageRelease(view->sourceImage);
        view->sourceImage = image;
        if (view->nativeBackground) CGImageRelease(view->nativeBackground);
        view->nativeBackground = nullptr;
        view->nativeDrawing = std::move(native);
        if (view->nativeDrawing) {
            const auto& background = *view->nativeDrawing;
            if (background.width != int(width) || background.height != int(height) ||
                background.background.size() != std::uint64_t(width) * height * 4)
                throw std::runtime_error("Invalid native drawing background");
            const auto baseData = CFDataCreate(nullptr, background.background.data(), background.background.size());
            const auto baseProvider = CGDataProviderCreateWithCFData(baseData);
            const auto baseColors = CGColorSpaceCreateDeviceRGB();
            view->nativeBackground = CGImageCreate(background.width, background.height, 8, 32, background.width * 4,
                baseColors, CGBitmapInfo(kCGImageAlphaNoneSkipFirst) | CGBitmapInfo(kCGBitmapByteOrder32Big),
                baseProvider, nullptr, false, kCGRenderingIntentDefault);
            CGColorSpaceRelease(baseColors); CGDataProviderRelease(baseProvider); CFRelease(baseData);
            if (!view->nativeBackground) view->nativeDrawing.reset();
        }
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
    // Reuse a reading for 4 ms: a hand cannot move or click meaningfully faster.
    const auto now = std::chrono::steady_clock::now();
    if (impl_->pointer_reading && now - impl_->pointer_read_at < std::chrono::milliseconds(4)) return *impl_->pointer_reading;
    const auto point = NSEvent.mouseLocation;
    const auto height = CGRectGetHeight(CGDisplayBounds(CGMainDisplayID()));
    const auto logical = impl_->logical_point({point.x, height - point.y});
    const auto x = impl_->layout ? std::floor(logical.x) : std::round(logical.x);
    const auto y = impl_->layout ? std::floor(logical.y) : std::round(logical.y);
    if (x < INT16_MIN || x > INT16_MAX || y < INT16_MIN || y > INT16_MAX)
        throw std::runtime_error("Native mouse position exceeds original Point coordinates");
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    const bool down = ::Button();
#pragma clang diagnostic pop
    impl_->pointer_reading = NativePointerState{std::int16_t(x), std::int16_t(y), std::uint16_t(::GetCurrentKeyModifiers() | (down ? 0 : 0x80)), down};
    impl_->pointer_read_at = now;
    return *impl_->pointer_reading;
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
bool NativeUI::poll(UIEvent& event, double timeout_seconds, bool throttled) {
    @autoreleasepool {
        if (!impl_->finished_launch) {
            impl_->finished_launch = true;
            [NSApp finishLaunching];
        }
        const auto now = std::chrono::steady_clock::now();
        if (throttled && timeout_seconds <= 0 && impl_->pending.empty() && now - impl_->pumped_at < std::chrono::milliseconds(2)) return false;
        impl_->pumped_at = now;
        NSDate* deadline = [NSDate dateWithTimeIntervalSinceNow:timeout_seconds];
        while (impl_->pending.empty()) {
            NSEvent* native = [NSApp nextEventMatchingMask:NSEventMaskAny
                untilDate:deadline
                inMode:NSDefaultRunLoopMode dequeue:YES];
            if (native) {
                const bool command_key = native.type == NSEventTypeKeyDown && (native.modifierFlags & NSEventModifierFlagCommand) &&
                    (native.keyCode == 1 || native.keyCode == 31 || native.keyCode == 36 || native.keyCode == 43 || native.keyCode == 33);
                // A host shortcut consumes the entire key gesture. Sending its
                // unmatched key-up to the original nested dialog can terminate
                // filename editing or invoke an unrelated original accelerator.
                if (native.type == NSEventTypeKeyDown) {
                    if (command_key) impl_->host_command_keyups.insert(native.keyCode);
                    else impl_->host_command_keyups.erase(native.keyCode);
                } else if (native.type == NSEventTypeKeyUp && impl_->host_command_keyups.erase(native.keyCode)) continue;
                if (command_key && NSProcessInfo.processInfo.systemUptime - native.timestamp > 1.0) {
                    impl_->command_status = "Refused: shortcut arrived while the game was busy";
                    impl_->refresh_commands(); NSBeep(); continue;
                }
                [NSApp sendEvent:native];
                [NSApp updateWindows];
            }
            if (!native) break;
        }
        if (impl_->pending.empty()) return false;
        // New input: the next pointer query reads the window server again, so
        // Button/GetMouse agree with the event the game is about to receive.
        impl_->pointer_reading.reset();
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
    impl_->menus_edited = true;
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
    impl_->menus_edited = true;
    @autoreleasepool {
        // AppKit permits only one parent for a submenu. Original SetMenuBar
        // can reinstall the same handles, so detach them before rebuilding.
        for (NSMenuItem* old in NSApp.mainMenu.itemArray) old.submenu = nil;
        NSMenu* bar = [[NSMenu alloc] initWithTitle:@""];
        bar.autoenablesItems = NO;
        for (auto token : menus) {
            const auto& menu = impl_->menus.at(token);
            NSMenuItem* top = [[NSMenuItem alloc] initWithTitle:menu.native.title action:nil keyEquivalent:@""];
            top.enabled = menu.enabled;
            top.submenu = menu.native;
            [bar addItem:top];
        }
        NSMenuItem* game_top = [[NSMenuItem alloc] initWithTitle:@"Game" action:nil keyEquivalent:@""];
        impl_->direct_menu = [[NSMenu alloc] initWithTitle:@"Game"];
        impl_->direct_menu.autoenablesItems = NO;
        NSArray<NSString*>* command_titles = @[@"Continue Game", @"Save Game…", @"Load Game…", @"Game Options…", @"Back or Cancel"];
        NSArray<NSString*>* command_keys = @[@"\r", @"", @"o", @",", @"["];
        for (std::size_t i = 0; i < direct_command_count; ++i) {
            NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:command_titles[i] action:@selector(directCommandAction:) keyEquivalent:command_keys[i]];
            item.tag = i; item.target = impl_->delegate;
            item.identifier = [NSString stringWithUTF8String:std::string(direct_command_names[i]).c_str()];
            item.keyEquivalentModifierMask = NSEventModifierFlagCommand;
            [impl_->direct_menu addItem:item];
        }
        NSMenuItem* status = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
        status.tag = 100; [impl_->direct_menu addItem:status];
        NSMenuItem* load_help = [[NSMenuItem alloc] initWithTitle:@"Load: start screen only; save and quit first" action:nil keyEquivalent:@""];
        load_help.tag = 101; [impl_->direct_menu addItem:load_help];
        game_top.submenu = impl_->direct_menu; [bar addItem:game_top];
        impl_->refresh_commands();
        NSMenuItem* view_top = [[NSMenuItem alloc] initWithTitle:@"View" action:nil keyEquivalent:@""];
        NSMenu* view_menu = [[NSMenu alloc] initWithTitle:@"View"];
        view_menu.autoenablesItems = NO;
        NSMenuItem* scale_top = [[NSMenuItem alloc] initWithTitle:@"Interface Scale" action:nil keyEquivalent:@""];
        impl_->scale_menu = [[NSMenu alloc] initWithTitle:@"Interface Scale"];
        impl_->scale_menu.autoenablesItems = NO;
        NSArray<NSString*>* titles = @[@"100% (Original)", @"125%", @"150%", @"200%", @"Fit to Screen"];
        for (NSInteger index = 0; index < 5; ++index) {
            NSMenuItem* choice = [[NSMenuItem alloc] initWithTitle:titles[index] action:@selector(interfaceScaleAction:) keyEquivalent:@""];
            choice.tag = index; choice.target = impl_->delegate;
            choice.identifier = [@"set_interface_scale." stringByAppendingString:[NSString stringWithUTF8String:std::string(scale_name(interface_scales[index])).c_str()]];
            choice.state = interface_scales[index] == impl_->requested_scale ? NSControlStateValueOn : NSControlStateValueOff;
            [impl_->scale_menu addItem:choice];
        }
        scale_top.submenu = impl_->scale_menu;
        [view_menu addItem:scale_top]; view_top.submenu = view_menu; [bar addItem:view_top];
        impl_->refresh_presentation();
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
void NativeUI::update_commands(const CommandContext& context, std::string_view status) {
    if (!impl_->menus_edited && impl_->command_context == context && impl_->command_status == status) return;
    impl_->menus_edited = false;
    impl_->command_context = context;
    impl_->command_status = status;
    impl_->refresh_commands();
}
bool NativeUI::menu_selection_enabled(std::uint32_t selection) const {
    const auto id = std::int16_t(selection >> 16);
    const auto item = std::uint16_t(selection);
    for (const auto token : impl_->menu_bar) {
        const auto& menu = impl_->menus.at(token);
        if (menu.id == id && menu.enabled && item && item <= menu.native.numberOfItems) {
            NSMenuItem* entry = [menu.native itemAtIndex:item - 1];
            return entry.enabled && !entry.separatorItem;
        }
    }
    return false;
}
void NativeUI::set_menu_item_enabled(std::uint32_t token, std::uint16_t item, bool enabled) {
    impl_->menus_edited = true;
    auto& menu = impl_->menus.at(token);
    if (!item) {
        menu.enabled = enabled;
        for (NSMenuItem* top in NSApp.mainMenu.itemArray) if (top.submenu == menu.native) top.enabled = enabled;
    } else if (item <= menu.native.numberOfItems) {
        NSMenuItem* entry = [menu.native itemAtIndex:item - 1];
        if (!entry.separatorItem) entry.enabled = enabled;
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
    impl_->menus_edited = true;
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
    impl_->menus_edited = true;
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
bool NativeUI::set_interface_scale(InterfaceScale scale) { return impl_->choose_scale(scale); }
InterfaceScale NativeUI::interface_scale() const { return impl_->layout ? impl_->layout->effective : impl_->requested_scale; }
double NativeUI::presentation_scale() const { return impl_->layout ? impl_->layout->scale : 1; }
void NativeUI::redraw_menu_bar() { @autoreleasepool { [NSApp.mainMenu update]; } }
void NativeUI::set_menu_item_text(std::uint32_t menu, std::uint16_t item, const std::string& text) {
    impl_->menus_edited = true;
    @autoreleasepool {
        NSMenu* native = impl_->menus.at(menu).native;
        if (!item || item > native.numberOfItems) return; // Original preparation may refer to a removed Quit item.
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
