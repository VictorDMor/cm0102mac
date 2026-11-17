#include "season_competition_rules.hpp"
#include "guest_abi.hpp"
#include "cm0001_cup_order.hpp"
#include <array>
#include <cstdio>
#include <span>

namespace cm::aot {
namespace {
using guest::Cpu;
constexpr unsigned lower_division_places = 10, conference_places = 22, candidate_limit = 50;
constexpr unsigned club_size = 0x248, entry_size = 8;
constexpr int lower_division_slot = -0x7a0c, conference_slot = -0x6874;

using cm0001_cups::Candidate;
using cm0001_cups::sort_by_rank;
}

bool apply_cm0001_fa_trophy_qualifiers(State& s) {
    if (english_season_rules(s).fa_trophy.qualification != TrophyQualification::english_lower_division)
        return false;
    Cpu c(s);
    // The PowerPC selector has allocated the original 32-entry array and saved
    // the caller's registers. Prepare the lower-division prefix, then let its
    // existing Conference loop append the remaining clubs in database order.
    const auto cup = c.r(30), entries = c.u32(cup + 0xa4);
    const auto england = c.u32(c.u32(cup + 4) + 8);
    const auto lower = c.i32(c.toc(lower_division_slot));
    const auto conference = c.i32(c.toc(conference_slot));
    const auto clubs = c.u32(c.toc(-0x7be8));
    const auto club_count = c.u32(c.toc(-0x7b50));
    std::array<Candidate, candidate_limit> candidates;
    unsigned count = 0, conference_count = 0;
    for (unsigned i = 0; i < club_count; ++i) {
        const auto club = clubs + i * club_size;
        const auto division = c.u32(club + 0x30);
        if (!division) continue;
        const auto id = c.i32(division);
        if (id == conference) ++conference_count;
        if (id == lower && count < candidate_limit && c.u32(club + 0x2c) == england)
            candidates[count++] = {club, c.i16(club + 0x1ec)};
    }
    if (count < lower_division_places || conference_count != conference_places) {
        std::fprintf(stderr, "CM 00/01 FA Trophy requires %u lower-division and %u Conference clubs; found %u and %u\n",
                     lower_division_places, conference_places, count, conference_count);
        c.set(3, entries);
        c.call(0x10775170, 0x104c746c);
        c.put32(cup + 0xa4, 0);
        // Constructor tests zero, next-season caller tests a negative result.
        c.set(3, c.u32(c.sp() + 0x238) == 0x104c756c ? 0xffffffffu : 0u);
        s.pc = 0x104c746c;
        return true;
    }

    sort_by_rank(std::span(candidates).first(count));
    const auto shuffle_size = count / 10;
    const auto draw = [&] {
        c.set(3, shuffle_size);
        c.call(0x10209d10, 0x104c72d0);
        return c.r(3);
    };
    for (unsigned i = 0; i < shuffle_size; ++i) {
        const auto first = draw();
        const auto second = draw();
        std::swap(candidates[first], candidates[second]);
    }
    for (unsigned i = 0; i < lower_division_places; ++i) {
        const auto entry = entries + i * entry_size;
        c.put32(entry, candidates[i].club);
        c.put16(entry + 4, 0);
    }
    c.set(31, lower_division_places);
    s.pc = 0x104c73d8;
    return true;
}

void apply_cm0001_fa_trophy_rounds(State& s) {
    const auto& rules = english_season_rules(s).fa_trophy;
    if (rules.semifinal_leg_rule == TrophyRules{}.semifinal_leg_rule) return;
    Cpu c(s);
    // The fourth round is the semifinal. Windows 3.89 sets its leg rule
    // to 2 at 0x542063 (round record +0x21).
    constexpr unsigned semifinal = 3, round_size = 0x78, leg_rule = 0x23;
    c.put8(c.r(31) + semifinal * round_size + leg_rule, rules.semifinal_leg_rule);
}
}
