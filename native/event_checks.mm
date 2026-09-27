#include "aot_runtime.hpp"
#include "platform.hpp"
#import <AppKit/AppKit.h>
#include <iostream>
#include <unistd.h>

using namespace cm::aot;
namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct Temporary {
    std::filesystem::path path;
    Temporary() {
        auto pattern = (std::filesystem::temp_directory_path() / "cm-native-input-XXXXXX").string();
        if (!mkdtemp(pattern.data())) throw std::runtime_error("Cannot create native input fixture");
        path = pattern;
    }
    ~Temporary() { std::error_code error; std::filesystem::remove_all(path, error); }
};
std::uint32_t call(State& s, const char* name, std::initializer_list<std::uint32_t> args) {
    unsigned r = 3;
    for (auto value : args) s.wr<4>(r++ * 4, value);
    s.pc = 0x1234; s.wr<4>(4128, return_sentinel);
    require(s.platform->dispatch(s, name) && s.pc == return_sentinel, "Original input API did not return to its caller");
    return std::uint32_t(s.r<4>(12));
}
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    @autoreleasepool {
        try {
            Temporary temporary;
            std::filesystem::create_directory(temporary.path / "Game");
            auto state = std::make_unique<State>(); auto& s = *state;
            load_program(s, argv[1]);
            s.platform = std::make_shared<NativePlatform>(temporary.path / "Game", temporary.path / "User");
            const auto scratch = allocate(s, 128, true), record = scratch + 36;
            call(s, "CarbonLib::SetRect", {scratch, 150, 100, 450, 300});
            require(call(s, "CarbonLib::CreateNewWindow", {13, 0x02200000, scratch, scratch + 8}) == 0, "Native input window creation");
            const auto token = std::uint32_t(s.memory.read<4>(scratch + 8));
            call(s, "CarbonLib::ShowWindow", {token});
            NSWindow* window = nil;
            for (NSWindow* candidate in NSApp.windows)
                if ([NSStringFromClass(candidate.class) isEqualToString:@"CMNativeWindow"]) {
                    require(window == nil, "Input fixture unexpectedly has multiple native game windows"); window = candidate;
                }
            require(window != nil, "Native input fixture window is missing");
            s.memory.write<4>(scratch + 80, 0xa5a5a5a5); s.memory.write<4>(scratch + 88, 0xa5a5a5a5);
            require(call(s, "CarbonLib::FindWindow", {(100u << 16) | 150u, scratch + 84}) == 3 &&
                    s.memory.read<4>(scratch + 84) == token && s.memory.read<4>(scratch + 80) == 0xa5a5a5a5 &&
                    s.memory.read<4>(scratch + 88) == 0xa5a5a5a5, "FindWindow global Point and original pointer output width");
            require(call(s, "CarbonLib::FindWindow", {(300u << 16) | 450u, scratch + 84}) == 0 &&
                    s.memory.read<4>(scratch + 84) == 0, "FindWindow must exclude right/bottom window edges");
            require(call(s, "CarbonLib::FindWindow", {0xffffffffu, scratch + 84}) == 0, "FindWindow lost signed desktop coordinates");
            call(s, "CarbonLib::SetRect", {scratch, 200, 150, 400, 250});
            require(call(s, "CarbonLib::CreateNewWindow", {13, 0x02200000, scratch, scratch + 8}) == 0, "Overlapping native test window");
            const auto top = std::uint32_t(s.memory.read<4>(scratch + 8));
            call(s, "CarbonLib::ShowWindow", {top});
            require(call(s, "CarbonLib::FindWindow", {(175u << 16) | 225u, scratch + 84}) == 3 &&
                    s.memory.read<4>(scratch + 84) == top, "FindWindow ignored native front-to-back ordering");
            call(s, "CarbonLib::ShowWindow", {token});
            require(call(s, "CarbonLib::FindWindow", {(175u << 16) | 225u, scratch + 84}) == 3 &&
                    s.memory.read<4>(scratch + 84) == token, "FindWindow did not follow native window reordering");
            call(s, "CarbonLib::DisposeWindow", {top});
            [window orderOut:nil];
            require(call(s, "CarbonLib::FindWindow", {(175u << 16) | 225u, scratch + 84}) == 0,
                    "FindWindow included a hidden native window");
            call(s, "CarbonLib::ShowWindow", {token});
            for (unsigned i = 0; i < 32 && call(s, "CarbonLib::WaitNextEvent", {0xffff, record, 0, 0}); ++i) {}
            s.memory.write<4>(scratch + 16, 0x77696e64); s.memory.write<4>(scratch + 20, 85);
            s.memory.write<4>(scratch + 24, 0x6d6f7573); s.memory.write<4>(scratch + 28, 10);
            constexpr std::uint32_t original_handler = 0x108bdad4;
            require(call(s, "CarbonLib::InstallEventHandler", {token, original_handler, 2, scratch + 16, 0, scratch + 64}) == 0,
                    "Original click and wheel handler installation");
            const auto button = std::uint32_t(s.memory.read<4>(0x108b5fb0));
            const auto wheel = std::uint32_t(s.memory.read<4>(0x108b5fb8));
            const auto secondary_modifier = std::uint32_t(s.memory.read<4>(0x108b5fc0));
            s.memory.write<2>(button, 0); s.memory.write<4>(wheel, 7); s.memory.write<4>(secondary_modifier, 0x1000);
            NSEvent* right = [NSEvent mouseEventWithType:NSEventTypeRightMouseDown location:NSMakePoint(23, 44)
                modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:window.windowNumber
                context:nil eventNumber:1 clickCount:1 pressure:1];
            [window.contentView rightMouseDown:right];
            const auto executed = s.executed;
            require(call(s, "CarbonLib::WaitNextEvent", {4, record, 0, 0}) == 0 && s.memory.read<2>(button) == 2 && s.executed > executed,
                    "Native right-click did not invoke the actual original content-click callback");
            require(call(s, "CarbonLib::WaitNextEvent", {2, record, 0, 0}) == 1 && s.memory.read<2>(record) == 1 &&
                    s.memory.read<2>(record + 10) == 256 && s.memory.read<2>(record + 12) == 173,
                    "Event mask filtering lost the queued click or its global EventRecord coordinates");
            NSEvent* control = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDown location:NSMakePoint(24, 45)
                modifierFlags:NSEventModifierFlagControl timestamp:NSProcessInfo.processInfo.systemUptime
                windowNumber:window.windowNumber context:nil eventNumber:2 clickCount:1 pressure:1];
            [window.contentView mouseDown:control];
            require(call(s, "CarbonLib::WaitNextEvent", {2, record, 0, 0}) == 1 && s.memory.read<2>(button) == 2 &&
                    (s.memory.read<2>(record + 14) & 0x1f00) == 0x1000,
                    "Original control-click secondary-button logic was not executed");
            NSEvent* left = [NSEvent mouseEventWithType:NSEventTypeLeftMouseDown location:NSMakePoint(24, 45)
                modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime
                windowNumber:window.windowNumber context:nil eventNumber:3 clickCount:1 pressure:1];
            [window.contentView mouseDown:left];
            require(call(s, "CarbonLib::WaitNextEvent", {2, record, 0, 0}) == 1 && s.memory.read<2>(button) == 1,
                    "Unmodified left click became a secondary click in the original callback");
            NSEvent* key = [NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint modifierFlags:0
                timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:window.windowNumber context:nil
                characters:@"a" charactersIgnoringModifiers:@"a" isARepeat:NO keyCode:0];
            [window.contentView keyDown:key];
            require(call(s, "CarbonLib::WaitNextEvent", {8, record, 0, 0}) == 1 && s.memory.read<2>(record) == 3 &&
                    s.memory.read<4>(record + 2) == 0x61, "Original key-down EventRecord message encoding");
            const auto scroll = CGEventCreateScrollWheelEvent(nullptr, kCGScrollEventUnitLine, 1, -3);
            require(scroll != nullptr, "Cannot create native scroll fixture");
            NSEvent* scroll_event = [NSEvent eventWithCGEvent:scroll];
            [window.contentView scrollWheel:scroll_event];
            CFRelease(scroll);
            require(call(s, "CarbonLib::WaitNextEvent", {0, record, 0, 0}) == 0 && s.memory.read<4>(wheel) == 4,
                    "Native wheel input did not execute the original wheel-delta accumulator");
            const auto region = call(s, "CarbonLib::NewRgn", {});
            require(call(s, "CarbonLib::WaitNextEvent", {0x8000, record, 0, region}) == 1 &&
                    s.memory.read<2>(record) == 15 && s.memory.read<4>(record + 2) == 0xfa000000,
                    "Mouse outside the supplied region did not produce the original mouse-moved event");
            call(s, "CarbonLib::DisposeRgn", {region});
            call(s, "CarbonLib::DisposeWindow", {token});
            std::cout << "Native input masks, original click/wheel callbacks, and EventRecord checks passed\n";
            return 0;
        } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
    }
}
