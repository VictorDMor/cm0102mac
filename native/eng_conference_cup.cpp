#include "season_competition_rules.hpp"
#include "cm0001_cup_order.hpp"
#include "guest_abi.hpp"
#include <cstdio>
#include <vector>

namespace cm::aot {
namespace {
using guest::Cpu;
constexpr unsigned original_england_slots = 10;
constexpr int cup_definition_slot = -0x687c, conference_slot = -0x6874,
    third_division_slot = -0x67ac, lower_division_slot = -0x7a0c;
constexpr unsigned definition_size = 0x6c, club_size = 0x248, entry_size = 8, round_size = 0x78;
constexpr unsigned england_finish = 0x103514dc, cup_constructor_exit = 0x104c6bec,
    entrant_selector_exit = 0x104c746c, calendar_exit = 0x104c712c;

unsigned cup_definition(const Cpu& c) {
    const auto id = c.u32(c.toc(cup_definition_slot));
    if (id >= c.u32(c.toc(-0x7b64))) return 0;
    return c.u32(c.toc(-0x7bbc)) + id * definition_size;
}

const ConferenceCupRules* rules_for_cup(const Cpu& c, unsigned cup) {
    const auto* rules = english_season_rules(c.s).conference_cup;
    if (!rules) return nullptr;
    const auto definition = cup_definition(c);
    return definition && definition == c.u32(cup + 4) ? rules : nullptr;
}

bool restoring_england(const Cpu& c, unsigned england_stack) {
    // The country constructor is shared by the fresh world and archive load.
    // Its active frame is 0xb0 bytes. The caller (country dispatcher) has a
    // 0x90-byte frame and restores its outer caller from the next link slot.
    return c.u32(england_stack + 0xb8) == 0x10191d28 &&
           c.u32(england_stack + 0x148) == 0x1034a8e8;
}

void release_entrants(Cpu& c, unsigned cup, unsigned entries) {
    c.set(3, entries);
    c.call(0x10775170, entrant_selector_exit);
    c.put32(cup + 0xa4, 0);
}
}

void reserve_cm0001_conference_cup_slot(State& s) {
    if (!english_season_rules(s).conference_cup) return;
    Cpu c(s);
    // Only grow the calloc request. Both original optional-division checks
    // still see ten objects until the extra cup has been constructed.
    if (c.r(0) == original_england_slots && cup_definition(c))
        c.set(0, original_england_slots + 1);
}

bool append_cm0001_conference_cup(State& s) {
    if (!english_season_rules(s).conference_cup) return false;
    Cpu c(s);
    const auto country = c.r(28);
    if (c.u32(country + 0xc) != original_england_slots) return false;
    const auto definition = cup_definition(c);
    if (!definition) return false;
    const bool loading = restoring_england(c, c.sp());
    const auto engine_status = c.u8(c.toc(-0x7bac));
    c.set(3, 0xb4);
    c.call(0x10773520, england_finish);
    const auto cup = c.r(3);
    if (cup) {
        c.set({{4, std::uint32_t(std::int16_t(c.r(29)))}, {5, definition}});
        c.call(0x104c6930, england_finish);
        const bool loaded = c.r(3) != 0;
        if (loaded && c.u32(cup + 8)) {
            c.put32(c.u32(country + 0x10) + original_england_slots * 4, cup);
            c.put32(country + 0xc, original_england_slots + 1);
            return false;
        }
        c.set({{3, cup}, {4, 1}});
        c.call(0x104c6c00, england_finish);
        if (loading && !loaded && c.u8(c.toc(-0x7bac)) == engine_status) return false;
    }
    std::fputs("Could not construct English Conference Cup\n", stderr);
    c.set(3, 0);
    s.pc = 0x10351520;
    return true;
}

bool skip_missing_saved_conference_cup(State& s) {
    if (!english_season_rules(s).conference_cup) return false;
    Cpu c(s);
    const auto cup = c.u32(c.sp() + 0x108);
    // The shared loader has already looked for the real competition member
    // when this boundary runs. Old saves only had the FA Trophy member.
    if (!rules_for_cup(c, cup) || c.u32(c.sp() + 0xf8) != england_finish ||
        !restoring_england(c, c.u32(c.sp()))) return false;
    // The constructor has not reached its member allocations, and operator
    // new does not clear the block. The caller destroys this object next, and
    // the destructor frees these fields when they are non-zero.
    c.put32(cup + 0xa0, 0); c.put32(cup + 0xa4, 0); c.put32(cup + 0xac, 0);
    c.put16(cup + 0x3c, 0);
    c.set(3, 0);
    s.pc = cup_constructor_exit;
    return true;
}

void size_cm0001_conference_cup_entrants(State& s) {
    Cpu c(s);
    if (const auto* rules = rules_for_cup(c, c.r(30))) {
        c.put16(c.r(30) + 0x3e, rules->entrants);
        c.set(3, rules->entrants * entry_size);
    }
}

bool apply_cm0001_conference_cup_entrants(State& s) {
    Cpu c(s);
    const auto cup = c.r(30);
    const auto* rules = rules_for_cup(c, cup);
    if (!rules) return false;
    const auto entries = c.u32(cup + 0xa4);
    const auto clubs = c.u32(c.toc(-0x7be8)), total = c.u32(c.toc(-0x7b50));
    const auto conference = c.u32(c.toc(conference_slot));
    const auto third = c.u32(c.toc(third_division_slot)), lower = c.u32(c.toc(lower_division_slot));
    std::vector<cm0001_cups::Candidate> entrants;
    entrants.reserve(rules->entrants);
    for (unsigned i = 0; i < total && entrants.size() < rules->entrants; ++i) {
        const auto club = clubs + i * club_size;
        const auto division = c.u32(club + 0x30);
        if (division && c.u32(division) == conference)
            entrants.push_back({club, c.i8(club + 0x23c)});
    }
    if (entrants.size() != rules->entrants) {
        std::fprintf(stderr, "English Conference Cup requires %u clubs; found %zu\n", rules->entrants, entrants.size());
        release_entrants(c, cup, entries);
        c.set(3, c.u32(c.sp() + 0x238) == 0x104c756c ? 0xffffffffu : 0u);
        s.pc = entrant_selector_exit;
        return true;
    }
    // Windows 3.89 walks the mutable array once, moves promoted clubs to
    // the front, and swaps each club relegated from Third with the last slot.
    // The counts exclude those ends from the subsequent position sort.
    std::size_t promoted = 0, relegated = 0;
    for (auto& entrant : entrants) {
        const auto previous = c.u32(entrant.club + 0x34);
        if (previous && c.u32(previous) == third) {
            std::swap(entrant, entrants.back());
            ++relegated;
        } else if (!previous || c.u32(previous) == lower) {
            std::swap(entrant, entrants[promoted++]);
        }
    }
    cm0001_cups::sort_by_rank(std::span(entrants).subspan(promoted, entrants.size() - promoted - relegated));
    for (unsigned i = 0; i < entrants.size(); ++i) {
        c.put32(entries + i * entry_size, entrants[i].club);
        c.put16(entries + i * entry_size + 4, 0);
    }
    c.set(3, 1);
    s.pc = entrant_selector_exit;
    return true;
}

bool apply_cm0001_conference_cup_rounds(State& s) {
    Cpu c(s);
    const auto cup = c.r(30);
    const auto* rules = rules_for_cup(c, cup);
    if (!rules) return false;
    const auto schedule = c.r(31), year = std::uint32_t(c.i16(cup + 0x40));
    for (unsigned i = 0; i < rules->rounds.size(); ++i) {
        const auto& rule = rules->rounds[i];
        const auto [day, month, next_year, weekday] = rule.main_date;
        c.set({{3, schedule}, {4, i}, {5, day}, {6, month}, {7, next_year}, {8, weekday}, {9, year}});
        c.call(0x104854f0, calendar_exit);
        const auto [second_day, second_month, second_year, second_weekday] = rule.second_date;
        c.put32(c.sp() + 0x38, 0);
        c.set({{3, schedule}, {4, i}, {5, second_day}, {6, second_month}, {7, second_year},
               {8, second_weekday}, {9, rule.second_date_mode}, {10, year}});
        c.call(0x10485570, calendar_exit);
        const auto round = schedule + i * round_size;
        c.put16(round + 8, rule.label);
        c.put16(round + 0xa, i);
        c.put16(round + 0xc, 0);
        c.put16(round + 0xe, rule.draw_flags);
        c.put16(round + 0x10, rule.first_leg_rules);
        c.put16(round + 0x12, rule.second_leg_rules);
        c.put8(round + 0x18, rule.date_rule);
        c.put16(round + 0x1a, rule.entrants);
        c.put16(round + 0x1c, rule.ties);
        c.put16(round + 0x1e, rule.direct_entrants);
        c.put16(round + 0x20, rule.first_direct_entrant);
        c.put8(round + 0x22, 0);
        c.put8(round + 0x23, rule.leg_rule);
        c.put8(round + 0x24, rule.days_between_legs);
        c.put32(round + 0x6c, 0);
        c.put32(round + 0x70, rule.field_70);
        c.put32(round + 0x74, 0);
    }
    c.set(3, schedule);
    s.pc = calendar_exit;
    return true;
}
}
