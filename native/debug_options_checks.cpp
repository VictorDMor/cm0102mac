#include "match_debugging.hpp"
#include "debug_localization.hpp"
#include "debug_match_results.hpp"
#include "debug_force_purchase.hpp"
#include "guest_abi.hpp"
#include "code_fragment_adapter.hpp"
#include "match_replay.hpp"
#include "platform.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <memory>
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
    require(s.memory.copy_out(c.toc(-0x7b90), 0x10) == rng, "Fixture-list results changed RNG");
}
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
        const auto club_name_style = style(control(30303, club));
        invoke(0x1076bb20, {0, 0xc0, 0, 0});
        require(available_result_style[6] == c.r(3), "Available result lacks tactics green text");
        require(selected_result_style[1] == 0x221 && selected_result_style[6] == c.u16(c.toc(-0x7bd0)),
                "Selected result lacks tactics yellow frame/text");
        require(club_name_style[6] == c.u16(c.toc(-0x7b10)), "Club name lacks tactics row-label color");
        invoke(0x1033f840, {control(30302, club)});
        require(match_result_debugging(s).clubs.front().result == ForcedClubResult::loss, "Loss cell was ignored");
        invoke(0x1033d030, {});
        invoke(0x1033f840, {control(30028, 0)});
        invoke(0x1033d030, {});
        require(match_result_debugging(s).removing_club, "Remove mode did not stay active");
        invoke(0x1033f840, {control(30303, club)});
        invoke(0x1033d030, {});
        require(match_result_debugging(s).confirm_removal == club, "Remove skipped confirmation");
        invoke(0x1033f840, {control(30305, 0)});
        require(match_result_debugging(s).clubs.size() == 1, "No removed the club");
        invoke(0x1033d030, {});
        invoke(0x1033f840, {control(30303, club)});
        invoke(0x1033d030, {});
        invoke(0x1033f840, {control(30304, 0)});
        require(match_result_debugging(s).clubs.empty(), "Yes did not remove the club");
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
            const auto index = control(30100 + feature, 1);
            invoke(0x1033f840, {index});
            require(native_debugging_features() == 0, "Unconfirmed option leaked");
            invoke(0x1033d030, {});
        }
        invoke(0x1033f840, {control(40, 0)});
        close_modal();
        require(native_debugging_features() == 0, "Cancel saved changes");
        invoke(0x1033ce70, {4});
        invoke(0x1033d030, {});
        invoke(0x1033f840, {control(30100 + unsigned(DebugFeature::remove_sale_clause), 1)});
        invoke(0x1033d030, {});
        invoke(0x1033f840, {control(30100 + unsigned(DebugFeature::force_purchase), 1)});
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
        for (unsigned feature = 0; feature < debug_feature_count; ++feature) {
            select_native_debugging_features(1u << feature);
            for (unsigned other = 0; other < debug_feature_count; ++other)
                require(native_debugging_feature(DebugFeature(other)) == (other == feature), "Features are not independent");
        }
        require(s.memory.copy_out(c.toc(-0x7b90), 0x10) == rng, "Options changed RNG");
        for (unsigned language = 0; language < 11; ++language)
            for (auto text : {DebugText::options, DebugText::underdogs, DebugText::underdogs_max,
                             DebugText::repeat_goal, DebugText::red_cards, DebugText::injuries,
                             DebugText::extend_all_contracts, DebugText::force_purchase})
                require(!debug_text(language, text).empty(), "Missing translation");
        require(debug_text(9, DebugText::extend_all_contracts) == "Renovar Todos os Contratos por 1 Ano" &&
                debug_text(255, DebugText::extend_all_contracts) == "Extend All Contracts by 1 Year", "Collective renewal labels");
        require(debug_text(9, DebugText::force_purchase) == "For\347ar Compra", "Portuguese purchase label");
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

        require(s.memory.copy_out(c.toc(-0x7b90), 0x10) == rng, "Collective renewal changed RNG");

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

        if (argc == 3) check_fixture_results(argv[2]);
        std::cout << "ok sidebar integration; independent options; deferred apply, cancel, reset; RNG; localization; forced purchase"
                  << (argc == 3 ? "; fixture-list right-click match results" : "") << '\n';
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
