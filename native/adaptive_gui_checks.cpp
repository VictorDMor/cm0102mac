#include "adaptive_gui.hpp"
#include "guest_abi.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include "platform.hpp"
#include "tactics_layout.hpp"
#include "match_replay.hpp"
#include "code_fragment_adapter.hpp"
#include "routine_check.hpp"
#include "new_screens.hpp"
#include "debug_localization.hpp"
#include "match_debugging.hpp"
#include "debug_match_results.hpp"
#include "preferences_actions.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>

using namespace cm::aot;
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
struct Temporary {
    std::filesystem::path path;
    Temporary() {
        auto pattern = (std::filesystem::temp_directory_path() / "cm-adaptive-XXXXXX").string();
        require(mkdtemp(pattern.data()), "Temporary directory"); path = pattern;
        std::filesystem::create_directory(path / "game");
    }
    ~Temporary() { std::error_code e; std::filesystem::remove_all(path, e); }
};
std::uint32_t imported(State& s, const char* name, std::initializer_list<unsigned> values) {
    unsigned n = 3; for (auto v : values) s.wr<4>(n++ * 4, v);
    s.wr<4>(4128, return_sentinel);
    require(s.platform->dispatch(s, name), "Import missing");
    return s.r<4>(12);
}
void world(State& s, unsigned rect, unsigned out) {
    guest::Cpu c(s);
    c.put16(rect, 0); c.put16(rect + 2, 0); c.put16(rect + 4, 600); c.put16(rect + 6, 800);
    require(imported(s, "CarbonLib::NewGWorld", {out, 16, rect, 0, 0, 0}) == 0, "Canvas allocation");
    const auto token = c.u32(out);
    c.put32(c.r(2) - 0x355c, token);
    c.put32(c.r(2) - 0x3560, imported(s, "CarbonLib::GetPortPixMap", {token}));
}
void instruction(State& s, unsigned address) {
    s.pc = address; s.budget = s.executed + 1;
    try { require(dispatch_page(s), "Instruction missing"); }
    catch (const std::runtime_error& error) {
        if (std::string_view(error.what()) != "Native instruction budget exhausted") throw;
    }
    require(s.pc == address + 4, "Instruction boundary did not advance exactly once");
}
void checks() {
    Temporary temp;
    auto state = std::make_unique<State>(); auto& s = *state; guest::Cpu c(s);
    s.platform = std::make_shared<NativePlatform>(temp.path / "game", temp.path / "user", AudioMode::offline);
    constexpr unsigned base = 0x70000000, toc = base + 0x10000;
    s.memory.map(base, 0x20000); c.set(2, toc); c.set(1, base + 0x1e000);
    c.put32(toc - 0x5af8, base + 0x100);
    c.put32(toc - 0x746c, base + 0x104); c.put32(toc - 0x7470, base + 0x108);
    c.put32(toc - 0x3bc0, base + 0x110);
    world(s, base + 0x200, base + 0x210);
    const auto handle = c.u32(toc - 0x3560), map = c.u32(handle);
    const auto unrelated = s.memory.copy_out(base + 0x300, 256);
    for (const auto canvas : {GuiCanvas{800, 600}, GuiCanvas{1280, 900}, GuiCanvas{801, 601}, GuiCanvas{3200, 1800}, GuiCanvas{800, 600}}) {
        const auto previous = c.u32(map);
        const auto changed = canvas != gui_canvas(s);
        require(s.platform->resize_gui_canvas(s, canvas), "Resize failed");
        require(gui_canvas(s) == canvas && c.u32(toc - 0x3560) == handle && c.u32(handle) == map,
                "Resize replaced stable port/PixMap identity");
        const auto stride = (canvas.width * 2 + 3) & ~3;
        require(c.u16(map + 4) == (stride | 0x8000) && c.u16(map + 10) == canvas.height && c.u16(map + 12) == canvas.width,
                "PixMap dimensions/stride inconsistent");
        if (changed) {
            require(!s.allocated.contains(previous), "Resize leaked old pixels");
            require(c.u32(toc - 0x3564) == c.u32(map) && c.u16(toc - 0x3566) == stride && c.u16(toc - 0x3568) == stride / 2,
                    "Drawing globals retained stale buffer/stride");
            require(c.i32(base + 0x104) == canvas.width && c.i32(base + 0x108) == canvas.height &&
                    c.u16(base + 0x114) == canvas.height && c.u16(base + 0x116) == canvas.width, "Input/drawing extents disagree");
        }
        c.put16(c.u32(map) + stride * (canvas.height - 1) + (canvas.width - 1) * 2, 0x7fff);
        require(s.memory.copy_out(base + 0x300, 256) == unrelated, "Resize touched unrelated memory");
        struct Li {unsigned pc, reg; bool bottom; int inset;};
        const Li loads[]{{0x10718f8c,5,0,1},{0x10718f90,6,1,1},{0x10718fa0,5,0,1},{0x10718fa4,6,1,1},
            {0x10719074,5,0,1},{0x10719078,6,1,1},{0x1071929c,5,0,1},{0x107192a0,6,1,1},
            {0x107192c8,5,0,1},{0x107192cc,6,1,1},{0x10719318,5,0,1},{0x1071931c,6,1,1},
            {0x10725b10,0,0,1},{0x10725b28,0,1,1},{0x1076add4,3,0,0},{0x1076ae34,4,0,0},
            {0x10713e28,0,0,1},{0x10713e40,0,1,1},
            {0x1074b0c4,5,1,45},{0x1074b0cc,7,1,10},{0x1074b100,6,0,10},{0x1001f064,0,1,55},
            {0x1031bdcc,3,1,55}};
        for (const auto& item : loads) {
            const auto registers = s.registers;
            c.set(item.reg, (item.bottom ? canvas.height : canvas.width) - item.inset);
            const auto expected = s.registers; s.registers = registers;
            instruction(s, item.pc); require(s.registers == expected, "Canvas constant clobbered registers");
        }
        for (const auto [pc, source, target] : {std::array{0x100240d0u,3u,16u}, std::array{0x10024190u,8u,16u},
                                               std::array{0x10029b4cu,0u,22u}, std::array{0x1002e48cu,3u,10u},
                                               std::array{0x1031d200u,5u,0u}})
            for (unsigned rows : {0u, 1u, 29u, 30u, 31u, 255u, 256u}) {
                c.set(source, rows); const auto registers = s.registers;
                c.set(target, canvas.original() ? rows & 255u : std::min(pc == 0x1031d200 ? 31u : 30u, rows & 255u));
                const auto expected = s.registers; s.registers = registers;
                instruction(s, pc); require(s.registers == expected, "Adaptive table capacity changed flags or other registers");
            }
        struct Compare { unsigned pc, reg; int limit; };
        const Compare comparisons[]{
            {0x10725b04,0,canvas.width-1},{0x1071b074,0,canvas.width-1},
            {0x10725b1c,0,canvas.height-1},{0x1071b0a0,0,canvas.height-1},
            {0x10713e1c,0,canvas.width-1},{0x10713e34,0,canvas.height-1},
            {0x10714f88,0,canvas.width-1},{0x10715054,0,canvas.height-1},
            {0x10714d00,0,canvas.height},{0x10714dbc,0,canvas.height},{0x10714e34,0,canvas.height},
            {0x10714df4,0,canvas.width/2},{0x10714e84,6,canvas.width/2}};
        for (const auto& [pc, reg, limit] : comparisons) {
            for (int value : {-1, limit - 1, limit, limit + 1}) for (unsigned so : {0u, 1u}) {
                c.set(reg, value); s.wr<1>(guest::reg::xer_so, so);
                const auto registers = s.registers;
                c.set_cr(0, (value < limit ? 8 : value == limit ? 2 : 4) | so);
                const auto expected = s.registers; s.registers = registers;
                instruction(s, pc); require(s.registers == expected, "Canvas compare lost signedness or XER.SO");
            }
        }
        struct Subtract { unsigned pc, from, to, limit; };
        const Subtract subtractions[]{
            {0x1071b07c,4,27,unsigned(canvas.width-1)},{0x1071b0a8,5,30,unsigned(canvas.height-1)},
            {0x10769c40,4,3,unsigned(canvas.width)},
            {0x10714fe4,0,28,unsigned(canvas.width-1)},{0x107150b0,0,27,unsigned(canvas.height-1)}};
        for (const auto& [pc, from, to, limit] : subtractions) {
            for (unsigned value : {0u, limit, limit + 1, 0xffffffffu}) {
                c.set(from, value); const auto registers = s.registers;
                c.set(to, limit - value); s.wr<1>(guest::reg::xer_ca, value <= limit);
                const auto expected = s.registers; s.registers = registers;
                instruction(s, pc); require(s.registers == expected, "Canvas subtraction lost carry/borrow");
            }
        }
        const TacticsLayout layout{canvas};
        int columns_width = 0;
        for (const auto view : layout.column_views()) if (view) columns_width += TacticsLayout::column_width(view);
        require(layout.list_right() >= 440 && layout.list_right() + columns_width + 10 == layout.pitch_left() &&
                layout.pitch_right() == canvas.width - 20 && layout.list_bottom() == canvas.height - 55,
                "Roster columns overlap the pitch or change original margins");
        require(std::abs(layout.pitch_width() * 400 - (layout.pitch_bottom() - 145) * 330) < 330 &&
                layout.pitch_bottom() <= layout.list_bottom(), "Pitch lost its original aspect ratio");
        const ScreenLayout shell{canvas};
        require(shell.title() == ScreenRect{100,10,canvas.width-10,70} &&
                shell.content() == ScreenRect{110,145,canvas.width-20,canvas.height-55} &&
                shell.footer() == ScreenRect{100,canvas.height-45,canvas.width-10,canvas.height-10},
                "Shared screen structure changed its original margins");
        const ScreenRect dialog{250,200,550,400};
        const auto centered = shell.anchor(dialog, LayoutAxis::centered, LayoutAxis::centered);
        require(centered.width() == dialog.width() && centered.height() == dialog.height() &&
                centered.left == dialog.left + (canvas.width-800)/2 && centered.top == dialog.top + (canvas.height-600)/2,
                "Center anchoring stretched a fixed-size component");
    }
    // Narrow and tall windows, including the points where an extra column
    // appears. In-match views must not reserve space for unavailable data.
    for (int width = 800; width <= 3200; width += 7) for (int height : {600, 680, 900, 1800})
        for (unsigned selected : {1u, 5u, 6u, 7u}) for (bool match : {false, true}) {
            const TacticsLayout layout{{width, height}, selected, match};
            int right = layout.list_right();
            for (const auto view : layout.column_views()) if (view) {
                require(view != selected && (!match || view == 1), "Supplementary column is duplicate or unavailable");
                right += TacticsLayout::column_width(view);
            }
            require(right == layout.pitch_left() - 10, "Empty strip reserved for unavailable columns");
            require(!layout.columns() || layout.list_right() - 110 >= 400, "Supplementary data squeezed the primary list");
            require(layout.pitch_left() >= 450 && layout.pitch_bottom() <= height - 55,
                    "Pitch overlaps the squad or footer");
            require(!match || selected != 1 || !layout.columns(), "Position match view added unavailable columns");
        }
    require(bounded_gui_canvas(-1, 0) == GuiCanvas{800,600} && bounded_gui_canvas(99999,99999) == GuiCanvas{3200,1800},
            "Canvas exceeded PixMap bounds");
    require(s.platform->resize_gui_canvas(s, {801, 601}), "Background fixture resize");
    const auto display = allocate(s, 0x12aa00, true), image = allocate(s, 16, true), source = allocate(s, 400, true);
    c.put32(display + 0x12a928, image); c.put32(image, 20); c.put32(image + 4, 10); c.put32(image + 12, source);
    for (unsigned i = 0; i < 200; ++i) c.put16(source + i * 2, 0x1234);
    const auto destination = c.u32(map);
    const auto stride = c.u16(map + 4) & 0x3fff;
    for (unsigned y = 0; y < 601; ++y) c.put16(destination + y * stride + 1602, 0x7abc);
    c.set(24, display); draw_adaptive_background(s);
    for (unsigned y = 0; y < 601; ++y)
        require(c.u16(destination + y * stride) == 0x1234 && c.u16(destination + y * stride + 1600) == 0x1234 &&
                c.u16(destination + y * stride + 1602) == 0x7abc, "Background missed edges or overwrote padding");
    require(c.u32(image) == 20 && c.u32(image + 4) == 10 && c.u16(source + 398) == 0x1234,
            "Background changed source dimensions/pixels");
    const auto allocated = s.allocated, available = s.available;
    const auto heap_end = s.heap_end;
    s.available.clear(); s.heap_end = 0x30000000;
    require(!s.platform->resize_gui_canvas(s, {1600,1000}) && gui_canvas(s) == GuiCanvas{801,601} &&
            c.u32(map) == destination && s.allocated == allocated, "Failed resize partially changed the active canvas");
    s.available = available; s.heap_end = heap_end;
    // Negative control: the unadapted original comparison must reject a
    // point that the wide canvas admits.
    require(s.platform->resize_gui_canvas(s, {1280,900}), "Clamp negative-control resize");
    c.set(0, 900); instruction(s, 0x10725b04); const auto wide_comparison = c.cr(0);
    const auto platform = s.platform; s.platform.reset();
    instruction(s, 0x10725b04); require(c.cr(0) != wide_comparison, "Clamp checker did not distinguish the original limit");
    s.platform = platform;
    c.set(0, 900); instruction(s, 0x10713e1c); const auto wide_grid = c.cr(0);
    s.platform.reset(); instruction(s, 0x10713e1c);
    require(c.cr(0) != wide_grid, "Grid checker did not distinguish the original 799 clamp");
    s.platform = platform;
}
struct Clock final : CodeFragmentBoundaryObserver {
    std::uint64_t microseconds = 1000000000ULL, step_us = 0;
    bool idle_input = false;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    bool import_fixture(State& s, const char* name) override {
        if (std::string_view(name) == "CarbonLib::GetDateTime") s.memory.write<4>(s.r<4>(12), 3000000000u);
        // Calendar construction pumps events while collecting fixtures. There
        // is no live window or input in this controlled-state check.
        else if (idle_input && std::string_view(name) == "CarbonLib::GetMouse") s.memory.write<4>(s.r<4>(12), 0);
        else if (idle_input && std::string_view(name) == "CarbonLib::WaitNextEvent") {
            s.memory.clear(s.r<4>(16), 16); s.wr<4>(12, 0);
        }
        else if (std::string_view(name) == "CarbonLib::Microseconds") {
            s.memory.write<8>(s.r<4>(12), microseconds);
            microseconds += step_us;
        }
        else if (!idle_input && std::string_view(name) == "CarbonLib::FSMakeFSSpec") s.wr<4>(12, std::uint32_t(-43));
        // The captured window does not exist in this headless fixture. Button
        // presses still draw into the real guest canvas; only the host blit is omitted.
        else if (std::string_view(name) == "CarbonLib::GetPort") s.memory.write<4>(s.r<4>(12), 0);
        else if (std::string_view(name) == "CarbonLib::SetPort" || std::string_view(name) == "CarbonLib::GetWindowPort" ||
                 std::string_view(name) == "CarbonLib::GetPortBitMapForCopyBits" || std::string_view(name) == "CarbonLib::CopyBits" ||
                 std::string_view(name) == "CarbonLib::QDFlushPortBuffer") s.wr<4>(12, 0);
        else return false;
        s.pc = s.r<4>(4128); return true;
    }
};
std::vector<std::uint8_t> random_state(const State& s) {
    auto result = s.memory.copy_out(0x108b3f00, 16);
    for (auto global : {0x108b3f00u, 0x108b3f08u, 0x108b3f0cu}) {
        const auto bytes = s.memory.copy_out(s.memory.read<4>(global), 4);
        result.insert(result.end(), bytes.begin(), bytes.end());
    }
    const auto samples = s.memory.copy_out(s.memory.read<4>(0x108b3f04), 102000);
    result.insert(result.end(), samples.begin(), samples.end()); return result;
}
void builder(const char* path) {
    Temporary temp; Clock clock;
    auto state = std::make_unique<State>(); auto& s = *state;
    load_match_snapshot(s, path); s.code_fragment_observer = &clock;
    guest::Cpu c(s); c.set(2, c.u32(c.sp() + 0x14));
    const auto manager = c.toc(-0x7bb0); c.put16(manager + 0x2c58, 0);
    // This existing capture begins on the club's squad page.
    const auto club = c.u32(c.u32(manager + 0x10) + 0x14);
    const auto invoke = [&](unsigned entry, std::initializer_list<unsigned> values) {
        unsigned n = 3; for (auto v : values) c.set(n++, v);
        s.budget = s.executed + 400000000;
        try { c.call(entry, return_sentinel); }
        catch (const std::exception& e) {
            throw std::runtime_error("Tactics fixture " + hex_address(entry) + " at " + hex_address(s.pc) + ": " + e.what());
        }
        return c.r(3);
    };
    invoke(0x103b3830, {club, 0, 0, 0, 0xffffffffu, 0});
    const auto history = c.u32(manager + 0x10), tactics = c.u32(history + 0x14), display = c.u32(manager + 0x24);
    require(tactics && c.u32(tactics) == club, "Tactics fixture did not open the expected club");
    c.put32(history + 0x14 + 10 * 8, 5); // Avg Rating, so extra Position must be read-only
    // Opening the original screen can lazily refresh status. Establish that
    // original baseline before checking redraws at different canvas sizes.
    invoke(0x10719560, {display, 1, 0});
    invoke(0x103b3c50, {});
    std::vector<std::pair<unsigned, std::vector<std::uint8_t>>> players;
    for (unsigned i = 0; i < 20; ++i) {
        const auto staff = invoke(0x103ca4e0, {tactics, i, 0});
        if (!staff || !c.u32(staff + 0x14)) continue;
        const auto player = c.u32(staff + 0x14);
        // The rare original formatter repair must never affect the real record.
        for (unsigned role = 0xf; role <= 0x1a; ++role) c.put8(player + role, 0);
        c.put8(player + 0x14, 15);
        players.emplace_back(player, s.memory.copy_out(player, 0x20));
    }
    require(!players.empty(), "Position mutation fixture has no players");
    const auto random = random_state(s), roster = s.memory.copy_out(tactics, 0xc6b4);
    const auto table = c.u32(c.toc(-0x7a6c)), teams = c.u32(table + 0x5c4);
    const auto team_bytes = s.memory.copy_out(teams, c.u32(table + 0x5c8) * 0x94);
    const auto saved_club = s.memory.copy_out(club, 0x248);
    s.platform = std::make_shared<NativePlatform>(temp.path / "game", temp.path / "user", AudioMode::offline);
    const auto scratch = allocate(s, 32, true);
    world(s, scratch, scratch + 16);
    unsigned original_count = 0;
    for (const auto canvas : {GuiCanvas{800,600}, GuiCanvas{801,601}, GuiCanvas{1200,680}, GuiCanvas{1280,900},
                              GuiCanvas{1600,1000}, GuiCanvas{1200,900}, GuiCanvas{800,600}}) {
        const TacticsLayout layout{canvas, 5};
        require(s.platform->resize_gui_canvas(s, canvas), "Fixture resize failed");
        invoke(0x10719560, {display, 1, 0});
        invoke(0x103b3c50, {});
        bool backdrop = false, primary_list = false;
        for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
            const auto grid = display + 4 + i * 0xbf0;
            require(c.i32(grid) < c.i32(grid + 8) && c.i32(grid + 4) < c.i32(grid + 12),
                    "Grid was inverted or collapsed by the legacy canvas clamp");
            invoke(0x10714630, {grid});
            invoke(0x10714110, {grid, 0});
            if (c.i32(grid) == 110 && c.i32(grid + 4) == 145 &&
                c.i32(grid + 8) == layout.pitch_left() - 10 && c.i32(grid + 12) == layout.list_bottom()) backdrop = true;
            if (!canvas.original() && c.i32(grid) >= 110 && c.i32(grid + 8) <= layout.pitch_left() - 10 &&
                c.i32(grid + 4) >= 145) {
                require(c.i32(grid + 4) == 145 && c.i32(grid + 12) == layout.list_bottom(),
                        "Squad columns have different row origins or heights");
                if (c.i32(grid) == 110 && c.i32(grid + 8) == layout.list_right()) primary_list = true;
            }
        }
        require(backdrop && (canvas.original() || primary_list), "Squad backdrop or primary list misses the panel bounds");
        const auto count = c.u16(display + 0x129da6);
        if (!original_count) original_count = count;
        require(!layout.columns() ? count == original_count : count > original_count,
                "Extra width did not reveal additional controls or shrink retained them");
        bool pitch = false, title = false, cancel = false, ok = false;
        int cancel_width = 0, ok_width = 0;
        for (unsigned i = 0; i < count; ++i) {
            const auto control = display + 0xba864 + i * 0x17c;
            require(c.i32(control + 0x18) < canvas.width && c.i32(control + 0x1c) < canvas.height,
                    "Control escaped the active canvas");
            if (c.u32(control + 8) == 0x1c) {
                require(c.i32(control + 0x10) > layout.pitch_left() &&
                        c.i32(control + 0x18) > canvas.width - 100, "Pitch hit target retained legacy bounds");
                pitch = true;
            }
            if (c.i32(control + 0x10) == 100 && c.i32(control + 0x14) == 10) {
                require(c.i32(control + 0x18) == canvas.width - 10, "Title retained legacy bounds"); title = true;
            }
            const auto action = c.u32(control + 8);
            if (action == 0x20 || action == 0x21) {
                require(c.i32(control + 0x14) == canvas.height - 45 && c.i32(control + 0x1c) == canvas.height - 10,
                        "Footer buttons retained the legacy bottom clamp");
                require(c.i32(control + 0x10) >= layout.footer_left() &&
                        c.i32(control + 0x10) < c.i32(control + 0x18), "Footer button has no visible width");
                if (action == 0x20) { cancel = true; cancel_width = c.i32(control + 0x18) - c.i32(control + 0x10); } else {
                    ok = true;
                    ok_width = c.i32(control + 0x18) - c.i32(control + 0x10);
                    require(c.i32(control + 0x18) == layout.footer_right(), "Ok button did not reach the right margin");
                }
                const auto pointer_x = c.u32(c.toc(-0x5c10)), pointer_y = c.u32(c.toc(-0x5c14));
                const auto primary = c.u8(c.toc(-0x7bac)), secondary = c.u8(c.toc(-0x5c0c));
                c.put32(c.toc(-0x5c10), (c.u32(control + 0x10) + c.u32(control + 0x18)) / 2);
                c.put32(c.toc(-0x5c14), (c.u32(control + 0x14) + c.u32(control + 0x1c)) / 2);
                c.put8(c.toc(-0x7bac), 1); c.put8(c.toc(-0x5c0c), 0);
                require(std::int16_t(invoke(0x1071a280, {display})) == int(i), "Footer click missed its visible button");
                c.put32(c.toc(-0x5c10), pointer_x); c.put32(c.toc(-0x5c14), pointer_y);
                c.put8(c.toc(-0x7bac), primary); c.put8(c.toc(-0x5c0c), secondary);
            }
        }
        require(pitch && title && cancel && ok, "Missing original pitch/title/footer controls");
        require(canvas.original() || (std::abs(cancel_width - ok_width) <= 1 && ok_width >= 140),
                "Adaptive footer buttons are unequal or too narrow");
        if (random_state(s) != random) throw std::runtime_error("Presentation changed RNG at " + std::to_string(canvas.width));
        const auto after = s.memory.copy_out(tactics, 0xc6b4);
        if (after != roster) {
            for (unsigned i = 0; i < after.size(); ++i) if (after[i] != roster[i])
                std::cerr << "Tactics difference at " << hex_address(i) << " in " << canvas.width << 'x' << canvas.height
                          << ": " << unsigned(roster[i]) << " -> " << unsigned(after[i]) << '\n';
            throw std::runtime_error("Presentation changed tactics state");
        }
        require(s.memory.copy_out(club, 0x248) == saved_club, "Presentation changed club state");
        require(s.memory.copy_out(teams, team_bytes.size()) == team_bytes, "Presentation changed a saved team roster");
        for (const auto& [address, bytes] : players)
            require(s.memory.copy_out(address, bytes.size()) == bytes, "Additional position formatter changed player roles");
    }
    // Instruction views keep their extra heading row; supplemental values
    // must still line up with the same player's row and remain read-only.
    c.put32(history + 0x14 + 10 * 8, 7);
    require(s.platform->resize_gui_canvas(s, {1200,680}), "Instruction view resize");
    invoke(0x10719560, {display, 1, 0}); invoke(0x103b3c50, {});
    for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
        const auto grid = display + 4 + i * 0xbf0;
        invoke(0x10714630, {grid}); invoke(0x10714110, {grid, 0});
        if (c.i32(grid) >= 110 && c.i32(grid + 8) <= TacticsLayout{{1200,680},7}.pitch_left() - 10 && c.i32(grid + 4) >= 145)
            require(c.i32(grid + 4) == 145 && c.i32(grid + 12) == 625, "Instruction view lost row alignment");
    }
    require(random_state(s) == random && s.memory.copy_out(tactics, 0xc6b4) == roster &&
            s.memory.copy_out(teams, team_bytes.size()) == team_bytes && s.memory.copy_out(club, 0x248) == saved_club,
            "Instruction view changed RNG or saved records");
    for (const auto& [address, bytes] : players)
        require(s.memory.copy_out(address, bytes.size()) == bytes, "Instruction view changed player roles");
    c.put32(history + 0x14 + 10 * 8, 5);
    c.put8(tactics + 0xec0, 1);
    require(s.platform->resize_gui_canvas(s, {1280,900}), "Read-only resize");
    invoke(0x10719560, {display, 1, 0}); invoke(0x103baec0, {});
    for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
        invoke(0x10714630, {display + 4 + i * 0xbf0});
        invoke(0x10714110, {display + 4 + i * 0xbf0, 0});
    }
    unsigned footer = 0;
    for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
        const auto control = display + 0xba864 + i * 0x17c;
        if (c.i32(control + 0x14) >= 855 && c.i32(control + 0x1c) <= 890) ++footer;
    }
    require(footer >= 2, "Read-only history buttons overlap the expanded roster");
    require(random_state(s) == random && s.memory.copy_out(teams, team_bytes.size()) == team_bytes,
            "Read-only adaptive screen changed RNG or saved rosters");
}
void club_builder(const char* path, unsigned section) {
    Temporary temp; Clock clock; clock.idle_input = true;
    auto state = std::make_unique<State>(); auto& s = *state;
    load_match_snapshot(s, path); s.code_fragment_observer = &clock;
    guest::Cpu c(s); c.set(2, c.u32(c.sp() + 0x14));
    const auto manager = c.toc(-0x7bb0); c.put16(manager + 0x2c58, 0);
    const auto history = c.u32(manager + 0x10), display = c.u32(manager + 0x24);
    const auto club = c.u32(history + 0x14);
    const auto setting = [&](unsigned slot) { return history + 0x14 + slot * 8; };
    c.put32(setting(4), section);
    const auto view_slot = section == 1 ? 7u : section == 2 ? 18u : 14u;
    const auto scroll_slot = section == 1 ? 10u : section == 2 ? 19u : 16u;
    const auto grid_slot = section == 1 ? 11u : section == 2 ? 20u : 17u;
    require(c.u32(c.u32(history)) == 0x1001e2b0, "Roster fixture is not a club page");
    const auto invoke = [&](unsigned entry, std::initializer_list<unsigned> values) {
        unsigned n = 3; for (auto v : values) c.set(n++, v);
        s.budget = s.executed + 400000000;
        try { c.call(entry, return_sentinel); }
        catch (const std::exception& error) {
            throw std::runtime_error("Club section " + std::to_string(section) + " view " + hex_address(c.u32(setting(view_slot))) +
                " entry " + hex_address(entry) + " at " + hex_address(s.pc) + ": " + error.what());
        }
        return c.r(3);
    };
    bool explicit_cell_fixture = false;
    unsigned explicit_cell = 0;
    const auto build = [&] {
        invoke(0x10719560, {display, 1, 0});
        invoke(0x1001e2b0, {});
        const auto grid = display + 4 + c.u32(setting(grid_slot)) * 0xbf0;
        if (explicit_cell_fixture) {
            // The captured calendar has official fixtures but no cancellable
            // friendlies/tours. Exercise their audited explicit-X cell form
            // on an existing row without inventing any saved event record.
            explicit_cell = 0;
            for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
                const auto at = display + 0xba864 + i * 0x17c;
                if (c.i16(at + 0x72) != int(c.u32(setting(grid_slot))) || c.u32(at + 0x24) != 2) continue;
                c.put32(at + 0x28, 110); c.put32(at + 0x2c, 758);
                explicit_cell = at; break;
            }
            require(explicit_cell, "Calendar explicit-cell fixture has no row");
        }
        const auto scroll = c.u32(grid + 0x1fc);
        const auto registers = s.registers;
        const auto unique = s.unique; const auto pc = s.pc, stack_base = c.sp() - 0x8000;
        const auto stack = s.memory.copy_out(stack_base, 0x8100);
        layout_built_screen(s, display, 0x1001e2b0);
        require(s.registers == registers && s.unique == unique && s.pc == pc &&
                s.memory.copy_out(stack_base, stack.size()) == stack,
                "Shared table layout clobbered the caller's registers or stack");
        require(c.u32(grid + 0x1fc) == scroll, "Shared table layout changed the builder's clamped scroll position");
        for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
            const auto grid = display + 4 + i * 0xbf0;
            invoke(0x10714630, {grid}); invoke(0x10714110, {grid, 0});
        }
    };
    // The original calendar and historical-transfer queries rotate year
    // caches through real files. Keep that I/O in this disposable backing.
    std::filesystem::create_directories(temp.path / "user" / "Files");
    s.platform = std::make_shared<NativePlatform>(temp.path / "game", temp.path / "user", AudioMode::offline);
    const auto scratch = allocate(s, 32, true); world(s, scratch, scratch + 16);
    initialize_new_screens(temp.path / "Screens.plist");
    c.put32(setting(27),1); // public General Info in section 5
    for (unsigned candidate : {1u, 2u, 3u, 4u, 5u}) {
        c.put32(setting(4), candidate);
        require(adaptive_screen_available(s, 0x1001e2b0) == (candidate >= 1 && candidate <= 5),
                "Other club section entered adaptive table layout");
    }
    c.put32(setting(4), section);
    const auto network = c.u32(manager + 0x2c64);
    c.put32(manager + 0x2c64, 1);
    require(!adaptive_screen_available(s, 0x1001e2b0), "Network squad entered adaptive layout");
    c.put32(manager + 0x2c64, network);
    select_new_screens(false);
    require(!adaptive_screen_available(s, 0x1001e2b0), "Disabled New Screens still adapts squad");
    select_new_screens(true);

    const auto actions = [&] {
        std::vector<std::pair<unsigned, unsigned>> result;
        for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
            const auto control = display + 0xba864 + i * 0x17c;
            result.emplace_back(c.u32(control + 8), c.u32(control + 0x48));
        }
        return result;
    };

    const auto views = section == 1 ? std::vector<unsigned>{1,2,0x1000,4,8,0x50,0x20} :
                       section == 2 ? std::vector<unsigned>{1,2,4,16,32,64,128} : std::vector<unsigned>{0,1};
    const auto year = c.u32(setting(23));
    const auto years = section == 2 ? std::vector<unsigned>{year,year-1} : std::vector<unsigned>{year};
    for (unsigned season : years) for (unsigned view : views) {
        if (section == 2) c.put32(setting(23), season);
        c.put32(setting(view_slot), view); c.put32(setting(scroll_slot), 2);
        if (section == 1) c.put32(setting(8), view == 1 ? 4 : 8); // Name / Position.
        const auto sort = c.u32(setting(8)), filter = c.u32(setting(24));
        require(s.platform->resize_gui_canvas(s, {800,600}), "Roster baseline resize");
        build(); // Warm the original formatters/medical status for this view.
        const auto baseline_grid = display + 4 + c.u32(setting(grid_slot)) * 0xbf0;
        const auto original_rows = c.u8(baseline_grid + 0xbb1);
        const auto original_actions = actions();
        const auto saved_club = s.memory.copy_out(club, 0x248), random = random_state(s);
        std::vector<std::pair<unsigned, std::vector<std::uint8_t>>> records;
        for (unsigned slot = 0; slot < 50; ++slot) {
            const auto staff = c.u32(club + 0x98 + slot * 4);
            if (!staff) continue;
            records.emplace_back(staff, s.memory.copy_out(staff, 0x70));
            if (const auto player = c.u32(staff + 0x14)) records.emplace_back(player, s.memory.copy_out(player, 0x46));
        }
        require(!records.empty(), "Roster fixture has no player records");
        for (const auto canvas : {GuiCanvas{1200,680}, GuiCanvas{1280,900}, GuiCanvas{3200,1800},
                                  GuiCanvas{822,680}, GuiCanvas{801,601}, GuiCanvas{800,600}}) {
            require(s.platform->resize_gui_canvas(s, canvas), "Roster canvas resize");
            build();
            require(actions() == original_actions, "Roster resize changed player/control actions or their order");
            require(c.u32(setting(8)) == sort && c.u32(setting(24)) == filter, "Roster resize changed sort/filter settings");
            const auto grid_index = c.u32(setting(grid_slot));
            require(grid_index < c.u16(display + 0x129da4), "Roster lost its active grid");
            const auto grid = display + 4 + grid_index * 0xbf0;
            require(c.i32(grid) == 110 && c.i32(grid + 8) == ScreenLayout{canvas}.content().right,
                    "Roster table does not fill shared content width");
            require(c.u8(grid + 0xbb1) <= 30 && c.u8(grid + 0xbb1) >= original_rows,
                    "Roster visible rows overflow AutoGrid or shrink on a larger canvas");
            if (canvas.height >= 900) require(c.u8(grid + 0xbb1) > original_rows, "Taller roster did not reveal more rows");
            bool print = false, calendar_cancel = false, transfer_menu = false;
            for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
                const auto control = display + 0xba864 + i * 0x17c;
                if (section == 4 && c.i16(control + 0x72) == int(grid_index) && c.i32(control + 0x28) == 110) {
                    require(c.i32(control + 0x2c) == canvas.width - 42 && c.i32(control + 0x18) == canvas.width - 42,
                            "Calendar row action retained its original explicit hit width");
                }
                if (section == 4 && c.u32(control + 8) == 25) {
                    require(c.i32(control + 0x10) == 0 && c.i32(control + 0x14) == 0 &&
                            c.i32(control + 0x18) == canvas.width - 1 && c.i32(control + 0x1c) == canvas.height - 1,
                            "Calendar cancellation mask did not cover the canvas");
                    calendar_cancel = true;
                    const auto x = c.u32(c.toc(-0x5c10)), y = c.u32(c.toc(-0x5c14));
                    const auto primary = c.u8(c.toc(-0x7bac)), secondary = c.u8(c.toc(-0x5c0c));
                    c.put32(c.toc(-0x5c10), canvas.width - 2); c.put32(c.toc(-0x5c14), canvas.height - 2);
                    c.put8(c.toc(-0x7bac), 1); c.put8(c.toc(-0x5c0c), 0);
                    require(std::int16_t(invoke(0x1071a280, {display})) == int(i),
                            "Calendar cancellation click missed the expanded corner");
                    c.put32(c.toc(-0x5c10), x); c.put32(c.toc(-0x5c14), y);
                    c.put8(c.toc(-0x7bac), primary); c.put8(c.toc(-0x5c0c), secondary);
                }
                if (section == 2 && c.i16(control + 0x72) == -1 && c.i32(control + 0x14) == 125 &&
                    c.i32(control + 0x1c) == 145 && c.i32(control + 0x10) == canvas.width - 145 &&
                    c.i32(control + 0x18) == canvas.width - 20) transfer_menu = true;
                if (c.i32(control + 0x14) != 15 || c.i32(control + 0x1c) != 35 ||
                    c.i32(control + 0x10) != canvas.width - 90 || c.i32(control + 0x18) != canvas.width - 15) continue;
                print = true;
                const auto x = c.u32(c.toc(-0x5c10)), y = c.u32(c.toc(-0x5c14));
                const auto primary = c.u8(c.toc(-0x7bac)), secondary = c.u8(c.toc(-0x5c0c));
                c.put32(c.toc(-0x5c10), canvas.width - 50); c.put32(c.toc(-0x5c14), 30);
                c.put8(c.toc(-0x7bac), 1); c.put8(c.toc(-0x5c0c), 0);
                const auto clicked = std::int16_t(invoke(0x1071a280, {display}));
                // Print opens a popup: the original intentionally returns -1
                // and records the popup area instead of dispatching an action.
                require(clicked == -1 && c.u32(control + 8) == 0 && c.i16(control + 0x74) >= 0 &&
                        c.u8(display) == 1 && c.i16(display + 0x12a930) == c.i16(control + 0x74),
                        "Roster header click did not open its displayed menu");
                c.put32(c.toc(-0x5c10), x); c.put32(c.toc(-0x5c14), y);
                c.put8(c.toc(-0x7bac), primary); c.put8(c.toc(-0x5c0c), secondary);
            }
            require(print, "Roster Print control did not follow its right anchor");
            require(section != 4 || calendar_cancel == bool(view), "Calendar cancellation mode lost its hit area");
            require(section != 2 || transfer_menu, "Transfer View menu did not follow the table's right edge");
            require(random_state(s) == random && s.memory.copy_out(club, saved_club.size()) == saved_club,
                    "Adaptive roster changed RNG or club records");
            for (const auto& [address, bytes] : records)
                require(s.memory.copy_out(address, bytes.size()) == bytes, "Adaptive roster changed a player record");
        }
    }
    if (section == 4) {
        explicit_cell_fixture = true;
        require(s.platform->resize_gui_canvas(s, {1280,900}), "Calendar explicit-cell resize");
        build();
        require(c.i32(explicit_cell + 0x28) == 110 && c.i32(explicit_cell + 0x2c) == 1238 &&
                c.i32(explicit_cell + 0x10) == 110 && c.i32(explicit_cell + 0x18) == 1238,
                "Calendar explicit cell failed to publish its expanded hit bounds");
    }
    initialize_new_screens();
}
void player_builder(const char* path, unsigned section = 1) {
    require(section >= 1 && section <= 4, "Unsupported player fixture section");
    Temporary temp;
    initialize_new_screens(temp.path / "Screens.plist");
    select_new_screens(true);
    for (bool goalkeeper : {false, true}) {
        Clock clock; clock.idle_input = true;
        auto state = std::make_unique<State>(); auto& s = *state;
        load_match_snapshot(s, path); s.code_fragment_observer = &clock;
        guest::Cpu c(s); c.set(2, c.u32(c.sp() + 0x14));
        const auto manager = c.toc(-0x7bb0);
        c.put16(manager + 0x2c58, 0);
        const auto captured_history = c.u32(manager + 0x10);
        require(c.u32(c.u32(captured_history)) == 0x1001e2b0, "Player fixture is not the captured club page");
        const auto club = c.u32(captured_history + 0x14);
        unsigned staff = 0;
        for (unsigned slot = 0; slot < 50; ++slot) {
            const auto person = c.u32(club + 0x98 + slot * 4);
            if (!person || c.u8(person + 0x62) != 11) continue;
            const auto player = c.u32(person + 0x14);
            if (player && (c.u8(player + 0xf) == 20) == goalkeeper) { staff = person; break; }
        }
        require(staff, "Captured club has no regular goalkeeper/outfield player");
        const auto invoke = [&](unsigned entry, std::initializer_list<unsigned> values) {
            unsigned reg = 3; for (auto value : values) c.set(reg++, value);
            s.budget = s.executed + 400000000;
            try { c.call(entry, return_sentinel); }
            catch (const std::exception& error) {
                throw std::runtime_error(std::string("Player section ") + std::to_string(section) +
                    (goalkeeper ? " GK " : " outfield ") +
                    hex_address(entry) + " at " + hex_address(s.pc) + ": " + error.what());
            }
            return c.r(3);
        };
        std::filesystem::create_directories(temp.path / "user" / "Files");
        s.platform = std::make_shared<NativePlatform>(temp.path / "game", temp.path / "user", AudioMode::offline);
        const auto scratch = allocate(s, 32, true); world(s, scratch, scratch + 16);
        // NewGWorld alone does not publish all the captured pixel globals;
        // cross a real resize before warming the original-size builder.
        require(s.platform->resize_gui_canvas(s, {801,601}) && s.platform->resize_gui_canvas(s, {800,600}),
                "Player baseline pixel globals");
        // The normal open-profile callers pass {staff, section, club-context,
        // preserve-old-history=false, Playing}; do not hand-invent its caches.
        invoke(0x10389660, {staff, section, 1, 0, 0});
        const auto history = c.u32(manager + 0x10), display = c.u32(manager + 0x24);
        const auto setting = [&](unsigned slot) { return history + 0x14 + slot * 8; };
        require(c.u32(c.u32(history)) == 0x10389ce0 && c.u32(setting(0)) == staff &&
                c.u32(setting(1)) == section && c.u32(setting(2)) == 1 && c.u32(setting(4)) == 0,
                "Original opener did not select the requested player section");
        const auto player = c.u32(staff + 0x14);
        const auto layout_pass = [&] {
            const auto registers = s.registers;
            const auto unique = s.unique;
            const auto pc = s.pc, stack_base = c.sp() - 0x8000;
            const auto stack = s.memory.copy_out(stack_base, 0x8100);
            std::vector<std::pair<unsigned, unsigned>> scroll_positions;
            for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
                const auto area = display + 4 + i * 0xbf0;
                scroll_positions.emplace_back(area, c.u32(area + 0x1fc));
            }
            layout_built_screen(s, display, 0x10389ce0);
            require(s.registers == registers && s.unique == unique && s.pc == pc &&
                    s.memory.copy_out(stack_base, stack.size()) == stack,
                    "Player layout changed the caller's registers, unique bytes, PC or stack");
            for (const auto& [area, offset] : scroll_positions)
                require(c.u32(area + 0x1fc) == offset, "Player layout changed a grid's scroll position");
        };
        const auto build = [&] {
            invoke(0x10719560, {display, 1, 0});
            invoke(0x10389ce0, {});
            layout_pass();
            for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
                const auto grid = display + 4 + i * 0xbf0;
                invoke(0x10714630, {grid}); invoke(0x10714110, {grid, 0});
            }
        };
        // Test the gate without invoking an unsupported page or replacing a
        // real saved player record. The altered selector/pointer is restored.
        require(adaptive_screen_available(s, 0x10389ce0), "Playing profile is not eligible");
        const auto history_view = c.u32(setting(5));
        c.put32(setting(5), 2); // Original Historial default; do not build that page here.
        for (unsigned candidate : {0u, 1u, 2u, 3u, 4u, 5u, 6u}) {
            c.put32(setting(1), candidate);
            require(adaptive_screen_available(s, 0x10389ce0) == (candidate >= 1 && candidate <= 5),
                    "Profile gate did not distinguish supported sections");
        }
        c.put32(setting(1), section);
        c.put32(setting(5), history_view);
        for (unsigned role : {1u, 2u}) {
            c.put32(setting(4), role);
            require(adaptive_screen_available(s, 0x10389ce0) == (section != 1),
                    "Playing-only subview gate affected the wrong section");
        }
        c.put32(setting(4), 0);
        c.put32(staff + 0x14, 0);
        require(!adaptive_screen_available(s, 0x10389ce0), "Absent player record passed the gate");
        c.put32(staff + 0x14, player);
        const auto status = c.u8(staff + 0x62);
        c.put8(staff + 0x62, 2);
        const bool status_excluded = !adaptive_screen_available(s, 0x10389ce0);
        c.put8(staff + 0x62, status);
        require(status_excluded == (section != 2), "Status-two gate disagrees with the selected original builder");
        c.put8(staff + 0x62, 1);
        const bool status_one_allowed = adaptive_screen_available(s, 0x10389ce0);
        c.put8(staff + 0x62, status);
        require(status_one_allowed == (section == 1 || section == 2),
                "Status-one gate disagrees with the selected original builder");
        if (section == 3) {
            const auto role = c.u8(staff + 0x60);
            const auto club_pointer = c.u32(staff + 0x24);
            c.put8(staff + 0x60, 5); c.put32(staff + 0x24, 0);
            const bool missing_club_excluded = !adaptive_screen_available(s, 0x10389ce0);
            c.put32(staff + 0x24, club_pointer); c.put8(staff + 0x62, 10);
            const bool wrong_status_excluded = !adaptive_screen_available(s, 0x10389ce0);
            c.put8(staff + 0x62, status); c.put8(staff + 0x60, role);
            require(missing_club_excluded && wrong_status_excluded, "Unsupported contract role passed the gate");
        }
        const auto staff_id = c.u32(staff), count_cell = c.toc(-0x7be4);
        require(count_cell && c.u32(count_cell) > 16, "Captured staff count cannot exercise sentinel boundary");
        const auto staff_count = c.u32(count_cell);
        for (const auto id : {staff_count - 16, staff_count - 1, staff_count}) {
            c.put32(staff, id);
            const bool sentinel_excluded = !adaptive_screen_available(s, 0x10389ce0);
            c.put32(staff, staff_id);
            require(sentinel_excluded, "Sentinel or past-end staff ID passed the profile gate");
        }
        require(adaptive_screen_available(s, 0x10389ce0), "Restored playing profile no longer passes its gate");
        c.put16(manager + 0x2c58, 1);
        require(!adaptive_screen_available(s, 0x10389ce0), "Secondary screen group passed the profile gate");
        c.put16(manager + 0x2c58, 0);
        const auto network = c.u32(manager + 0x2c64);
        c.put32(manager + 0x2c64, 1);
        require(!adaptive_screen_available(s, 0x10389ce0), "Network profile passed the gate");
        c.put32(manager + 0x2c64, network);
        select_new_screens(false);
        require(!adaptive_screen_available(s, 0x10389ce0), "Disabled preference still adapts profile");
        select_new_screens(true);

        struct Meaning {
            unsigned action, payload, kind, style, alignment;
            unsigned foreground, background, font, size;
            std::string text;
            bool operator==(const Meaning&) const = default;
        };
        const auto meanings = [&] {
            std::vector<Meaning> result;
            for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
                const auto at = display + 0xba864 + i * 0x17c;
                std::string text;
                for (unsigned j = 0; j < 256 && c.u8(at + 0x78 + j); ++j)
                    text.push_back(char(c.u8(at + 0x78 + j)));
                result.push_back({c.u32(at + 8), c.u32(at + 0x48), c.u32(at + 0xc),
                    c.u32(at + 0x38), c.u32(at + 0x3c), c.u16(at + 0x6a), c.u16(at + 0x6c),
                    c.u16(at + 0x6e), c.u16(at + 0x70), std::move(text)});
            }
            return result;
        };
        require(s.platform->resize_gui_canvas(s, {800,600}), "Original profile canvas");
        // Original builders populate history3 (offers), update status and may
        // prepare other caches. The warmed original, not the unopened capture,
        // defines the saved-state/RNG baseline for subsequent presentation.
        build(); build();
        const auto original_meanings = meanings();
        const auto rectangle = [&](unsigned at) {
            return ScreenRect{c.i32(at), c.i32(at + 4), c.i32(at + 8), c.i32(at + 12)};
        };
        const auto grid_at = [&](unsigned i) { return display + 4 + i * 0xbf0; };
        const auto control_at = [&](unsigned i) { return display + 0xba864 + i * 0x17c; };
        const auto hit_control = [&](unsigned index) {
            const auto at = control_at(index);
            const auto x = c.u32(c.toc(-0x5c10)), y = c.u32(c.toc(-0x5c14));
            const auto primary = c.u8(c.toc(-0x7bac)), secondary = c.u8(c.toc(-0x5c0c));
            c.put32(c.toc(-0x5c10), (c.u32(at + 0x10) + c.u32(at + 0x18)) / 2);
            c.put32(c.toc(-0x5c14), (c.u32(at + 0x14) + c.u32(at + 0x1c)) / 2);
            c.put8(c.toc(-0x7bac), 1); c.put8(c.toc(-0x5c0c), 0);
            const auto clicked = std::int16_t(invoke(0x1071a280, {display}));
            c.put32(c.toc(-0x5c10), x); c.put32(c.toc(-0x5c14), y);
            c.put8(c.toc(-0x7bac), primary); c.put8(c.toc(-0x5c0c), secondary);
            return clicked;
        };
        const auto grid_count = c.u16(display + 0x129da4), control_count = c.u16(display + 0x129da6);
        constexpr unsigned absent = 0xffffffffu;
        unsigned attributes = absent, summary = absent, footer = absent, position = absent;
        for (unsigned i = 0; i < grid_count; ++i) {
            const auto at = grid_at(i);
            const auto bounds = rectangle(at);
            if (bounds.left == 110 && bounds.right == 780) {
                if (c.u8(at + 0xbb0) == (section == 1 ? 6 : 2)) attributes = i;
                if (c.u8(at + 0xbb0) == 12 && c.u8(at + 0xbb1) == 7) summary = i;
            }
            if (bounds == ScreenRect{100,555,790,590}) footer = i;
        }
        require(attributes != absent && summary != absent && footer != absent,
                "Original player section lacks its body, summary or footer grid");
        const auto attribute_bounds = rectangle(grid_at(attributes)), summary_bounds = rectangle(grid_at(summary));
        require(c.u8(grid_at(attributes) + 0xbb1) == 12, "Original player body does not have twelve visible rows");
        struct CenteredText { unsigned control, grid; ScreenRect text, bounds; };
        std::vector<CenteredText> centered;
        std::vector<unsigned> summary_panels, loan_spans;
        unsigned body_action = absent, last_row_cell = absent;
        for (unsigned i = 0; i < grid_count; ++i)
            if (rectangle(grid_at(i)) == summary_bounds) summary_panels.push_back(i);
        bool title_text = false, biography_text = false;
        for (unsigned i = 0; i < control_count; ++i) {
            const auto at = control_at(i);
            if (c.i16(at + 0x72) < 0) {
                if (rectangle(at + 0x10) == ScreenRect{110,515,780,545}) position = i;
                continue;
            }
            const auto owner = unsigned(c.i16(at + 0x72));
            const auto bounds = rectangle(grid_at(owner));
            if (owner == attributes) {
                if (c.u32(at + 8) == 5) body_action = i;
                if (c.u32(at + 0x24) == 11) last_row_cell = i;
            }
            if (section == 1 && owner == attributes && c.i32(at + 0x28) == 558 && c.i32(at + 0x2c) == 780)
                loan_spans.push_back(i);
            if (c.i32(at + 0x28) <= 0 || c.i32(at + 0x2c) <= 0) continue;
            const bool title = bounds == ScreenRect{100,25,790,55};
            const bool biography = bounds == ScreenRect{110,125,780,155};
            if (!title && !biography) continue;
            centered.push_back({i, owner, rectangle(at + 0x10), bounds});
            title_text |= title; biography_text |= biography;
        }
        require(position != absent && (section != 1 || !loan_spans.empty()) && title_text && biography_text,
                "Original profile lacks its position, loan span or explicit rich text");
        require(last_row_cell != absent && (section == 1 || section == 2 || body_action != absent),
                "Original details fixture lacks its last row or status/value action");
        unsigned loan_label_source = absent, loan_link_source = absent;
        for (const auto& item : centered) {
            if (item.bounds != ScreenRect{110,125,780,155}) continue;
            if (!c.u32(control_at(item.control) + 8)) loan_label_source = item.control;
            else loan_link_source = item.control;
        }
        require(loan_label_source != absent && loan_link_source != absent,
                "Original biography lacks text/link sources for the controlled loan overlay");
        const auto uniform_rows = [&](unsigned owner, unsigned rows) {
            const auto bounds = rectangle(grid_at(owner));
            std::array<int, 30> tops{}, bottoms{};
            std::array<bool, 30> seen{};
            for (unsigned i = 0; i < control_count; ++i) {
                const auto at = control_at(i);
                if (c.i16(at + 0x72) != int(owner)) continue;
                const auto row = c.u32(at + 0x24);
                require(row < rows, "Profile control escaped its fixed row count");
                const auto cell = rectangle(at + 0x10);
                require(cell.left >= bounds.left && cell.right <= bounds.right && cell.width() > 0 &&
                        cell.top >= bounds.top && cell.bottom <= bounds.bottom && cell.height() > 0,
                        "Profile cell escaped its panel or inverted its bounds");
                if (seen[row]) require(tops[row] == cell.top && bottoms[row] == cell.bottom,
                                       "Profile row has inconsistent drawing/input bounds");
                else { seen[row] = true; tops[row] = cell.top; bottoms[row] = cell.bottom; }
            }
            int smallest = bounds.height(), largest = 0;
            for (unsigned row = 0; row < rows; ++row) {
                require(seen[row], "Profile lost a populated row");
                const auto height = bottoms[row] - tops[row];
                smallest = std::min(smallest, height); largest = std::max(largest, height);
                if (row) require(tops[row] - bottoms[row - 1] == 2, "Profile row gutters changed");
            }
            require(largest - smallest <= 1 && tops[0] == bounds.top && bottoms[rows - 1] == bounds.bottom,
                    "Profile rows no longer fill the panel with uniform spacing");
        };
        const auto random = random_state(s);
        const auto saved_club = s.memory.copy_out(club, 0x248);
        std::vector<std::pair<unsigned, std::vector<std::uint8_t>>> records;
        for (unsigned slot = 0; slot < 50; ++slot) {
            const auto person = c.u32(club + 0x98 + slot * 4);
            if (!person) continue;
            records.emplace_back(person, s.memory.copy_out(person, 0x70));
            if (const auto record = c.u32(person + 0x14))
                records.emplace_back(record, s.memory.copy_out(record, 0x48));
        }
        const auto masking = c.u8(c.toc(-0x797c));
        require(!original_meanings.empty(), "Original playing profile has no semantic controls");
        for (const auto canvas : {GuiCanvas{800,600}, GuiCanvas{801,601}, GuiCanvas{822,680},
                                  GuiCanvas{1200,680}, GuiCanvas{1280,900}, GuiCanvas{3200,1800}}) {
            require(s.platform->resize_gui_canvas(s, canvas), "Player profile canvas resize");
            build();
            require(meanings() == original_meanings, "Profile resize changed text, style, action or payload");
            require(c.u16(display + 0x129da4) == grid_count && c.u16(display + 0x129da6) == control_count,
                    "Profile layout changed its original grid/control counts");
            const ScreenLayout screen{canvas};
            const DetailsLayout details{screen};
            require(rectangle(grid_at(attributes)) == details.rows(attribute_bounds, 0, 12, 19),
                    "Player attribute panel did not share available width/height");
            for (const auto i : summary_panels)
                require(rectangle(grid_at(i)) == details.rows(summary_bounds, 12, 7, 19),
                        "Player summary panel/background disagrees with its rows");
            const auto expected_position = screen.anchor({110,515,780,545}, LayoutAxis::stretch, LayoutAxis::trailing);
            require(rectangle(control_at(position) + 0x10) == expected_position &&
                    rectangle(grid_at(summary)).bottom + 5 == expected_position.top &&
                    rectangle(grid_at(attributes)).bottom + 5 == rectangle(grid_at(summary)).top,
                    "Player details panels overlap the position strip or lose their gutters");
            require(rectangle(grid_at(footer)) == screen.footer(), "Player footer did not follow the canvas bottom");
            uniform_rows(attributes, 12); uniform_rows(summary, 7); uniform_rows(footer, 1);
            // Independent known-size expectations reject a broken shared helper.
            if (canvas == GuiCanvas{1280,900})
                require(rectangle(grid_at(attributes)) == ScreenRect{110,160,1260,568} &&
                        rectangle(grid_at(summary)) == ScreenRect{110,573,1260,810},
                        "Profile no longer distributes extra height evenly across all nineteen rows");
            for (const auto& item : centered) {
                const auto shift = (canvas.width - 800) / 2;
                const ScreenRect expected{item.text.left + shift, item.text.top, item.text.right + shift, item.text.bottom};
                const auto at = control_at(item.control);
                require(rectangle(at + 0x10) == expected && c.i32(at + 0x28) == expected.left &&
                        c.i32(at + 0x2c) == expected.right, "Player rich text or hyperlink lost its shared center");
            }
            const auto area = grid_at(attributes);
            const auto loan_left = c.i32(area) + c.i32(area + 0x1c + 4 * 4);
            const auto loan_right = c.i32(area + 8);
            for (const auto i : loan_spans) {
                const auto at = control_at(i);
                require(c.i32(at + 0x28) == loan_left && c.i32(at + 0x2c) == loan_right &&
                        c.i32(at + 0x10) == loan_left && c.i32(at + 0x18) == loan_right,
                        "Player loan span did not cover its final label/value columns");
            }
            bool tabs = false;
            for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
                const auto grid = display + 4 + i * 0xbf0;
                require(c.u8(grid + 0xbb0) <= 30 && c.u8(grid + 0xbb1) <= 30,
                        "Player profile exceeded AutoGrid capacity");
                if (c.i32(grid) == 100 && c.i32(grid + 4) == 80) {
                    require(c.i32(grid + 8) == canvas.width - 10, "Player section tabs retained original width");
                    tabs = true;
                }
            }
            require(tabs, "Playing profile lost its section tabs");
            if (section == 3 || section == 4)
                require(hit_control(body_action) == int(body_action) && c.u32(control_at(body_action) + 8) == 5,
                        "Status/value click did not retain the original displayed control/action");
            // Open only the original menus, never dispatch an item that could
            // buy, release, shortlist or otherwise change the career.
            bool action_opened = false;
            for (unsigned menu : {0u, 1u}) {
                if (menu) build(); // Close/recreate the first popup before the next click.
                unsigned found = 0xffffffffu;
                for (unsigned i = 0; i < c.u16(display + 0x129da6); ++i) {
                    const auto at = display + 0xba864 + i * 0x17c;
                    if (c.i16(at + 0x74) < 0 || c.i16(at + 0x72) != -1) continue;
                    const bool action = c.i32(at + 0x14) == 4 && c.i32(at + 0x1c) == 24;
                    const bool history_menu = rectangle(at + 0x10) == ScreenRect{105,15,120,35};
                    if ((menu == 0 && action) || (menu == 1 && history_menu)) { found = i; break; }
                }
                require(found != 0xffffffffu, "Original profile Action/history menu disappeared");
                const auto at = display + 0xba864 + found * 0x17c;
                if (menu == 0)
                    require(rectangle(at + 0x10) == screen.anchor({660,4,785,24}, LayoutAxis::trailing, LayoutAxis::fixed),
                            "Profile Action menu did not follow its right edge");
                else require(rectangle(at + 0x10) == ScreenRect{105,15,120,35},
                             "Profile history menu moved from its original left edge");
                const auto x = c.u32(c.toc(-0x5c10)), y = c.u32(c.toc(-0x5c14));
                const auto primary = c.u8(c.toc(-0x7bac)), secondary = c.u8(c.toc(-0x5c0c));
                c.put32(c.toc(-0x5c10), (c.u32(at + 0x10) + c.u32(at + 0x18)) / 2);
                c.put32(c.toc(-0x5c14), (c.u32(at + 0x14) + c.u32(at + 0x1c)) / 2);
                c.put8(c.toc(-0x7bac), 1); c.put8(c.toc(-0x5c0c), 0);
                const auto clicked = std::int16_t(invoke(0x1071a280, {display}));
                require(clicked == -1 && c.u32(at + 8) == 0 && c.u8(display) == 1 &&
                        c.i16(display + 0x12a930) == c.i16(at + 0x74), "Profile click did not open its displayed popup");
                c.put32(c.toc(-0x5c10), x); c.put32(c.toc(-0x5c14), y);
                c.put8(c.toc(-0x7bac), primary); c.put8(c.toc(-0x5c0c), secondary);
                if (menu == 0) action_opened = true;
            }
            require(action_opened, "Player Action menu was not exercised");
            // Controlled GUIO fixtures cover loan/injury links and populated
            // offer cells without changing career records. Only the original
            // display/AREA APIs handle them; hit testing never dispatches an
            // action handler. Injuries exercise both presence and absence of
            // the optional match link, with and without scrollbar overflow.
            for (unsigned fixture = 0; fixture < (section == 2 ? 4u : 2u); ++fixture) {
                const bool overflow = fixture & 1u;
                const bool injury_match_link = section == 2 && (fixture & 2u);
                if (section == 1 && overflow) continue;
                invoke(0x10719560, {display, 1, 0});
                invoke(0x10389ce0, {});
                require(c.u16(display + 0x129da6) == control_count, "Controlled details fixture changed base controls");
                const auto area = grid_at(attributes);
                const auto append = [&](unsigned source, unsigned row, unsigned column, ScreenRect x, bool explicit_x) {
                    const auto index = unsigned(c.u16(display + 0x129da6)), at = control_at(index);
                    require(index < 1199, "Controlled details fixture has no GUIO capacity");
                    s.memory.copy_in(at, s.memory.copy_out(control_at(source), 0x17c));
                    c.put16(at + 0x68, index); c.put16(at + 0x72, attributes);
                    c.put32(at + 0x20, column); c.put32(at + 0x24, row);
                    c.put32(at + 0x28, explicit_x ? x.left : 0);
                    c.put32(at + 0x2c, explicit_x ? x.right : 0);
                    c.put32(at + 0x30, 0); c.put32(at + 0x34, 0);
                    c.put16(display + 0x129da6, index + 1);
                    invoke(0x107144c0, {area, index});
                    return index;
                };
                std::vector<unsigned> added;
                std::vector<unsigned> sources{loan_label_source, loan_link_source};
                std::vector<ScreenRect> spans = section == 1
                    ? std::vector{ScreenRect{568,0,660,0}, ScreenRect{610,0,650,0}}
                    : std::vector{ScreenRect{258,0,780,0}, ScreenRect{300,0,350,0}};
                std::vector<std::pair<unsigned, std::vector<std::uint8_t>>> fixture_records;
                std::vector<std::pair<unsigned, unsigned>> injury_links;
                if (section == 2) {
                    // 0x1018fbe0 adds two club links (2005, club pointer) and
                    // optionally the score link (2003, match key). Private
                    // club-shaped records keep these fixture payloads out of
                    // the career; no injury or match registration is needed.
                    for (unsigned j = 0; j < 2; ++j) {
                        const auto payload = allocate(s, 0x248, true);
                        s.memory.copy_in(payload, saved_club);
                        fixture_records.emplace_back(payload, s.memory.copy_out(payload, 0x248));
                        injury_links.emplace_back(2005, payload);
                    }
                    sources.push_back(loan_link_source); spans.push_back({370,0,420,0});
                    if (injury_match_link) {
                        sources.push_back(loan_link_source); spans.push_back({440,0,480,0});
                        injury_links.emplace_back(2003, 0x13572468);
                    }
                }
                if (section != 4) {
                    for (unsigned j = 0; j < sources.size(); ++j) {
                        const auto index = append(sources[j], 11, section == 1 ? 4 : 1, spans[j], true);
                        if (section == 2) {
                            const auto at = control_at(index);
                            // The formatter's non-centered mode determines
                            // explicit X; the emitted GUIO alignment is 1.
                            c.put32(at + 0x3c, 1);
                            c.put32(at + 8, j ? injury_links[j - 1].first : 0);
                            c.put32(at + 0x48, j ? injury_links[j - 1].second : 0);
                        }
                        added.push_back(index);
                    }
                    if (overflow) {
                        // Extend only the display's logical rows, through the
                        // original sorted-entry registration; no saved bonus
                        // or clause is invented to force the scrollbar.
                        for (unsigned row = 12; row <= 18; ++row) {
                            const auto index = append(last_row_cell, row, 1, {}, false);
                            c.put32(control_at(index) + 8, 0); c.put32(control_at(index) + 0x48, 0);
                        }
                    }
                } else {
                    // Original 0x1039a8d8/938/978 publishes action19/14 with an
                    // offer-slot payload, or action2005 with a club pointer.
                    // These private bytes supply the same payload shape but
                    // are never inserted into the transfer manager/history.
                    const auto offer = allocate(s, 0x54, true), slot = allocate(s, 4, true);
                    c.put32(offer + 8, c.u32(club)); c.put32(offer + 12, c.u32(staff));
                    c.put32(offer + 0x20, 1000); c.put32(slot, offer);
                    fixture_records.emplace_back(offer, s.memory.copy_out(offer, 0x54));
                    fixture_records.emplace_back(slot, s.memory.copy_out(slot, 4));
                    const std::array<unsigned, 3> actions{19,14,2005}, payloads{slot,slot,club};
                    for (unsigned j = 0; j < actions.size(); ++j) {
                        const auto index = append(body_action, (overflow ? 12 : 9) + j, 1, {}, false);
                        c.put32(control_at(index) + 8, actions[j]);
                        c.put32(control_at(index) + 0x48, payloads[j]);
                        added.push_back(index);
                    }
                }
                invoke(0x10714630, {area});
                invoke(0x107145f0, {area, overflow ? 3u : 0u});
                invoke(0x10714630, {area}); invoke(0x10714110, {area, 0});
                const auto previous_value_start = c.i32(area) + c.i32(area + 0x20);
                require(c.u32(area + 0x1fc) == (overflow ? 3u : 0u), "Controlled details scroll setup failed");
                if (overflow) require(c.u8(area + 0xbef) && c.i32(area + 0x14) >= 14,
                                      "Controlled details fixture did not reach scrollbar overflow");
                const auto controlled_meanings = meanings();
                layout_pass();
                require(meanings() == controlled_meanings, "Controlled details layout changed text, style, action or payload");
                require(c.u32(area + 0x1fc) == (overflow ? 3u : 0u), "Controlled details layout lost its scroll offset");
                if (section != 4) {
                    const auto row = rectangle(control_at(last_row_cell) + 0x10);
                    const int shift = section == 1
                        ? (c.i32(area) + c.i32(area + 0x1c + 4 * 4) + 10 + c.i32(area + 8) - 568 - 780) / 2
                        : c.i32(area) + c.i32(area + 0x20) - previous_value_start;
                    if (canvas == GuiCanvas{1280,900})
                        require(shift == (section == 1 ? 400 : 106), "Controlled details text lost its known wide-canvas alignment");
                    for (unsigned j = 0; j < added.size(); ++j) {
                        const auto at = control_at(added[j]);
                        const auto right = section != 1 && j == 0 ? c.i32(area + 8) : spans[j].right + shift;
                        const ScreenRect expected{spans[j].left + shift,row.top,right,row.bottom};
                        require(c.i32(at + 0x28) == expected.left && c.i32(at + 0x2c) == expected.right &&
                                rectangle(at + 0x10) == expected,
                                "Controlled details text/link lost its alignment or scrolled row bounds");
                    }
                    if (section == 2) {
                        require(added.size() == (injury_match_link ? 4u : 3u),
                                "Controlled injury Type fixture lost its optional match-link variant");
                        for (unsigned j = 1; j < added.size(); ++j) {
                            const auto at = control_at(added[j]);
                            require(hit_control(added[j]) == int(added[j]) && c.u32(at + 0x3c) == 1 &&
                                    c.u32(at + 8) == injury_links[j - 1].first &&
                                    c.u32(at + 0x48) == injury_links[j - 1].second,
                                    "Controlled injury link lost its club/match payload, alignment or hit rectangle");
                        }
                    } else {
                        require(hit_control(added[1]) == int(added[1]) &&
                                c.u32(control_at(added[1]) + 8) == c.u32(control_at(loan_link_source) + 8) &&
                                c.u32(control_at(added[1]) + 0x48) == c.u32(control_at(loan_link_source) + 0x48),
                                "Controlled loan hyperlink lost its original action/payload or hit rectangle");
                    }
                } else {
                    for (const auto index : added) {
                        const auto at = control_at(index);
                        const auto bounds = rectangle(at + 0x10);
                        require(bounds.left == c.i32(area) + c.i32(area + 0x20) &&
                                bounds.right == c.i32(area) + c.i32(area + 0x110) &&
                                bounds.top >= c.i32(area + 4) && bounds.bottom <= c.i32(area + 12),
                                "Controlled offer cell escaped its visible value column");
                        const auto action = c.u32(at + 8), payload = c.u32(at + 0x48);
                        require(hit_control(index) == int(index) && c.u32(at + 8) == action && c.u32(at + 0x48) == payload,
                                "Controlled offer click changed its original action/payload shape");
                    }
                }
                for (const auto& [address, bytes] : fixture_records)
                    require(s.memory.copy_out(address, bytes.size()) == bytes, "Details layout changed private link/offer payload records");
            }
            require(random_state(s) == random && s.memory.copy_out(club, saved_club.size()) == saved_club &&
                    c.u8(c.toc(-0x797c)) == masking, "Profile presentation changed RNG, club or masking setting");
            for (const auto& [address, bytes] : records)
                require(s.memory.copy_out(address, bytes.size()) == bytes, "Profile presentation changed staff/player records");
        }
        // Off remains a true no-op even while the host canvas is still wide.
        build(); select_new_screens(false);
        const auto disabled = s.memory.copy_out(display, 0x12a940);
        layout_pass();
        require(s.memory.copy_out(display, disabled.size()) == disabled, "Disabled profile layout changed the display table");
        select_new_screens(true);
        std::cout << "ok player section " << section << ' ' << (goalkeeper ? "goalkeeper" : "outfield")
                  << ": 6 canvases, original text/actions/styles, popup/details input, records/RNG and scope\n";
    }
    initialize_new_screens();
}

