#include "match_debugging.hpp"
#include "debug_localization.hpp"
#include "debug_match_results.hpp"
#include "debug_force_purchase.hpp"
#include "debug_force_staff_signing.hpp"
#include "transfer_manager_native_adapter.hpp"
#include "routine_check.hpp"
#include "guest_abi.hpp"
#include "code_fragment_adapter.hpp"
#include "match_replay.hpp"
#include "platform.hpp"
#include "adaptive_gui.hpp"
#include "debug_options_layout.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unistd.h>
#include <vector>
using namespace cm::aot;
namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct Clock final : CodeFragmentBoundaryObserver {
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    bool import_fixture(State& s, const char* name) override {
        if (std::string_view(name) == "CarbonLib::GetDateTime") s.memory.write<4>(s.r<4>(12), 3000000000u);
        else if (std::string_view(name) == "CarbonLib::Microseconds") s.memory.write<8>(s.r<4>(12), 1000000000ULL);
        else if (!s.platform && std::string_view(name) == "CarbonLib::FSMakeFSSpec") s.wr<4>(12, std::uint32_t(-43));
        else return false;
        s.pc = s.r<4>(4128);
        return true;
    }
};

struct TransferFiles {
    std::filesystem::path path;
    TransferFiles() {
        auto pattern = (std::filesystem::temp_directory_path() / "cm-debug-purchase-XXXXXX").string();
        const auto directory = mkdtemp(pattern.data());
        require(directory != nullptr, "Could not create the transfer fixture directory");
        path = directory;
    }
    ~TransferFiles() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
};

// The real fixture list (MATCH_SCREENS 1028ae80) built from a match-day queue,
// clicked through the original mouse-down handler, hit test and dispatcher.
void check_fixture_results(const char* path) {
    auto state = std::make_unique<State>();
    load_match_snapshot(*state, path);
    auto& s = *state;
    Clock clock;
    s.code_fragment_observer = &clock;
    guest::Cpu c(s);
    c.set(2, c.u32(c.sp() + 0x14));
    const auto ui = c.toc(-0x7bb0);
    c.put16(ui + 0x2c58, 0);
    const auto invoke = [&](unsigned entry, std::initializer_list<unsigned> args) {
        s.budget = s.executed + 400000000;
        unsigned reg = 3;
        for (auto value : args) c.set(reg++, value);
        try { c.call(entry, 0xfffffff0); }
        catch (const std::exception& error) { throw std::runtime_error(hex_address(entry) + ": " + error.what()); }
        require(s.pc == 0xfffffff0, "UI operation failed to return");
        return c.r(3);
    };
    const auto page = [&] { return c.u32(ui + 0x24 + c.i16(ui + 0x2c58) * 0x2c0); };
    const auto count = [&] { return unsigned(c.u16(page() + 0x129da6)); };
    const auto item = [&](unsigned i) { return page() + 0xba864 + i * 0x17c; };
    const auto text = [&](unsigned i) {
        std::string result;
        for (unsigned j = 0; j < 200 && c.u8(item(i) + 0x78 + j); ++j) result += char(c.u8(item(i) + 0x78 + j));
        return result;
    };
    const auto ink = [&](unsigned i) { return c.u16(item(i) + 0x70); };
    const auto table = [&] {
        // Without the save-under pixel buffer each drawn control allocates.
        auto bytes = s.memory.copy_out(item(0), count() * 0x17c);
        for (unsigned i = 0; i < count(); ++i) std::fill_n(bytes.begin() + i * 0x17c + 0x4c, 4, 0);
        return bytes;
    };
    const auto find = [&](unsigned action, std::uint32_t data) {
        for (unsigned i = 0; i < count(); ++i)
            if (c.u32(item(i) + 8) == action && c.u32(item(i) + 0x48) == data) return int(i);
        return -1;
    };
    const auto rng = s.memory.copy_out(c.toc(-0x7b90), 0x10);
    const auto queue = c.u32(c.toc(-0x7420));
    require(queue && c.u32(queue + 0x10) && c.u32(queue + 0x1c), "Match-day snapshot has no fixture queue");
    const auto fixture_at = [&](unsigned entry) { return c.u32(c.u32(queue + 0x1c) + entry * 0x6c); };
    // Competition lists: one with two selectable fixtures, one holding a
    // human manager's unplayed fixture (never selectable).
    std::uint32_t list = 0, human_list = 0, human = 0;
    std::vector<std::uint32_t> eligible;
    for (unsigned g = 0; g < c.u32(queue + 0xc); ++g) {
        const auto group = c.u32(queue + 4) + g * 0x18;
        for (auto n = c.u32(group + 4); n <= c.u32(group + 8); ++n) {
            const auto sub = c.u32(queue + 0x10) + n * 0x20;
            std::vector<std::uint32_t> found;
            for (auto e = c.u32(sub + 4); e <= c.u32(sub + 8); ++e) {
                const auto fixture = fixture_at(e);
                if (debug_match_result_eligible(s, fixture)) found.push_back(fixture);
                else if (fixture && c.i8(fixture + 0x43) == -1 && !human) { human = fixture; human_list = sub; }
            }
            if (!list && found.size() >= 2) { list = sub; eligible = found; }
        }
    }
    require(list && human, "Match-day snapshot lacks a selectable list or a human fixture");
    const auto build = [&](std::uint32_t sub) {
        invoke(0x1073f1e0, {ui, 1, sub, 0});
        invoke(0x10719560, {page()});
        invoke(0x1028ae80, {});
        // Grid cells get their rectangles when the grid lays out its columns
        // and rows (10714630) and draws (10714110), as the page draw does.
        const auto grid = std::int16_t(invoke(0x1073ef70, {ui, 3}));
        require(grid >= 0, "Fixture list grid missing");
        invoke(0x10714630, {page() + 4 + unsigned(grid) * 0xbf0});
        invoke(0x10714110, {page() + 4 + unsigned(grid) * 0xbf0, 0});
    };
    invoke(0x10773520, {16});
    const auto record = c.r(3);
    const auto press = [&](unsigned button) {
        // Mouse-down through the original handler 10770570, which records
        // the native button (2 right/Control-click) as the left/right bytes.
        s.memory.copy_in(record, std::vector<std::uint8_t>(16));
        c.put16(record, 1);
        c.put16(c.toc(-0x5ae0), button);
        if (button == 2) c.put16(record + 0xe, c.u32(c.toc(-0x5ad0)));
        invoke(0x10770570, {record});
        require(c.u8(c.toc(-0x5c0c)) == (button == 2) && c.u8(c.toc(-0x7bac)) == (button != 2),
                "Original mouse-down handler did not record the button");
    };
    const auto hit = [&](int cell, unsigned button) {
        require(cell >= 0, "Missing fixture cell");
        const auto at = item(unsigned(cell));
        c.put32(c.toc(-0x5c10), (c.u32(at + 0x10) + c.u32(at + 0x18)) / 2);
        c.put32(c.toc(-0x5c14), (c.u32(at + 0x14) + c.u32(at + 0x1c)) / 2);
        press(button);
        return int(std::int16_t(invoke(0x1071a280, {page()})));
    };
    const auto right_click = [&](int cell) {
        const auto selected = hit(cell, 2);
        const auto rebuild = std::int32_t(invoke(0x1028bc90, {std::uint32_t(selected)}));
        // The press is consumed: a held button must not select the cell again on the next pass.
        require(c.u8(c.toc(-0x5c0c)) == 0 && c.u8(c.toc(-0x7bac)) == 0, "Right-click on a result cell was not consumed");
        require(std::int16_t(invoke(0x1071a280, {page()})) < 0 || selected < 0, "A held right button selects a result cell again");
        return std::pair{selected, rebuild};
    };
    struct Row { std::uint32_t fixture; int home, versus, away; };
    const auto row = [&](std::uint32_t fixture) {
        const auto versus = find(0x7d2, fixture);
        require(versus >= 0, "Unplayed fixture has no v cell");
        Row r{fixture, -1, versus, -1};
        for (unsigned i = 0; i < count(); ++i) {
            if (c.u16(item(i) + 0x72) != c.u16(item(unsigned(versus)) + 0x72) ||
                c.u32(item(i) + 0x24) != c.u32(item(unsigned(versus)) + 0x24)) continue;
            if (c.u32(item(i) + 0x20) == 2 && c.u32(item(i) + 0x48) == c.u32(fixture + 0x1c)) r.home = int(i);
            if (c.u32(item(i) + 0x20) == 6 && c.u32(item(i) + 0x48) == c.u32(fixture + 0x20)) r.away = int(i);
        }
        require(r.home >= 0 && r.away >= 0, "Fixture row names not found by column and club");
        return r;
    };
    const auto result_feature = 1u << unsigned(DebugFeature::match_result);
    initialize_native_debugging(false);
    build(list);
    const auto off = table();
    const auto off_count = count();
    const auto a = row(eligible[0]), b = row(eligible[1]);
    const auto normal_home = ink(unsigned(a.home)), normal_away = ink(unsigned(a.away));
    const auto versus_text = text(unsigned(a.versus));
    require(find(30230, a.fixture) < 0 && find(30231, a.fixture) < 0, "Disabled feature added right-click cells");
    // Today's left click and a right click without the feature.
    const auto left = hit(a.home, 1);
    require(left == a.home && c.u32(item(unsigned(left)) + 0x48) == c.u32(a.fixture + 0x1c),
            "Left click no longer selects the home name");
    c.set(3, unsigned(left));
    require(!dispatch_fixture_debugging_control(s), "Left click on a name must keep the original club action");
    require(hit(a.home, 2) == -1, "Disabled feature reacted to a right click");

    select_native_debugging_features(result_feature);
    build(list);
    for (unsigned i = 0; i < count(); ++i) require(c.u32(item(i) + 8) != 30022, "Match Results menu item remains");
    // Only invisible overlays are added: every original control is unchanged
    // apart from its table index.
    unsigned first_overlay = 0;
    while (first_overlay < count() && (c.u32(item(first_overlay) + 8) < 30230 || c.u32(item(first_overlay) + 8) > 30232))
        ++first_overlay;
    const auto added = count() - off_count;
    require(added == 3 * eligible.size(), "Expected three right-click cells per selectable fixture");
    const auto on = table();
    for (unsigned i = 0; i < off_count; ++i) {
        const auto j = i < first_overlay ? i : i + added;
        auto original = std::vector<std::uint8_t>(off.begin() + i * 0x17c, off.begin() + (i + 1) * 0x17c);
        auto current = std::vector<std::uint8_t>(on.begin() + j * 0x17c, on.begin() + (j + 1) * 0x17c);
        original[0x68] = current[0x68] = original[0x69] = current[0x69] = 0;
        require(original == current, "Enabled feature changed an original fixture-list control");
    }
    const auto overlays = [&](const Row& r) {
        std::array<int, 3> cells{find(30230, r.fixture), find(30231, r.fixture), find(30232, r.fixture)};
        const std::array<int, 3> under{r.home, r.versus, r.away};
        for (unsigned k = 0; k < 3; ++k) {
            require(cells[k] >= 0, "Missing right-click cell");
            const auto o = item(unsigned(cells[k])), u = item(unsigned(under[k]));
            require(c.u32(o + 12) == 0x102 && text(unsigned(cells[k])).empty() && c.u32(o + 0x38) == 1 &&
                    c.u16(o + 0x72) == c.u16(u + 0x72) && s.memory.copy_out(o + 0x10, 16) == s.memory.copy_out(u + 0x10, 16),
                    "Right-click cell must be an invisible right-only control over its name");
        }
        return cells;
    };
    overlays(a); overlays(b);
    require(find(30230, human) < 0, "Human fixture got right-click cells");
    require(text(unsigned(a.versus)) == versus_text && ink(unsigned(a.home)) == normal_home, "No choice changed a row");

    const auto green = std::uint16_t(invoke(0x1076bb20, {0x90, 0xee, 0x90, 0}));
    require(green != normal_home && green != normal_away, "Light green must differ from the original inks");
    const auto expect = [&](const Row& r, ForcedMatchResult result) {
        require(debug_match_result(s, r.fixture) == result, "Right click selected the wrong result");
        const auto now = row(r.fixture);
        require(ink(unsigned(now.home)) == (result == ForcedMatchResult::home_win ? green : normal_home) &&
                ink(unsigned(now.away)) == (result == ForcedMatchResult::away_win ? green : normal_away) &&
                text(unsigned(now.versus)) == (result == ForcedMatchResult::draw ?
                    std::string(debug_text(s, DebugText::draw)) : versus_text),
                "Fixture row does not show its selected result");
        return now;
    };
    // Home name: home win; same cell again clears; "v": draw; away switches.
    auto [selected, rebuild] = right_click(overlays(a)[0]);
    require(selected == find(30230, a.fixture) && rebuild == -11, "Right click on the home name did not rebuild");
    build(list);
    expect(a, ForcedMatchResult::home_win);
    expect(b, ForcedMatchResult::normal);
    std::tie(selected, rebuild) = right_click(overlays(a)[0]);
    require(rebuild == -11, "Clearing right click did not rebuild");
    build(list);
    expect(a, ForcedMatchResult::normal);
    require(table() == on, "Cleared choice left the fixture list changed");
    right_click(overlays(a)[1]);
    build(list);
    const auto draw_row = expect(a, ForcedMatchResult::draw);
    {
        const auto v = item(unsigned(draw_row.versus));
        const auto width = c.u32(v + 0x18) - c.u32(v + 0x10) + 1;
        for (unsigned language = 0; language < 11; ++language) {
            const auto word = std::string(debug_text(language, DebugText::draw));
            const auto scratch = record;
            s.memory.copy_in(scratch, std::vector<std::uint8_t>(word.begin(), word.begin() + std::min<std::size_t>(word.size(), 15)));
            c.put8(scratch + std::min<std::size_t>(word.size(), 15), 0);
            require(word.size() < 16 && invoke(0x1076d1e0, {c.u32(v + 0x3c), scratch}) <= width,
                    "Localized draw word does not fit the v column");
        }
    }
    right_click(overlays(a)[2]);
    build(list);
    expect(a, ForcedMatchResult::away_win);
    right_click(overlays(b)[0]);
    build(list);
    expect(a, ForcedMatchResult::away_win);
    expect(b, ForcedMatchResult::home_win);

    // Stale or forged cells: an ineligible fixture and a disabled feature.
    const auto forged = unsigned(overlays(a)[1]);
    c.put32(item(forged) + 0x48, human);
    require(std::int32_t(invoke(0x1028bc90, {forged})) == 0 && debug_match_result(s, human) == ForcedMatchResult::normal,
            "Ineligible fixture accepted a right click");
    build(list);
    select_native_debugging_features(0);
    require(std::int32_t(invoke(0x1028bc90, {unsigned(overlays(a)[1])})) == 0 &&
            debug_match_result(s, a.fixture) == ForcedMatchResult::away_win, "Disabled feature accepted a stale right click");
    build(list);
    require(table() == off, "Disabled feature changed the fixture list");
    select_native_debugging_features(result_feature);
    build(human_list);
    const auto h = row(human);
    require(find(30230, human) < 0 && hit(h.home, 2) == -1, "Human fixture reacted to a right click");
    // All Draw has its own toggle and marks the selected competition/day,
    // including when individual Match Results controls are disabled.
    clear_match_result_debugging(s);
    const auto all_draws_feature = 1u << unsigned(DebugFeature::all_draws);
    select_native_debugging_features(all_draws_feature);
    build(list);
    require(find(30030, 0) >= 0 && find(30230, a.fixture) < 0, "All Draw alone did not expose its own menu action");
    require(std::int32_t(invoke(0x1028bc90, {unsigned(find(30030, 0))})) == -11, "All Draw did not rebuild the list");
    require(debug_draw_day(s, a.fixture) && debug_draw_day(s, b.fixture), "All Draw did not select the whole competition/day");
    build(list);
    require(text(unsigned(find(30030, 0))).find("* ") != std::string::npos &&
            text(unsigned(row(a.fixture).versus)) == debug_text(s, DebugText::draw) &&
            text(unsigned(row(b.fixture).versus)) == debug_text(s, DebugText::draw), "All Draw selection was not visible");
    select_native_debugging_features(result_feature | all_draws_feature);
    build(list);
    right_click(overlays(row(a.fixture))[0]);
    build(list);
    require(effective_debug_match_result(s, a.fixture) == ForcedMatchResult::home_win &&
            effective_debug_match_result(s, b.fixture) == ForcedMatchResult::draw &&
            ink(unsigned(row(a.fixture).home)) == green, "Individual choice did not override All Draw");
    require(std::int32_t(invoke(0x1028bc90, {unsigned(find(30030, 0))})) == -11, "All Draw cancellation did not rebuild");
    require(!debug_draw_day(s, a.fixture) && !debug_draw_day(s, b.fixture) &&
            debug_match_result(s, a.fixture) == ForcedMatchResult::home_win, "Canceling All Draw erased an individual choice");
    build(list);
    const auto stale_all_draws = unsigned(find(30030, 0));
    select_native_debugging_features(result_feature);
    require(std::int32_t(invoke(0x1028bc90, {stale_all_draws})) == 0 && !debug_draw_day(s, a.fixture),
            "Disabled All Draw accepted a stale action");
    require(debug_text(9, DebugText::all_draws) == "Todos Empatam", "Portuguese All Draw label differs");
    require(s.memory.copy_out(c.toc(-0x7b90), 0x10) == rng, "Fixture-list results changed RNG");
}
}
// What a well-formed options layout must satisfy: everything inside the canvas, the panel holding its
// parts, every group once with the rows its grid has room for, and nothing on a page overlapping.
std::string layout_problem(const DebugOptionsLayout& layout, GuiCanvas canvas, std::span<const unsigned> rows,
                           const DebugOptionsMetrics& metrics = {}) {
    const auto inside = [](ScreenRect inner, ScreenRect outer) {
        return inner.left >= outer.left && inner.top >= outer.top && inner.right <= outer.right && inner.bottom <= outer.bottom;
    };
    const auto overlap = [](ScreenRect a, ScreenRect b) {
        return a.left < b.right && b.left < a.right && a.top < b.bottom && b.top < a.bottom;
    };
    const ScreenRect whole{0, 0, canvas.width, canvas.height};
    if (!inside(layout.panel, whole)) return "panel outside the canvas";
    for (auto part : {layout.title, layout.body, layout.footer, layout.all_on, layout.all_off})
        if (!inside(part, layout.panel)) return "part outside the panel";
    if (layout.pages > 1)
        for (auto part : {layout.previous, layout.page, layout.next})
            if (!inside(part, layout.panel) || part.width() <= 0) return "pager outside the panel";
    if (layout.footer.width() <= 0 || layout.footer.height() <= 0) return "empty footer";
    std::vector<ScreenRect> fixed{layout.title, layout.body, layout.all_on, layout.all_off, layout.footer};
    if (layout.pages > 1) fixed.insert(fixed.end(), {layout.previous, layout.page, layout.next});
    for (unsigned i = 0; i < fixed.size(); ++i)
        for (unsigned j = i + 1; j < fixed.size(); ++j)
            if (overlap(fixed[i], fixed[j])) return "controls overlap";
    if (layout.placements.size() != layout.pages) return "page count";
    std::vector<unsigned> seen(rows.size());
    for (const auto& page : layout.placements) {
        for (unsigned i = 0; i < page.size(); ++i) {
            const auto& p = page[i];
            if (p.group >= rows.size()) return "unknown group";
            ++seen[p.group];
            if (!inside(p.heading, layout.body) || !inside(p.grid, layout.body)) return "group outside the body";
            if (p.grid.height() != int(rows[p.group]) * metrics.row || p.heading.height() != metrics.heading)
                return "row spacing";
            if (p.heading.bottom != p.grid.top) return "heading apart from its rows";
            for (unsigned j = i + 1; j < page.size(); ++j)
                if (overlap(p.heading, page[j].heading) || overlap(p.heading, page[j].grid) ||
                    overlap(p.grid, page[j].heading) || overlap(p.grid, page[j].grid)) return "groups overlap";
        }
    }
    for (auto count : seen) if (count != 1) return "a group is missing or repeated";
    return {};
}

