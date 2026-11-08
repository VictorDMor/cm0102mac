#include "match_debugging.hpp"
#include "debug_localization.hpp"
#include "guest_abi.hpp"
#include "code_fragment_adapter.hpp"
#include "match_replay.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace cm::aot;
namespace {
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct Clock final : CodeFragmentBoundaryObserver {
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    bool import_fixture(State& s, const char* name) override {
        if (std::string_view(name) == "CarbonLib::GetDateTime") s.memory.write<4>(s.r<4>(12), 3000000000u);
        else if (std::string_view(name) == "CarbonLib::Microseconds") s.memory.write<8>(s.r<4>(12), 1000000000ULL);
        else if (std::string_view(name) == "CarbonLib::FSMakeFSSpec") s.wr<4>(12, std::uint32_t(-43));
        else return false;
        s.pc = s.r<4>(4128);
        return true;
    }
};
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Expected a private UI snapshot");
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
        const auto label_control = [&](std::string_view label, unsigned flags) {
            for (unsigned i = 0; i < c.u16(page() + 0x129da6); ++i) {
                const auto item = item_address(i);
                std::string text;
                for (unsigned j = 0; j < 200 && c.u8(item + 0x78 + j); ++j) text += char(c.u8(item + 0x78 + j));
                if (text == label && c.u32(item + 12) == flags) return i;
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
        require(label_control(debug_text(s, DebugText::options_sidebar), 2) == sidebar,
                "Sidebar must retain its localized line break");
        require(debug_text(9, DebugText::options_sidebar) == "Op\347\365es\nde Debugging", "Portuguese sidebar wrapping");
        invoke(0x102a54b0, {sidebar});
        invoke(0x1033d030, {});
        require(native_debugging_features() == 0, "Opening options changed preferences");
        const auto active_style = style(label_control(debug_text(s, DebugText::off), 1));
        const auto inactive_style = style(control(30100, 1));
        const auto footer_style = style(control(40, 0));
        const auto cancel_grid = page() + 4 + c.u16(item_address(control(40, 0)) + 0x72) * 0xbf0;
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
        invoke(0x1033f840, {control(41, 0)});
        close_modal();
        require(native_debugging_features() == (1u << unsigned(DebugFeature::remove_sale_clause)), "OK lost independent option");
        // Ordinary Game Options must not turn a partial selection into all-on.
        invoke(0x1033ce70, {1});
        invoke(0x1033d030, {});
        require(style(label_control(debug_text(s, DebugText::on), 1)) == active_style &&
                style(control(30001, 0)) == inactive_style && style(control(40, 0)) == footer_style,
                "Debugging modal must use Game Settings cell, selection and footer styles");
        c.set(3, control(41, 0));
        require(!dispatch_debugging_option(s), "Ordinary preferences must retain their original handler");
        require(native_debugging_features() == (1u << unsigned(DebugFeature::remove_sale_clause)), "Game Options flattened selection");
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
                             DebugText::repeat_goal, DebugText::red_cards, DebugText::injuries, DebugText::extend_all_contracts})
                require(!debug_text(language, text).empty(), "Missing translation");
        require(debug_text(9, DebugText::extend_all_contracts) == "Renovar Todos os Contratos por 1 Ano" &&
                debug_text(255, DebugText::extend_all_contracts) == "Extend All Contracts by 1 Year", "Collective renewal labels");
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
        // A control retained from an older frame cannot bypass a disabled toggle.
        select_native_debugging_features(0);
        c.set(3, collective);
        const auto executed = s.executed;
        require(dispatch_best_xi_control(s) && c.r(3) == 0 && s.executed == executed,
                "Stale collective control bypassed its toggle");
        build_squad_controls();
        control(30004, 0); // Best XI remains available with all debugging disabled.
        for (unsigned i = 0; i < c.u16(page() + 0x129da6); ++i)
            require(c.u32(item_address(i) + 8) != 30021, "Disabled collective action remained visible");
        std::cout << "ok sidebar integration; independent options; deferred apply, cancel, reset; RNG; localization\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
