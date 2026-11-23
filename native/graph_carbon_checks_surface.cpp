// graph_carbon.cpp, block `surface` (0x10768820, 0x10768990, 0x107689e0, 0x10768b40, 0x10768c50, 0x10768c90, 0x10768dc0, 0x10768e10,
// 0x107690f0, 0x107691b0, 0x107692f0, 0x1076e7c0, 0x1076e870, 0x1076e920, 0x1076e9d0): cases. The Toolbox has no host behind it in a
// snapshot, so each case scripts the imports it reaches BY NAME (Case::imports: the checker answers on both sides, and logs every import
// call - the name, r3-r10, the return address after the `bl`, r13-r31 - into memory the comparison sees). The import answers that matter
// are written out per case (the resource handle, the GetPort result written through its pointer, the window variable CreateNewWindow
// writes); the rest are the defaults at the end of each list (the first matching entry of a call wins: the entries for one call come first,
// an entry with nth 0 answers every call). The variables the routines read (the TOC's and the small data) are written in guest memory
// over the snapshot's values, and a resource, a device record or a rectangle is built in the scratch area. Labels start with "surface:".
// No capture reaches the window, surface or cursor creation (they run at start-up), so these cases are the only evidence of those paths.
#include "graph_carbon_checks.hpp"
#include "graph_carbon_surface_detail.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <random>

