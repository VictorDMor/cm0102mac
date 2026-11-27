#include "croatia_super_cup.hpp"

#include "career_rules.hpp"
#include "comp_util_detail.hpp"
#include "cup_base_rounds_detail.hpp"
#include "gui_guest_scope.hpp"
#include "platform.hpp"
#include "season_competition_rules.hpp"

#include <cstdio>
#include <string>

namespace cm::aot::croatia_super_cup {
namespace {
using guest::Cpu;
using comp::invoke;
using u32 = std::uint32_t;
namespace object = comp::competition;
namespace cup = cup_base::cup;
namespace round = cup_base::rounds::round_row;
namespace tie = cup_base::rounds::tie;
namespace history = comp_util::history_log;
constexpr int definition_slot = -0x6adc, first_slot = -0x6a40, domestic_cup_slot = -0x6a38;
constexpr int today_slot = -0x7b80, single_match_vtable = -0x5d04;
constexpr int record_count_slot = -0x7b64;
constexpr u32 context_return = 0xfffffff0u;
constexpr u32 cup_size = 0xb4, stats_size = 0x634, single_match_destroy = 0x106cfc00;
constexpr u32 country_stride = 0x4c, country_count = 0x0c, country_objects = 0x10;
constexpr u32 league_finished = 0xe0, entrant_result = 5;
constexpr u32 league_champion_venue = 4, super_cup_fixture_kind = 6;
constexpr u32 penalties = 0x01, extra_time = 0x02, golden_goal = 0x80;

struct Winners {
    u32 league = 0, cup = 0;
    bool qualify() const { return league && cup && league != cup; }
};
struct Rollover {
    State* state = nullptr;
    u32 country = 0;
    int year = 0;
    Winners winners;
};
thread_local Rollover pending;

u32 definition(Cpu& c, int slot) {
    const auto id = c.u32(c.toc(slot));
    const auto records = c.u32(c.toc(comp::toc::records));
    return records && id < c.u32(c.toc(record_count_slot)) ? records + id * comp::record::size : 0;
}

bool is_croatia(Cpu& c, u32 country) {
    if (!country) return false;
    const auto factory = c.u32(country + 8);
    return factory && c.u32(factory) == 0x10350bb0;
}

u32 competition(Cpu& c, u32 country, u32 record) {
    if (!record) return 0;
    const auto count = c.u32(country + country_count);
    const auto objects = c.u32(country + country_objects);
    for (u32 i = 0; objects && i < count; ++i) {
        const auto current = c.u32(objects + i * 4);
        if (current && c.u32(current + object::record) == record) return current;
    }
    return 0;
}

u32 prior_champion(Cpu& c, u32 record, int year) {
    if (!record) return 0;
    u32 winner = 0;
    const auto rows = c.u32(c.toc(history::rows));
    const auto count = c.u32(c.toc(history::count));
    for (u32 i = 0; rows && i < count; ++i) {
        const auto row = rows + i * history::row_size;
        if (c.u32(row + history::competition) != record || c.u16(row + history::year) != year) continue;
        const auto candidate = c.u32(row + history::winner);
        if (winner && candidate != winner) return 0;
        winner = candidate;
    }
    return winner;
}

u32 cup_champion(Cpu& c, u32 knockout, int year) {
    if (!knockout || c.u16(knockout + object::year) != year || !c.u8(knockout + cup::byte_b0)) return 0;
    const auto entrants = c.u32(knockout + cup::entrants);
    const auto count = c.u16(knockout + object::club_count);
    for (u32 i = 0; entrants && i < count; ++i) {
        const auto row = entrants + i * cup::entrant_row;
        if (c.u8(row + entrant_result) == 1) return c.u32(row + cup_base::Entrant::club);
    }
    return 0;
}

u32 allocate(Cpu& c, u32 size, u32 entry) {
    const auto address = invoke(c, entry, context_return, {size});
    if (address) c.s.memory.clear(address, size);
    return address;
}

u32 make_cup(Cpu& c, u32 record, int year) {
    const auto object = allocate(c, cup_size, comp::callee::allocate_object);
    if (!object) return 0;
    cup_base::construct(c, object);
    // The existing single-match class uses standard cup result handling,
    // cup storage, and a destructor that owns the same guest allocations.
    c.put32(object + object::vtable, c.toc(single_match_vtable));
    c.put32(object + object::record, record);
    c.put16(object + object::year, year);
    c.put8(object + object::owner, 0xff);
    c.put8(object + object::bench, 7);
    c.put8(object + object::ruling_body, 0x21);
    return object;
}

bool publish(Cpu& c, u32 country, u32 object, u32 registry_entry = 0) {
    const auto count = c.u32(country + country_count);
    const auto objects = c.u32(country + country_objects);
    const auto old = registry_entry ? c.u32(registry_entry) : 0;
    if (old) {
        for (u32 i = 0; i < count; ++i) {
            const auto slot = objects + i * 4;
            if (c.u32(slot) != old) continue;
            c.put32(slot, object);
            c.put32(registry_entry, object);
            invoke(c, single_match_destroy, context_return, {old, 1});
            return true;
        }
        return false;
    }
    const auto extended = invoke(c, comp::callee::resize_block, context_return, {objects, (count + 1) * 4});
    if (!extended) return false;
    c.put32(extended + count * 4, object);
    c.put32(country + country_objects, extended);
    c.put32(country + country_count, count + 1);
    if (registry_entry) c.put32(registry_entry, object);
    return true;
}

bool populate(Cpu& c, u32 object, Winners winners, unsigned day, CroatianSuperCupFormat format) {
    const auto entrants = allocate(c, 2 * cup::entrant_row, comp::callee::allocate_block);
    const auto calendar = allocate(c, cup::round_row, comp::callee::allocate_block);
    const auto matches = allocate(c, 4, comp::callee::allocate_block);
    c.put32(object + cup::entrants, entrants);
    c.put32(object + cup::rounds, calendar);
    c.put32(object + cup::round_matches, matches);
    if (!entrants || !calendar || !matches) return false;
    const auto match = allocate(c, cup::match_row, comp::callee::allocate_block);
    if (!match) return false;
    c.put32(matches, match);
    c.put16(object + object::rounds, 1);
    c.put16(object + object::club_count, 2);
    c.put32(entrants, winners.league);
    c.put32(entrants + cup::entrant_row, winners.cup);
    c.put16(calendar, c.u16(c.toc(today_slot)));
    c.put16(calendar + round::fixture_day, day);
    c.put8(calendar + round::fixture_kind, 1);
    c.put16(calendar + round::flags, penalties | extra_time |
            (format == CroatianSuperCupFormat::golden_goal ? golden_goal : 0));
    c.put32(calendar + round::rules_id, league_champion_venue);
    c.put8(calendar + round::byte_18, super_cup_fixture_kind);
    c.put16(calendar + cup_base::Round::entrants, 2);
    c.put16(calendar + round::ties, 1);
    c.put16(calendar + round::entrants, 2);
    c.put8(calendar + round::leg_rule, 1);

    // The one match is already drawn, with no feeder or next round.
    c.put16(match + tie::first, 0);
    c.put16(match + tie::second, 1);
    c.put16(match + 6, 0xffff);
    for (u32 offset = tie::first_feeder_match; offset <= 0x12; offset += 2) c.put16(match + offset, 0xffff);
    for (u32 offset = 0x14; offset < 0x32; ++offset) c.put8(match + offset, 0xff);
    c.put8(match + tie::state, 2);
    const auto stats = allocate(c, stats_size, comp::callee::allocate_object);
    if (!stats) return false;
    invoke(c, 0x104533a0, context_return, {stats, object});
    c.put32(object + object::stats, stats);
    return true;
}

void schedule(Cpu& c, u32 country, u32 league, Winners winners, int year) {
    const auto format = competition_rules(c.s, RuleCompetition::croatian_super_cup, year).croatian_super_cup;
    if (!winners.qualify() || format == CroatianSuperCupFormat::absent) return;
    const auto record = definition(c, definition_slot);
    const auto directory = c.u32(c.toc(comp::toc::competitions));
    const auto calendar = c.u32(league + league_base::league::schedule);
    if (!record || !directory || !calendar || c.u16(calendar + 2)) return;
    const auto id = c.u32(record);
    const auto today = c.toc(today_slot);
    const int day = int(c.u16(calendar)) - 7;
    if (id >= c.u32(c.toc(comp::toc::competition_count)) || day < 0 ||
        c.u16(today + 2) > year || (c.u16(today + 2) == year && c.u16(today) >= day)) return;
    const auto slot = directory + id * 4;
    const auto old = c.u32(slot);
    if (old && (c.u16(old + object::year) >= year || !c.u8(old + object::byte_49))) return;

    GuiGuestScope preserve(c.s, 8);
    const auto object = make_cup(c, record, year);
    if (object && populate(c, object, winners, unsigned(day), format) && publish(c, country, object, slot)) {
        cup_base::rounds::advance_round(c, object, 0);
        return;
    }
    if (object) invoke(c, single_match_destroy, context_return, {object, 1});
    std::fprintf(stderr, "Croatian Super Cup: could not allocate or register edition %d\n", year);
}

std::string member_name(Cpu& c, u32 record) {
    std::string name;
    for (unsigned i = 0; i < 51; ++i) {
        const auto ch = char(c.u8(record + comp::record::name + i));
        if (!ch) break;
        name.push_back(ch == '/' ? '_' : ch);
    }
    return name + "_" + std::to_string(c.u32(record)) + ".tmp";
}
}

bool owns(State& s, u32 object) {
    if (!object) return false;
    Cpu c(s);
    if (c.u32(object + object::vtable) != c.toc(single_match_vtable)) return false;
    const auto record = c.u32(object + object::record);
    return record && record == definition(c, definition_slot);
}

void new_career(State& s) {
    Cpu c(s);
    if (!s.platform || !s.platform->career_rules().historical_rules()) return;
    const auto descriptors = c.toc(comp::toc::descriptors);
    for (unsigned i = 0; i < comp::descriptor::count; ++i) {
        const auto country = descriptors + i * country_stride;
        if (!is_croatia(c, country)) continue;
        const auto first = definition(c, first_slot);
        if (const auto league = competition(c, country, first)) {
            const auto year = c.u16(league + object::year);
            schedule(c, country, league, {prior_champion(c, first, year - 1),
                prior_champion(c, definition(c, domestic_cup_slot), year - 1)}, year);
        }
        return;
    }
}

bool restore(State& s, u32 country) {
    if (!s.platform || !s.platform->career_loading()) return true;
    Cpu c(s);
    const auto record = definition(c, definition_slot);
    if (!record || !s.platform->has_career_member(member_name(c, record))) return true;
    GuiGuestScope preserve(s, 8);
    const auto object = make_cup(c, record, 0);
    if (object && cup_base::load_from_disk(c, object, 1) && c.u32(object + object::record) == record &&
        publish(c, country, object)) return true;
    if (object) invoke(c, single_match_destroy, context_return, {object, 1});
    return false;
}

void before_rollover(State& s, u32 country) {
    pending = {};
    if (!s.platform || !s.platform->career_rules().historical_rules()) return;
    Cpu c(s);
    if (!is_croatia(c, country)) return;
    const auto league = competition(c, country, definition(c, first_slot));
    if (!league || !c.u8(league + league_finished)) return;
    const auto rows = c.u32(league + league_base::league::standings);
    const auto year = c.u16(league + object::year);
    if (!rows) return;
    pending = {&s, country, year + 1, {c.u32(rows), cup_champion(c, competition(c, country,
                                                                                definition(c, domestic_cup_slot)), year)}};
}

void after_rollover(State& s, u32 country) {
    if (pending.state != &s || pending.country != country) return;
    const auto completed = pending;
    pending = {};
    Cpu c(s);
    const auto league = competition(c, country, definition(c, first_slot));
    if (league && c.u16(league + object::year) == completed.year)
        schedule(c, country, league, completed.winners, completed.year);
}

bool calendar(State& s) {
    Cpu c(s);
    const auto object = c.r(3);
    if (!owns(s, object)) return false;
    const auto back = c.lr();
    const auto count = c.r(5), code = c.r(6), enabled = c.r(7);
    const auto stored = std::int8_t(c.r(4)) == -1 ? c.u32(object + cup::rounds) : 0;
    const auto copy = stored ? invoke(c, comp::callee::allocate_block, context_return, {cup::round_row}) : 0;
    if (copy) {
        s.memory.copy_in(copy, s.memory.copy_out(stored, cup::round_row));
        c.put16(count, 1);
        c.put16(code, c.u16(object + object::code));
        if (enabled) c.put8(enabled, 0);
    }
    c.set(3, copy);
    c.set_lr(back);
    s.pc = back;
    return true;
}
}
