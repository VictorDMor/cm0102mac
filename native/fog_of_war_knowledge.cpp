// fog_of_war.cpp: the rules for what a manager knows of a person.
//
// Levels: 0 unknown, 1 and 2 partly known, 3 fully known. A new manager's first level for each person
// comes from `initial_level`; each day `age_knowledge` lowers a level not refreshed for 540 days and
// raises every level to `level_floor`; a sighting (`sight`: a match, a scout's report) stamps the date
// and raises the level, by chance when someone judges the person.
#include "fog_of_war_detail.hpp"

namespace cm::aot::fog_of_war {
void age_knowledge(Cpu& c, std::uint32_t knowledge) {
    const Frame frame(c, 0x106fe130, 0x50, {.first_gpr = 26});
    const auto today = c.toc(toc::today);
    auto person_at = c.u32(c.toc(toc::people));
    for (std::int32_t n = 0; n < people_count(c); ++n, person_at += person::size) {
        const auto index = std::uint32_t(n);
        if (const auto at = entry_of(c, knowledge, index); c.i8(at + entry::level) != 0) {
            const auto days = std::int32_t(invoke(c, callee::days_between, 0x106fe184,
                                                  {today, c.u32(at + entry::seen), c.u32(at + entry::seen + 4)}));
            if (days > memory_days) {
                stamp_today(c, entry_of(c, knowledge, index) + entry::seen);
                const auto level = entry_of(c, knowledge, index) + entry::level;
                c.put8(level, c.u8(level) - 1u);
            }
        }
        const auto floor = std::int8_t(level_floor(c, c.u32(knowledge + knowledge::manager), person_at));
        if (const auto level = entry_of(c, knowledge, index) + entry::level; c.i8(level) < floor) c.put8(level, std::uint8_t(floor));
    }
}

void forget(Cpu& c, std::uint32_t knowledge, std::uint32_t person_at) {
    if (!person_at || !c.u32(knowledge + knowledge::manager)) return;
    if (c.i32(person_at + person::id) >= people_count(c) || !fog_enabled(c)) return;
    c.put8(entry_of(c, knowledge, c.u32(person_at + person::id)) + entry::level, 0);
}

std::uint8_t level_of(Cpu& c, std::uint32_t knowledge, std::uint32_t person_at) {
    const Frame frame(c, 0x106fe280, 0x50, {.first_gpr = 29});
    constexpr std::int32_t code_out_at = 0x38;
    if (!person_at || !c.u32(knowledge + knowledge::manager)) return 0;
    if (c.i32(person_at + person::id) >= people_count(c) || !fog_enabled(c)) return entry::full;
    const auto manager_club = [&] { return c.u32(c.u32(knowledge + knowledge::manager) + person::club); };
    // A manager knows his own club's people, those contracted to it and his national side's.
    if (const auto club_at = manager_club()) {
        if (c.u32(person_at + person::club) == club_at) return entry::full;
        if (contracted_club(c, person_at, 0x106fe320) == manager_club()) return entry::full;
    }
    const auto side = c.u32(person_at + person::national_side);
    if (side && side == c.u32(c.u32(knowledge + knowledge::manager) + person::national_side)) return entry::full;
    if (const auto club_at = c.u32(person_at + person::club); club_at && manager_club()) {
        if (invoke(c, callee::club_of_code, 0x106fe3ac, {club_at, frame.at(code_out_at), 1}) == manager_club()) return entry::full;
    }
    return c.u8(entry_of(c, knowledge, c.u32(person_at + person::id)) + entry::level);
}

std::int32_t initial_level(Cpu& c, std::uint32_t knowledge, std::uint32_t manager, std::uint32_t person_at) {
    const Frame frame(c, 0x106fe3f0, 0x60, {.first_gpr = 24});
    if (!c.u32(person_at + person::player)) return entry::full;  // staff are known to all
    const auto manager_nation = c.u32(manager + person::nation), person_nation = c.u32(person_at + person::nation);
    const bool same_nation = manager_nation == person_nation;
    const auto side = c.u32(person_at + person::national_side);
    if (side && side == c.u32(manager + person::national_side)) return entry::full;
    const auto manager_club = c.u32(manager + person::club), person_club = c.u32(person_at + person::club);
    const auto nation_of = [&](std::uint32_t club_at) { return c.u32(club_at + club::nation); };
    bool clubs_share_nation = false;
    if (manager_club && person_club && nation_of(manager_club) == nation_of(person_club)) clubs_share_nation = true;
    else if (const auto manager_side = c.u32(manager + person::national_side);
             manager_side && person_club && nation_of(manager_side) == nation_of(person_club)) clubs_share_nation = true;
    const bool person_club_in_manager_nation = person_club && nation_of(person_club) == manager_nation;
    const bool manager_club_in_person_nation = manager_club && nation_of(manager_club) == person_nation;
    const bool same_division = manager_club && person_club &&
                               c.u32(manager_club + club::division) == c.u32(person_club + club::division);
    // The history checks reach the history file (an import): the original holds the object, the two people and
    // its five flags in r24-r31 there.
    CallRegisters regs(c);
    regs.hold({{24, knowledge}, {25, manager}, {26, person_at}, {27, person_club_in_manager_nation}, {28, manager_club_in_person_nation},
               {29, same_division}, {30, clubs_share_nation}, {31, same_nation}});
    const auto at_club_within = [&](bool (*check)(Cpu&, std::uint32_t, std::uint32_t)) {
        regs.apply();
        return check(c, person_at, c.u32(manager + person::club));
    };
    if (famous(c, person_at)) return entry::full;
    if (same_division && age_of(c, person_at) > 0x13) return entry::full;
    if (at_club_within(at_club_within_a_season)) return entry::full;
    // A known person of the manager's circle: fully if well known, else by age or a recent spell at his club.
    const auto by_age = [&](std::int32_t older_than) -> std::int32_t {
        if (well_known(c, person_at)) return entry::full;
        if (age_of(c, person_at) > older_than) return 2;
        return at_club_within(at_club_within_two_seasons) ? 2 : 1;
    };
    if (clubs_share_nation) return by_age(0x15);
    if (person_club_in_manager_nation) return by_age(0x17);
    if (same_nation || manager_club_in_person_nation) return by_age(0x17);
    // A national side's manager knows a little of its nation's people, for a nation ranked below 1000.
    const auto manager_side = c.u32(manager + person::national_side);
    const auto side_nation = manager_side ? nation_of(manager_side) : 0;
    return side_nation && c.i32(side_nation + nation::rank) < 1000 && side_nation == c.u32(person_at + person::nation) ? 1 : 0;
}

bool famous(Cpu& c, std::uint32_t person_at) {
    if (!person_at) return false;
    const auto nation_at = c.u32(person_at + person::nation);
    return nation_at && (capped_for_reputable_nation(c, person_at, nation_at) || world_reputation_high(c, person_at));
}

bool well_known(Cpu& c, std::uint32_t person_at) { return home_reputation_high(c, person_at); }

std::int32_t level_floor(Cpu& c, std::uint32_t manager, std::uint32_t person_at) {
    const Frame frame(c, 0x106fe820, 0x80, {.first_gpr = 22});
    constexpr std::int32_t code_out_at = 0x38;
    const auto manager_nation = c.u32(manager + person::nation), person_nation = c.u32(person_at + person::nation);
    const bool same_nation = manager_nation == person_nation;
    const auto manager_club = c.u32(manager + person::club), person_club = c.u32(person_at + person::club);
    const auto nation_of = [&](std::uint32_t club_at) { return c.u32(club_at + club::nation); };
    const bool clubs_share_nation = manager_club && person_club && nation_of(manager_club) == nation_of(person_club);
    const bool manager_club_in_person_nation = manager_club && nation_of(manager_club) == person_nation;
    const bool person_club_in_manager_nation = person_club && nation_of(person_club) == manager_nation;
    const bool same_division = manager_club && person_club &&
                               c.u32(manager_club + club::division) == c.u32(person_club + club::division);
    // `famous`, inline: the person row is read above before this null test, as in the original.
    if (person_at && person_nation &&
        (capped_for_reputable_nation(c, person_at, person_nation) || world_reputation_high(c, person_at)))
        return entry::full;
    if (person_club && person_club == manager_club) return entry::full;
    const auto side = c.u32(person_at + person::national_side);
    if (side && side == c.u32(manager + person::national_side)) return entry::full;
    if (contracted_club(c, person_at, 0x106fea0c) == c.u32(manager + person::club)) return entry::full;
    if (const auto club_at = c.u32(person_at + person::club);
        club_at && invoke(c, callee::club_of_code, 0x106fea5c, {club_at, frame.at(code_out_at), 1}) == c.u32(manager + person::club))
        return entry::full;
    if (contracted_club(c, person_at, 0x106fea7c)) {
        const auto club_at = contracted_club(c, person_at, 0x106feab4);
        if (invoke(c, callee::club_of_code, 0x106feae4, {club_at, frame.at(code_out_at), 1}) == c.u32(manager + person::club))
            return entry::full;
    }
    if (same_division) return 2;
    if (clubs_share_nation) {
        if (home_reputation_high(c, person_at)) return 2;
        if (age_of(c, person_at) > 0x15) return 1;
    }
    if (same_nation || manager_club_in_person_nation) {
        if (home_reputation_high(c, person_at)) return 2;
        if (age_of(c, person_at) > 0x15) return 1;
    }
    return person_club_in_manager_nation && home_reputation_high(c, person_at) ? 1 : 0;
}

void sight(Cpu& c, std::uint32_t knowledge, std::uint32_t person_at, std::uint32_t observer) {
    const Frame frame(c, 0x106fec70, 0x70, {.first_gpr = 26});
    if (!person_at || c.i32(person_at + person::id) >= people_count(c)) return;
    const auto level_at = [&] { return entry_of(c, knowledge, c.u32(person_at + person::id)) + entry::level; };
    stamp_today(c, entry_of(c, knowledge, c.u32(person_at + person::id)) + entry::seen);
    const auto staff = observer ? c.u32(observer + person::staff) : 0;
    if (!staff) { c.put8(level_at(), entry::full); return; }
    if (c.i8(level_at()) == entry::full) return;
    // The observer's judgement sets the odds: a roll below it, of 0, raises the level by one.
    const auto judgement = invoke(c, callee::attribute_rating, 0x106fed2c, {staff, c.u8(staff + 0x37)});
    const auto range = std::int32_t(std::int8_t(judgement));
    // The original's wider draw (0x106fef10's) needs a range past +-0xffff: a signed byte never is.
    const auto roll = range == 0 ? 0 : remainder_in(draw_sample(c, 0x106fed68), range);
    const auto level = level_at();
    if (std::int8_t(judgement) < 15 && std::int8_t(roll) == 0) c.put8(level, c.u8(level) + 1u);
    else if (c.i8(level) == 0 && std::int8_t(roll) < 6) c.put8(level, c.u8(level) + 2u);
    else c.put8(level, entry::full);
}

std::int32_t random_below(Cpu& c, std::int32_t limit) {
    const Frame frame(c, 0x106fef10, 0x80, {.first_gpr = 20});
    if (limit == 0) return 0;
    const auto fits = [](std::int32_t range) { return range >= -0xffff && range <= 0xffff; };
    const auto sample = draw_sample(c, 0x106fef64);
    if (fits(limit)) return remainder_in(sample, limit);
    // A wide range: a coarse part scaled from this sample, plus a tenth of a draw below range * 10 / 65535
    // (the product wraps in a word). That range is at most 32768 in size for any word, so the second draw
    // always fits: the original's two further inline levels and its call of itself never run.
    guest::Fpu fpu(c);
    const auto pool = c.toc(toc::doubles);
    const double scale = fpu.fdiv(fpu.fsub(as_double_bits(std::uint32_t(limit) ^ 0x80000000u), c.f64(pool + doubles::signed_magic)),
                                  c.f64(pool + doubles::sample_span));
    const auto rest = std::int32_t(sample) % 0xffff;
    const double scaled = fpu.fmul(scale, fpu.fsub(as_double_bits(std::uint32_t(rest) ^ 0x80000000u), c.f64(pool + doubles::signed_magic)));
    const auto coarse = std::int32_t(std::uint32_t(fpu.fctiwz(0, scaled)));
    const auto finer_range = std::int32_t(std::uint32_t(limit) * 10u) / 0xffff;
    const auto finer = finer_range == 0 ? 0 : remainder_in(draw_sample(c, 0x106ff050), finer_range);
    const auto result = std::int32_t(std::uint32_t(coarse) + std::uint32_t(std::int32_t(std::uint32_t(finer) + 5u) / 10));
    const auto highest = std::int32_t(std::uint32_t(limit) - 1u);
    return result > highest ? highest : result;
}

namespace {
// Seasons since the person was last at the club (0: there now, -1: never), within `within` (excluded).
bool at_club_within(Cpu& c, std::uint32_t entry, std::uint32_t back, std::uint32_t person_at, std::uint32_t club_at,
                    std::int16_t within) {
    const Frame frame(c, entry, 0x40, {});
    if (!club_at || !person_at) return false;
    const auto seasons = std::int16_t(invoke(c, callee::seasons_since_club, back, {c.u32(c.toc(toc::stats)), person_at, club_at}));
    return seasons != -1 && seasons < within;
}
}
bool at_club_within_a_season(Cpu& c, std::uint32_t person_at, std::uint32_t club_at) {
    return at_club_within(c, 0x106ff490, 0x106ff4c0, person_at, club_at, 2);
}
bool at_club_within_two_seasons(Cpu& c, std::uint32_t person_at, std::uint32_t club_at) {
    return at_club_within(c, 0x106ff4f0, 0x106ff520, person_at, club_at, 3);
}

void record_match(Cpu& c, std::uint32_t match, std::uint32_t sheets) {
    const Frame frame(c, 0x106fdb30, 0x60, {.first_gpr = 22});
    constexpr std::uint32_t home_club = 0x20, away_club = 0x1c, sheet_size = 0x664, lineup = 0x194, line_size = 0x4c,
                            line_extra = 4, line_came_on = 0x32, lines = 20, starters = 11;
    for (std::uint32_t side = 0; side < 2; ++side) {
        const auto club_at = c.u32(match + (side ? away_club : home_club));
        if (std::int8_t(invoke(c, callee::human_runs_club, 0x106fdb90, {club_at, 0, 1, 0, 0})) == 0) continue;
        const auto manager = c.u32(club_at + club::human_manager);
        const auto knowledge = manager ? knowledge_of_manager(c, c.u32(c.toc(toc::tables)), manager) : 0;
        if (!knowledge) continue;
        for (std::uint32_t n = 0; n < lines; ++n) {
            const auto line = sheets + side * sheet_size + n * line_size + lineup;
            if (c.i32(line) == -1 || c.i32(line + line_extra) == -1) continue;
            if (n >= starters && c.i8(line + line_came_on) == -1) continue;  // an unused substitute
            const auto person_at = c.u32(c.toc(toc::people)) + c.u32(line) * person::size;
            if (!person_at || c.i32(person_at + person::id) < people_count(c)) sight(c, knowledge, person_at, 0);
        }
    }
}
}