namespace cm::aot::graph_carbon_checks {
namespace {
using namespace cm::aot::graph_carbon;
using Import = Case::Import;
using Regs = std::vector<std::pair<unsigned, std::uint32_t>>;
constexpr std::uint32_t game_toc = 0x108bba90;
[[maybe_unused]] constexpr std::uint32_t start_entry = 0x10768820, close_entry = 0x10768990, full_entry = 0x107689e0, display_entry = 0x10768b40,
                        leave_entry = 0x10768c50, surface_entry = 0x10768c90, dispose_entry = 0x10768dc0, window_entry = 0x10768e10,
                        bounds_entry = 0x107690f0, close_window_entry = 0x107691b0, flush_entry = 0x107692f0, load_entry = 0x1076e7c0,
                        release_entry = 0x1076e870, cursor_entry = 0x1076e920, paint_entry = 0x1076e9d0;
// Scratch layout: rectangles, a resource handle (a master pointer to its data), a device handle and its GDevice record, a PixMap handle
// and record, and cursor handles.
[[maybe_unused]] constexpr std::uint32_t area_at = scratch_base + 0x10000, out_at = scratch_base + 0x10100, handle_at = scratch_base + 0x11000,
                        data_at = scratch_base + 0x11100, device_handle_at = scratch_base + 0x12000, device_at = scratch_base + 0x12100,
                        pixmap_handle_at = scratch_base + 0x13000, pixmap_at = scratch_base + 0x13100, cursor_at = scratch_base + 0x14000,
                        rect_in = scratch_base + 0x15000;
// Every made-up pointer (a port, a window, a handle, a cursor) lies in mapped scratch memory: a routine that reads leftover stack and dereferences what it
// finds (as the original's printf chain does) must find memory, as it does in the game where those slots held real pointers.
constexpr std::uint32_t mapped(std::uint32_t pointer) { return scratch_base + 0x30000 + ((pointer & 0x00ff0000) >> 6) + (pointer & 0xffff); }
constexpr std::uint32_t window_pointer = mapped(0x61008000), port_value = mapped(0x61020000), gray_region = mapped(0x61030000);
constexpr std::uint32_t small_at(std::int32_t offset) { return game_toc + std::uint32_t(offset); }
std::vector<std::pair<unsigned, std::uint32_t>> regs(std::initializer_list<std::pair<unsigned, std::uint32_t>> used) {
    Regs registers;
    for (unsigned n = 3; n <= 10; ++n) registers.emplace_back(n, 0x7a000000u + n * 0x01010101u);
    for (const auto& [n, value] : used) registers[n - 3].second = value;
    return registers;
}
std::vector<Write> concat(std::vector<Write> a, const std::vector<Write>& b) {
    append(a, b);
    return a;
}
// A resource's data: a handle at handle_at whose master pointer is data_at, holding the Point {vertical, horizontal}.
std::vector<Write> resource(std::int32_t vertical, std::int32_t horizontal) {
    return {w32(handle_at, data_at), w16(data_at, std::uint32_t(vertical)), w16(data_at + 2, std::uint32_t(horizontal)), w32(data_at + 4, 0xdeadbeef)};
}
// A Mac Rect {top, left, bottom, right} at `at`.
std::vector<Write> rect_words(std::uint32_t at, std::int32_t top, std::int32_t left, std::int32_t bottom, std::int32_t right) {
    return {w16(at, std::uint32_t(top)), w16(at + 2, std::uint32_t(left)), w16(at + 4, std::uint32_t(bottom)), w16(at + 6, std::uint32_t(right))};
}
// The default answers of every import the block uses (nth 0: every call, last in a case's list). Results are distinct, nonzero where an
// answer is a handle or a port, so a register that is used in the wrong place shows in the import log.
std::vector<Import> defaults(const Scenario& scenario) {
    const auto window_variable = scenario.toc(surface_toc::window);
    return {
        {"CarbonLib::GetGrayRgn", 0, gray_region},
        {"CarbonLib::GetRegionBounds", 0, 0},
        {"CarbonLib::InsetRect", 0, 0},
        {"CarbonLib::GetPort", 0, 0},
        {"CarbonLib::CreateNewWindow", 0, 0, {w32(window_variable, window_pointer)}},
        {"CarbonLib::NewEventHandlerUPP", 0, mapped(0x61040000)},
        {"CarbonLib::GetWindowEventTarget", 0, mapped(0x61050000)},
        {"CarbonLib::InstallEventHandler", 0, 0},
        {"CarbonLib::ShowWindow", 0, 0},
        {"CarbonLib::SetPort", 0, 0},
        {"CarbonLib::BringToFront", 0, 0},
        {"CarbonLib::DisposeWindow", 0, 0},
        {"CarbonLib::InvalMenuBar", 0, 0},
        {"CarbonLib::GetMaxDevice", 0, device_handle_at},
        {"CarbonLib::GetMainDevice", 0, device_handle_at},
        {"CarbonLib::SetRect", 0, 0},
        {"CarbonLib::NewGWorld", 0, 0, {w32(small_at(surface_small::gworld), mapped(0x61060000))}},
        {"CarbonLib::GetPortPixMap", 0, pixmap_handle_at},
        {"CarbonLib::LockPixels", 0, 1},
        {"CarbonLib::GetPixBaseAddr", 0, mapped(0x61070000)},
        {"CarbonLib::DisposeGWorld", 0, 0},
        {"CarbonLib::GetResource", 0, 0},
        {"CarbonLib::PtInRect", 0, 0},
        {"CarbonLib::ReleaseResource", 0, 0},
        {"CarbonLib::SetPt", 0, 0},
        {"CarbonLib::PtInRgn", 0, 0},
        {"CarbonLib::GetWindowPort", 0, mapped(0x61090000)},
        {"CarbonLib::CFStringGetSystemEncoding", 0, 0x600},
        {"CarbonLib::CFStringCreateWithCStringNoCopy", 0, mapped(0x610a0000)},
        {"CarbonLib::SetWindowTitleWithCFString", 0, 0},
        {"CarbonLib::CFRelease", 0, 0},
        {"CarbonLib::GetWindowBounds", 0, 0},
        {"CarbonLib::NewHandle", 0, handle_at},
        {"CarbonLib::LocalToGlobal", 0, 0},
        {"CarbonLib::AddResource", 0, 0},
        {"CarbonLib::WriteResource", 0, 0},
        {"CarbonLib::ChangedResource", 0, 0},
        {"CarbonLib::GetPortBitMapForCopyBits", 0, mapped(0x610b0000)},
        {"CarbonLib::CopyBits", 0, 0},
        {"CarbonLib::QDFlushPortBuffer", 0, 0},
        {"CarbonLib::GetCCursor", 0, 0},
        {"CarbonLib::GetCursor", 0, 0},
        {"CarbonLib::InitCursor", 0, 0},
        {"CarbonLib::DisposeCCursor", 0, 0},
        {"CarbonLib::SetCCursor", 0, 0},
        {"CarbonLib::SetCursor", 0, 0},
        {"CarbonLib::ForeColor", 0, 0},
        {"CarbonLib::PaintRect", 0, 0},
    };
}
// A case: its imports are `first` (one call each, by nth) then the defaults.
Case make(const Scenario& scenario, std::uint32_t entry, Regs registers, std::vector<Write> memory, std::vector<Import> first, std::string label) {
    // The scratch objects every default answer points at (a PixMap with its row bytes, a resource's data), first so that a case's own writes win.
    std::vector<Write> base{w32(pixmap_handle_at, pixmap_at), w16(pixmap_at + 4, 0x8640), w32(handle_at, data_at), w32(data_at, 0), w32(data_at + 4, 0)};
    append(base, memory);
    Case item{.entry = entry, .registers = std::move(registers), .memory = std::move(base), .label = "surface: " + std::move(label)};
    item.imports = std::move(first);
    for (const auto& answer : defaults(scenario)) item.imports.push_back(answer);
    return item;
}
std::string number(std::uint32_t value) { return hex(value); }
// The assertion box 0x10769b60 made quiet and run for real on both sides (Case::real_dialog): its re-entrancy bytes (small data -0x3570 "has run",
// -0x356f "is showing") make it return at once. Needed where the assertion is a call inside the box's own page (0x10769xxx), which only the
// original makes in place.
Case quiet_dialog(Case item) {
    item.real_dialog = true;
    item.memory.push_back(w8(small_at(-0x3570), 1));
    item.memory.push_back(w8(small_at(-0x356f), 1));
    return item;
}
std::vector<std::pair<std::uint32_t, std::uint32_t>> concat_answers(std::vector<std::pair<std::uint32_t, std::uint32_t>> a, const std::vector<std::pair<std::uint32_t, std::uint32_t>>& b) {
    a.insert(a.end(), b.begin(), b.end());
    return a;
}
}

GRAPH_CARBON_CASES("surface") {
    const auto sp = scenario.sp();
    const auto window_variable = scenario.toc(surface_toc::window);
    const auto full_screen_variable = scenario.toc(surface_toc::full_screen);
    const auto width_variable = scenario.toc(surface_toc::width), height_variable = scenario.toc(surface_toc::height);
    const auto cursors = scenario.toc(surface_toc::cursors), cursors_enabled = scenario.toc(surface_toc::cursors_enabled);
    if (std::getenv("CM_GRAPH_CARBON_SURFACE_DUMP")) {
        std::cerr << "surface snapshot: window var " << hex(window_variable) << " = " << hex(scenario.r32(window_variable)) << " again " << hex(scenario.toc(surface_toc::window_again))
                  << " full " << hex(full_screen_variable) << " = " << hex(scenario.u8(full_screen_variable)) << " width " << hex(scenario.r32(width_variable))
                  << " height " << hex(scenario.r32(height_variable)) << "\n  small: base " << hex(scenario.r32(small_at(surface_small::surface_base))) << " pixmap "
                  << hex(scenario.r32(small_at(surface_small::pixmap))) << " gworld " << hex(scenario.r32(small_at(surface_small::gworld))) << " grey "
                  << hex(scenario.u8(small_at(surface_small::grey))) << " device " << hex(scenario.r32(small_at(surface_small::device))) << " display "
                  << hex(scenario.r32(small_at(surface_small::display_object))) << " initialising " << hex(scenario.u8(small_at(surface_small::initialising)))
                  << " ready " << hex(scenario.u8(small_at(surface_small::ready))) << " cursor " << hex(scenario.u8(small_at(small_data::current_cursor)))
                  << "\n  cursors " << hex(cursors) << ": " << hex(scenario.r32(cursors)) << "/" << hex(scenario.u8(cursors + 4)) << " " << hex(scenario.r32(cursors + 8)) << "/"
                  << hex(scenario.u8(cursors + 12)) << " " << hex(scenario.r32(cursors + 16)) << "/" << hex(scenario.u8(cursors + 20)) << " enabled "
                  << hex(scenario.u8(cursors_enabled)) << " sp " << hex(sp) << '\n';
    }
    const auto set16 = [](std::uint32_t at, std::uint32_t value) { return w16(at, value); };

    // ---- 0x10768dc0: the GWorld given back: the PixMap variable decides, the GWorld goes to DisposeGWorld, three variables are cleared.
    for (const auto& [pixmap, gworld, base] : std::vector<std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>>{
             {mapped(0x61001000), mapped(0x61002000), mapped(0x61003000)}, {0, mapped(0x61002000), mapped(0x61003000)}, {mapped(0x61001000), 0, 0}, {1, 0x80000000, 0xffffffff}, {0, 0, 0}})
        cases.push_back(make(scenario, dispose_entry, regs({}),
                             {w32(small_at(surface_small::pixmap), pixmap), w32(small_at(surface_small::gworld), gworld), w32(small_at(surface_small::surface_base), base)},
                             {{"CarbonLib::DisposeGWorld", 1, 0x77}}, "dispose pixmap " + number(pixmap) + " gworld " + number(gworld)));
    cases.push_back(make(scenario, dispose_entry, regs({}), {}, {}, "dispose, the snapshot's state"));

    // ---- 0x10768c90: the surface. NewGWorld's error (the low halfword counts: 0x10000 is success), the PixMap's row bytes (the flag bits
    // dropped, halved), the window's size (halfword truncation) into the rectangle.
    for (const std::uint32_t error : {1u, 0xffffff94u, 0x8000u, 0xffffu, 0x7fffu, 0x12345678u, 0x10001u, 0xffff8000u}) {
        cases.push_back(make(scenario, surface_entry, regs({}), {w32(width_variable, 800), w32(height_variable, 600)},
                             {{"CarbonLib::NewGWorld", 1, error}}, "surface NewGWorld error " + number(error)));
    }
    for (const std::uint32_t row : {0x8640u, 0xc640u, 0x0640u, 0x3fffu, 0x0001u, 0x8001u, 0x0000u, 0x4000u, 0xffffu, 0x2aaau, 0x8003u})
        for (const auto& [width, height] : std::vector<std::pair<std::uint32_t, std::uint32_t>>{{800, 600}, {0x10320, 0x1fffff}, {0x8000, 0xffff8000}, {0, 0}, {0xffffffff, 1}})
            cases.push_back(make(scenario, surface_entry, regs({}),
                                 concat({w32(width_variable, width), w32(height_variable, height), w32(pixmap_handle_at, pixmap_at), w16(pixmap_at + 4, row)},
                                        {w16(small_at(surface_small::row_bytes), 0x1111), w16(small_at(surface_small::row_pixels), 0x2222)}),
                                 {{"CarbonLib::NewGWorld", 1, 0x10000}, {"CarbonLib::GetPixBaseAddr", 1, mapped(0x61070010)}},
                                 "surface rows " + number(row) + " size " + number(width) + "x" + number(height)));
    cases.push_back(make(scenario, surface_entry, regs({}), {}, {}, "surface, the snapshot's state"));

    // ---- 0x107690f0: the window's bounds: the current window (GetWindowBounds), else the remembered position (the resource 'WinP': the
    // vertical halfword, then the horizontal one) as an 800x600 rectangle; neither gives 0. Halfword wrap of position + size.
    cases.push_back(make(scenario, bounds_entry, regs({{3, out_at}}), {w32(window_variable, window_pointer)}, {{"CarbonLib::GetWindowBounds", 1, 0x1234}}, "bounds, a window"));
    cases.push_back(make(scenario, bounds_entry, regs({{3, out_at}}), {w32(window_variable, 0)}, {{"CarbonLib::GetResource", 1, 0}}, "bounds, no window, no resource"));
    for (const auto& [vertical, horizontal] : std::vector<std::pair<std::int32_t, std::int32_t>>{
             {0, 0}, {100, 200}, {-5, -7}, {0x7ffe, 0x7ffe}, {0x7fff, 0x7fff}, {-0x8000, -0x8000}, {0x7da7, 0x7cdf}, {0x7da8, 0x7ce0}, {-1, 0x100}})
        cases.push_back(make(scenario, bounds_entry, regs({{3, out_at}}), concat({w32(window_variable, 0)}, resource(vertical, horizontal)),
                             {{"CarbonLib::GetResource", 1, handle_at}},
                             "bounds, resource " + number(std::uint32_t(vertical)) + "," + number(std::uint32_t(horizontal))));
    cases.push_back(make(scenario, bounds_entry, regs({{3, out_at}}), {}, {}, "bounds, the snapshot's state"));

    // ---- 0x1076e7c0: the cursors 0x80..0x82: a colour cursor (flag 1) or the black and white one (flag 0). Every combination of
    // GetCCursor answers (the three calls), the black and white cursors found or not.
    for (unsigned mask = 0; mask < 8; ++mask) {
        std::vector<Import> first;
        unsigned black = 0;
        for (unsigned index = 0; index < 3; ++index) {
            first.push_back({"CarbonLib::GetCCursor", index + 1, (mask >> index & 1) ? mapped(0x61100000) + index * 0x100 : 0});
            if (!(mask >> index & 1)) first.push_back({"CarbonLib::GetCursor", ++black, (index == 1 && mask == 0) ? 0 : mapped(0x61200000) + index * 0x100});
        }
        cases.push_back(make(scenario, load_entry, regs({}),
                             {w32(cursors, 0x11111111), w8(cursors + 4, 0x22), w32(cursors + 8, 0x33333333), w8(cursors + 12, 0x44), w32(cursors + 16, 0x55555555), w8(cursors + 20, 0x66),
                              w8(cursors + 5, 0x77), w8(cursors + 13, 0x88)},
                             first, "load cursors colour mask " + number(mask)));
    }
    cases.push_back(make(scenario, load_entry, regs({}), {}, {}, "load cursors, the snapshot's state"));

    // ---- 0x1076e870: InitCursor, then each of the three entries given back: nothing for a zero handle, DisposeCCursor for a colour one
    // (flag nonzero), ReleaseResource for the other.
    for (unsigned code = 0; code < 27; ++code) {
        std::vector<Write> memory;
        for (unsigned index = 0; index < 3; ++index) {
            const unsigned kind = (code / (index == 0 ? 1 : index == 1 ? 3 : 9)) % 3;  // 0 none, 1 black and white, 2 colour
            memory.push_back(w32(cursors + index * 8, kind == 0 ? 0 : mapped(0x61300000) + index * 0x100));
            memory.push_back(w8(cursors + index * 8 + 4, kind == 2 ? (index == 1 ? 0x80 : 1) : 0));
        }
        cases.push_back(make(scenario, release_entry, regs({}), memory, {{"CarbonLib::InitCursor", 1, 0x99}}, "release cursors " + number(code)));
    }
    cases.push_back(make(scenario, release_entry, regs({}), {}, {}, "release cursors, the snapshot's state"));

    // ---- 0x1076e920: make a cursor current: the enabled byte (signed), the current index (signed byte compare), the table's handle (zero:
    // InitCursor) and flag (colour: SetCCursor with the handle; else SetCursor with the data pointer).
    for (const std::uint32_t enabled : {0u, 1u, 0x80u, 0xffu})
        for (const std::uint32_t current : {0u, 1u, 2u, 0x80u, 0xffu})
            for (const std::uint32_t index : {0u, 1u, 2u, 0x80u, 0xffu, 0x100u, 0x102u, 0xffffff81u})
                for (const unsigned kind : {0u, 1u, 2u}) {
                    if (kind != 0 && !(enabled == 1 && current != index)) continue;
                    std::vector<Write> memory{w8(cursors_enabled, enabled), w8(small_at(small_data::current_cursor), current)};
                    const auto at = cursors + (std::uint32_t(std::int8_t(index)) << 3);
                    memory.push_back(w32(at, kind == 0 ? 0 : cursor_at));
                    memory.push_back(w8(at + 4, kind == 2 ? 1 : 0));
                    memory.push_back(w32(cursor_at, mapped(0x61400000)));
                    cases.push_back(make(scenario, cursor_entry, regs({{3, index}}), memory, {}, "cursor enabled " + number(enabled) + " current " + number(current) + " index " +
                                                                                                    number(index) + " kind " + number(kind)));
                }
    cases.push_back(make(scenario, cursor_entry, regs({{3, 1}}), {}, {}, "cursor, the snapshot's state"));

    // ---- 0x1076e9d0: the event handler's paint.
    cases.push_back(make(scenario, paint_entry, regs({{3, 0x1111}, {4, 0x2222}, {5, 0x3333}}), {w32(window_variable, window_pointer)}, {}, "paint window"));
    cases.push_back(make(scenario, paint_entry, regs({{3, 0x1111}, {4, 0x2222}, {5, 0x3333}}), {w32(window_variable, 0)}, {}, "paint, no window"));
    cases.push_back(make(scenario, paint_entry, regs({}), {}, {}, "paint, the snapshot's state"));

    // ---- Helpers of the window cases. `answers`: the shared inline block's display object routines and MAIN's two queries are answered
    // (another module's, called at fixed sites: the case decides what they return), and the fonts / colours / fill routines of start-up.
    using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
    const Answers helpers{{0x1074daf0, 0}, {0x1074d910, 0}, {0x1074dab0, 0}, {0x1074d8d0, 0}, {0x1074daa0, 0}, {0x1074da90, 0}};
    const auto with_answers = [](Case item, const Answers& more) {
        item.answers.insert(item.answers.end(), more.begin(), more.end());
        return item;
    };
    const auto port_of = [](std::uint32_t at, unsigned nth) { return Import{"CarbonLib::GetPort", nth, 0, {w32(at, port_value + nth)}}; };
    // GetPort for every call of a case, writing the port at each slot a routine of the path reads it back from.
    const auto port_any = [](std::initializer_list<std::uint32_t> slots) {
        Import port{"CarbonLib::GetPort", 0, 0, {}};
        for (const auto slot : slots) port.writes.push_back(w32(slot, port_value));
        return port;
    };

    // ---- 0x107689e0: full screen on / off. Mode on: GetGrayRgn, GetRegionBounds, InsetRect(-100), GetPort, CreateNewWindow (class 0xd), the
    // event handler (UPP, target, install) and ShowWindow when it is made, SetPort back, and the display object shown when there is one.
    // Mode off: BringToFront, the display object hidden and deleted, DisposeWindow, InvalMenuBar.
    {
        const auto frame = sp - 0x50;
        for (const std::uint32_t mode : {1u, 0x80u, 0xffu, 0x101u, 0xffffff01u, 0x7fffff80u})
            for (const std::uint32_t error : {0u, 0xffffffe0u, 0x10000u})
                for (const std::uint32_t display : {0u, mapped(0x61500000)}) {
                    Import create{"CarbonLib::CreateNewWindow", 1, error, error == 0 ? std::vector<Write>{w32(window_variable, window_pointer)} : std::vector<Write>{}};
                    cases.push_back(with_answers(make(scenario, full_entry, regs({{3, mode}}),
                                                      {w32(window_variable, mapped(0x61009000)), w32(small_at(surface_small::display_object), display), w8(full_screen_variable, 0)},
                                                      {port_of(frame + 0x38, 1), create}, "full screen on mode " + number(mode) + " error " + number(error) + " display " + number(display)),
                                                 helpers));
                }
        for (const std::uint32_t mode : {0u, 0x100u, 0x200u, 0xffffff00u})
            for (const std::uint32_t window : {0u, window_pointer})
                for (const std::uint32_t display : {0u, mapped(0x61500000)})
                    cases.push_back(with_answers(make(scenario, full_entry, regs({{3, mode}}),
                                                      {w32(window_variable, window), w32(small_at(surface_small::display_object), display), w8(full_screen_variable, 1)}, {},
                                                      "full screen off mode " + number(mode) + " window " + number(window) + " display " + number(display)),
                                                 helpers));
    }

    // ---- 0x10768c50: leaves full screen (the byte, signed) by calling the off branch of 0x107689e0.
    for (const std::uint32_t byte : {0u, 1u, 0x80u, 0xffu, 0x7fu})
        for (const std::uint32_t window : {0u, window_pointer})
            cases.push_back(with_answers(make(scenario, leave_entry, regs({}),
                                              {w8(full_screen_variable, byte), w32(window_variable, window), w32(small_at(surface_small::display_object), window ? mapped(0x61500000) : 0u)}, {},
                                              "leave full screen byte " + number(byte) + " window " + number(window)),
                                         helpers));

    // ---- 0x10768b40: the display's bounds. The display object (zero: built), the window's bounds (a window, the resource, neither), the
    // device (GetMaxDevice when there is a window bounds, else the variable's; GetMainDevice when it is zero), the mode compared with the
    // full-screen byte (signed bytes; different: 0x107689e0 runs), the GDevice's bounds copied to the out Rect.
    {
        const std::vector<Write> device_record{w32(device_handle_at, device_at), w16(device_at + 0x22, 0x1111), w16(device_at + 0x24, 0x2222), w16(device_at + 0x26, 0x3333),
                                               w16(device_at + 0x28, 0x4444), w32(out_at, 0xa5a5a5a5), w32(out_at + 4, 0x5a5a5a5a)};
        const auto frame = sp - 0x50;
        // 0: window (GetWindowBounds), 1: no window with the resource, 2: neither
        for (unsigned bounds = 0; bounds < 3; ++bounds)
            for (const std::uint32_t device : {0u, device_handle_at})
                for (const std::uint32_t max_device : {device_handle_at, 0u})
                    for (const std::uint32_t object : {0u, mapped(0x61500000)})
                        for (const std::uint32_t make_result : {mapped(0x61600000), 0u}) {
                            if (object != 0 && (bounds != 0 || device == 0 || max_device == 0 || make_result == 0)) continue;
                            if (bounds == 2 && max_device == 0) continue;
                            std::vector<Write> memory = concat(device_record, {w32(small_at(surface_small::display_object), object), w32(small_at(surface_small::device), device),
                                                                              w32(window_variable, bounds == 0 ? window_pointer : 0), w8(full_screen_variable, 0)});
                            if (bounds == 1) append(memory, resource(10, 20));
                            std::vector<Import> first{{"CarbonLib::GetResource", 1, bounds == 1 ? handle_at : 0}};
                            if (bounds == 0) first = {{"CarbonLib::GetWindowBounds", 1, 0}};
                            first.push_back({"CarbonLib::GetMaxDevice", 1, max_device});
                            first.push_back(port_of(frame + 0x38, 1));
                            // the make result is the object new returns: the real allocator runs, so it is `make_result` only for the constructor
                            auto item = with_answers(make(scenario, display_entry, regs({{3, 0x1357}, {4, 0}, {5, out_at}}), memory, first,
                                                          "display bounds " + std::string(bounds == 0 ? "window" : bounds == 1 ? "resource" : "neither") + " device " + number(device) +
                                                              " max " + number(max_device) + " object " + number(object) + " mode same"),
                                                     helpers);
                            (void)make_result;
                            cases.push_back(item);
                        }
        // The mode against the full-screen byte, with the object present: equal as signed bytes (no call) or not (full screen on / off).
        for (const std::uint32_t mode : {0u, 1u, 0x100u, 0x101u, 0xffu, 0x80u, 0xffffff00u, 0x7f01u})
            for (const std::uint32_t byte : {0u, 1u, 0xffu, 0x80u}) {
                auto item = with_answers(make(scenario, display_entry, regs({{3, 0x1357}, {4, mode}, {5, out_at}}),
                                              concat(device_record, {w32(small_at(surface_small::display_object), mapped(0x61500000)), w32(small_at(surface_small::device), device_handle_at),
                                                                     w32(window_variable, window_pointer), w8(full_screen_variable, byte)}),
                                              {port_of(frame - 0x50 + 0x38, 1), port_of(frame - 0x50 + 0x38, 2)}, "display bounds mode " + number(mode) + " byte " + number(byte)),
                                         helpers);
                cases.push_back(item);
            }
        // The object's construction with the real allocator, the helpers answered, the device from GetMainDevice.
        cases.push_back(with_answers(make(scenario, display_entry, regs({{3, 0x1357}, {4, 0}, {5, out_at}}),
                                          concat(device_record, {w32(small_at(surface_small::display_object), 0), w32(small_at(surface_small::device), 0), w32(window_variable, 0), w8(full_screen_variable, 0)}),
                                          {{"CarbonLib::GetResource", 1, 0}}, "display bounds, main device"),
                                     helpers));
        // The allocator's refusal: new returns 0, so the object is not constructed and the variable is cleared (new's owner runs its original body
        // here, so that the answer reaches both sides).
        for (const std::uint32_t window : {window_pointer, 0u}) {
            auto item = with_answers(make(scenario, display_entry, regs({{3, 0x1357}, {4, 0}, {5, out_at}}),
                                          concat(device_record, {w32(small_at(surface_small::display_object), 0), w32(small_at(surface_small::device), 0), w32(window_variable, window), w8(full_screen_variable, 0)}),
                                          {{"CarbonLib::GetResource", 1, 0}}, "display bounds, new fails window " + number(window)),
                                     concat_answers(helpers, {{0x10773520, 0}}));
            item.original_owners = {0x10773520};
            cases.push_back(item);
        }
        cases.push_back(with_answers(make(scenario, display_entry, regs({{3, 0x1357}, {4, 1}, {5, out_at}}), {}, {}, "display bounds, the snapshot's state"), helpers));
    }

    // ---- 0x10768e10: the window. Full screen (class 0xd, attributes 0x2000000, 0x2200000 when MAIN says so) centred in the area; windowed
    // (class 6, attributes 0x2000008) at the remembered position when PtInRect likes it and PtInRgn finds it on the desktop, else centred,
    // else (16, 32). CreateNewWindow's error raises the assertion; the CFString and the title.
    {
        const auto frame = sp - 0xa0;
        const std::uint32_t title_at = scratch_base + 0x16000;
        const std::vector<Write> title{w8(title_at, 'C'), w8(title_at + 1, 'M'), w8(title_at + 2, 0)};
        struct Area { std::int32_t top, left, bottom, right; };
        const std::vector<Area> areas{{0, 0, 600, 800}, {22, 0, 1000, 1600}, {-30, -100, 900, 1500}, {0, 0, 599, 800}, {5, 7, 612, 811}, {0, 0, 0, 0}, {0x7000, 0x7000, 0x7fff, 0x7fff}, {-0x8000, -0x8000, -0x7000, -0x7000}};
        const std::vector<std::pair<std::uint32_t, std::uint32_t>> sizes{{800, 600}, {801, 601}, {1700, 1200}, {0, 0}, {0xffffffff, 0x10000}};
        // Full screen: the gap rounding (an odd negative gap rounds toward zero), MAIN's answer's low byte, the error, the string.
        for (const auto& area : areas)
            for (const auto& [width, height] : sizes)
                for (const std::uint32_t query : {0u, 1u})
                    for (const std::uint32_t error : {0u, 0x2au})
                        for (const std::uint32_t string : {mapped(0x610a0000), 0u}) {
                            if (error != 0 && (query != 0 || string == 0)) continue;
                            if (string == 0 && (query != 0 || &area != &areas[0])) continue;
                            auto item = make(scenario, window_entry, regs({{3, title_at}, {4, area_at}}),
                                             concat(concat(title, rect_words(area_at, area.top, area.left, area.bottom, area.right)),
                                                    {w32(width_variable, width), w32(height_variable, height), w8(full_screen_variable, 1), w32(window_variable, mapped(0x61009000))}),
                                             {port_of(frame + 0x3c, 1), {"CarbonLib::CreateNewWindow", 1, error, error == 0 ? std::vector<Write>{w32(window_variable, window_pointer)} : std::vector<Write>{}},
                                              {"CarbonLib::CFStringCreateWithCStringNoCopy", 1, string}},
                                             "window full screen area " + number(std::uint32_t(area.top)) + "," + number(std::uint32_t(area.left)) + "," + number(std::uint32_t(area.bottom)) +
                                                 "," + number(std::uint32_t(area.right)) + " size " + number(width) + "x" + number(height) + " query " + number(query) + " error " + number(error) +
                                                 " string " + number(string));
                            item = with_answers(item, {{0x10770ee0, query ? 0x01u : 0x100u}, {0x1076fd90, 0}});
                            cases.push_back(error != 0 ? quiet_dialog(item) : item);
                        }
        // MAIN's answer by its low byte only.
        for (const std::uint32_t query : {0x80u, 0x100u, 0xffffff00u, 0x1ffu})
            cases.push_back(with_answers(make(scenario, window_entry, regs({{3, title_at}, {4, area_at}}),
                                              concat(concat(title, rect_words(area_at, 0, 0, 600, 800)), {w32(width_variable, 640), w32(height_variable, 480), w8(full_screen_variable, 0x80), w32(window_variable, 0)}),
                                              {port_of(frame + 0x3c, 1)}, "window full screen query " + number(query)),
                                         {{0x10770ee0, query}, {0x1076fd90, 0}}));
        // Windowed: the resource (found, inside, outside, absent), PtInRgn, the rounding of the centring, the error.
        for (const auto& [vertical, horizontal] : std::vector<std::pair<std::int32_t, std::int32_t>>{{100, 200}, {-5, -7}, {0x7fff, -0x8000}, {0, 0}})
            for (const std::uint32_t inside : {1u, 0x100u, 0xffffff01u, 0u, 0x80u})
                for (const std::uint32_t in_region : {1u, 0u, 0x100u, 0xffffff01u})
                    for (const auto& [width, height] : std::vector<std::pair<std::uint32_t, std::uint32_t>>{{800, 600}, {801, 601}}) {
                        if ((width != 800 || (inside & 0xff) == 0) && (in_region == 0x100 || in_region == 0xffffff01u)) continue;
                        auto item = make(scenario, window_entry, regs({{3, title_at}, {4, area_at}}),
                                         concat(concat(concat(title, rect_words(area_at, 22, 0, 1000, 1600)), resource(vertical, horizontal)),
                                                {w32(width_variable, width), w32(height_variable, height), w8(full_screen_variable, 0), w32(window_variable, 0)}),
                                         {port_of(frame + 0x3c, 1), {"CarbonLib::GetResource", 1, handle_at}, {"CarbonLib::PtInRect", 1, inside},
                                          {"CarbonLib::SetPt", 1, 0, {w32(frame + 0x38, 0x00a000b0)}}, {"CarbonLib::PtInRgn", 1, in_region}},
                                         "window windowed resource " + number(std::uint32_t(vertical)) + "," + number(std::uint32_t(horizontal)) + " inside " + number(inside) + " region " +
                                             number(in_region) + " size " + number(width) + "x" + number(height));
                        cases.push_back(with_answers(item, {{0x1076fd90, 0}}));
                    }
        for (const auto& area : areas)
            for (const std::uint32_t in_region : {1u, 0u})
                cases.push_back(with_answers(make(scenario, window_entry, regs({{3, title_at}, {4, area_at}}),
                                                  concat(concat(title, rect_words(area_at, area.top, area.left, area.bottom, area.right)),
                                                         {w32(width_variable, 801), w32(height_variable, 599), w8(full_screen_variable, 0), w32(window_variable, 0)}),
                                                  {port_of(frame + 0x3c, 1), {"CarbonLib::GetResource", 1, 0}, {"CarbonLib::SetPt", 1, 0, {w32(frame + 0x38, 0x00a000b0)}},
                                                   {"CarbonLib::PtInRgn", 1, in_region}},
                                                  "window windowed no resource area " + number(std::uint32_t(area.top)) + "," + number(std::uint32_t(area.left)) + " region " + number(in_region)),
                                             {{0x1076fd90, 0}}));
        for (const bool full : {false, true})
            cases.push_back(quiet_dialog(with_answers(make(scenario, window_entry, regs({{3, title_at}, {4, area_at}}),
                                              concat(concat(title, rect_words(area_at, 0, 0, 600, 800)), {w32(width_variable, 800), w32(height_variable, 600), w8(full_screen_variable, full ? 1 : 0), w32(window_variable, 0)}),
                                              {port_of(frame + 0x3c, 1), {"CarbonLib::GetResource", 1, 0}, {"CarbonLib::PtInRgn", 1, 1}, {"CarbonLib::CreateNewWindow", 1, 0xffffffe0}},
                                              std::string("window, CreateNewWindow error ") + (full ? "full screen" : "windowed")),
                                         {{0x10770ee0, 0}, {0x1076fd90, 0}})));
        cases.push_back(with_answers(make(scenario, window_entry, regs({{3, title_at}, {4, area_at}}), {}, {}, "window, the snapshot's state"), {{0x10770ee0, 0}, {0x1076fd90, 0}}));
    }

    // ---- 0x107691b0: closes the window; with the low byte of the argument zero it records the window's position in the resource 'WinP':
    // found (SetPt, LocalToGlobal, ChangedResource, WriteResource), absent (NewHandle, SetPt, LocalToGlobal, AddResource under the name at pool +
    // 0x24, WriteResource; a failed NewHandle writes nothing). The bytes the resource ends with and the order of the calls are checked against
    // the expectation (verify), not only against the original.
    {
        const auto frame = sp - 0x50;
        const std::uint32_t expected_vertical = 0x0123, expected_horizontal = 0x0456;
        const std::vector<Write> position{w16(data_at, 0), w16(data_at + 2, 0)};
        const auto sequence = [](std::initializer_list<const char*> names) { return std::vector<std::string>(names.begin(), names.end()); };
        const auto hash = [](const std::string& name) {
            std::uint32_t value = 2166136261u;
            for (const char ch : name) value = (value ^ std::uint8_t(ch)) * 16777619u;
            return value;
        };
        const auto check = [hash](std::vector<std::string> names, std::uint32_t vertical, std::uint32_t horizontal, bool bytes) {
            return std::function<bool(State&)>([=](State& s) {
                constexpr std::uint32_t log = scratch_base + 0x80000;
                for (std::size_t n = 0; n < names.size(); ++n)
                    if (std::uint32_t(s.memory.read<4>(log + n * 0x80)) != hash("CarbonLib::" + names[n])) return false;
                if (std::uint32_t(s.memory.read<4>(log + names.size() * 0x80)) != 0) return false;
                return !bytes || (std::uint32_t(s.memory.read<2>(data_at)) == vertical && std::uint32_t(s.memory.read<2>(data_at + 2)) == horizontal);
            });
        };
        const Import local{"CarbonLib::LocalToGlobal", 1, 0, {w16(data_at, expected_vertical), w16(data_at + 2, expected_horizontal)}};
        for (const std::uint32_t keep : {0u, 0x100u, 0xffffff00u, 1u, 0xffu, 0x80u, 0x1234u}) {
            const bool records = (keep & 0xff) == 0;
            for (const unsigned kind : {0u, 1u, 2u}) {  // 0 the resource exists, 1 it does not (NewHandle), 2 it does not and NewHandle fails
                if (!records && kind != 0) continue;
                std::vector<Import> first{port_of(frame + 0x38, 1), {"CarbonLib::GetResource", 1, kind == 0 ? handle_at : 0}, {"CarbonLib::NewHandle", 1, kind == 1 ? handle_at : 0}, local,
                                          {"CarbonLib::SetPt", 1, 0, position}};
                auto item = make(scenario, close_window_entry, regs({{3, keep}}), concat(resource(0x7777, 0x6666), {w32(window_variable, window_pointer)}), first,
                                 "close window keep " + number(keep) + " resource " + (kind == 0 ? "found" : kind == 1 ? "created" : "NewHandle fails"));
                if (records && kind == 0)
                    item.verify = check(sequence({"GetPort", "GetWindowPort", "SetPort", "GetResource", "SetPt", "LocalToGlobal", "ChangedResource", "WriteResource", "SetPort", "DisposeWindow"}),
                                        expected_vertical, expected_horizontal, true);
                if (records && kind == 1)
                    item.verify = check(sequence({"GetPort", "GetWindowPort", "SetPort", "GetResource", "NewHandle", "SetPt", "LocalToGlobal", "AddResource", "WriteResource", "SetPort", "DisposeWindow"}),
                                        expected_vertical, expected_horizontal, true);
                if (records && kind == 2)
                    item.verify = check(sequence({"GetPort", "GetWindowPort", "SetPort", "GetResource", "NewHandle", "SetPort", "DisposeWindow"}), 0, 0, false);
                if (!records) item.verify = check(sequence({"GetPort", "GetWindowPort", "SetPort", "SetPort", "DisposeWindow"}), 0, 0, false);
                cases.push_back(item);
            }
        }
        cases.push_back(make(scenario, close_window_entry, regs({{3, 0}}), {}, {}, "close window, the snapshot's state"));
    }

    // ---- 0x107692f0: puts a rectangle on the window (a leaver: its frame is what MAIN's printf reads). Nothing while 0x10768820 runs
    // (initialising); the assertion when it has not finished; the grey conversion first when the switch is set. The corners are exclusive
    // after +1 and truncated to halfwords.
    {
        const auto frame = sp - 0x90;
        // The grey conversion (0x107697c0, on the same page: it runs for real on both sides) works on the snapshot's real 320x258 surface: only
        // rectangles inside it go with the switch set.
        const std::vector<std::array<std::uint32_t, 4>> rectangles{{0, 0, 799, 599}, {10, 20, 30, 40}, {0xffffffff, 0xffffffff, 0xffff, 0x7fff}, {0x7fff, 0x7fff, 0x7fff, 0x7fffffff},
                                                                   {0x10000, 0x18000, 0x1ffff, 0xffff8000}, {5, 6, 1, 2}, {0, 0, 319, 257}};
        const auto inside_surface = [&](const std::array<std::uint32_t, 4>& rectangle) { return &rectangle == &rectangles[1] || &rectangle == &rectangles[6] || &rectangle == &rectangles[5]; };
        for (const std::uint32_t initialising : {0u, 1u, 0x80u, 0xffu})
            for (const std::uint32_t ready : {1u, 0u, 0x80u})
                for (const std::uint32_t grey : {0u, 1u, 0x80u, 0xffu}) {
                    if (initialising != 0 && (ready != 1 || grey != 0)) continue;
                    for (const auto& rectangle : rectangles) {
                        if (grey != 0 && !inside_surface(rectangle)) continue;
                        if ((initialising != 0 || ready != 1 || grey > 1) && &rectangle != &rectangles[0] && &rectangle != &rectangles[2] && !(grey != 0 && &rectangle == &rectangles[6])) continue;
                        auto item = with_answers(make(scenario, flush_entry, regs({{3, rectangle[0]}, {4, rectangle[1]}, {5, rectangle[2]}, {6, rectangle[3]}}),
                                                          {w8(small_at(surface_small::initialising), initialising), w8(small_at(surface_small::ready), ready), w8(small_at(surface_small::grey), grey),
                                                           w32(small_at(surface_small::gworld), mapped(0x61060000)), w32(window_variable, window_pointer)},
                                                          {port_of(frame + 0x38, 1), {"CarbonLib::GetPortBitMapForCopyBits", 1, mapped(0x610b1000)}, {"CarbonLib::GetPortBitMapForCopyBits", 2, mapped(0x610b2000)}},
                                                          "flush initialising " + number(initialising) + " ready " + number(ready) + " grey " + number(grey) + " rect " + number(rectangle[0]) + "," +
                                                              number(rectangle[1]) + "," + number(rectangle[2]) + "," + number(rectangle[3])),
                                                     {});
                        cases.push_back(ready == 0 ? quiet_dialog(item) : item);
                    }
                }
        cases.push_back(make(scenario, flush_entry, regs({{3, 0}, {4, 0}, {5, 799}, {6, 599}}), {}, {}, "flush, the snapshot's state"));
    }

    // ---- 0x10768990: shuts down. Its callees' frames lie below its own, so the imports they make see this routine's depth.
    {
        const auto frame = sp - 0x40;
        for (const std::uint32_t byte : {0u, 1u, 0x80u})
            for (const std::uint32_t pixmap : {0u, mapped(0x61001000)})
                for (const std::uint32_t window : {0u, window_pointer}) {
                    if (window == 0 && (byte != 0 || pixmap != 0)) continue;
                    cases.push_back(with_answers(make(scenario, close_entry, regs({}),
                                                      concat({w8(full_screen_variable, byte), w32(small_at(surface_small::pixmap), pixmap), w32(small_at(surface_small::gworld), mapped(0x61002000)),
                                                              w32(window_variable, window), w32(small_at(surface_small::display_object), byte ? mapped(0x61500000) : 0u),
                                                              w8(small_at(surface_small::ready), 1)},
                                                             {w32(cursors, 0), w8(cursors + 4, 0), w32(cursors + 8, mapped(0x61300100)), w8(cursors + 12, 1), w32(cursors + 16, mapped(0x61300200)), w8(cursors + 20, 0)}),
                                                      {port_any({frame - 0x50 + 0x38, frame - 0x40 - 0x50 + 0x38})},
                                                      "close graphics byte " + number(byte) + " pixmap " + number(pixmap) + " window " + number(window)),
                                                 concat_answers(helpers, {{0x1076c050, 0}})));
                }
        cases.push_back(make(scenario, close_entry, regs({}), {}, {}, "close graphics, the snapshot's state"));
    }

    // ---- 0x10768820: starts the graphics: all the callees in order. The fonts' answer (an assertion when its low byte is zero), the flags'
    // 0x1000 bit (no fill, no flush), the "ready" byte (the position is saved and the window re-made instead of a new surface), the mode.
    {
        const auto frame = sp - 0x90;
        const std::vector<Write> device_record{w32(device_handle_at, device_at), w16(device_at + 0x22, 0x1111), w16(device_at + 0x24, 0x2222), w16(device_at + 0x26, 0x3333),
                                               w16(device_at + 0x28, 0x4444), w32(out_at, 0xa5a5a5a5), w32(out_at + 4, 0x5a5a5a5a)};
        const std::uint32_t title_at = scratch_base + 0x16000;
        for (const std::uint32_t fonts : {1u, 0u, 0x100u, 0xffffff00u, 0x80u})
            for (const std::uint32_t flags : {0u, 0x1000u, 0x2000u, 0xefffu})
                for (const std::uint32_t ready : {0u, 1u})
                    for (const std::uint32_t mode : {0u, 1u, 0x100u})
                        for (const std::uint32_t current : {0u, 1u}) {
                            if (fonts != 1 && (flags != 0 || ready != 0 || mode != 0 || current != 0) && fonts != 0) continue;
                            if (fonts == 0 && (ready != 0 || mode != 0 || current != 0) && flags != 0x1000) continue;
                            auto memory = concat(device_record, {w8(small_at(surface_small::ready), ready), w8(small_at(surface_small::initialising), 0), w8(full_screen_variable, current),
                                                                 w32(small_at(surface_small::display_object), mapped(0x61500000)), w32(small_at(surface_small::device), device_handle_at),
                                                                 w32(window_variable, window_pointer), w8(title_at, 'T'), w8(title_at + 1, 0), w16(scenario.toc(-0x7bd4), 0x7c1f)});
                            std::vector<Import> first{{"CarbonLib::GetWindowBounds", 1, 0}, {"CarbonLib::GetMaxDevice", 1, device_handle_at}};
                            // GetPort's calls in the order the path makes them, each writing the port only at the slot its routine reads it from (a write
                            // anywhere else would land in a live frame: create_window saves r26 at frame - 0x18): 0x107689e0 (frame - 0xa0 + 0x38) when the
                            // mode is switched on, 0x107691b0 (frame - 0x50 + 0x38) when the "ready" byte is set, 0x10768e10 (never read back), 0x107692f0
                            // (frame - 0x90 + 0x38) when the fonts loaded and no fill is asked for.
                            unsigned port_call = 0;
                            if (std::int8_t(current) != std::int8_t(mode) && std::int8_t(mode) != 0) first.push_back(port_of(frame - 0xa0 + 0x38, ++port_call));
                            if (ready != 0) first.push_back(port_of(frame - 0x50 + 0x38, ++port_call));
                            ++port_call;
                            if (std::int8_t(fonts) != 0 && (flags & 0x1000) == 0) first.push_back(port_of(frame - 0x90 + 0x38, ++port_call));
                            cases.push_back(with_answers(make(scenario, start_entry, regs({{3, 800}, {4, 600}, {5, flags}, {6, 0x4321}, {7, mode}, {8, title_at}}), memory, first,
                                                              "start fonts " + number(fonts) + " flags " + number(flags) + " ready " + number(ready) + " mode " + number(mode) + " current " + number(current)),
                                                         concat_answers(helpers, {{0x1076b830, 0}, {0x1076bbf0, fonts}, {0x1076a580, 0}, {0x10770ee0, 0}, {0x1076fd90, 0}})));
                        }
        // The surface's and the window's errors inside the start.
        for (const std::uint32_t gworld_error : {0u, 0x2au})
            for (const std::uint32_t window_error : {0u, 0x2au})
                cases.push_back(quiet_dialog(with_answers(make(scenario, start_entry, regs({{3, 640}, {4, 480}, {5, 0}, {6, 0x4321}, {7, 0}, {8, title_at}}),
                                                  concat(device_record, {w8(small_at(surface_small::ready), 0), w8(small_at(surface_small::initialising), 0), w8(full_screen_variable, 0),
                                                                         w32(small_at(surface_small::display_object), mapped(0x61500000)), w32(small_at(surface_small::device), device_handle_at),
                                                                         w32(window_variable, 0), w32(pixmap_handle_at, pixmap_at), w16(pixmap_at + 4, 0x8640)}),
                                                  {{"CarbonLib::GetResource", 1, 0}, {"CarbonLib::NewGWorld", 1, gworld_error}, {"CarbonLib::CreateNewWindow", 1, window_error}},
                                                  "start NewGWorld error " + number(gworld_error) + " CreateNewWindow error " + number(window_error)),
                                             concat_answers(helpers, {{0x1076b830, 0}, {0x1076bbf0, 1}, {0x1076a580, 0}, {0x10770ee0, 0}, {0x1076fd90, 0}}))));
        cases.push_back(with_answers(make(scenario, start_entry, regs({{3, 800}, {4, 600}, {5, 0}, {6, 1}, {7, 0}, {8, title_at}}), {}, {}, "start, the snapshot's state"),
                                     concat_answers(helpers, {{0x1076b830, 0}, {0x1076bbf0, 1}, {0x1076a580, 0}, {0x10770ee0, 0}, {0x1076fd90, 0}})));
    }
    (void)set16;
}
}
