#include "profiled_main_thread.hpp"
#include "ui_bridge.hpp"
#import <AppKit/AppKit.h>
#include <pthread.h>
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <thread>

using namespace cm::aot;
using namespace std::chrono_literals;
namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
void drain(NativeUI& ui) {
    UIEvent event{UIEvent::Kind::update};
    for (unsigned i = 0; i < 100 && ui.poll(event, 0); ++i) {}
}
void check_headless() {
    struct QualifiedCall {
        int operator()() & { return 1; }
        int operator()() && { return 2; }
    };
    require(CM_PROFILE_ON_MAIN("Test direct qualification", QualifiedCall{}) == 2,
            "Profiled direct call lost rvalue qualification");
    const auto main_stack = pthread_get_stacksize_np(pthread_self());
    {
        // A busy game must not find one queued event per pointer movement.
        UIEventQueue busy;
        const auto push = [&](UIEvent::Kind kind, std::int16_t x, std::uint32_t message = 0, std::uint16_t modifiers = 0) {
            UIEvent event{kind}; event.x = x; event.message = message; event.modifiers = modifiers; busy.push_back(event);
        };
        push(UIEvent::Kind::mouse_moved, 1); push(UIEvent::Kind::mouse_moved, 2); push(UIEvent::Kind::mouse_moved, 3);
        push(UIEvent::Kind::mouse_down, 3);
        push(UIEvent::Kind::mouse_moved, 4); push(UIEvent::Kind::mouse_moved, 5, 0, 0x80);
        push(UIEvent::Kind::auto_key, 0, 7); push(UIEvent::Kind::auto_key, 0, 7); push(UIEvent::Kind::auto_key, 0, 8);
        push(UIEvent::Kind::key_down, 0, 7); push(UIEvent::Kind::key_down, 0, 7);
        const std::pair<UIEvent::Kind, int> expected[] = {
            {UIEvent::Kind::mouse_moved, 3}, {UIEvent::Kind::mouse_down, 3}, {UIEvent::Kind::mouse_moved, 4},
            {UIEvent::Kind::mouse_moved, 5}, {UIEvent::Kind::auto_key, 7}, {UIEvent::Kind::auto_key, 8},
            {UIEvent::Kind::key_down, 7}, {UIEvent::Kind::key_down, 7}};
        for (const auto& [kind, value] : expected) {
            UIEvent event;
            require(busy.pop(event) && event.kind == kind &&
                    (kind == UIEvent::Kind::mouse_moved || kind == UIEvent::Kind::mouse_down ? event.x == value : int(event.message) == value),
                    "Input queue coalescing changed order or kept stale pointer movement");
        }
        require(busy.empty(), "Input queue kept a coalesced event");
    }
    UIEventQueue queue;
    std::atomic<unsigned> beats{0};
    auto* counter = &beats;
    auto* messages = &queue;
    NSTimer* timer = [NSTimer timerWithTimeInterval:0.01 repeats:YES block:^(NSTimer*) {
        const auto value = counter->fetch_add(1) + 1;
        if (value == 2 || value == 3) {
            UIEvent event{UIEvent::Kind::key_down}; event.message = value;
            messages->push_back(event);
        }
    }];
    [NSRunLoop.mainRunLoop addTimer:timer forMode:NSRunLoopCommonModes];
    int result;
    try {
        result = run_game_thread([&] {
            require(!on_main_thread(), "Game work ran on main");
            require(pthread_get_stacksize_np(pthread_self()) >= main_stack, "Worker lost original stack capacity");
            require(on_main([] { return on_main_thread() && on_main([] { return true; }); }), "Main dispatch ownership");
            require(CM_PROFILE_ON_MAIN("Test worker qualification", QualifiedCall{}) == 1,
                    "Profiled worker call changed on_main lvalue invocation");
            auto owned = CM_PROFILE_ON_MAIN("Test move-only transfer", [value = std::make_unique<int>(37)]() mutable {
                require(on_main_thread(), "Profiled callable ran off main");
                require(CM_PROFILE_ON_MAIN("Test nested dispatch", [] { return on_main_thread(); }),
                        "Profiled nested dispatch lost main ownership");
                return std::move(value);
            });
            require(owned && *owned == 37, "Profiled dispatch lost move-only capture/result");
            const auto until = std::chrono::steady_clock::now() + 250ms;
            while (std::chrono::steady_clock::now() < until) std::atomic_signal_fence(std::memory_order_seq_cst);
            require(beats.load() >= 5, "Main run loop stopped during busy game work");
            UIEvent event{UIEvent::Kind::update};
            require(queue.pop(event) && event.message == 2 && queue.pop(event) && event.message == 3 && !queue.pop(event),
                    "Concurrent input changed FIFO order");
            on_main([&] {
                [NSTimer scheduledTimerWithTimeInterval:0.01 repeats:NO block:^(NSTimer*) {
                    messages->push_back({UIEvent::Kind::quit});
                }];
            });
            require(queue.pop(event, 1) && event.kind == UIEvent::Kind::quit, "Sleeping game was not woken by queued input");
            const auto wait_start = std::chrono::steady_clock::now();
            require(!queue.pop(event, 0.01) && std::chrono::steady_clock::now() - wait_start >= 5ms,
                    "Empty input wait returned early");
            bool caught = false;
            try { CM_PROFILE_ON_MAIN("Test exception", [] { throw std::runtime_error("main failure"); }); }
            catch (const std::runtime_error& error) { caught = std::string_view(error.what()) == "main failure"; }
            require(caught, "Main exception did not reach game thread");
            return 23;
        }, service_main_queue);
    } catch (...) { [timer invalidate]; throw; }
    [timer invalidate];
    require(result == 23 && !game_thread_active(), "Worker result or shutdown lost");
    queue.push_back({UIEvent::Kind::key_down}); queue.push_back({UIEvent::Kind::mouse_up}); queue.push_back({UIEvent::Kind::key_down});
    require(queue.flush(0xffff, 4), "Flush did not stop at mouse-up");
    UIEvent event{UIEvent::Kind::update};
    require(queue.pop(event) && event.kind == UIEvent::Kind::mouse_up && queue.pop(event) && event.kind == UIEvent::Kind::key_down,
            "Flush removed its stop or subsequent input");
    queue.push_back({UIEvent::Kind::key_down}); queue.push_back({UIEvent::Kind::update, 9}); queue.push_back({UIEvent::Kind::quick_save});
    require(queue.take(event, UIEvent::Kind::update) && event.window == 9 && queue.contains(UIEvent::Kind::quick_save),
            "Paint extraction reordered or discarded commands");
    queue.remove_window(9);
    require(queue.pop(event) && event.kind == UIEvent::Kind::key_down && queue.pop(event) && event.kind == UIEvent::Kind::quick_save,
            "Input around paint extraction changed order");
    bool caught = false;
    try {
        run_game_thread([]() -> int {
            struct Cleanup { ~Cleanup() { on_main([] {}); } } cleanup;
            throw std::runtime_error("worker failure");
        }, service_main_queue);
    } catch (const std::runtime_error& error) { caught = std::string_view(error.what()) == "worker failure"; }
    require(caught && !game_thread_active(), "Worker cleanup/error deadlocked main");
    require(run_game_thread([] { return on_main([] { return 29; }); }, service_main_queue) == 29, "Host restart failed");
    struct CapturedOwner {
        std::atomic<bool>& released;
        ~CapturedOwner() { released.store(!on_main_thread() && on_main([] { return on_main_thread(); })); }
    };
    std::atomic<bool> released{false};
    require(run_game_thread([owner = std::make_shared<CapturedOwner>(released)] { return owner ? 1 : 0; }, service_main_queue) == 1 && released.load(),
            "Captured game owner was destroyed after the main host stopped");
    std::cout << "Headless worker/stack ownership, main-loop responsiveness (" << beats.load()
              << " ticks during 250 ms of work), input FIFO/masks and exception cleanup passed\n";
}
void check_bootstrap() {
    prepare_native_ui();
    // Deliver through the real delegate before the worker has created UI.
    // Files are message payloads only; no save is opened or changed.
    [NSApp.delegate application:NSApp openFiles:@[@"/tmp/cm-thread-launch.sav"]];
    [NSApp.delegate application:NSApp printFiles:@[@"/tmp/cm-thread-print.sav"] withSettings:@{} showPrintPanels:NO];
    require([NSApp.delegate applicationShouldTerminate:NSApp] == NSTerminateCancel, "Bootstrap quit bypassed the game");
    require(run_game_thread([] {
        NativeUI ui;
        UIEvent event{UIEvent::Kind::update};
        std::vector<UIEvent::Kind> kinds;
        for (unsigned i = 0; i < 100 && ui.poll(event, 0); ++i) {
            if (event.kind == UIEvent::Kind::open_files) require(event.files == std::vector<std::filesystem::path>{"/tmp/cm-thread-launch.sav"}, "Lost launch document");
            if (event.kind == UIEvent::Kind::print_files) require(event.files == std::vector<std::filesystem::path>{"/tmp/cm-thread-print.sav"}, "Lost print document");
            if (event.kind == UIEvent::Kind::launch || event.kind == UIEvent::Kind::open_files ||
                event.kind == UIEvent::Kind::print_files || event.kind == UIEvent::Kind::quit) kinds.push_back(event.kind);
        }
        require(kinds == std::vector{UIEvent::Kind::open_files, UIEvent::Kind::print_files, UIEvent::Kind::quit},
                "Bootstrap dropped/reordered events or duplicated launch-with-documents");
        return 0;
    }, service_native_ui) == 0, "Bootstrap worker failed");
    std::cout << "Pre-worker Finder document/print/quit buffering and launch suppression passed\n";
}
void check_geometry() {
    std::atomic<bool> pause_requested{false}, main_paused{false}, reads_done{false}, pause_finished{false};
    bool read_without_main = false;
    const auto status = run_game_thread([&] {
        NativeUI ui;
        require(ui.adaptive_canvas(true) == GuiCanvas{}, "No-window canvas changed");
        auto missing = [&](std::uint32_t token) {
            bool caught = false;
            try { (void)ui.window_bounds(token, true); } catch (const std::out_of_range&) { caught = true; }
            require(caught, "Snapshot lost invalid-window exception");
        };
        missing(1);
        const LegacyRect original{100, 100, 700, 900};
        ui.create_window(1, 13, 0x02200000, original);
        auto number = ui.window_number(1);
        drain(ui);
        auto maximum = [&](double scale) {
            return on_main([&] {
                auto* window = [NSApp windowWithWindowNumber:number];
                auto* screen = window.screen ? window.screen : NSScreen.mainScreen;
                const auto area = [window contentRectForFrameRect:screen.visibleFrame];
                return bounded_gui_canvas(int(area.size.width / scale), int(area.size.height / scale));
            });
        };
        const auto expanded = ui.adaptive_canvas(true);
        require(expanded == maximum(1), "Initial host canvas differs from desktop constraints");
        require(ui.window_bounds(1, true) == original && ui.window_bounds(1, false) == original,
                "Requesting a canvas committed guest bounds early");
        // Deliberately pause main while the worker reads. A hidden synchronous
        // dispatch cannot pass this check; the bounded pause releases failures.
        pause_requested.store(true);
        while (!main_paused.load()) std::this_thread::yield();
        for (unsigned i = 0; i < 1000; ++i) {
            require(ui.adaptive_canvas(true) == expanded && ui.window_bounds(1, true) == original,
                    "Repeated geometry read changed without a publication");
        }
        reads_done.store(true);
        while (!pause_finished.load()) std::this_thread::yield();
        require(read_without_main, "Geometry reads still waited for main");
        ui.commit_canvas({900, 640});
        require(ui.window_bounds(1, true) == LegacyRect{100, 100, 740, 1000}, "Committed bounds were not published");
        // Model a rejected guest allocation: committing the retained viewport
        // must not replace the requested host size or partially resize it.
        ui.commit_canvas({800, 600});
        require(ui.window_bounds(1, false) == original && ui.adaptive_canvas(true) == expanded,
                "Retaining a failed-resize viewport lost the pending host request");
        const auto native_window_available = on_main([&] { return [NSApp windowWithWindowNumber:number] != nil; });
        if (!native_window_available)
            std::cout << "Native fixture window unavailable: resize/window-delegate/style checks are unexercised\n";
        on_main([&] {
            auto* window = [NSApp windowWithWindowNumber:number];
            [window setContentSize:NSMakeSize(900, 640)];
            [window.delegate windowDidEndLiveResize:[NSNotification notificationWithName:NSWindowDidEndLiveResizeNotification object:window]];
        });
        auto cap = maximum(1);
        if (native_window_available)
            require(ui.adaptive_canvas(true) == GuiCanvas{std::min(900, cap.width), std::min(640, cap.height)},
                    "Completed resize did not publish its preferred canvas");
        for (auto scale : {InterfaceScale::percent125, InterfaceScale::percent150, InterfaceScale::percent200}) {
            const auto before = ui.adaptive_canvas(true);
            if (ui.set_interface_scale(scale)) {
                const double factor = scale == InterfaceScale::percent125 ? 1.25 : scale == InterfaceScale::percent150 ? 1.5 : 2.;
                cap = maximum(factor);
                require(ui.adaptive_canvas(true) == GuiCanvas{std::min(900, cap.width), std::min(640, cap.height)},
                        "Scale change left stale adaptive geometry");
            } else require(ui.adaptive_canvas(true) == before, "Rejected scale changed published geometry");
        }
        const auto has_display = on_main([&] {
            auto* window = [NSApp windowWithWindowNumber:number];
            auto* screen = window.screen ? window.screen : NSScreen.mainScreen;
            const auto area = [window contentRectForFrameRect:screen.visibleFrame];
            return screen && area.size.width > 0 && area.size.height > 0;
        });
        const auto before_fit = ui.adaptive_canvas(true);
        require(ui.set_interface_scale(InterfaceScale::fit) == has_display,
                "Fit acceptance differs from desktop availability");
        const auto after_fit = has_display ? maximum(1) : before_fit;
        require(ui.adaptive_canvas(true) == after_fit, "Fit left incorrect published geometry");
        if (!has_display)
            std::cout << "No AppKit display: checked rejected scale requests; accepted scale/display-size changes are unexercised\n";
        on_main([&] {
            auto* window = [NSApp windowWithWindowNumber:number];
            [window.delegate windowDidChangeScreen:[NSNotification notificationWithName:NSWindowDidChangeScreenNotification object:window]];
            [window.delegate windowDidChangeBackingProperties:[NSNotification notificationWithName:NSWindowDidChangeBackingPropertiesNotification object:window]];
            [NSApp.delegate applicationDidChangeScreenParameters:[NSNotification notificationWithName:NSApplicationDidChangeScreenParametersNotification object:NSApp]];
        });
        require(ui.adaptive_canvas(true) == after_fit, "Display notification lost geometry");
        require(ui.adaptive_canvas(false) == GuiCanvas{}, "Classic mode did not publish its canvas");
        if (native_window_available)
            require(on_main([&] { return !([NSApp windowWithWindowNumber:number].styleMask & NSWindowStyleMaskResizable); }),
                    "Mode transition returned before updating native style");
        const LegacyRect overlay{125, 140, 325, 440};
        ui.create_window(2, 13, 0x02200000, overlay);
        require(ui.window_bounds(2, false) == overlay, "Overlay bounds were not published");
        ui.dispose_window(2); missing(2);
        const LegacyRect replacement{150, 160, 350, 460};
        ui.create_window(2, 13, 0x02200000, replacement);
        require(ui.window_bounds(2, true) == replacement, "Reused token retained old bounds");
        ui.dispose_window(1); missing(1);
        require(ui.adaptive_canvas(true) == GuiCanvas{} && ui.window_bounds(2, true) == replacement,
                "Primary disposal lost remaining window geometry");
        ui.create_window(1, 13, 0x02200000, original);
        number = ui.window_number(1);
        require(ui.window_bounds(1, true) == original && ui.adaptive_canvas(true) == maximum(1),
                "Replacement primary window retained stale geometry");
        ui.dispose_window(2); ui.dispose_window(1);
        return 0;
    }, [&] {
        if (pause_requested.exchange(false)) {
            main_paused.store(true);
            const auto deadline = std::chrono::steady_clock::now() + 500ms;
            while (!reads_done.load() && std::chrono::steady_clock::now() < deadline) std::this_thread::yield();
            read_without_main = reads_done.load();
            pause_finished.store(true);
        }
        service_native_ui();
    });
    require(status == 0, "Geometry worker failed");
    std::cout << "Geometry publication, main-independent reads, lifecycle and mode transitions passed\n";
}
void benchmark_geometry() {
    require(run_game_thread([] {
        NativeUI ui;
        ui.create_window(1, 13, 0x02200000, {100, 100, 700, 900});
        for (const bool adaptive : {true, false}) {
            const auto canvas = ui.adaptive_canvas(adaptive);
            const auto bounds = ui.window_bounds(1, true);
            const auto start = std::chrono::steady_clock::now();
            for (unsigned i = 0; i < 10000; ++i)
                require(ui.adaptive_canvas(adaptive) == canvas && ui.window_bounds(1, true) == bounds,
                        "Benchmark geometry changed");
            const auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
            std::cout << "{\"adaptive\":" << (adaptive ? "true" : "false")
                      << ",\"pairs\":10000,\"wall_ms\":" << ms << "}\n";
        }
        ui.dispose_window(1);
        return 0;
    }, service_native_ui) == 0, "Geometry benchmark failed");
}
}
int main(int argc, char** argv) {
    @autoreleasepool {
        try {
            if (argc == 1) { check_headless(); return 0; }
            if (argc == 2 && std::string_view(argv[1]) == "--bootstrap") { check_bootstrap(); return 0; }
            if (argc == 2 && std::string_view(argv[1]) == "--geometry") { check_geometry(); return 0; }
            if (argc == 2 && std::string_view(argv[1]) == "--geometry-benchmark") { benchmark_geometry(); return 0; }
            require(argc == 2 && std::string_view(argv[1]) == "--appkit", "Unknown thread-host check option");
            [NSApplication sharedApplication];
            const auto main_stack = pthread_get_stacksize_np(pthread_self());
            std::atomic<unsigned> beats{0};
            std::atomic<bool> busy{false};
            const auto status = run_game_thread([&] {
                require(!on_main_thread(), "Game work ran on main");
                require(pthread_get_stacksize_np(pthread_self()) >= main_stack, "Worker lost the original stack budget");
                require(on_main([] { return on_main_thread(); }), "UI request did not reach main");
                require(on_main([] { return on_main([] { return 17; }); }) == 17, "Nested main request deadlocked");
                NativeUI ui;
                ui.create_window(1, 13, 0x02200000, {100, 100, 700, 900});
                ui.show_window(1, false);
                const auto number = ui.window_number(1);
                drain(ui);
                // A native timer delivers input while the game deliberately
                // makes no UI calls. No career or captured game is launched.
                auto* counter = &beats;
                auto* working = &busy;
                on_main([=] {
                    require([NSApp windowWithWindowNumber:number].contentView != nil, "Native fixture window is unavailable");
                    NSTimer* timer = [NSTimer timerWithTimeInterval:0.01 repeats:YES block:^(NSTimer* timer) {
                        if (!working->load()) { [timer invalidate]; return; }
                        const auto count = counter->fetch_add(1) + 1;
                        if (count == 2 || count == 3) {
                            NSString* text = count == 2 ? @"a" : @"b";
                            NSEvent* event = [NSEvent keyEventWithType:NSEventTypeKeyDown location:NSZeroPoint
                                modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:number
                                context:nil characters:text charactersIgnoringModifiers:text isARepeat:NO keyCode:0];
                            [[NSApp windowWithWindowNumber:number].contentView keyDown:event];
                        }
                    }];
                    working->store(true);
                    [NSRunLoop.mainRunLoop addTimer:timer forMode:NSRunLoopCommonModes];
                });
                const auto until = std::chrono::steady_clock::now() + 250ms;
                while (std::chrono::steady_clock::now() < until) std::atomic_signal_fence(std::memory_order_seq_cst);
                busy.store(false);
                require(beats.load() >= 5, "Native event loop stopped while the game was busy");
                std::string keys;
                UIEvent event{UIEvent::Kind::update};
                for (unsigned i = 0; i < 100 && ui.poll(event, 0); ++i)
                    if (event.kind == UIEvent::Kind::key_down) keys += char(event.message);
                if (keys != "ab") throw std::runtime_error("Input queued during processing: expected ab, received " + keys);
                bool caught = false;
                try { ui.window_number(999); } catch (const std::out_of_range&) { caught = true; }
                require(caught, "UI exception did not return to the game worker");
                // Invalid buffers must also fail on the worker, without an
                // exception crossing libdispatch or killing the host loop.
                caught = false;
                try { ui.present_window(1, 800, 600, 3200, {}); } catch (const std::runtime_error&) { caught = true; }
                require(caught, "Presentation error escaped the UI boundary");
                const std::vector<std::uint8_t> pixels(800 * 600 * 4, 0x7f);
                ui.present_window(1, 800, 600, 3200, pixels);
                on_main([=] {
                    [[NSApp windowWithWindowNumber:number] performClose:nil];
                });
                bool quit = false;
                for (unsigned i = 0; i < 100 && ui.poll(event, 0); ++i) quit |= event.kind == UIEvent::Kind::quit;
                require(quit && ui.window_number(1) == number, "Close bypassed the game's save/quit decision");
                ui.dispose_window(1);
                return 23;
            }, service_native_ui);
            require(status == 23 && !game_thread_active(), "Worker result or shutdown was lost");
            bool caught = false;
            try {
                run_game_thread([]() -> int {
                    NativeUI ui;
                    ui.create_window(2, 13, 0x02200000, {100, 100, 700, 900});
                    ui.create_dialog(9, {{150, 150, 250, 450}, "Worker cleanup", 0}, {});
                    ui.show_dialog(9);
                    ui.create_dialog(4, {{175, 175, 275, 475}, "Nested cleanup", 0}, {});
                    ui.show_dialog(4);
                    throw std::runtime_error("worker failure");
                }, service_native_ui);
            } catch (const std::runtime_error& error) { caught = std::string(error.what()) == "worker failure"; }
            require(caught && !game_thread_active(), "Worker exception or destructor deadlocked shutdown");
            require(NSApp.modalWindow == nil, "Worker teardown left a modal session active");
            require(run_game_thread([] { NativeUI ui; return 29; }, service_native_ui) == 29,
                    "Host could not restart after a worker exception");
            std::cout << "Worker ownership, main-loop responsiveness (" << beats.load()
                      << " ticks during 250 ms of work), FIFO input, exception and shutdown checks passed\n";
            return 0;
        } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
    }
}
