#include "ui_bridge.hpp"
#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h>
#include <iostream>
#include <stdexcept>
namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
void post(NSEventType type, NSString* text, bool repeat = false) {
    NSEvent* event = [NSEvent keyEventWithType:type location:NSZeroPoint modifierFlags:0 timestamp:0
        windowNumber:0 context:nil characters:text charactersIgnoringModifiers:text isARepeat:repeat keyCode:0];
    [NSApp postEvent:event atStart:NO];
}
NSEvent* next() {
    return [NSApp nextEventMatchingMask:NSEventMaskKeyDown | NSEventMaskKeyUp untilDate:NSDate.distantPast
                                inMode:NSDefaultRunLoopMode dequeue:YES];
}
}
int main() {
    @autoreleasepool {
        try {
            cm::aot::NativeUI ui;
            cm::aot::UIEvent startup{cm::aot::UIEvent::Kind::update};
            for (unsigned i = 0; i < 16 && ui.poll(startup, 0); ++i) {}
            post(NSEventTypeKeyDown, @"a"); post(NSEventTypeKeyUp, @"a"); post(NSEventTypeKeyDown, @"b");
            ui.flush_events(8, 16);
            const auto stop = next(), later = next();
            if (!(stop.type == NSEventTypeKeyUp && later.type == NSEventTypeKeyDown))
                std::cerr << "Observed native event types: " << (stop ? long(stop.type) : -1) << ", "
                          << (later ? long(later.type) : -1) << '\n';
            require(stop.type == NSEventTypeKeyUp && [stop.characters isEqualToString:@"a"] &&
                    later.type == NSEventTypeKeyDown && [later.characters isEqualToString:@"b"] && !next(),
                    "FlushEvents did not preserve its stop event and later native queue order");
            post(NSEventTypeKeyDown, @"c", true); post(NSEventTypeKeyDown, @"d");
            ui.flush_events(32, 0);
            const auto key = next();
            require(key && !key.isARepeat && [key.characters isEqualToString:@"d"] && !next(),
                    "Automatic key repeats were not flushed independently of key-down events");
            post(NSEventTypeKeyDown, @"e"); post(NSEventTypeKeyUp, @"e");
            ui.flush_events(8 | 16, 8);
            require(next().type == NSEventTypeKeyDown && next().type == NSEventTypeKeyUp && !next(),
                    "Stop mask must take precedence when an event also matches the flush mask");
            ui.create_window(1, 13, 0x02200000, {100, 150, 300, 450});
            ui.show_window(1, true);
            cm::aot::UIEvent event{cm::aot::UIEvent::Kind::update};
            for (unsigned i = 0; i < 100 && ui.poll(event, 0); ++i) {}
            const auto number = ui.window_number(1);
            auto receive = [&](cm::aot::UIEvent::Kind kind) {
                for (unsigned i = 0; i < 100; ++i)
                    if (ui.poll(event, 0) && event.kind == kind) return event;
                std::cerr << "Missing event kind " << int(kind) << "; active=" << NSApp.isActive
                          << "; keyWindow=" << (NSApp.keyWindow != nil) << '\n';
                throw std::runtime_error("Native view did not deliver the queued input event");
            };
            NSEvent* accented = [NSEvent keyEventWithType:NSEventTypeKeyDown location:NSMakePoint(23, 44)
                modifierFlags:NSEventModifierFlagShift timestamp:NSProcessInfo.processInfo.systemUptime
                windowNumber:number context:nil characters:@"é" charactersIgnoringModifiers:@"é" isARepeat:NO keyCode:14];
            [NSApp postEvent:accented atStart:NO];
            const auto key_input = receive(cm::aot::UIEvent::Kind::key_down);
            require(key_input.window == 1 && key_input.message == 0x0e8e && (key_input.modifiers & 0x1f00) == 0x200,
                    "Native key event lost its virtual key, MacRoman character, or shift modifier");
            require(key_input.when == EventTimeToTicks(accented.timestamp), "Native key timestamp differs from Carbon's tick conversion");
            NSEvent* arrow = [NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0
                timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:number context:nil
                characters:@"\uf702" charactersIgnoringModifiers:@"\uf702" isARepeat:YES keyCode:123];
            [NSApp postEvent:arrow atStart:NO];
            require(receive(cm::aot::UIEvent::Kind::auto_key).message == 0x7b1c,
                    "Native repeating arrow key did not retain its original control-character code");
            NSEvent* click = [NSEvent mouseEventWithType:NSEventTypeRightMouseDown location:NSMakePoint(23, 44)
                modifierFlags:NSEventModifierFlagControl timestamp:NSProcessInfo.processInfo.systemUptime
                windowNumber:number context:nil eventNumber:1 clickCount:1 pressure:1];
            // AppKit suppresses posted mouse events in an inactive app. Exercise
            // its actual view responder without depending on physical focus.
            [[NSApp windowWithWindowNumber:number].contentView rightMouseDown:click];
            const auto mouse = receive(cm::aot::UIEvent::Kind::mouse_down);
            if (mouse.button != 2 || mouse.x != 173 || mouse.y != 256 || (mouse.modifiers & 0x1f00) != 0x1000)
                std::cerr << "Mouse fields: button=" << mouse.button << ", x=" << mouse.x << ", y=" << mouse.y
                          << ", modifiers=" << mouse.modifiers << ", nativeButton=" << click.buttonNumber << '\n';
            require(mouse.window == 1 && mouse.button == 2 && mouse.x == 173 && mouse.y == 256 &&
                    (mouse.modifiers & 0x1f00) == 0x1000, "Native right-click coordinates, button number, or control modifier");
            ui.begin_event(100, kEventClassWindow, 85, mouse); // Legacy kEventWindowClickContentRgn is 32-bit only in the SDK.
            auto parameter = ui.event_parameter(100, kEventParamMouseButton, typeMouseButton, 2, true);
            require(!parameter.error && parameter.type == typeMouseButton && parameter.size == 2 &&
                    parameter.bytes == std::vector<std::uint8_t>({0, 2}), "Native button parameter was not encoded as a PowerPC UInt16");
            parameter = ui.event_parameter(100, kEventParamKeyModifiers, typeUInt32, 4, true);
            require(!parameter.error && parameter.type == typeUInt32 && parameter.bytes == std::vector<std::uint8_t>({0, 0, 0x10, 0}),
                    "Native key-modifier parameter retained button/active bits or the host byte order");
            parameter = ui.event_parameter(100, kEventParamKeyModifiers, typeUInt32, 0, false);
            require(!parameter.error && parameter.size == 4 && parameter.bytes.empty(), "Native parameter size-only query");
            parameter = ui.event_parameter(100, kEventParamMouseWheelDelta, typeSInt32, 4, true);
            require(parameter.error == eventParameterNotFoundErr && parameter.bytes.empty(), "Absent native event parameter did not report the native error");
            ui.end_event(100);
            auto wheel = mouse; wheel.kind = cm::aot::UIEvent::Kind::wheel; wheel.wheel_delta = -3;
            ui.begin_event(101, kEventClassMouse, kEventMouseWheelMoved, wheel);
            parameter = ui.event_parameter(101, kEventParamMouseWheelDelta, typeSInt32, 4, true);
            require(!parameter.error && parameter.bytes == std::vector<std::uint8_t>({0xff, 0xff, 0xff, 0xfd}),
                    "Native negative wheel delta was not encoded in the original signed format");
            ui.end_event(101);
            ui.dispose_window(1);
            std::cout << "Native AppKit queue ordering, keyboard, mouse coordinates, and Carbon input parameter checks passed\n";
            return 0;
        } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
    }
}
