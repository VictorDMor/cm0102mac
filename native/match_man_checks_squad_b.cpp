// MATCH_MAN.CPP checker cases for the squad_b block (match_man_squad_b.hpp): 0x1020c5d0.
#include "match_man_checks.hpp"
#include "match_man_squad_b_detail.hpp"
#include <algorithm>
#include <random>

namespace cm::aot::match_man_checks {
namespace {
using namespace match_man;
constexpr std::uint32_t entry = 0x1020c5d0;
constexpr std::uint32_t tie_class = 0x10464880, day_difference = 0x10718130;
// The scratch rows a case builds for the stats lookup (0x10459b50 searches a source's sorted rows of 0x7c bytes
// by the club's first word): the source object and its two rows.
constexpr std::uint32_t source_at = scratch_base + 0x1000, rows_at = scratch_base + 0x2000, row_size = 0x7c;
}

MATCH_MAN_CASES("squad_b") {
    if (!scenario.match) return;
    // Every case calls the routine on one of the match's two real side records. The variants write values the
    // routine reads (ratings, tallies, clock, flags, scores, the fixture's records) into the real match, clubs and
    // fixture; the checker undoes the writes after each case. The callees that fill the situation bytes
    // (0x101f21d0) and judge the players (0x10219e40) run for real. The real match's competitions have no source
    // for the stats lookup, so the variants that need club records build a source of two rows in scratch memory
    // for the real lookup to search.
    std::mt19937 random(0x5b0d);
    const auto pick = [&](std::initializer_list<std::uint32_t> values) {
        return *(values.begin() + random() % values.size());
    };
    const std::uint32_t floats[] = {0, 0x3f000000, 0x3f800000, 0x3fc00000, 0x40000000, 0x40400000, 0x42c80000, 0xbf800000};
    const std::uint32_t ratings[] = {0, 0x1000, 0x1d4c, 0x1d4d, 0x1f40, 0x1f41, 0x2134, 0x2135, 0x2800, 0x7000};
    for (unsigned n = 0; n < 2; ++n) {
        const auto side = scenario.side(n);
        const auto match = scenario.match;
        const auto name = "side " + std::to_string(n) + ": ";
        cases.push_back({.entry = entry, .registers = {{3, side}}, .label = name + "real"});
        const auto own_club = scenario.r32(side + tactic_side::club), rival_club = scenario.r32(side + tactic_side::rival_club);
        const auto fixture = scenario.r32(side + tactic_side::fixture);
        const auto manager = scenario.r32(side + tactic_side::manager);
        const auto table = scenario.global(-0x7b5c);
        const auto table_entry = scenario.r32(table + scenario.r32(fixture + tactic_fixture::index) * 4);
        const auto strength = match + tactic_match::strength + n * 4;
        // Live or not, and the side number itself (the clubs in the record stay).
        cases.push_back({.entry = entry, .registers = {{3, side}}, .memory = {{1, match + tactic_match::live, 0}},
                         .label = name + "not live"});
        cases.push_back({.entry = entry, .registers = {{3, side}}, .memory = {{1, match + tactic_match::live, 1}},
                         .label = name + "live"});
        cases.push_back({.entry = entry, .registers = {{3, side}}, .memory = {{1, side + tactic_side::number, 1 - n}},
                         .label = name + "other side number"});
        cases.push_back({.entry = entry, .registers = {{3, side}}, .memory = {{4, side + tactic_side::manager, 0}},
                         .label = name + "no manager"});
        // Player table entries with no player: the slot byte is negative.
        cases.push_back({.entry = entry, .registers = {{3, side}},
                         .memory = {{1, match + tactic_match::players + n * tactic_match::player_stride + 0x19, 0xff},
                                    {1, match + tactic_match::players + (1 - n) * tactic_match::player_stride + 0x19 + 0x1c4 * 3, 0x80},
                                    {1, match + tactic_match::live, 1}},
                         .label = name + "empty slots"});
        for (unsigned variant = 0; variant < 900; ++variant) {
            std::vector<Write> memory;
            std::vector<std::pair<std::uint32_t, std::uint32_t>> answers;
            memory.push_back({1, match + tactic_match::live, pick({0, 1, 1, 1, 1})});
            memory.push_back({2, match + tactic_match::clock, pick({0, 3, 11, 33, 45, 46, 60, 89, 90, 91, 120, 400, 0xfff0, 0x7fff})});
            memory.push_back({2, match + tactic_match::wins, pick({0, 0, 1, 7, 20, 0x7ff0})});
            memory.push_back({2, match + tactic_match::draws, pick({0, 0, 2, 9, 30})});
            memory.push_back({2, match + tactic_match::losses, pick({0, 0, 1, 5, 17, 0xfff0})});
            memory.push_back({2, own_club + tactic_club::rating, ratings[random() % 10]});
            memory.push_back({2, rival_club + tactic_club::rating, ratings[random() % 10]});
            memory.push_back({1, match + tactic_match::flag_b, pick({0, 1, 0xff, 5})});
            memory.push_back({1, match + tactic_match::flag_b + 1, pick({0, 1, 0xff, 5})});
            memory.push_back({1, match + tactic_match::flag_a + n, pick({0, 1, 2})});
            memory.push_back({2, match + tactic_match::period_end, pick({0x3de, 0x3de, 0x1ef, 0x100, 0})});
            // The scores 0x101f21d0 totals (0xe03d: home, 0xe073: away), the fixture's first-leg bytes and flags,
            // and whether its competition kind is a knockout (0x10464880 answered 3).
            memory.push_back({1, match + 0xe03d, pick({0, 0, 1, 2, 3, 5})});
            memory.push_back({1, match + 0xe073, pick({0, 0, 1, 2, 3, 5})});
            if (random() % 2) {
                memory.push_back({1, fixture + 0x4b, pick({0, 1, 2, 0xff})});
                memory.push_back({1, fixture + 0x4c, pick({0, 1, 2, 0xff})});
            }
            memory.push_back({2, fixture + 0x38, pick({0, 4, 0x4000, 0x4004})});
            if (random() % 2) answers.push_back({tie_class, 3});
            if (random() % 3 == 0) memory.push_back({4, side + tactic_side::manager, 0});
            else memory.push_back({1, manager + 0x2e, pick({0x80, 0xf6, 0, 10, 11, 20, 0x7f})});
            // The clubs' region and link fields and the fixture's record (the rules chain).
            if (random() % 6) memory.push_back({4, rival_club + tactic_club::region, scenario.r32(own_club + tactic_club::region)});
            if (random() % 6) memory.push_back({4, own_club + tactic_club::link, scenario.r32(fixture + tactic_fixture::record)});
            if (random() % 15 == 0) memory.push_back({4, fixture + tactic_fixture::record, 0});
            memory.push_back({4, strength, floats[random() % 8]});
            memory.push_back({4, match + tactic_match::strength + (1 - n) * 4, floats[random() % 8]});
            // A tie decided on aggregate (0x101f21d0's third byte): first-leg bytes that level the totals.
            if (random() % 6 == 0) {
                const bool home = n == 0;
                memory.push_back({1, match + 0xe03d, home ? 1u : 4u});
                memory.push_back({1, match + 0xe073, home ? 1u : 3u});
                memory.push_back({1, fixture + 0x4b, home ? 3u : 1u});
                memory.push_back({1, fixture + 0x4c, home ? 3u : 2u});
                memory.push_back({2, fixture + 0x38, 0});
            }
            // The rival club's three persons include the own club's (far-class check).
            if (random() % 8 == 0) memory.push_back({4, rival_club + tactic_club::persons[random() % 3], own_club});
            // The stats rows of both clubs.
            if (random() % 4 != 0 && table_entry) {
                memory.push_back({4, table_entry + tactic_table_entry::source, source_at});
                memory.push_back({4, source_at + 0x5cc, 0});
                memory.push_back({4, source_at + 0xc, rows_at});
                memory.push_back({4, source_at + 0x14, 2});
                memory.push_back({1, source_at + 0x18, 0xff});
                memory.push_back({2, table_entry + tactic_table_entry::weight, pick({0, 1, 5, 40, 0xffff})});
                memory.push_back({2, scenario.r32(fixture + tactic_fixture::record) + tactic_fixture::record_value,
                                  pick({0, 5, 9, 10, 12, 30})});
                // Rows sorted by the clubs' first words (the search key).
                const std::uint32_t low = std::min(scenario.r32(own_club), scenario.r32(rival_club));
                const std::uint32_t high = std::max(scenario.r32(own_club), scenario.r32(rival_club));
                memory.push_back({4, rows_at, low});
                memory.push_back({4, rows_at + row_size, high});
                for (const auto row : {rows_at, rows_at + row_size}) {
                    memory.push_back({4, row + tactic_stats::a1c, pick({0, 1, 5, 20})});
                    memory.push_back({4, row + tactic_stats::a30, pick({0, 1, 3, 10, 0xfffffff0u})});
                    memory.push_back({4, row + tactic_stats::a44, pick({0, 1, 2, 7, 30})});
                    memory.push_back({4, row + tactic_stats::a50, 0x11111111});
                    memory.push_back({4, row + tactic_stats::a54, 0x22222222});
                    memory.push_back({4, row + tactic_stats::a58, pick({0, 1, 2, 8, 40})});
                    memory.push_back({4, row + tactic_stats::a64, 0x33333333});
                    memory.push_back({4, row + tactic_stats::a68, 0x44444444});
                    memory.push_back({1, row + tactic_stats::rank, pick({0, 1, 2, 4, 5, 9, 20})});
                }
                answers.push_back({day_difference, pick({0, 3, 9, 10, 20, 29, 30, 100, 0xfffffffe})});
            }
            cases.push_back({.entry = entry, .registers = {{3, side}}, .memory = std::move(memory),
                             .label = name + "variant " + std::to_string(variant), .answers = std::move(answers)});
        }
    }
}
}