#include "adaptive_player_history_checks.hpp"
#include "adaptive_search_checks.hpp"
#include "adaptive_filter_checks.hpp"
#include "adaptive_proposal_checks.hpp"
#include "adaptive_contract_checks.hpp"
#include "adaptive_negotiation_dialog_checks.hpp"
#include "adaptive_news_checks.hpp"
#include "adaptive_finances_checks.hpp"
#include "adaptive_club_info_checks.hpp"
#include "adaptive_chrome_checks.hpp"
#include "adaptive_next_match_checks.hpp"
#include "adaptive_league_standings_checks.hpp"
#include "adaptive_knockout_checks.hpp"
#include "adaptive_group_stage_checks.hpp"
#include "adaptive_archived_commentary_checks.hpp"
#include "adaptive_live_match_checks.hpp"
#include "adaptive_competition_fixtures_checks.hpp"
#include "adaptive_competition_history_checks.hpp"
#include "adaptive_daily_fixtures_checks.hpp"
#include "adaptive_nations_and_clubs_checks.hpp"
#include "adaptive_training_schedule_checks.hpp"
#include "adaptive_staff_profile_checks.hpp"
#include "adaptive_training_checks.hpp"
#include "adaptive_transfer_history_checks.hpp"
#include "adaptive_startup_checks.hpp"
#include "adaptive_load_browser_checks.hpp"

