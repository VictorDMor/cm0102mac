#include "match_debugger_factory.hpp"
#include "match_debugging.hpp"
#include "debug_localization.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <set>
#include <stdexcept>

namespace {
using namespace cm::aot;
using namespace cm::aot::match_debugger;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
std::uint32_t invoke(State& s, std::uint32_t entry, std::initializer_list<std::uint32_t> args) {
    unsigned reg = 3;
    for (auto value : args) s.wr<4>(reg++ * 4, value);
    s.wr<4>(4128, return_sentinel); s.pc = entry;
    try { run_until(s, return_sentinel); }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string(e.what()) + " entry=" + hex_address(entry) + " pc=" + hex_address(s.pc));
    }
    return s.r<4>(12);
}
std::array<std::uint32_t, 20> squad(State& s, std::uint32_t tactics, std::uint32_t club) {
    std::array<std::uint32_t, 20> result{};
    for (unsigned i = 0; i < result.size(); ++i) result[i] = invoke(s, 0x103af800, {tactics, club, i, 0});
    return result;
}
}
int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("Expected original program, read-only game and private root");
        require(debug_text(9, DebugText::best_xi) == "Melhor 11" && debug_text(0, DebugText::best_xi) == "Best XI",
                "Portuguese/English Best XI labels");
        require(debug_text(9, DebugText::add_goal) == "Adicionar Gol" && debug_text(0, DebugText::add_goal) == "Add Goal" &&
                debug_text(9, DebugText::goal) == "Gol" && debug_text(0, DebugText::goal) == "Goal", "Goal labels");
        for (unsigned language = 0; language < 11; ++language)
            for (auto key : {DebugText::best_xi, DebugText::add_goal, DebugText::goal, DebugText::on, DebugText::off})
                require(!debug_text(language, key).empty(), "Missing supported-language label");
        require(debug_text(255, DebugText::best_xi) == "Best XI", "Unknown language fallback");
        const auto root = std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        auto live = create_match({argv[1], argv[2], root}, {677, 5677, 12345, 2001, 194});
        auto& s = *live.state;
        s.budget = s.executed + 500000000;
        // The standalone match world does not normally need the career's
        // transfer/availability services; construct the real empty services.
        for (const auto spec : std::array<std::array<unsigned, 3>, 5>{{
                 {0x7bf0, 0x20, 0x102cb930}, {0x7b40, 0xa7c, 0x103df0d0},
                 {0x7ab8, 4, 0x100f84e0}, {0x7b24, 0x10, 0x100031b0},
                 {0x7aa4, 0x2c, 0x100de410}}}) {
            const auto object = invoke(s, 0x10773520, {spec[1]});
            invoke(s, spec[2], {object, 0});
            s.memory.write<4>(s.memory.read<4>(s.r<4>(8) - spec[0]), object);
        }
        std::cout << "Original availability services ready\n" << std::flush;
        const auto club = std::uint32_t(s.memory.read<4>(live.match + 0x7aac + 0x34));
        const auto formation = std::uint32_t(s.memory.read<4>(live.match + 0x7aac + 0x30));
        const auto tactics = std::uint32_t(s.memory.read<4>(s.memory.read<4>(s.r<4>(8) - 0x7a6c)));
        require(club && formation && tactics, "Missing original selection context");
        const auto before = squad(s, tactics, club);
        initialize_native_debugging(false);
        const auto executed = s.executed;
        require(!select_debug_best_xi(s, club, 0, formation, 0) && s.executed == executed && squad(s, tactics, club) == before,
                "Disabled Best XI changed selection");
        select_native_debugging(true);
        std::cout << "Selecting actual squad\n" << std::flush;
        const auto original_formation = s.memory.copy_out(formation, 0x18e0);
        const auto registers = s.registers;
        const auto unique = s.unique;
        const auto pc = s.pc;
        require(select_debug_best_xi(s, club, 0, formation, 0), "Best XI failed on an actual club");
        require(s.registers == registers && s.unique == unique && s.pc == pc, "Best XI clobbered caller");
        require(s.memory.copy_out(formation, 0x18e0) == original_formation, "Best XI changed chosen tactics");
        const auto first = squad(s, tactics, club);
        std::set<std::uint32_t> seen;
        for (auto staff : first) {
            require(staff && seen.insert(staff).second, "Incomplete or duplicate actual squad");
            const auto status = invoke(s, 0x10060c00, {staff, club, 0});
            require(!((status >= 1 && status <= 6) || status == 8 || status == 21), "Unavailable player selected");
        }
        require(invoke(s, 0x10117490, {std::uint32_t(s.memory.read<4>(first[0] + 0x14))}) & 1, "Starting goalkeeper is not a keeper");
        // A real fixture with five bench places, and no extra stale reserves.
        const auto fixture = invoke(s, 0x10773520, {0x54});
        s.memory.copy_in(fixture, s.memory.copy_out(s.memory.read<4>(live.match + 0x3118), 0x54));
        s.memory.write<1>(fixture + 0x4e, 0x53);
        require(select_debug_best_xi(s, club, 0, formation, fixture), "Fixture selection failed");
        const auto five = squad(s, tactics, club);
        for (unsigned i = 0; i < 20; ++i) require(bool(five[i]) == (i < 16), "Competition bench limit ignored");
        // A changed formation must be read at click time, not cached or replaced.
        const auto alternate = invoke(s, 0x10773520, {0x18e0});
        s.memory.copy_in(alternate, original_formation);
        for (unsigned i = 1; i < 11; ++i) s.memory.write<2>(alternate + 0x12e + i * 2, 0x240);
        require(select_debug_best_xi(s, club, 0, alternate, 0), "Alternate formation failed");
        require(squad(s, tactics, club) != first, "Formation did not affect selection");
        // Keep seven real eligible players: never duplicate one to invent an XI.
        const auto roster = s.memory.copy_out(club + 0x98, 200);
        for (unsigned i = 0; i < 50; ++i) s.memory.write<4>(club + 0x98 + i * 4, i < 7 ? first[i] : 0);
        require(select_debug_best_xi(s, club, 0, formation, 0), "Depleted club selection failed");
        seen.clear();
        for (auto staff : squad(s, tactics, club)) if (staff) require(seen.insert(staff).second, "Depleted squad duplicated a player");
        require(seen.size() == 7, "Depleted squad lost available players");
        for (unsigned i = 0; i < 50; ++i) s.memory.write<4>(club + 0x98 + i * 4, 0);
        require(select_debug_best_xi(s, club, 0, formation, 0), "Empty pool selection failed");
        require(squad(s, tactics, club) == std::array<std::uint32_t, 20>{}, "Empty pool retained unavailable players");
        s.memory.copy_in(club + 0x98, roster);
        select_native_debugging(false);
        std::cout << "Best XI: live selection, availability, unique players, formation, bench limits, depleted pool, gating and 11 languages passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