int main(int argc, char** argv) {
    try {
        require(argc == 2 || argc == 3, "Expected a private UI snapshot and optionally a match-day snapshot");
        auto state = std::make_unique<State>();
        load_match_snapshot(*state, argv[1]);
        auto& s = *state;
        Clock clock;
        s.code_fragment_observer = &clock;
        guest::Cpu c(s);
        c.set(2, c.u32(c.sp() + 0x14));
        const auto ui = c.toc(-0x7bb0);
        c.put16(ui + 0x2c58, 0);
        s.budget = s.executed + 100000000;
        const auto invoke = [&](unsigned entry, std::initializer_list<unsigned> args) {
            if (entry == 0x1033d030) {
                // As in the real screen monitor, discard the previous controls
                // before rebuilding the callback (including pitch shapes).
                c.set(3, c.u32(ui + 0x24 + c.i16(ui + 0x2c58) * 0x2c0));
                c.call(0x10719560, 0xfffffff0);
            }
            unsigned reg = 3;
            for (auto value : args) c.set(reg++, value);
            try { c.call(entry, 0xfffffff0); }
            catch (const std::exception& error) {
                throw std::runtime_error(hex_address(entry) + ": " + error.what());
            }
            require(s.pc == 0xfffffff0, "UI operation failed to return");
        };
        const auto page = [&] { return c.u32(ui + 0x24 + c.i16(ui + 0x2c58) * 0x2c0); };
        const auto control = [&](unsigned action, unsigned value) {
            const auto current = page();
            for (unsigned i = 0; i < c.u16(current + 0x129da6); ++i) {
                const auto item = current + 0xba864 + i * 0x17c;
                if (c.u32(item + 8) == action && c.u32(item + 0x48) == value) return i;
            }
            throw std::runtime_error("Missing control " + std::to_string(action));
        };
        const auto item_address = [&](unsigned index) { return page() + 0xba864 + index * 0x17c; };
        const auto grid_address = [&](unsigned index) { return page() + 4 + index * 0xbf0; };
        const auto control_grid = [&](unsigned index) { return grid_address(c.u16(item_address(index) + 0x72)); };
        const auto label_control = [&](std::string_view label, unsigned flags) {
            for (unsigned i = 0; i < c.u16(page() + 0x129da6); ++i) {
                const auto item = item_address(i);
                std::string text;
                for (unsigned j = 0; j < 200 && c.u8(item + 0x78 + j); ++j) text += char(c.u8(item + 0x78 + j));
                if (text == label && c.u32(item + 12) == flags) return i;
            }
            for (unsigned i = 0; i < c.u16(page() + 0x129da6); ++i) {
                const auto item = item_address(i);
                if (c.u32(item + 8) < 30000) continue;
                std::string text;
                for (unsigned j = 0; j < 200 && c.u8(item + 0x78 + j); ++j) text += char(c.u8(item + 0x78 + j));
                std::cerr << "Control " << c.u32(item + 8) << " flags=" << c.u32(item + 12) << " label=" << text << '\n';
            }
            throw std::runtime_error("Missing styled label: " + std::string(label) + " flags=" + std::to_string(flags));
        };
        const auto style = [&](unsigned index) {
            const auto item = item_address(index);
            return std::array<unsigned, 7>{c.u32(item + 12), c.u32(item + 0x38), c.u32(item + 0x3c),
                c.u16(item + 0x6a), c.u16(item + 0x6c), c.u16(item + 0x6e), c.u16(item + 0x70)};
        };
        const auto close_modal = [&] {
            // The production monitor consumes -9 after the action returns.
            // Reproduce its history pop without running the interactive loop.
            require(std::int32_t(c.r(3)) == -9, "Modal did not request close");
            const auto group = ui + c.i16(ui + 0x2c58) * 0x2c0;
            const auto node = c.u32(group + 0x10), previous = c.u32(node + 0x1f4);
            require(previous && c.u8(node + 0x10), "Expected a modal history frame");
            c.put32(group + 0x10, previous); c.put32(group + 0xc, previous);
            c.put32(previous + 0x1f8, 0);
            c.put16(group + 0x14, c.i16(group + 0x14) - 1);
            c.put16(group + 0x16, c.i16(group + 0x16) - 1);
            invoke(0x107735a0, {node});
        };
        // The Debugging Options screen pages its switches when the canvas cannot hold them: Previous and Next
        // are controls of the screen itself (30060 and 30061, present only when the page can move).
        const auto has_control = [&](unsigned action, unsigned value) {
            const auto current = page();
            for (unsigned i = 0; i < c.u16(current + 0x129da6); ++i) {
                const auto item = current + 0xba864 + i * 0x17c;
                if (c.u32(item + 8) == action && c.u32(item + 0x48) == value) return true;
            }
            return false;
        };
        const auto turn = [&](unsigned action) {
            invoke(0x1033f840, {control(action, 0)});
            invoke(0x1033d030, {});
        };
        const auto first_page = [&] { while (has_control(30060, 0)) turn(30060); };
        // The control of a switch's On/Off cell, wherever its page is.
        const auto feature_control = [&](unsigned feature, unsigned value) {
            first_page();
            for (unsigned turns = 0; turns < 8; ++turns) {
                if (has_control(30100 + feature, value)) return control(30100 + feature, value);
                if (!has_control(30061, 0)) break;
                turn(30061);
            }
            throw std::runtime_error("Switch " + std::to_string(feature) + " is not on any page");
        };
        initialize_native_debugging(false);
        const auto rng = s.memory.copy_out(c.toc(-0x7b90), 0x10);
        // Build the actual career menu, including the generated integration hook.
        invoke(0x102a1070, {4, 0});
        const auto sidebar = control(30020, 0);
        label_control(debug_text(s, DebugText::options_sidebar), 0x82);
        require(label_control("     " + std::string(debug_text(s, DebugText::toggle_options)), 2) == sidebar,
                "Debugging submenu lacks its options child");
        require(debug_text(9, DebugText::options_sidebar) == "Op\347\365es\nde Debugging", "Portuguese sidebar wrapping");
        const auto club = c.u32(c.toc(-0x7be8)) + 677 * 0x248;
        require(valid_debug_result_club(s, club), "Expected fixture club");
        invoke(0x102a54b0, {control(30026, 0)});
        invoke(0x1033d030, {});
        require(match_result_debugging(s).clubs.empty(), "Default club list was not empty");
        const bool portuguese_list = c.u8(c.r(2) - 0x35ac) == 9;
        label_control(portuguese_list ? "Nenhum time adicionado" : "No teams added", 1);
        label_control(portuguese_list ? "P\341gina 1 / 1" : "Page 1 / 1", 1);
        label_control(debug_text(s, DebugText::previous), 1);
        label_control(debug_text(s, DebugText::next), 1);
        label_control(debug_text(s, DebugText::options_sidebar), 0x82);
        invoke(0x1073edb0, {ui});
        const auto history_title = c.r(3);
        std::string list_title;
        for (unsigned i = 0; c.u8(history_title + i); ++i) list_title += char(c.u8(history_title + i));
        require(list_title == debug_text(s, DebugText::forced_clubs), "List cleared its history title");
        invoke(0x10192250, {});
        const auto browser_continent = c.r(3);
        invoke(0x101922e0, {});
        const auto browser_nation = c.r(3);
        require(browser_continent && browser_nation && c.u32(browser_nation + 0x74) == browser_continent,
                "Career fixture lacks the original default nation/continent");
        invoke(0x1033f840, {control(30027, 0)});
        require(match_result_debugging(s).choosing_club, "Add did not arm the club browser");
        invoke(0x1073ef70, {ui, 0});
        require(c.r(3) == 1, "Add did not open the original Nations browser");
        invoke(0x1073ef70, {ui, 1});
        require(c.r(3) == browser_continent, "Add passed a nation as the continent filter");
        invoke(0x1073ef70, {ui, 10});
        require(c.r(3) == browser_nation, "Add lost the default nation for the clubs tab");
        invoke(0x10719560, {page()});
        invoke(0x10103850, {});
        unsigned nation_rows = 0;
        for (unsigned i = 0; i < c.u16(page() + 0x129da6); ++i) {
            const auto item = item_address(i);
            if (c.u32(item + 8) == 2005 && (c.u32(item + 12) & 2) && c.u32(item + 0x48)) ++nation_rows;
        }
        require(nation_rows > 0, "Add opened an empty Nations & Clubs browser");
        require(match_result_debugging(s).choosing_club, "Rendering the browser canceled club selection");
        // Browse to a club; selection is host-owned and survives unrelated history.
        invoke(0x1073f1e0, {ui, 0, club, 0});
        invoke(0x10719560, {page()});
        c.set(31, club); c.set(26, ui);
        constexpr unsigned club_ink = 0x7c00, club_paper = 0x03e0;
        const auto header_colors = s.memory.copy_out(c.sp() + 0x6a, 4);
        c.put16(c.sp() + 0x6a, club_ink); c.put16(c.sp() + 0x6c, club_paper);
        require(append_forced_club_control(s), "Club selection header was not built");
        s.memory.copy_in(c.sp() + 0x6a, header_colors);
        const auto force_style = style(control(30029, club));
        require(force_style[3] == club_paper && force_style[6] == club_ink,
                "Force Results did not use the Apply for Job club colors");
        c.set(3, control(30029, club)); s.wr<4>(4128, return_sentinel);
        require(dispatch_best_xi_control(s), "Club toolbar did not accept Force Results");
        require(match_result_debugging(s).clubs.size() == 1 &&
            match_result_debugging(s).clubs.front().result == ForcedClubResult::win &&
            !match_result_debugging(s).choosing_club, "Added club did not default to victory or finish selection");
        invoke(0x1033d030, {});
        const auto selected_result_style = style(label_control(debug_text(s, DebugText::win), 1));
        const auto available_result_style = style(control(30302, club));
        invoke(0x100d0940, {club, 0, c.sp() + 0x200, 200, 1, 0});
        std::string club_name;
        for (unsigned i = 0; c.u8(c.sp() + 0x200 + i); ++i) club_name += char(c.u8(c.sp() + 0x200 + i));
        const auto club_name_style = style(label_control(club_name, 1));
        require(c.u8(control_grid(control(30028, club)) + 0xbb0) == 5, "Club table must have five columns");
        invoke(0x1076bb20, {0, 0xc0, 0, 0});
        require(available_result_style[6] == c.r(3), "Available result lacks tactics green text");
        require(selected_result_style[1] == 0x221 && selected_result_style[6] == c.u16(c.toc(-0x7bd0)),
                "Selected result lacks tactics yellow frame/text");
        require(club_name_style[6] == c.u16(c.toc(-0x7b10)), "Club name lacks tactics row-label color");
        invoke(0x1033f840, {control(30302, club)});
        require(match_result_debugging(s).clubs.front().result == ForcedClubResult::loss, "Loss cell was ignored");
        invoke(0x1033d030, {});
        invoke(0x1033f840, {control(30028, club)});
        invoke(0x1033d030, {});
        require(match_result_debugging(s).confirm_removal == club, "Remove skipped confirmation");
        invoke(0x1033f840, {control(30305, 0)});
        require(match_result_debugging(s).clubs.size() == 1 && !match_result_debugging(s).confirm_removal,
                "Cancel removed the club or retained confirmation");
        invoke(0x1033d030, {});
        invoke(0x1033f840, {control(30028, club)});
        invoke(0x1033d030, {});
        c.put16(ui + 0x25819e, 40);
        invoke(0x1033f840, {0xffffffffu});
        require(c.r(3) == std::uint32_t(-11) && !match_result_debugging(s).confirm_removal &&
                match_result_debugging(s).clubs.size() == 1, "Escape did not cancel removal in place");
        c.put16(ui + 0x25819e, 0);
        invoke(0x1033d030, {});
        invoke(0x1033f840, {control(30028, club)});
        invoke(0x1033d030, {});
        invoke(0x1033f840, {control(30304, 0)});
        require(match_result_debugging(s).clubs.empty(), "Confirm did not remove the club");
        // Page boundaries and deletion of the last row on the last page must
        // not strand the user on a blank page or change another club's rule.
        auto& club_choices = match_result_debugging(s);
        const auto all_clubs = c.u32(c.toc(-0x7be8));
        for (unsigned id = 0; id < c.u32(c.toc(-0x7b50)) && club_choices.clubs.size() < 13; ++id)
            if (valid_debug_result_club(s, all_clubs + id * 0x248)) add_debug_result_club(s, all_clubs + id * 0x248);
        require(club_choices.clubs.size() == 13, "Missing clubs for pagination fixture");
        invoke(0x1033d030, {});
        label_control(portuguese_list ? "P\341gina 1 / 2" : "Page 1 / 2", 1);
        label_control(portuguese_list ? "1-12 de 13 times" : "1-12 of 13 teams", 1);
        label_control(debug_text(s, DebugText::previous), 1);
        invoke(0x1033f840, {control(30307, 0)});
        invoke(0x1033d030, {});
        require(club_choices.club_page == 1, "Next did not change pages");
        label_control(portuguese_list ? "P\341gina 2 / 2" : "Page 2 / 2", 1);
        label_control(portuguese_list ? "13 de 13 times" : "13 of 13 teams", 1);
        label_control(debug_text(s, DebugText::next), 1);
        const auto final_club = club_choices.clubs.back().club;
        invoke(0x1033f840, {control(30028, final_club)});
        invoke(0x1033d030, {});
        invoke(0x1033f840, {control(30304, 0)});
        invoke(0x1033d030, {});
        require(club_choices.clubs.size() == 12 && club_choices.club_page == 0,
                "Last-page deletion did not return to the remaining page");
        for (const auto& rule : club_choices.clubs)
            require(rule.result == ForcedClubResult::win && rule.club != final_club, "Removal altered another rule");
        club_choices.clubs.clear();
        invoke(0x1033d030, {});
        // The native history monitor interprets -2 as Back; -4 only redraws.
        invoke(0x1033f840, {control(30308, 0)});
        require(c.r(3) == std::uint32_t(-2), "Back did not request previous history");
        c.put16(ui + 0x25819e, 40);
        invoke(0x1033f840, {0xffffffffu});
        require(c.r(3) == std::uint32_t(-2), "Escape did not request previous history");
        c.put16(ui + 0x25819e, 41);
        invoke(0x1033f840, {0xffffffffu});
        require(c.r(3) == 0, "Unrelated options key reached the original preferences handler");
        c.put16(ui + 0x25819e, 0);
        // Reopen the sidebar on a clean screen before testing the existing modal.
        invoke(0x10719560, {page()});
        invoke(0x102a1070, {4, 0});
        invoke(0x102a54b0, {control(30020, 0)});
        invoke(0x1033d030, {});
        require(native_debugging_features() == 0, "Opening options changed preferences");
        const auto active_style = style(label_control(debug_text(s, DebugText::off), 1));
        const auto inactive_style = style(control(30100, 1));
        const auto footer_style = style(control(40, 0));
        const auto cancel_grid = control_grid(control(40, 0));
        require(c.u8(cancel_grid + 0xbb0) == 2, "Footer must use the original two-column grid");
        for (unsigned feature = 0; feature < debug_feature_count; ++feature) {
            const auto index = feature_control(feature, 1);
            invoke(0x1033f840, {index});
            require(native_debugging_features() == 0, "Unconfirmed option leaked");
            invoke(0x1033d030, {});
        }
        invoke(0x1033f840, {control(40, 0)});
        close_modal();
        require(native_debugging_features() == 0, "Cancel saved changes");
        invoke(0x1033ce70, {4});
        invoke(0x1033d030, {});
        invoke(0x1033f840, {feature_control(unsigned(DebugFeature::remove_sale_clause), 1)});
        invoke(0x1033d030, {});
        invoke(0x1033f840, {feature_control(unsigned(DebugFeature::force_purchase), 1)});
        invoke(0x1033d030, {});
        invoke(0x1033f840, {control(41, 0)});
        close_modal();
        const auto selected_features = (1u << unsigned(DebugFeature::remove_sale_clause)) |
                                       (1u << unsigned(DebugFeature::force_purchase));
        require(native_debugging_features() == selected_features, "OK lost independent options");
        // Ordinary Game Options must not turn a partial selection into all-on.
        invoke(0x1033ce70, {1});
        invoke(0x1033d030, {});
        require(style(label_control(debug_text(s, DebugText::on), 1)) == active_style &&
                style(control(30001, 0)) == inactive_style && style(control(40, 0)) == footer_style,
                "Debugging modal must use Game Settings cell, selection and footer styles");
        c.set(3, control(41, 0));
        require(!dispatch_debugging_option(s), "Ordinary preferences must retain their original handler");
        require(native_debugging_features() == selected_features, "Game Options flattened selection");
        invoke(0x1073ef70, {ui, 0});
        invoke(0x1013f4d0, {c.r(3), 1});
        c.set(3, std::uint32_t(-9));
        close_modal();
        invoke(0x1033ce70, {4});
        invoke(0x1033d030, {});
        invoke(0x1033f840, {control(39, 0)});
        invoke(0x1033d030, {});
        invoke(0x1033f840, {control(41, 0)});
        close_modal();
        require(native_debugging_features() == 0, "Reset did not disable features");
        {
            // The options screen's geometry, then the screen itself at the classic canvas and at wide ones.
            constexpr std::array<unsigned, 5> group_rows{4, 2, 5, 4, 4};
            for (const auto canvas : {GuiCanvas{800, 600}, GuiCanvas{1024, 768}, GuiCanvas{1280, 720}, GuiCanvas{1920, 1080},
                                      GuiCanvas{3200, 1800}}) {
                const auto layout = debug_options_layout(canvas, group_rows);
                const auto problem = layout_problem(layout, canvas, group_rows);
                require(problem.empty(), ("Options layout " + std::to_string(canvas.width) + "x" +
                                         std::to_string(canvas.height) + ": " + problem).c_str());
                const bool wide = canvas.width >= 1200;
                require(layout.columns == (wide ? 2u : 1u), "Options layout column count");
                require(layout.pages == (layout.columns == 1 ? 2u : 1u), "Options layout page count");
            }
            // The check must fail when the layout is wrong: a mutant with two groups on top of each other,
            // one with a missing group and one whose rows are squeezed.
            {
                auto layout = debug_options_layout(GuiCanvas{1920, 1080}, group_rows);
                require(layout_problem(layout, GuiCanvas{1920, 1080}, group_rows).empty(), "Unmutated layout");
                auto overlapping = layout;
                overlapping.placements[0][1].grid = overlapping.placements[0][0].grid;
                require(!layout_problem(overlapping, GuiCanvas{1920, 1080}, group_rows).empty(), "Overlap mutant accepted");
                auto missing = layout;
                missing.placements[0].pop_back();
                require(!layout_problem(missing, GuiCanvas{1920, 1080}, group_rows).empty(), "Missing-group mutant accepted");
                auto squeezed = layout;
                squeezed.placements[0][0].grid.bottom -= 10;
                require(!layout_problem(squeezed, GuiCanvas{1920, 1080}, group_rows).empty(), "Squeezed-row mutant accepted");
                auto outside = layout;
                outside.panel.right = 5000;
                require(!layout_problem(outside, GuiCanvas{1920, 1080}, group_rows).empty(), "Outside-panel mutant accepted");
            }
            static GuiCanvas probed{800, 600};
            adaptive_gui_canvas_probe = [](const State&) { return probed; };
            struct DropProbe { ~DropProbe() { adaptive_gui_canvas_probe = nullptr; } } drop_probe;
            const auto open_options = [&] {
                invoke(0x1033ce70, {4});
                invoke(0x1033d030, {});
            };
            const auto finish = [&](unsigned action) {
                invoke(0x1033f840, {control(action, 0)});
                close_modal();
            };
            for (const auto canvas : {GuiCanvas{800, 600}, GuiCanvas{1280, 720}, GuiCanvas{1920, 1080}}) {
                probed = canvas;
                const bool classic = canvas.width == 800;
                select_native_debugging_features(0);
                open_options();
                // Every switch has an Off and an On cell on some page, and the pages are exactly what the layout said.
                first_page();
                unsigned reachable = 0, pages = 1;
                for (;; ++pages) {
                    for (unsigned feature = 0; feature < debug_feature_count; ++feature)
                        if (has_control(30100 + feature, 0) || has_control(30100 + feature, 1)) {
                            // The selected choice is a plain cell; only the other one is a control.
                            require(has_control(30100 + feature, 0) != has_control(30100 + feature, 1), "A switch lacks its choice");
                            require(!(reachable & (1u << feature)), "A switch is offered twice");
                            reachable |= 1u << feature;
                        }
                    if (!has_control(30061, 0)) break;
                    turn(30061);
                }
                require(reachable == all_debug_features, "Some debugging switch is not reachable");
                require(pages == (classic ? 2u : 1u), "Options page count at this canvas");
                require(has_control(30062, 0) && has_control(39, 0) && has_control(40, 0) && has_control(41, 0),
                        "All On, All Off, Cancel or OK missing");
                require(has_control(30061, 0) == false && (classic ? has_control(30060, 0) : !has_control(30060, 0)),
                        "Pager controls at the last page");
                // Every switch toggles exactly its own feature, and only OK commits it.
                for (unsigned feature = 0; feature < debug_feature_count; ++feature) {
                    invoke(0x1033f840, {feature_control(feature, 1)});
                    invoke(0x1033d030, {});
                    require(native_debugging_features() == 0, "A pending choice leaked before OK");
                    first_page();
                    finish(40);
                    require(native_debugging_features() == 0, "Cancel committed a choice");
                    open_options();
                    invoke(0x1033f840, {feature_control(feature, 1)});
                    invoke(0x1033d030, {});
                    first_page();
                    finish(41);
                    require(native_debugging_features() == (1u << feature), "A switch toggled the wrong feature");
                    // Off again.
                    open_options();
                    invoke(0x1033f840, {feature_control(feature, 0)});
                    invoke(0x1033d030, {});
                    first_page();
                    finish(41);
                    require(native_debugging_features() == 0, "Off did not turn the feature off");
                    open_options();
                }
                // All On and All Off act on the pending choice.
                invoke(0x1033f840, {control(30062, 0)});
                invoke(0x1033d030, {});
                require(native_debugging_features() == 0, "All On committed without OK");
                first_page();
                finish(41);
                require(native_debugging_features() == all_debug_features, "All On did not enable every switch");
                open_options();
                invoke(0x1033f840, {control(39, 0)});
                invoke(0x1033d030, {});
                first_page();
                finish(41);
                require(native_debugging_features() == 0, "All Off did not disable every switch");
                // The panel and every switch stay inside the canvas.
                open_options();
                for (unsigned i = 0; i < c.u16(page() + 0x129da6); ++i) {
                    const auto item = item_address(i);
                    if (c.u32(item + 8) < 30100 || c.u32(item + 8) >= 30100 + debug_feature_count) continue;
                    require(c.i32(item + 0x10) >= 0 && c.i32(item + 0x18) <= int(canvas.width) &&
                            c.i32(item + 0x14) >= 0 && c.i32(item + 0x1c) <= int(canvas.height),
                            "A switch cell lies outside the canvas");
                }
                first_page();
                finish(40);
            }
        }
        for (unsigned feature = 0; feature < debug_feature_count; ++feature) {
            select_native_debugging_features(1u << feature);
            for (unsigned other = 0; other < debug_feature_count; ++other)
                require(native_debugging_feature(DebugFeature(other)) == (other == feature), "Features are not independent");
        }
        require(s.memory.copy_out(c.toc(-0x7b90), 0x10) == rng, "Options changed RNG");
        for (unsigned language = 0; language < 11; ++language)
            for (auto text : {DebugText::options, DebugText::underdogs, DebugText::underdogs_max,
                             DebugText::repeat_goal, DebugText::red_cards, DebugText::injuries,
                             DebugText::extend_all_contracts, DebugText::force_purchase, DebugText::maximize_attributes,
                             DebugText::club_rule_pair_hint, DebugText::recover_player})
                require(!debug_text(language, text).empty(), "Missing translation");
        require(debug_text(9, DebugText::extend_all_contracts) == "Renovar Todos os Contratos por 1 Ano" &&
                debug_text(255, DebugText::extend_all_contracts) == "Extend All Contracts by 1 Year", "Collective renewal labels");
        require(debug_text(9, DebugText::force_purchase) == "For\347ar Compra", "Portuguese purchase label");
        require(debug_text(9, DebugText::recover_player) == "Recuperar Jogador" &&
                debug_text(0, DebugText::recover_player) == "Recover Player" &&
                debug_text(255, DebugText::recover_player) == "Recover Player", "Recover Player labels");
        std::uint32_t managed_club = 0;
        for (unsigned i = 0; i < c.u32(c.toc(-0x7b50)) && !managed_club; ++i) {
            const auto club = c.u32(c.toc(-0x7be8)) + i * 0x248;
            invoke(0x1014c490, {club, 0, 1});
            if (c.r(3)) managed_club = club;
        }
        require(managed_club, "Expected a managed club in career snapshot");
        invoke(0x1073f1e0, {ui, 7, 0, 0});
        const auto build_squad_controls = [&] {
            invoke(0x10719560, {page()});
            c.set(25, managed_club); c.set(31, ui); c.set(14, 195);
            append_best_xi_control(s);
        };
        select_native_debugging_features(1u << unsigned(DebugFeature::extend_all_contracts));
        build_squad_controls();
        const auto collective = control(30021, 0);
        require(label_control("     " + std::string(debug_text(s, DebugText::extend_all_contracts)), 2) == collective,
                "Collective renewal missing from squad popup");
        // The Lineup cell owns a popup with Best XI, Best U23 and B Team, in that order,
        // beside the Debugging cell; the actions are unique and the items share one menu.
        const auto text_of = [&](unsigned index) {
            std::string text;
            for (unsigned j = 0; j < 200 && c.u8(item_address(index) + 0x78 + j); ++j) text += char(c.u8(item_address(index) + 0x78 + j));
            return text;
        };
        const auto check_lineup_menu = [&](bool debugging_cell) {
            std::vector<unsigned> cells;
            for (unsigned i = 0; i < c.u16(page() + 0x129da6); ++i)
                if (text_of(i) == debug_text(s, DebugText::lineup) && c.u32(item_address(i) + 8) == 0) cells.push_back(i);
            require(cells.size() == 1, "Expected one Lineup drop-down cell");
            const auto best = control(30004, 0), u23 = control(30023, 0), b_team = control(30024, 0);
            require(best > cells[0] && u23 == best + 1 && b_team == u23 + 1, "Lineup items are out of order");
            require(text_of(best) == "     " + std::string(debug_text(s, DebugText::best_xi)) &&
                    text_of(u23) == "     " + std::string(debug_text(s, DebugText::best_u23)) &&
                    text_of(b_team) == "     " + std::string(debug_text(s, DebugText::b_team)), "Lineup item labels");
            for (auto index : {best, u23, b_team})
                require(c.u32(item_address(index) + 12) == 2, "Lineup item flags");
            require(control_grid(best) == control_grid(u23) && control_grid(u23) == control_grid(b_team) &&
                    control_grid(best) != control_grid(cells[0]), "Lineup items must share their own menu");
            unsigned best_count = 0;
            for (unsigned i = 0; i < c.u16(page() + 0x129da6); ++i) best_count += c.u32(item_address(i) + 8) == 30004;
            require(best_count == 1, "Best XI must only be a menu item");
            const auto lineup_grid = control_grid(cells[0]);
            require(c.u32(lineup_grid) == 334 && c.u32(lineup_grid + 8) == (debugging_cell ? 444u : 556u),
                    "Lineup cell geometry");
            if (debugging_cell) {
                const auto debugging = label_control(debug_text(s, DebugText::debugging), c.u32(item_address(cells[0]) + 12));
                require(c.u32(control_grid(debugging)) == 446 && c.u32(control_grid(debugging) + 8) == 556,
                        "Debugging cell geometry");
            }
        };
        check_lineup_menu(true);
        for (auto action : {30004u, 30023u, 30024u}) {
            // Menu items run the selection only when the original click flag is set.
            c.set(3, control(action, 0));
            const auto executed_before = s.executed;
            c.put32(item_address(c.r(3)) + 12, 0);
            require(dispatch_best_xi_control(s) && c.r(3) == 0 && s.executed == executed_before,
                    "Unclicked lineup item changed state");
            c.put32(item_address(control(action, 0)) + 12, 2);
        }
        // A control retained from an older frame cannot bypass a disabled toggle.
        select_native_debugging_features(0);
        c.set(3, collective);
        const auto executed = s.executed;
        require(dispatch_best_xi_control(s) && c.r(3) == 0 && s.executed == executed,
                "Stale collective control bypassed its toggle");
        build_squad_controls();
        control(30004, 0); // Best XI remains available with all debugging disabled.
        check_lineup_menu(false);
        for (unsigned i = 0; i < c.u16(page() + 0x129da6); ++i)
            require(c.u32(item_address(i) + 8) != 30021, "Disabled collective action remained visible");

        for (unsigned i = 0; i < c.u16(page() + 0x129da6); ++i)
            require(c.u32(item_address(i) + 8) != 30031, "Call Up appeared for an ordinary club");
        const auto domestic = managed_club;
        const auto nations = c.u32(c.toc(-0x7b04)), club_count = c.u32(c.toc(-0x7b50));
        const auto national = c.u32(c.toc(-0x7be8)) + (club_count - 2 * nations) * 0x248;
        const auto previous_manager = c.u32(national + 0x90), manager = c.u32(domestic + 0x90);
        c.put32(national + 0x90, manager);
        managed_club = national;
        build_squad_controls();
        const auto call_up = control(30031, 0);
        require(text_of(call_up) == "     " + std::string(debug_text(s, DebugText::call_up)) &&
                control_grid(call_up) == control_grid(control(30004, 0)),
                "Call Up must belong to Lineup with debugging disabled");
        invoke(0x1073f1e0, {ui, 0, national, 0});
        invoke(0x1073f1e0, {ui, 1, 0, 0});
        invoke(0x1073f1e0, {ui, 4, 1, 0});
        c.put32(item_address(call_up) + 12, 0); c.set(3, call_up);
        const auto unclicked = s.executed;
        require(dispatch_best_xi_control(s) && c.r(3) == 0 && s.executed == unclicked,
                "Unclicked Call Up changed state");
        c.put32(item_address(call_up) + 12, 2); c.set(3, call_up);
        require(dispatch_best_xi_control(s), "Managed-national Call Up did not dispatch");
        require(c.r(3) == std::uint32_t(-11), "Call Up did not request a squad refresh");
        const auto national_roster = s.memory.copy_out(national + 0x98, 200);
        c.put32(national + 0x90, 0); c.set(3, call_up);
        require(dispatch_best_xi_control(s) && c.r(3) == 0 &&
                s.memory.copy_out(national + 0x98, 200) == national_roster, "Unmanaged national team accepted Call Up");
        c.put32(national + 0x90, manager);
        // A stale national menu action dispatched on a domestic page must be a no-op.
        invoke(0x1073f1e0, {ui, 0, domestic, 0});
        invoke(0x1073f1e0, {ui, 4, 1, 0});
        const auto domestic_roster = s.memory.copy_out(domestic + 0x98, 200);
        c.set(3, call_up);
        require(dispatch_best_xi_control(s) && c.r(3) == 0 &&
                s.memory.copy_out(domestic + 0x98, 200) == domestic_roster, "Stale Call Up changed an ordinary club");
        c.put32(national + 0x90, previous_manager);
        managed_club = domestic;

        require(s.memory.copy_out(c.toc(-0x7b90), 0x10) == rng, "Collective renewal changed RNG");

        {
            // Remove Unhappiness: a squad action of the human's own club. Unhappiness is kept in the
            // primary contract (+0x50 grievances, +0x48 status bits, +0x44 happiness, +0x45/+0x46 counters).
            const auto ledger_now = c.u32(c.toc(-0x7b7c));
            const auto contract_of = [&](std::uint32_t person) {
                invoke(0x10082f20, {ledger_now, person});
                return std::uint32_t(c.r(3));
            };
            const auto roster = [&](std::uint32_t club) {
                std::vector<std::uint32_t> contracts;
                for (unsigned i = 0; i < 50; ++i) {
                    const auto person = c.u32(club + 0x98 + i * 4);
                    if (!person || !c.u32(person + 0x14)) continue;
                    const auto contract = contract_of(person);
                    if (contract && c.u32(contract + 4) == c.u32(club) &&
                        std::find(contracts.begin(), contracts.end(), contract) == contracts.end())
                        contracts.push_back(contract);
                }
                return contracts;
            };
            const auto bytes_of = [&](const std::vector<std::uint32_t>& contracts) {
                std::vector<std::uint8_t> all;
                for (auto contract : contracts) {
                    const auto row = s.memory.copy_out(contract, 0x68);
                    all.insert(all.end(), row.begin(), row.end());
                }
                return all;
            };
            select_native_debugging_features(1u << unsigned(DebugFeature::remove_unhappiness));
            const auto squad = roster(managed_club);
            require(squad.size() >= 8, "Expected a managed squad with at least eight contracts");
            std::uint32_t rival = 0;
            for (unsigned i = 0; i < c.u32(c.toc(-0x7b50)) && !rival; ++i) {
                const auto club = c.u32(c.toc(-0x7be8)) + i * 0x248;
                invoke(0x1014c490, {club, 0, 1});
                if (club != managed_club && !c.r(3) && roster(club).size() >= 3) rival = club;
            }
            require(rival, "Expected an AI club with contracts");
            const auto rivals = roster(rival);
            const auto open_squad = [&](std::uint32_t club) {
                invoke(0x1073f1e0, {ui, 0, club, 0});
                invoke(0x1073f1e0, {ui, 1, 0, 0});
                invoke(0x1073f1e0, {ui, 4, 1, 0});
            };
            const auto remove_unhappiness = [&](std::uint32_t club, bool clicked) {
                // The entry is only built for the human's own squad; an AI club can only be reached with a
                // control retained from that screen.
                open_squad(managed_club);
                build_squad_controls();
                const auto index = control(30035, 0);
                require(text_of(index) == "     " + std::string(debug_text(s, DebugText::remove_unhappiness)),
                        "Remove Unhappiness label in the squad menu");
                c.put32(item_address(index) + 12, clicked ? 2 : 0);
                open_squad(club);
                c.set(3, index);
                require(dispatch_best_xi_control(s), "Remove Unhappiness control was not claimed");
                return std::int32_t(c.r(3));
            };
            const auto random_now = s.memory.copy_out(c.toc(-0x7b90), 0x10);
            // The real squad first: it may hold unhappy players already. Running the action twice must leave
            // the second pass without effect (a happy squad is unchanged).
            require(remove_unhappiness(managed_club, true) == -11, "Remove Unhappiness did not request a refresh");
            const auto settled = bytes_of(squad);
            for (auto contract : squad) {
                require(c.u32(contract + 0x50) == 0 && !(c.u32(contract + 0x48) & 0x800) &&
                        std::int8_t(c.u8(contract + 0x44)) >= 0, "Squad still holds unhappiness after the action");
            }
            require(remove_unhappiness(managed_club, true) == -11 && bytes_of(squad) == settled,
                    "A happy squad was changed");
            // Plant each kind of unhappiness in different players, with neighbours that must not move.
            const auto plant = [&](std::uint32_t contract, std::uint32_t reasons, std::uint32_t status, int happiness,
                                   int run, int grievances) {
                c.put32(contract + 0x50, reasons); c.put32(contract + 0x48, status);
                c.put8(contract + 0x44, std::uint32_t(happiness)); c.put8(contract + 0x45, run); c.put8(contract + 0x46, grievances);
            };
            // Players 0..4 are unhappy in one or more ways; 5 and 6 carry other state that is not unhappiness.
            c.put8(squad[0] + 0x4c, 0x13); c.put8(squad[0] + 0x54, 1);
            plant(squad[0], 0x00000105, 0x880, -50, 7, 2);        // grievances, unsettled plus another mark
            plant(squad[1], 0, 0, -100, 0, 0);                    // only a very low happiness
            plant(squad[2], 0, 0x800, 30, 0, 0);                  // only the unsettled mark
            plant(squad[3], 0x20000000, 0, 40, 3, 1);            // grievance with a good happiness
            plant(squad[4], 0, 0x40080, 5, 4, 0);                 // not unhappy: a press mark and a low but positive happiness
            plant(squad[5], 0, 0, 0, 0, 0);                       // happiness 0 is not negative
            c.put8(squad[6] + 0x4c, 0xf0); c.put8(squad[6] + 0x54, 1); c.put8(squad[6] + 0x60, 0x23);
            const auto rival_before = [&] {
                plant(rivals[0], 0x00000105, 0x800, -50, 7, 2);
                plant(rivals[1], 0, 0x800, -100, 0, 0);
                return bytes_of(rivals);
            }();
            const auto before = bytes_of(squad);
            const auto rows = [&](unsigned n) { return s.memory.copy_out(squad[n], 0x68); };
            const auto first_before = rows(0), fifth_before = rows(4), sixth_before = rows(5), seventh_before = rows(6);
            // Not clicked: the control is only a hover.
            require(remove_unhappiness(managed_club, false) == 0 && bytes_of(squad) == before, "Unclicked action changed the squad");
            // An AI club is refused (its squad is shown, the action is not offered for it).
            require(remove_unhappiness(rival, true) == 0 && bytes_of(rivals) == rival_before && bytes_of(squad) == before,
                    "An AI club's squad was accepted");
            require(remove_unhappiness(managed_club, true) == -11, "Remove Unhappiness was refused for the human's squad");
            require(c.u32(squad[0] + 0x50) == 0 && c.u32(squad[0] + 0x48) == 0x80 && c.i8(squad[0] + 0x44) == 15 &&
                    c.u8(squad[0] + 0x45) == 0 && c.u8(squad[0] + 0x46) == 0 && c.u8(squad[0] + 0x4c) == 0x13 &&
                    c.u8(squad[0] + 0x54) == 1, "Grievances, unsettled mark, happiness and counters were not cleared as defined");
            require(c.i8(squad[1] + 0x44) == 15, "A very unhappy player was not lifted to the game's resolved level");
            require(c.u32(squad[2] + 0x48) == 0 && c.i8(squad[2] + 0x44) == 30, "Unsettled mark removal changed happiness");
            require(c.u32(squad[3] + 0x50) == 0 && c.i8(squad[3] + 0x44) == 40 && c.u8(squad[3] + 0x45) == 0 &&
                    c.u8(squad[3] + 0x46) == 0, "A grievance with good happiness was mishandled");
            require(rows(4) == fifth_before && rows(5) == sixth_before && rows(6) == seventh_before,
                    "A player without unhappiness was changed");
            for (unsigned n = 7; n < squad.size(); ++n) {
                const auto position = n * 0x68;
                require(std::equal(before.begin() + position, before.begin() + position + 0x68,
                                   bytes_of(squad).begin() + position), "A happy neighbour was changed");
            }
            require(bytes_of(rivals) == rival_before, "Another club's contracts were touched");
            require(s.memory.copy_out(c.toc(-0x7b90), 0x10) == random_now, "Remove Unhappiness changed RNG");
            // The original grievance bookkeeping: the same routine the game uses on a reset leaves its own store clean.
            require(first_before != rows(0), "The unhappy player did not change");
            // Switch off: the entry disappears and a stale control does nothing.
            plant(squad[1], 0x4, 0x800, -20, 3, 1);
            select_native_debugging_features(0);
            build_squad_controls();
            for (unsigned i = 0; i < c.u16(page() + 0x129da6); ++i)
                require(c.u32(item_address(i) + 8) != 30035, "Remove Unhappiness stayed visible with its switch off");
            const auto switched_off = bytes_of(squad);
            require(!remove_debug_unhappiness(s, managed_club) && bytes_of(squad) == switched_off,
                    "Disabled Remove Unhappiness changed the squad");
            select_native_debugging_features(1u << unsigned(DebugFeature::remove_unhappiness));
            require(!remove_debug_unhappiness(s, 0), "A missing club was accepted");
            for (unsigned language = 0; language < 11; ++language)
                require(!debug_text(language, DebugText::remove_unhappiness).empty(), "Missing Remove Unhappiness label");
            require(debug_text(9, DebugText::remove_unhappiness) == "Remover Insatisfa\347\365es" &&
                    debug_text(0, DebugText::remove_unhappiness) == "Remove Unhappiness" &&
                    debug_text(255, DebugText::remove_unhappiness) == "Remove Unhappiness", "Remove Unhappiness labels");
            open_squad(managed_club);
            select_native_debugging_features(0);
        }

        // Exercise the original proposal callbacks and transfer ledger on the
        // fixture. Advance the offer directly through its scheduled decisions.
        select_native_debugging_features(1u << unsigned(DebugFeature::force_purchase));
        const auto ledger = c.u32(c.toc(-0x7b7c)), transfers = c.u32(c.toc(-0x7b40));
        const auto active_manager = c.u32(ui + 4);
        require(active_manager && c.u32(active_manager + 0x24) == managed_club && ledger && transfers,
                "Missing transfer fixture context");
        const auto clubs = c.u32(c.toc(-0x7be8));
        std::uint32_t target = 0, seller = 0;
        for (unsigned i = 0; i < c.u32(c.toc(-0x7b50)) && !target; ++i) {
            const auto club = clubs + i * 0x248;
            if (club == managed_club || c.u32(club + 0x30) != c.u32(managed_club + 0x30)) continue;
            for (unsigned row = 0; row < 50 && !target; ++row) {
                const auto player = c.u32(club + 0x98 + row * 4);
                if (player && forced_purchase_seller(s, player) == club) {
                    target = player;
                    seller = club;
                }
            }
        }
        require(target && seller && !forced_purchase_seller(s, c.u32(managed_club + 0x98)),
                "Forced purchase must target an AI club's contracted player");
        const auto command = [&](unsigned handler, unsigned action) {
            invoke(0x10719560, {page()});
            c.put16(page() + 0x129da6, 1);
            c.put32(item_address(0) + 8, action);
            c.put32(item_address(0) + 12, 2);
            invoke(handler, {0});
            return std::int32_t(c.r(3));
        };
        invoke(0x1073f1e0, {ui, 0, target, 0});
        // Build the production submenu with the original hook's live frame;
        // the entire player page requires an unavailable QuickDraw port here.
        // Dispatch still uses the original command handler and production hook.
        select_native_debugging_features(1u << unsigned(DebugFeature::maximize_attributes));
        const auto profile = c.u32(target + 0x14);
        const auto before_max_staff = s.memory.copy_out(target, 0x70);
        const auto before_max_profile = s.memory.copy_out(profile, 0x48);
        const auto build_profile_debug_menu = [&](std::uint32_t person) {
            invoke(0x10719560, {page()});
            const auto frame = s.memory.copy_out(c.sp() + 0x6c, 6);
            const auto prior_staff = c.r(23);
            c.set(23, person);
            c.put16(c.sp() + 0x6c, 0xffff); // No existing top-level menu.
            c.put16(c.sp() + 0x6e, 100); c.put16(c.sp() + 0x70, 100);
            const auto saved_registers = s.registers; const auto saved_unique = s.unique; const auto saved_pc = s.pc;
            append_debug_contract_action(s);
            require(s.registers == saved_registers && s.unique == saved_unique && s.pc == saved_pc,
                    "Profile submenu changed its interrupted caller");
            c.set(23, prior_staff); s.memory.copy_in(c.sp() + 0x6c, frame);
        };
        build_profile_debug_menu(target);
        const auto maximize = control(30009, 0);
        require(text_of(maximize) == "     " + std::string(debug_text(s, DebugText::maximize_attributes)),
                "Missing Maximize Attributes in player Debugging menu");
        const auto ledger_slot = c.toc(-0x7b7c);
        c.put32(ledger_slot, 0); c.put32(target + 0x24, 0);
        build_profile_debug_menu(target);
        require(control(30009, 0) < c.u16(page() + 0x129da6), "Contractless player lost Maximize Attributes");
        c.put32(ledger_slot, ledger); c.put32(target + 0x24, seller);
        require(command(0x103a72c0, 30009) == -11 && c.u8(profile + 0x1b) == 20 &&
                c.u8(profile + 0x25) == 1 && c.u8(profile + 0x2d) == 1, "Player action did not maximize/minimize attributes");
        c.put8(profile + 0x1b, 7);
        select_native_debugging_features(1u << unsigned(DebugFeature::force_purchase));
        require(command(0x103a72c0, 30009) == 0 && c.u8(profile + 0x1b) == 7,
                "Stale disabled Maximize Attributes action changed the player");
        s.memory.copy_in(target, before_max_staff);
        s.memory.copy_in(profile, before_max_profile);
        {
            // Recover Player shares the player Debugging menu and appears only for a person who is
            // injured or suspended; its own switch gates it.
            const auto medical = c.u32(c.toc(-0x7ba0));
            const auto condition = [&](std::uint32_t person) { return c.u32(medical) + c.u32(person) * 0x24; };
            const auto has_action = [&](unsigned action) {
                for (unsigned i = 0; i < c.u16(page() + 0x129da6); ++i)
                    if (c.u32(item_address(i) + 8) == action) return true;
                return false;
            };
            const auto healthy_row = [&] { return c.u32(condition(target) + 0x10) == 0xffffffffu; };
            if (!healthy_row()) { invoke(0x1018b9f0, {medical, target, 0}); }
            require(healthy_row(), "Fixture player must start without an injury");
            select_native_debugging_features(1u << unsigned(DebugFeature::recover_player));
            build_profile_debug_menu(target);
            require(!has_action(30034), "Recover Player offered for a person with nothing to recover");
            const auto neighbours = [&] {
                std::vector<std::uint8_t> rows;
                for (const auto person : {target - 0x70, target + 0x70}) {
                    const auto row = s.memory.copy_out(condition(person), 0x24);
                    rows.insert(rows.end(), row.begin(), row.end());
                }
                return rows;
            };
            const auto untouched = neighbours();
            const auto untouched_target = s.memory.copy_out(target, 0x70);
            // Closing an injury files it in the original history files.
            TransferFiles files;
            std::filesystem::create_directories(files.path / "game");
            std::filesystem::create_directories(files.path / "working/Files");
            for (const auto name : {"staff history.tmp", "player stats history.tmp"}) {
                std::ofstream(files.path / "working/Files" / name, std::ios::binary).close();
                std::filesystem::resize_file(files.path / "working/Files" / name, 2 * 1024 * 1024);
            }
            s.platform = std::make_shared<NativePlatform>(files.path / "game", files.path / "working",
                                                         AudioMode::offline, EventMode::isolated);
            struct DropPlatform { State& state; ~DropPlatform() { state.platform.reset(); } } drop_platform{s};
            c.put32(c.sp() + 0x38, 0);
            invoke(0x10188cc0, {medical, 5, 0, target, 0, 1, 0xffffffffu, 0});
            require(c.r(3) != 0xffffffffu && !healthy_row(), "Could not injure the fixture player");
            const auto random = s.memory.copy_out(c.toc(-0x7b90), 0x10);
            build_profile_debug_menu(target);
            require(has_action(30034) && text_of(control(30034, 0)) == "     " + std::string(debug_text(s, DebugText::recover_player)),
                    "Missing Recover Player in the injured player's Debugging menu");
            select_native_debugging_features(1u << unsigned(DebugFeature::maximize_attributes));
            build_profile_debug_menu(target);
            require(!has_action(30034), "Recover Player shown with its switch off");
            require(command(0x103a72c0, 30034) == 0 && !healthy_row(), "Stale disabled Recover Player changed the player");
            select_native_debugging_features(1u << unsigned(DebugFeature::recover_player));
            require(command(0x103a72c0, 30034) == -11 && healthy_row() &&
                    c.u16(condition(target) + 8) == 10000 && c.u16(condition(target) + 10) == 10000,
                    "Recover Player did not end the injury");
            require(s.memory.copy_out(target, 0x70) == untouched_target && neighbours() == untouched &&
                    s.memory.copy_out(c.toc(-0x7b90), 0x10) == random, "Recover Player changed another record or the RNG");
            require(command(0x103a72c0, 30034) == 0, "Recover Player acted on a recovered player");
            build_profile_debug_menu(target);
            require(!has_action(30034), "Recover Player offered again after recovery");
            // Suspension: a real match-count ban in the original discipline store.
            const auto discipline = c.u32(c.toc(-0x7aa4));
            const auto rule_slots = c.u32(discipline);
            unsigned group = 42;
            for (unsigned i = 0; i < 42 && group == 42; ++i) {
                const auto rules = c.u32(rule_slots + i * 4);
                if (rules && c.u8(rules + 4) == 0) group = i;
            }
            if (group == 42) {
                for (unsigned i = 0; i < 42 && group == 42; ++i)
                    if (!c.u32(rule_slots + i * 4)) group = i;
                require(group < 42, "No empty disciplinary rule slot");
                invoke(0x10773520, {0x24});
                const auto rules = c.r(3);
                invoke(0x105b7030, {rules, 0});
                c.put8(rules + 4, 0); c.put8(rules + 5, group);
                c.put32(rule_slots + group * 4, rules);
            }
            const auto rules = c.u32(rule_slots + group * 4);
            const auto banned = [&] {
                invoke(0x105bc600, {rules, target, 0, 0, 0});
                return c.r(3) != 0;
            };
            invoke(0x10773520, {128});
            const auto scratch = c.r(3);
            s.memory.copy_in(scratch, std::vector<std::uint8_t>(24, 0xff));
            c.put8(scratch, 3); c.put8(scratch + 1, 0); c.put16(scratch + 2, 0xffff);
            s.memory.copy_in(scratch + 4, s.memory.copy_out(c.toc(-0x7b80), 5));
            c.put8(scratch + 0xc, group);
            c.put32(scratch + 0x14, 0x40000000);
            invoke(0x100dfb30, {discipline, c.u32(target), scratch});
            require(banned() && healthy_row(), "Could not suspend the fixture player");
            const auto fit_row = s.memory.copy_out(condition(target), 0x24);
            const auto untouched_neighbours = neighbours();
            build_profile_debug_menu(target);
            require(has_action(30034), "Recover Player missing for a suspended player");
            require(command(0x103a72c0, 30034) == -11 && !banned() && healthy_row() &&
                    s.memory.copy_out(condition(target), 0x24) == fit_row && neighbours() == untouched_neighbours,
                    "Recover Player did not clear only the suspension");
            // Injured and suspended together: both end.
            invoke(0x100dfb30, {discipline, c.u32(target), scratch});
            c.put32(c.sp() + 0x38, 0);
            invoke(0x10188cc0, {medical, 5, 0, target, 0, 1, 0xffffffffu, 0});
            require(banned() && !healthy_row(), "Could not injure and suspend the fixture player");
            require(command(0x103a72c0, 30034) == -11 && !banned() && healthy_row(),
                    "Recover Player did not end both the injury and the suspension");
            require(neighbours() == untouched_neighbours && s.memory.copy_out(c.toc(-0x7b90), 0x10) == random,
                    "Combined recovery changed another record or the RNG");
            select_native_debugging_features(1u << unsigned(DebugFeature::force_purchase));
        }
        require(command(0x103a72c0, 30025) == -4, "Force Purchase did not open the original proposal screen");
        invoke(0x1073ef70, {ui, forced_proposal_seller_slot});
        require(c.r(3) == c.u32(seller) + 1, "Proposal did not retain its original seller");
        invoke(0x10719560, {page()});
        invoke(0x10417b60, {});
        invoke(0x10148930, {c.u32(c.toc(-0x7a54)), 0});
        const auto draft = c.u32(c.r(3));
        require(draft && c.u32(draft + 12) == c.u32(target), "Original proposal did not create a draft");
        c.put32(draft + 0x20, 0);
        require(command(0x1041b600, 1) == -4, "Proposal did not open normal confirmation");
        invoke(0x1073ef70, {ui, forced_confirmation_seller_slot});
        require(c.r(3) == c.u32(seller) + 1, "Confirmation lost the force flag");
        const auto revision = c.u32(transfers + 0x98c);
        require(command(0x1041f3a0, 3) == -10, "Confirmation did not submit the original proposal");
        const auto& marked = forced_purchases(static_cast<const State&>(s)).offers;
        require(marked.size() == 1, "Expected only the selected proposal to be forced");
        const auto& [id, purchase] = *marked.begin();
        const auto offer = purchase.offer;
        require(purchase.revision == revision && c.u32(offer + 0x20) == 0 && !c.u8(offer + 0x34),
                "Submitted bid must keep its zero fee and original pending state");
        invoke(0x103eb440, {transfers, id});
        const auto link = c.r(3);
        const auto compare_transfer = [&](unsigned entry, std::initializer_list<unsigned> args, unsigned results) {
            const auto registers = s.registers;
            const auto unique = s.unique;
            const auto pc = s.pc;
            const bool previous_contract = routine_check_native_contract;
            unsigned reg = 3;
            for (const auto value : args) c.set(reg++, value);
            c.set_lr(0xfffffff0);
            routine_check_native_contract = true;
            const auto differences = compare_routine_call(s, entry,
                transfer_manager_native_run_original, transfer_manager_native_run_rewrite, results);
            routine_check_native_contract = previous_contract;
            s.registers = registers; s.unique = unique; s.pc = pc;
            for (const auto& difference : differences)
                std::cerr << "Forced purchase " << hex_address(entry) << ": " << difference << '\n';
            require(differences.empty(), "Forced-purchase native path differs from the existing original override");
        };
        require(forced_purchase_for_seller(s, offer, seller) &&
                !forced_purchase_for_seller(s, offer, managed_club), "Force flag leaked to the buying club");
        // The generated override skips the flag-only review guard as well as
        // the seller decision. Check both scheduler modes against that path.
        compare_transfer(0x103ee1b0, {transfers, link, 0}, 0);
        compare_transfer(0x103ee1b0, {transfers, link, 1}, 0);
        compare_transfer(0x103e4740, {transfers, link, 0}, 0);
        // A revised bid is an ordinary bid until explicitly marked again.
        c.put32(offer + 0x2c, revision + 1);
        require(!forced_purchase_for_seller(s, offer, seller), "Another offer revision retained forced acceptance");
        compare_transfer(0x103ee1b0, {transfers, link, 0}, 0);
        compare_transfer(0x103e4740, {transfers, link, 0}, 0);
        c.put32(offer + 0x2c, revision);
        invoke(0x103eb650, {transfers, link});
        require(c.u8(offer + 0x34) == 5, "CPU seller did not approve the zero-fee bid");

        invoke(0x100965c0, {link, target, 1, 11, 1});
        invoke(0x10719560, {page()});
        invoke(0x10096c60, {});
        invoke(0x101487d0, {c.u32(c.toc(-0x7a54)), 0});
        const auto terms = c.r(3);
        require(c.u32(terms) == c.u32(target) && c.u32(terms + 4) == c.u32(managed_club),
                "Negotiation did not draft a contract for the buyer");
        c.put32(terms + 8, 0);
        c.put32(terms + 12, 0);
        require(command(0x10097b70, 1) == -4 && command(0x1009e290, 0x14) == -10,
                "Original negotiation did not submit the contract");
        invoke(0x1007f720, {ledger, c.u32(target), c.u32(managed_club)});
        const auto pending = c.r(3);
        require(pending && forced_purchase_contract(s, pending), "Contract did not link to the forced bid");
        c.put32(pending + 0x74, revision + 1);
        require(!forced_purchase_contract(s, pending), "Another revision matched the forced bid");
        c.put32(pending + 0x74, revision);
        invoke(0x10081bd0, {ledger, target});
        require(c.u8(offer + 0x10) == 2 && c.u8(pending + 0x64) == 2 && c.u32(pending + 8) == 0,
                "Player did not accept the submitted zero-wage terms");
        require(c.u8(offer + 0x34) == 12, "Seller did not give final approval");
        // Reopen the seller's pending review: an already approved offer skips
        // this branch even without the force override.
        const auto seller_approved = c.u8(offer + 0x37);
        c.put8(offer + 0x37, 0);
        // This AI fixture's board does not normally object. Supply a refusal
        // at the external board boundary, retaining the real transfer checks.
        // It must be skipped only for the marked seller, never for the buyer
        // or a stale offer revision.
        const auto previous_probe = dispatch_probe;
        // Generated callers can enter a native owner directly. Decline that
        // one owner so both compared paths receive the scripted board verdict.
        RoutineChecker board_boundary([](std::uint32_t entry) { return entry != 0x101a0f50; }, 0, 0);
        const auto previous_checker = routine_checker;
        routine_checker = &board_boundary;
        dispatch_probe = [](State& state) {
            if (state.pc != 0x101a0f50) return false;
            guest::Cpu cpu(state);
            cpu.set(3, 0x800);
            cpu.finish();
            return true;
        };
        compare_transfer(0x103f9d60, {transfers, offer, seller, 0, 0}, RoutineChecker::result_r3);
        compare_transfer(0x103f9d60, {transfers, offer, managed_club, 0, 0}, RoutineChecker::result_r3);
        c.put32(offer + 0x2c, revision + 1);
        compare_transfer(0x103f9d60, {transfers, offer, seller, 0, 0}, RoutineChecker::result_r3);
        dispatch_probe = previous_probe;
        routine_checker = previous_checker;
        c.put32(offer + 0x2c, revision);
        c.put8(offer + 0x37, seller_approved);
        invoke(0x103e80e0, {transfers, offer, managed_club});
        require(c.u8(offer + 0x34) == 14, "Buyer did not confirm the transfer");
        TransferFiles files;
        std::filesystem::create_directories(files.path / "game");
        std::filesystem::create_directories(files.path / "working/Files");
        const auto history_file = files.path / "working/Files/staff history.tmp";
        std::ofstream(history_file, std::ios::binary).close();
        // The captured ledger refers to pre-existing statistics offsets.
        std::filesystem::resize_file(history_file, 2 * 1024 * 1024);
        const auto player_archive = files.path / "working/Files/player stats history.tmp";
        std::ofstream(player_archive, std::ios::binary).close();
        std::filesystem::resize_file(player_archive, 2 * 1024 * 1024);
        s.platform = std::make_shared<NativePlatform>(files.path / "game", files.path / "working",
                                                     AudioMode::offline, EventMode::isolated);
        invoke(0x103eb650, {transfers, link});
        s.platform.reset();
        require(c.u32(target + 0x24) == managed_club, "Confirmed player did not join the buyer");
        clear_forced_purchases(s);
        require(marked.empty() && !forced_purchase_for_seller(s, offer, seller), "Reset retained a forced purchase");

        const auto staff_feature = 1u << unsigned(DebugFeature::force_staff_signing);
        // The command harness submits through the original callbacks without
        // executing the screen monitor's post-submit history navigation.
        invoke(0x1073ee30, {ui});
        invoke(0x1073e4b0, {ui, 0x108b92a4, 0x108b929c, 0, 0, 0});
        require(c.r(3) == 1, "Could not open the original profile history for the staff fixture");
        // The normal hiring flow still requires a vacancy. Use the original
        // release action to make the fixture's assistant-manager slot available.
        if (const auto assistant = c.u32(managed_club + 0x94)) {
            invoke(0x1073f1e0, {ui, 0, assistant, 0});
            require(command(0x1009eb30, 1) == -10 && c.u32(managed_club + 0x94) == 0,
                    "Could not open an assistant-manager vacancy in the fixture");
        }
        require(debug_text(9, DebugText::force_staff_signing) == "For\347ar Assinar", "Portuguese staff signing label");
        const auto people = c.u32(c.toc(-0x7be0)), people_count = c.u32(c.toc(-0x7be4));
        const auto club_count_for_staff = c.u32(c.toc(-0x7b50));
        std::array<std::uint32_t, 11> staff_by_role{};
        for (unsigned i = 0; i < people_count; ++i) {
            const auto staff = people + i * 0x70, employer = c.u32(staff + 0x24);
            const auto role = c.u8(staff + 0x62);
            if ((role != 5 && role != 6 && role != 8 && role != 9 && role != 10) || c.u32(staff + 0x14) ||
                !c.u32(staff + 0x1c) || employer < clubs || (employer - clubs) % 0x248 ||
                (employer - clubs) / 0x248 >= club_count_for_staff || employer == managed_club ||
                c.u32(employer + 0x30) != c.u32(managed_club + 0x30)) continue;
            staff_by_role[role] = staff;
        }
        const auto candidate = staff_by_role[5];
        require(candidate, "Missing AI manager for staff-signing fixture");
        const auto employer = c.u32(candidate + 0x24);
        invoke(0x1073f1e0, {ui, 0, candidate, 0});
        const auto has_staff_action = [&] {
            for (unsigned i = 0; i < c.u16(page() + 0x129da6); ++i)
                if (c.u32(item_address(i) + 8) == 30033) return true;
            return false;
        };
        build_profile_debug_menu(candidate);
        require(!has_staff_action() && command(0x103a72c0, 30033) == 0, "Disabled staff action remained available");
        select_native_debugging_features(staff_feature);
        build_profile_debug_menu(target);
        require(!has_staff_action(), "A player showed the staff signing action");
        build_profile_debug_menu(c.u32(managed_club + 0x90));
        require(!has_staff_action(), "The user's own manager showed the staff signing action");
        for (const auto role : {5, 6, 8, 9, 10}) {
            require(staff_by_role[role], "Missing AI staff role for staff signing fixture");
            build_profile_debug_menu(staff_by_role[role]);
            require(has_staff_action() && text_of(control(30033, 0)) == "     " + std::string(debug_text(s, DebugText::force_staff_signing)),
                    "Eligible staff did not show Forçar Assinar in the profile menu");
        }

        require(command(0x103a72c0, 30033) == -4, "Forçar Assinar did not open the original negotiation");
        invoke(0x1073ef70, {ui, 8});
        require(c.r(3) == 6, "Forced negotiation with a manager selected a role other than assistant manager");
        const auto staff_signing = staff_signing_clubs(c, staff_signing_history(c, ui));
        require(staff_signing.employer == c.u32(employer) + 1 && staff_signing.buyer == c.u32(managed_club) + 1,
                "Negotiation did not retain both clubs");
        invoke(0x10719560, {page()});
        invoke(0x10096c60, {});
        invoke(0x101487d0, {c.u32(c.toc(-0x7a54)), 0});
        const auto staff_terms = c.r(3);
        require(c.u32(staff_terms) == c.u32(candidate) && c.u32(staff_terms + 4) == c.u32(managed_club),
                "Original staff negotiation did not create terms for the correct parties");
        require(command(0x1009a440, 0x1c) == -11, "Staff role control did not redraw");
        invoke(0x1073ef70, {ui, 8});
        require(c.r(3) == 6, "The forced staff negotiation allowed switching away from assistant manager");
        c.put32(staff_terms + 8, 0);
        c.put32(staff_terms + 12, 0);
        require(command(0x10097b70, 1) == -4, "Staff terms did not open the original confirmation");
        require(bool(staff_signing_clubs(c, staff_signing_history(c, ui))), "Confirmation lost the staff signing override");
        require(command(0x1009e290, 0x14) == -10, "Staff confirmation did not submit the original contract");
        invoke(0x1007f720, {ledger, c.u32(candidate), c.u32(managed_club)});
        const auto staff_pending = c.r(3);
        require(staff_pending && forced_staff_contract(s, staff_pending, candidate), "Staff offer did not retain its override");
        const auto staff_revision = c.u32(staff_pending + 0x74), staff_index = c.u32(staff_pending + 0x70);
        c.put32(staff_pending + 0x74, staff_revision + 1);
        require(!forced_staff_contract(s, staff_pending, candidate), "Another revision matched the staff signing override");
        c.put32(staff_pending + 0x74, staff_revision);
        c.put32(candidate + 0x24, managed_club);
        require(!forced_staff_contract(s, staff_pending, candidate), "An outdated employer matched the staff signing override");
        c.put32(candidate + 0x24, employer);
        const auto previous_employer_manager = c.u32(employer + 0x90);
        c.put32(employer + 0x90, c.u32(managed_club + 0x90));
        require(!forced_staff_contract(s, staff_pending, candidate), "A human-controlled employer matched the staff signing override");
        c.put32(employer + 0x90, previous_employer_manager);
        const auto staff_offer = c.u32(c.u32(transfers) + staff_index * 4);
        invoke(0x10081bd0, {ledger, candidate});
        require(c.u8(staff_pending + 0x64) == 2 && c.u32(staff_pending + 8) == 0 && c.u8(staff_offer + 0x34) == 12,
                "Staff did not accept the submitted terms through the original transfer flow");
        invoke(0x103e80e0, {transfers, staff_offer, managed_club});
        require(c.u8(staff_offer + 0x34) == 14, "Buyer did not confirm the staff transfer");
        invoke(0x103eb440, {transfers, staff_index});
        const auto staff_link = c.r(3);
        s.platform = std::make_shared<NativePlatform>(files.path / "game", files.path / "working",
                                                     AudioMode::offline, EventMode::isolated);
        invoke(0x103eb650, {transfers, staff_link});
        s.platform.reset();
        require(c.u32(candidate + 0x24) == managed_club && c.u8(candidate + 0x62) == 6 &&
                c.u32(managed_club + 0x94) == candidate && c.u32(employer + 0x90) != candidate,
                "Confirmed manager did not join the buyer as assistant manager");

        // With the toggle still enabled, an ordinary offer must use the ordinary decision.
        invoke(0x1073ee30, {ui});
        const auto ordinary_scout = staff_by_role[9];
        invoke(0x100960d0, {ordinary_scout, 0, 9, 1});
        require(!staff_signing_clubs(c, staff_signing_history(c, ui)), "A normal negotiation inherited a staff override");
        invoke(0x10719560, {page()});
        invoke(0x10096c60, {});
        require(command(0x10097b70, 1) == -4 && command(0x1009e290, 0x14) == -10,
                "Ordinary staff negotiation did not submit a proposal");
        invoke(0x1007f720, {ledger, c.u32(ordinary_scout), c.u32(managed_club)});
        require(c.r(3) && !forced_staff_contract(s, c.r(3), ordinary_scout) && forced_staff_signings(s).size() == 1,
                "A normal staff proposal was recorded as forced");
        clear_forced_staff_signings(s);
        require(forced_staff_signings(s).empty(), "Reset retained a forced staff signing");

        if (argc == 3) check_fixture_results(argv[2]);
        std::cout << "ok sidebar integration; independent options; deferred apply, cancel, reset; RNG; localization; forced purchase and staff signing"
                  << (argc == 3 ? "; fixture-list right-click match results" : "") << '\n';
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
