#include "adaptive_gui.hpp"
#include "guest_abi.hpp"
#include "platform.hpp"
#include "tactics_layout.hpp"
#include "match_replay.hpp"
#include "code_fragment_adapter.hpp"
#include "new_screens.hpp"
#include "debug_localization.hpp"
#include "match_debugging.hpp"
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
            {0x1074b0c4,5,1,45},{0x1074b0cc,7,1,10},{0x1074b100,6,0,10}};
        for (const auto& item : loads) {
            const auto registers = s.registers;
            c.set(item.reg, (item.bottom ? canvas.height : canvas.width) - item.inset);
            const auto expected = s.registers; s.registers = registers;
            instruction(s, item.pc); require(s.registers == expected, "Canvas constant clobbered registers");
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
        require(layout.list_right() >= 440 && layout.list_right() + layout.columns() * 160 + 10 == layout.pitch_left() &&
                layout.pitch_right() == canvas.width - 20 && layout.list_bottom() == canvas.height - 55,
                "Roster columns overlap the pitch or change original margins");
        require(std::abs(layout.pitch_width() * 400 - (layout.pitch_bottom() - 145) * 330) < 330 &&
                layout.pitch_bottom() <= layout.list_bottom(), "Pitch lost its original aspect ratio");
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
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    bool import_fixture(State& s, const char* name) override {
        if (std::string_view(name) == "CarbonLib::GetDateTime") s.memory.write<4>(s.r<4>(12), 3000000000u);
        else if (std::string_view(name) == "CarbonLib::Microseconds") s.memory.write<8>(s.r<4>(12), 1000000000ULL);
        else if (std::string_view(name) == "CarbonLib::FSMakeFSSpec") s.wr<4>(12, std::uint32_t(-43));
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
    for (const auto canvas : {GuiCanvas{800,600}, GuiCanvas{1200,680}, GuiCanvas{1280,900}, GuiCanvas{1600,1000}, GuiCanvas{800,600}}) {
        require(s.platform->resize_gui_canvas(s, canvas), "Fixture resize failed");
        invoke(0x10719560, {display, 1, 0});
        invoke(0x103b3c50, {});
        for (unsigned i = 0; i < c.u16(display + 0x129da4); ++i) {
            const auto grid = display + 4 + i * 0xbf0;
            require(c.i32(grid) < c.i32(grid + 8) && c.i32(grid + 4) < c.i32(grid + 12),
                    "Grid was inverted or collapsed by the legacy canvas clamp");
            invoke(0x10714630, {grid});
            invoke(0x10714110, {grid, 0});
        }
        const auto count = c.u16(display + 0x129da6);
        if (!original_count) original_count = count;
        require(canvas.original() ? count == original_count : count > original_count,
                "Extra width did not reveal additional controls or shrink retained them");
        bool pitch = false, title = false, cancel = false, ok = false;
        for (unsigned i = 0; i < count; ++i) {
            const auto control = display + 0xba864 + i * 0x17c;
            require(c.i32(control + 0x18) < canvas.width && c.i32(control + 0x1c) < canvas.height,
                    "Control escaped the active canvas");
            if (c.u32(control + 8) == 0x1c) {
                require(c.i32(control + 0x10) > TacticsLayout{canvas}.pitch_left() &&
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
                require(c.i32(control + 0x10) >= TacticsLayout{canvas}.footer_left() &&
                        c.i32(control + 0x10) < c.i32(control + 0x18), "Footer button has no visible width");
                if (action == 0x20) cancel = true; else {
                    ok = true;
                    require(c.i32(control + 0x18) == canvas.width - 10, "Ok button did not reach the right margin");
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
        if (argc == 2) {
            builder(argv[1]);
            screen_options(argv[1], 9, false);
            for (unsigned language = 0; language < 11; ++language) screen_options(argv[1], language, true);
        }
        std::cout << "Adaptive buffers, input bounds, tactics state and persistent localized screen options passed\n";
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
