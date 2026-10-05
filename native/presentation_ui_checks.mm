#include "ui_bridge.hpp"
#import <AppKit/AppKit.h>
#include <fstream>
#include <iostream>
#include <unistd.h>

// A menu chosen with the mouse has a non-key currentEvent. Calling isARepeat
// on it raises in AppKit; a sendAction with a nil currentEvent misses that bug.
@interface CMMenuCheckApplication : NSApplication
@property(nonatomic, strong) NSEvent* menuTestEvent;
@end
@implementation CMMenuCheckApplication
- (NSEvent*)currentEvent { return self.menuTestEvent ? self.menuTestEvent : [super currentEvent]; }
@end

using namespace cm::aot;
namespace {
void require(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
NSMenuItem* named(NSMenu* menu, NSString* identifier) {
    for (NSMenuItem* item in menu.itemArray) {
        if ([item.identifier isEqualToString:identifier]) return item;
        if (item.submenu) if (auto found = named(item.submenu, identifier)) return found;
    }
    return nil;
}
void drain(NativeUI& ui) { UIEvent event{UIEvent::Kind::update}; for (int i = 0; i < 64 && ui.poll(event,0); ++i) {} }

}
int main() {
    @autoreleasepool {
        auto app = (CMMenuCheckApplication*)[CMMenuCheckApplication sharedApplication];
        auto pattern = (std::filesystem::temp_directory_path() / "cm-presentation-XXXXXX").string();
        if (!mkdtemp(pattern.data())) return 2;
        const std::filesystem::path temporary(pattern), preference = temporary / "candidate/Presentation.plist";
        try {
            {
                NativeUI ui(preference);
                MenuDefinition menu; menu.id = 2; menu.title = "File"; menu.enabled = true;
                MenuItem save; save.text = "Save"; save.key = 'S'; save.enabled = true;
                menu.items.push_back(save);
                ui.create_menu(7, menu);
                ui.create_window(11,13,0x02200000,{200,100,800,900});
                ui.show_window(11,true);
                require(ui.set_interface_scale(InterfaceScale::percent125), "125 percent should fit this fixture display");
                for (int i = 0; i < 2; ++i) {
                    ui.set_menu_bar({7});
                    require(named(NSApp.mainMenu,@"set_interface_scale.125").state == NSControlStateValueOn,
                        "Repeated SetMenuBar lost checked scale");
                    require(named(NSApp.mainMenu,@"continue_game") && !named(NSApp.mainMenu,@"continue_game").enabled,
                        "Uninitialized game command must be disabled");
                }
                drain(ui);
                NSWindow* window = [NSApp windowWithWindowNumber:ui.window_number(11)];
                require(window != nil, "Missing scaled native window");
                auto input = [&](NSEventType type, SEL selector, UIEvent::Kind kind, double x, double y) {
                    NSEvent* native = [NSEvent mouseEventWithType:type location:NSMakePoint(x, window.contentView.bounds.size.height - y)
                        modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:window.windowNumber
                        context:nil eventNumber:1 clickCount:1 pressure:1];
                    [NSApp sendAction:selector to:window.contentView from:native];
                    UIEvent event{UIEvent::Kind::update};
                    require(ui.poll(event,0) && event.kind == kind && event.x == 300 && event.y == 300,
                        "Scaled event did not share the logical inverse");
                };
                input(NSEventTypeRightMouseDown,@selector(rightMouseDown:),UIEvent::Kind::mouse_down,250,125);
                input(NSEventTypeRightMouseUp,@selector(rightMouseUp:),UIEvent::Kind::mouse_up,250,125);
                input(NSEventTypeLeftMouseDragged,@selector(mouseDragged:),UIEvent::Kind::mouse_moved,250,125);
                CommandContext context; context.ready = true; context.identity = 123; context.actions[0] = 1000;
                ui.update_commands(context,"Ready");
                auto item = named(NSApp.mainMenu,@"continue_game");
                require(item.enabled, "Ready Continue menu unavailable");
                [NSApp sendAction:item.action to:item.target from:item];
                require(!item.enabled, "Accepted command did not immediately disable duplicates");
                [NSApp sendAction:item.action to:item.target from:item];
                UIEvent event{UIEvent::Kind::update};
                require(ui.poll(event,0) && event.kind == UIEvent::Kind::command && event.message == 0 && event.command_context == 123,
                    "Named menu did not queue the single original command identity");
                require(!ui.poll(event,0), "Duplicate command was queued");
                context.actions[1] = 1022;
                auto expect_save = [&] {
                    require(ui.poll(event,0) && event.kind == UIEvent::Kind::command && event.message == 1 &&
                        event.command_context == 123, "Save menu/shortcut diverged from named command");
                    require(!ui.poll(event,0), "Save produced duplicate input");
                };
                ui.update_commands(context,"Ready");
                auto file_save = named(NSApp.mainMenu,@"save_game");
                [NSApp sendAction:file_save.action to:file_save.target from:file_save];
                expect_save();
                for (NSString* identifier in @[@"continue_game", @"save_game"]) {
                    ui.update_commands(context,"Ready");
                    app.menuTestEvent = [NSEvent mouseEventWithType:NSEventTypeLeftMouseUp location:NSZeroPoint
                        modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:window.windowNumber
                        context:nil eventNumber:1 clickCount:1 pressure:0];
                    auto mouse_item = named(NSApp.mainMenu,identifier);
                    @try { [NSApp sendAction:mouse_item.action to:mouse_item.target from:mouse_item]; }
                    @catch (NSException* exception) {
                        app.menuTestEvent = nil;
                        throw std::runtime_error(std::string("Mouse menu action raised: ") + exception.reason.UTF8String);
                    }
                    app.menuTestEvent = nil;
                    require(ui.poll(event,0) && event.kind == UIEvent::Kind::command && event.command_context == 123 &&
                        event.message == ([identifier isEqualToString:@"save_game"] ? 1u : 0u),
                        "Mouse menu selection did not dispatch the original command");
                    require(!ui.poll(event,0), "Mouse menu selection dispatched twice");
                }
                auto key = [&](NSString* characters, unsigned short code, bool repeat, double age, NSEventModifierFlags modifiers) {
                    NSEvent* native = [NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:modifiers
                        timestamp:NSProcessInfo.processInfo.systemUptime-age windowNumber:window.windowNumber context:nil
                        characters:characters charactersIgnoringModifiers:characters isARepeat:repeat keyCode:code];
                    [NSApp postEvent:native atStart:NO];
                };
                ui.update_commands(context,"Ready");
                key(@"s",1,false,0,NSEventModifierFlagCommand); expect_save();
                // Releasing Command before S must still consume S's key-up;
                // a nested original filename editor never received its down.
                NSEvent* released = [NSEvent keyEventWithType:NSEventTypeKeyUp location:NSZeroPoint modifierFlags:0
                    timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:window.windowNumber context:nil
                    characters:@"s" charactersIgnoringModifiers:@"s" isARepeat:NO keyCode:1];
                [NSApp postEvent:released atStart:NO];
                require(!ui.poll(event,0), "Host shortcut release leaked into the original dialog");
                ui.update_commands(context,"Ready");
                key(@"\r",36,true,0,NSEventModifierFlagCommand);
                require(!ui.poll(event,0), "Autorepeat dispatched Continue");
                key(@"\r",36,false,3,NSEventModifierFlagCommand);
                require(!ui.poll(event,0), "Stale busy shortcut dispatched Continue");
                ui.update_commands({},"File dialog");
                key(@"s",1,false,0,NSEventModifierFlagCommand);
                require(!ui.poll(event,0), "Disabled Save reached a text field");
                key(@"s",1,false,0,0);
                require(ui.poll(event,0) && event.kind == UIEvent::Kind::key_down && (event.message & 255) == 's',
                    "Ordinary filename character was intercepted");
                require(ui.window_bounds(11,true) == LegacyRect{200,100,800,900}, "Presentation changed original window bounds");
                require(ui.find_window(900,800).window == 0, "Right/bottom edge became clickable");
                const std::vector<std::uint8_t> bitmap(800 * 600 * 4, 0x7f);
                ui.present_window(11,800,600,800*4,bitmap);
                const auto first = ui.save_window_images(temporary / "source1");
                require(ui.set_interface_scale(InterfaceScale::original), "Return to 100 percent");
                const auto second = ui.save_window_images(temporary / "source2");
                auto bytes = [](const auto& path) { std::ifstream f(path, std::ios::binary); return std::string(std::istreambuf_iterator<char>(f),{}); };
                require(first.size()==1 && second.size()==1 && bytes(first[0]) == bytes(second[0]), "Scale changed source bitmap bytes");
                require(ui.set_interface_scale(InterfaceScale::percent125), "Restore persisted choice");
                ui.dispose_window(11);
            }
            { NativeUI reopened(preference); require(reopened.interface_scale() == InterfaceScale::percent125, "Preference did not survive reopening"); }
            { NativeUI other(temporary / "other/Presentation.plist"); require(other.interface_scale() == InterfaceScale::original, "Candidate preferences leaked across runtimes"); }
            for (const auto* invalid : {"broken", "<?xml version=\"1.0\"?><plist version=\"1.0\"><dict><key>Version</key><integer>1</integer><key>InterfaceScale</key><string>300</string></dict></plist>"}) {
                { std::ofstream file(preference); file << invalid; }
                NativeUI reopened(preference); require(reopened.interface_scale() == InterfaceScale::original, "Corrupt preference did not fall back safely");
            }
            std::filesystem::remove_all(temporary);
            std::cout << "Native scale menus, bitmap invariance, isolated preferences and named command queue passed\n";
            return 0;
        } catch (const std::exception& e) {
            std::cerr << e.what() << '\n'; std::filesystem::remove_all(temporary); return 1;
        }
    }
}
