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
    // The constructor tests the result for zero, the next-season caller
    // (0x104c7570) for a negative one.
    const bool rollover = c.u32(c.sp() + 0x238) == 0x104c756c;
    const auto fail = [&] {
        c.set(3, entries);
        c.call(0x10775170, 0x104c746c);
        c.put32(cup + 0xa4, 0);
        c.set(3, rollover ? 0xffffffffu : 0u);
        s.pc = 0x104c746c;
        return true;
    };
    std::array<Candidate, candidate_limit> candidates;
    unsigned count = 0;
    for (unsigned i = 0; i < club_count && count < candidate_limit; ++i) {
        const auto club = clubs + i * club_size;
        const auto division = c.u32(club + 0x30);
        if (division && c.i32(division) == lower && c.u32(club + 0x2c) == england)
            candidates[count++] = {club, c.i16(club + 0x1ec)};
    }
    if (count < lower_division_places) {
        // Windows 3.89 (0x542295) copies ten slots of its stack array whatever
        // the pool held, reading uninitialised memory here. That has no
        // faithful port: fail before drawing.
        std::fprintf(stderr, "CM 00/01 FA Trophy requires %u lower-division clubs; found %u\n",
                     lower_division_places, count);
        return fail();
    }

    // On the constructor path, as in Windows 3.89 (0x542170), the Conference
    // count is not checked first: the qualifiers are sorted, drawn and stored,
    // the original loop appends Conference clubs up to the 32 entrants, and
    // the selector returns whether it reached them (0x104c7454, the same test
    // as 0x542321). The next-season path checks it here.
    unsigned conference_count = 0;
    for (unsigned i = 0; i < club_count; ++i)
        if (const auto division = c.u32(clubs + i * club_size + 0x30); division && c.i32(division) == conference)
            ++conference_count;
    if (conference_count < conference_places) {
        std::fprintf(stderr, "CM 00/01 FA Trophy: the Conference holds %u clubs, %u needed%s\n",
                     conference_count, conference_places, rollover ? "" : "; entrants stay incomplete");
        // Here the port leaves Windows 3.89 (Victor, 2026-11-18): its selector
        // returns 0, which the next-season caller takes for success and then
        // draws over entrant slots nobody wrote. Fail before drawing instead,
        // as the Conference Cup does for the same state. The constructor path
        // keeps the Windows order, since it raises the error itself.
        if (rollover) return fail();
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
}