void screens_preference() {
    Temporary temp;
    const auto path = temp.path / "Screens.plist";
    initialize_new_screens(path);
    require(new_screens_enabled() && !std::filesystem::exists(path), "Missing preference must retain the current interface");
    require(adaptive_screen_enabled(0x103b3c50) && adaptive_screen_enabled(0x103baec0) && !adaptive_screen_enabled(0x1033d030),
            "Central screen registration did not respect eligible builders");
    select_new_screens(false); initialize_new_screens(path);
    require(!new_screens_enabled() && !adaptive_screen_enabled(0x103b3c50) && !adaptive_screen_enabled(0x103baec0),
            "Off did not persist or bypassed the central gate");
    select_new_screens(true); initialize_new_screens(path);
    require(new_screens_enabled(), "On did not survive restart");
    for (const auto& contents : {std::string("invalid"), std::string(5000, 'x'),
            std::string("<?xml version=\"1.0\"?><plist version=\"1.0\"><dict><key>Version</key><integer>1</integer><key>NewScreens</key><string>false</string></dict></plist>")}) {
        { std::ofstream out(path); out << contents; }
        initialize_new_screens(path); require(new_screens_enabled(), "Malformed preference was accepted");
    }
    const auto blocker = temp.path / "file";
    { std::ofstream out(blocker); out << "not a directory"; }
    initialize_new_screens(blocker / "Screens.plist");
    bool failed = false;
    try { select_new_screens(false); } catch (const std::runtime_error&) { failed = true; }
    require(failed && new_screens_enabled(), "Failed write published an unsaved option");
    initialize_new_screens();
    require(new_screens_label(9) == "Novos \311cr\343s" && new_screens_label(99) == new_screens_label(0),
            "Portuguese label or language fallback changed");
    for (unsigned language = 0; language < 11; ++language) require(!new_screens_label(language).empty(), "Missing screen option translation");
}
void screen_options(const char* path, unsigned language, bool debugging) {
    Temporary temp; Clock clock;
    const auto preference = temp.path / "Screens.plist";
    initialize_new_screens(preference);
    if (debugging) initialize_native_debugging(false);
    auto state = std::make_unique<State>(); auto& s = *state;
    load_match_snapshot(s, path); s.code_fragment_observer = &clock;
    guest::Cpu c(s); c.set(2, c.u32(c.sp() + 0x14));
    struct SettingsServices final : SemanticDependencyDispatch {
        unsigned applied = 0, defaults = 0;
        void call(State& state, std::uint32_t entry, std::uint32_t continuation) override {
            // Original preferences' disk/video apply and hardware defaults are
            // separate components.
            // Record its real argument while testing the entire custom modal
            // flow, without importing the captured user's resource file.
            if (entry == 0x1013feb0) { applied = state.r<4>(12); state.pc = continuation; }
            else if (entry == 0x1013f570) { ++defaults; state.pc = continuation; }
            else run_until(state, continuation);
        }
    } services;
    c.put8(c.r(2) - 0x35ac, language);
    const auto root = c.toc(-0x7bb0); c.put16(root + 0x2c58, 0);
    const auto display = [&] { return c.u32(root + 0x24); };
    const auto invoke = [&](unsigned entry, std::initializer_list<unsigned> args) {
        unsigned n = 3; for (auto v : args) c.set(n++, v);
        s.budget = s.executed + 400000000;
        try {
            if (entry == 0x1033f840) {
                c.link(return_sentinel); preferences_actions::dispatch_action(s, &services);
            } else c.call(entry, return_sentinel);
        }
        catch (const std::exception& e) { throw std::runtime_error("Settings " + hex_address(entry) + ": " + e.what()); }
        return c.r(3);
    };
    const auto controls = [&] {
        invoke(0x10719560, {display(), 1, 0}); invoke(0x1033d030, {});
        for (unsigned i = 0; i < c.u16(display() + 0x129da4); ++i) {
            const auto grid = display() + 4 + i * 0xbf0;
            invoke(0x10714630, {grid}); invoke(0x10714110, {grid, 0});
        }
    };
    const auto find = [&](unsigned action, unsigned value) {
        for (unsigned i = 0; i < c.u16(display() + 0x129da6); ++i) {
            const auto control = display() + 0xba864 + i * 0x17c;
            if (c.u32(control + 8) == action && c.u32(control + 0x48) == value) return i;
        }
        throw std::runtime_error("Missing settings control " + std::to_string(action));
    };
    const auto close = [&] {
        const auto node = c.u32(root + 0x10), previous = c.u32(node + 0x1f4);
        require(previous && c.u8(node + 0x10), "Settings did not create a modal frame");
        c.put32(root + 0x10, previous); c.put32(root + 0xc, previous); c.put32(previous + 0x1f8, 0);
        c.put16(root + 0x14, c.i16(root + 0x14) - 1); c.put16(root + 0x16, c.i16(root + 0x16) - 1);
        invoke(0x107735a0, {node});
    };
    const auto original = c.u32(c.toc(-0x7ac8));
    const auto original_bytes = s.memory.copy_out(original, 0x150), random = random_state(s);
    invoke(0x1033ce70, {1}); controls();
    const auto edit = invoke(0x1073ef70, {root, 0});
    const auto edited_bytes = s.memory.copy_out(edit, 0x150);
    const auto off = find(new_screens_action, 0), at = display() + 0xba864 + off * 0x17c;
    require(c.i32(at + 0x10) >= 170 && c.i32(at + 0x18) <= 719 && c.i32(at + 0x1c) < 490 &&
            c.i32(at + 0x1c) > c.i32(at + 0x14), "New option overlaps the settings footer");
    unsigned labels = 0;
    for (unsigned i = 0; i < c.u16(display() + 0x129da6); ++i) {
        std::string label; const auto item = display() + 0xba864 + i * 0x17c;
        for (unsigned j = 0; j < 200 && c.u8(item + 0x78 + j); ++j) label += char(c.u8(item + 0x78 + j));
        labels += label == " " + std::string(new_screens_label(language));
    }
    require(labels == 1, "Missing or duplicated localized screen option");
    // Selecting the custom toggle must not write any original preference or RNG byte.
    require(std::int32_t(invoke(0x1033f840, {off})) == -11 && new_screens_enabled(), "Choice was not deferred until OK");
    require(s.memory.copy_out(edit, 0x150) == edited_bytes && s.memory.copy_out(original, 0x150) == original_bytes && random_state(s) == random,
            "Custom option changed original settings or RNG");
    controls();
    c.put16(root + 0x25819e, 40);
    require(std::int32_t(invoke(0x1033f840, {0xffffffffu})) == -9 && new_screens_enabled() && !std::filesystem::exists(preference),
            "Escape/Cancel committed the pending option");
    require(services.applied == original, "Cancel did not keep the original preference application");
    c.put16(root + 0x25819e, 0); close();
    invoke(0x1033ce70, {1}); controls();
    invoke(0x1033f840, {find(new_screens_action, 0)}); controls();
    const auto ok = find(41, 0), expected_edit = invoke(0x1073ef70, {root, 0});
    // The original dispatcher uses only the signed low half of the action.
    c.put32(display() + 0xba864 + ok * 0x17c + 8, 0x12340029);
    require(std::int32_t(invoke(0x1033f840, {ok})) == -9 && !new_screens_enabled(), "OK did not disable new screens");
    require(services.applied == expected_edit, "OK did not keep the edited preference application");
    close(); initialize_new_screens(preference); require(!new_screens_enabled(), "Game Settings choice did not persist");
    invoke(0x1033ce70, {1}); controls();
    invoke(0x1033f840, {find(39, 0)}); require(!new_screens_enabled(), "Defaults applied before OK");
    require(services.defaults == 1, "Custom defaults swallowed the original reset action");
    controls();
    require(std::int32_t(invoke(0x1033f840, {find(40, 0)})) == -9 && !new_screens_enabled(), "Cancel did not discard Defaults");
    close();
    invoke(0x1033ce70, {1}); controls();
    invoke(0x1033f840, {find(new_screens_action, 1)}); controls();
    const auto fallback = find(41, 0);
    c.put32(display() + 0xba864 + fallback * 0x17c + 8, 0x10000); c.put16(root + 0x25819e, 41);
    require(std::int32_t(invoke(0x1033f840, {fallback})) == -9 && new_screens_enabled(), "Disabled option could not be enabled again");
    c.put16(root + 0x25819e, 0);
    close();
    // A committed Off is consumed only at the same safe canvas-rebuild boundary.
    s.platform = std::make_shared<NativePlatform>(temp.path / "game", temp.path / "user", AudioMode::offline);
    const auto scratch = allocate(s, 32, true); world(s, scratch, scratch + 16);
    require(s.platform->resize_gui_canvas(s, {1200,680}), "Settings gate fixture resize");
    select_new_screens(false);
    require(s.platform->request_adaptive_screen(0x103b3c50) && gui_canvas(s) == GuiCanvas{1200,680}, "Off bypassed deferred resizing");
    c.put32(display() + 0x12a928, 0);
    s.platform->apply_adaptive_screen(s, display());
    require(gui_canvas(s).original() && !s.platform->request_adaptive_screen(0x103b3c50), "Off did not restore the original canvas");
    initialize_new_screens();
}
}
int main(int argc, char** argv) {
    try {
        checks(); screens_preference();
        if (argc == 3 && std::string_view(argv[2]) == "--roster") club_builder(argv[1], 1);
        else if (argc == 3 && std::string_view(argv[2]) == "--club-lists") {
            club_builder(argv[1], 2); club_builder(argv[1], 4);
        }
        else if (argc == 3 && std::string_view(argv[2]) == "--player") player_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--player-injuries") player_builder(argv[1], 2);
        else if (argc == 3 && std::string_view(argv[2]) == "--player-history") player_history_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--search") search_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--recruitment") search_builder(argv[1], true);
        else if (argc == 3 && std::string_view(argv[2]) == "--filters") filter_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--proposal") proposal_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--contract") contract_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--negotiation-dialogs") negotiation_dialog_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--news") news_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--finances") finances_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--club-info") club_info_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--chrome") chrome_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--next-match") next_match_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--live-match") live_match_builder(argv[1]);
        else if (argc == 4 && std::string_view(argv[2]) == "--archived-commentary") archived_commentary_builder(argv[1], argv[3]);
        else if (argc == 4 && std::string_view(argv[2]) == "--group-stage") group_stage_builder(argv[1], argv[3]);
        else if (argc == 3 && std::string_view(argv[2]) == "--knockout-rounds") knockout_rounds_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--league-standings") league_standings_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--competition-history") competition_history_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--competition-fixtures") competition_fixtures_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--daily-fixtures") daily_fixtures_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--staff-profile") training_builder(argv[1], TrainingPage::staff_profile);
        else if (argc == 3 && std::string_view(argv[2]) == "--training-coaches") training_builder(argv[1], TrainingPage::coaches);
        else if (argc == 3 && std::string_view(argv[2]) == "--training-schedule") training_builder(argv[1], TrainingPage::schedule);
        else if (argc == 3 && std::string_view(argv[2]) == "--training") training_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--nations-and-clubs") nations_and_clubs_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--transfer-history") transfer_history_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--startup") startup_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--load-browser") load_browser_builder(argv[1]);
        else if (argc == 3 && std::string_view(argv[2]) == "--player-tabs") {
            player_builder(argv[1], 3); player_builder(argv[1], 4);
        }
        else if (argc == 2) {
            builder(argv[1]);
            screen_options(argv[1], 9, false);
            for (unsigned language = 0; language < 11; ++language) screen_options(argv[1], language, true);
        }
        std::cout << "Adaptive buffers, input bounds, tactics state and persistent localized screen options passed\n";
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
