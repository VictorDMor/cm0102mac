// CUP.CPP, block `result`: CUP::process_fixture (0x1047bfb0, vt +0xc), a played fixture applied to the cup.
// MATCH_DAY.CPP calls it through the vtable for each fixture of a cup; EUROPEAN_CUP, INTERTOTO_CUP and UEFA_CUP
// call it directly. It records the result in the cup's match table and decides, through the derived cup's two
// virtuals (+0xa0 for a tie, +0xa4 for a single leg of a two-legged tie), what the result means:
//   -1 an error, 0 level (another leg or a replay is scheduled), 1 the home club goes through, 2 the away club
//   goes through, 3 a replay at once, 4 postponed (the fixture is moved a week or four days on).
// Then it pays prize money, tells the clubs and the fixture manager, and, when the winner's next tie has both
// clubs, schedules it (cup_base_result_next.cpp). The phases below follow the original's order of calls, stores
// and RNG-free branches; the non-volatile registers it holds (r14-r22 change as it runs) are mirrored in
// `held` and written before each call, because the callees' prologues save them and the imports' observers
// compare them. The fixture manager's copy of the fixture is a 0x50-byte local at sp+0x360, written field by
// field at the original offsets: its other bytes keep what the stack held, which the callees copy.
// Files: cup_base_result.cpp (this: the checks, the statistics, the match row, the outcome), _through.cpp (a
// club goes through), _play_on.cpp (level or postponed), _next.cpp (the winner's next tie and its news).
#include "cup_base_result_detail.hpp"
#include "cup_base_result.hpp"

namespace cm::aot::cup_base::result {

Process::Process(Cpu& cpu, std::uint32_t cup_, std::uint32_t fixture_, std::uint32_t context_)
    : c(cpu), cup(cup_), fixture(fixture_), context(context_), frame(c, 0x1047bfb0, 0x4e0, {.first_gpr = 14}) {
    for (unsigned n = 14; n < 32; ++n) held[n] = c.r(n);  // r19, r21, r22 hold the caller's until set below
    held[14] = c.toc(toc::competitions);
    held[15] = c.toc(comp::toc::text);
    held[16] = 0;
    held[18] = c.toc(cell::finance);
    held[23] = c.toc(cell::fixtures);
    held[24] = c.toc(cell::today);
    held[25] = c.toc(toc::news);
    held[26] = c.toc(toc::success);
    held[27] = c.toc(toc::message);
    held[28] = c.toc(toc::pool);
    held[29] = cup;
    held[30] = fixture;
    held[31] = context;
}

void Process::sync() {
    for (unsigned n = 14; n < 32; ++n) c.set(n, held[n]);
}
std::uint32_t Process::call(std::uint32_t entry, std::uint32_t bl, std::initializer_list<std::uint32_t> arguments, Leftovers leftovers) {
    sync();
    unsigned n = 3;
    for (const auto argument : arguments) c.set(n++, argument);
    for (const auto& [reg, value] : leftovers) c.set(reg, value);
    c.call(entry, bl + 4);
    return c.r(3);
}
std::uint32_t Process::vcall(std::uint32_t slot, std::uint32_t bl, std::initializer_list<std::uint32_t> arguments) {
    sync();
    return comp::virtual_call(c, slot, bl + 4, arguments);
}
std::uint32_t Process::fail(const CheckSite& where, std::uint32_t value) {
    sync();
    report(c, frame, where);
    return value;
}

std::uint32_t Process::run() {
    if (const auto done = check_arguments()) return *done;
    if (const auto done = record_statistics()) return *done;
    if (const auto done = locate_match()) return *done;
    if (const auto done = store_leg_score()) return *done;
    const auto outcome = ask_outcome();
    note_goals();
    switch (outcome) {
    case outcome_error: return fail(site::no_outcome, 0xffffffffu);
    case outcome_replay: return schedule_replay();
    case outcome_level: return play_on();
    case outcome_postponed: return postpone();
    case outcome_home_through: return go_through(true);
    case outcome_away_through: return go_through(false);
    default: return fail(site::unknown_outcome, 0xffffffffu);
    }
}

// 0x1047bff4..0x1047c124: the fixture must exist; a stage's cup (owner -1) hands a fixture of one of its stages on
// to that stage (vt +0xc); any other fixture belongs to the cup with the same owner byte, or it is not ours (0).
std::optional<std::uint32_t> Process::check_arguments() {
    held[17] = c.u32(c.u32(cup + competition::record));  // the competition's id
    held[20] = c.u8(fixture + fixture::stage);
    if (fixture == 0) return fail(site::no_fixture, 0xffffffffu);
    const auto owner = sext8(c.u8(cup + competition::owner));
    if (owner == 0xffffffffu) {
        const auto stage = sext8(held[20]);
        c.set(6, stage);
        if (stage != 0xffffffffu) {
            if (std::int32_t(stage) > c.i32(cup + cup_last_stage) || std::int32_t(sext8(held[20])) < 0) return fail(site::bad_stage, 0);
            const auto object = c.u32(c.u32(cup + cup_stages) + stage * 4);
            // r4 and r5 still hold the fixture and the context the caller passed.
            return vcall(slot::process_stage, 0x1047c0f8, {object, fixture, context});
        }
    }
    if (sext8(c.u8(cup + competition::owner)) != sext8(c.u8(fixture + fixture::stage))) return 0;
    return std::nullopt;
}

// 0x1047c124..0x1047c284: the competition's statistics take the fixture; a fixture of an earlier year is ignored
// (-1); the entrants of all rounds are counted; each club earns the round's prize money; the match row is found.
std::optional<std::uint32_t> Process::record_statistics() {
    const auto table = c.u32(held[14]);
    c.set(4, table);
    const auto object = c.u32(table + c.u32(c.u32(cup + competition::record)) * 4);
    if (object != 0) {
        const auto statistics = c.u32(object + cup_stats);
        if (statistics != 0) call(callee::comp_stats, 0x1047c15c, {statistics, fixture, context, 0, 0});
    }
    if (std::int32_t(c.u16(fixture + fixture::year)) < std::int32_t(c.i16(cup + competition::year))) return 0xffffffffu;
    std::uint32_t index = 0;
    while (std::int32_t(sext16(index)) < std::int32_t(sext16(c.u16(cup + competition::rounds)))) {
        c.set(5, rounds());
        held[16] += lha(round_row_at(sext16(index)) + round_row::entrants);
        ++index;
    }
    c.set(4, sext16(index));
    // The prize money of the fixture's round (r4 keeps the rounds table, r5 does for the payments).
    const auto prize = [&] { c.set(4, rounds()); return c.u32(round_row_at(round_of_fixture()) + round_row::prize); };
    if (std::int32_t(prize()) > 0) {
        auto club = call(callee::finance_club, 0x1047c1d4, {c.u32(held[18]), c.u32(fixture + fixture::home)});
        call(callee::finance_pay, 0x1047c1ec, {club, prize()}, {{5, rounds()}});
        club = call(callee::finance_club, 0x1047c1f8, {c.u32(held[18]), c.u32(fixture + fixture::away)});
        call(callee::finance_pay, 0x1047c210, {club, prize()}, {{5, rounds()}});
    }
    return std::nullopt;
}

std::optional<std::uint32_t> Process::locate_match() {
    match_index = call(callee::find_match, 0x1047c21c, {cup, fixture});
    held[18] = match_index;
    if (match_index == 0xffffffffu) return fail(site::no_match, 0xffffffffu);
    return std::nullopt;
}

// 0x1047c284..0x1047c38c: the fixture's score block goes into the match row's slot for the leg being played (the
// legs played so far plus the replay flag: 0, 1 or 2; anything else is an error).
std::optional<std::uint32_t> Process::store_leg_score() {
    const auto row = [&] { return matches_of(round_of_fixture()) + match_index * cup::match_row; };
    legs = std::uint32_t(c.i8(row() + match::legs_played)) + std::uint32_t(c.i8(row() + match::replay));
    held[19] = legs;
    next_slot = lha(row() + match::slot);
    held[21] = next_slot;
    static constexpr struct { std::uint32_t offset, bl; } legs_slots[3] = {
        {match::first_leg, 0x1047c2f0}, {match::second_leg, 0x1047c30c}, {match::third_leg, 0x1047c328}};
    if (std::int32_t(legs) < 0 || std::int32_t(legs) > 2) return fail(site::bad_legs, 0xffffffffu);
    const auto& slot_of_leg = legs_slots[legs];
    // BlockMoveData(source, destination, count): the fixture's block into the row.
    call(callee::block_move, slot_of_leg.bl, {fixture + fixture::score, row() + slot_of_leg.offset, fixture::score_size},
         {{6, match_index * cup::match_row}});
    c.set(2, c.u32(c.sp() + 0x14));
    return std::nullopt;
}

// 0x1047c38c..0x1047c404: what the result means, from the derived cup. A two-legged round (rule 2) asks about the
// leg unless this was its replay (the third); every other asks about the tie.
std::uint32_t Process::ask_outcome() {
    const auto round = round_of_fixture();
    const auto row = matches_of(round) + match_index * cup::match_row;
    next_target = lha(row + match::next_round);
    held[22] = next_target;
    // r6 and r7 keep the round's match table and the row's offset (+0x12) for the virtual, which the glue passes on.
    c.set({{6, matches_of(round)}, {7, match_index * cup::match_row + match::next_round}});
    if (sext8(c.u8(round_row_at(round) + round_row::rule)) == 2 && legs != 2) return vcall(slot::leg_decider, 0x1047c3e0, {cup, row, fixture});
    c.set(5, rounds());
    return vcall(slot::tie_decider, 0x1047c3fc, {cup, fixture});
}

// 0x1047c404..0x1047c488: the two clubs' goals into the match row, in the order of the row's clubs.
void Process::note_goals() {
    const auto round = round_of_fixture();
    const auto offset = match_index * cup::match_row;
    const auto first_club = c.u32(entrant_row(lha(matches_of(round) + offset + match::home)));
    const bool first_is_home = c.u32(fixture + fixture::home) == first_club;
    // The goals of the row's first club, then of its second; the registers keep the second goal byte (r6), the
    // first (r7), the round's match table (r4) and the table of tables (r5).
    const auto first_goals = first_is_home ? fixture::home_goals : fixture::away_goals;
    const auto second_goals = first_is_home ? fixture::away_goals : fixture::home_goals;
    c.put8(matches_of(round) + offset + match::first_goals, c.u8(fixture + first_goals));
    c.put8(matches_of(round_of_fixture()) + offset + match::second_goals, c.u8(fixture + second_goals));
    c.set({{4, matches_of(round_of_fixture())}, {5, c.u32(cup + cup::round_matches)}, {6, c.u8(fixture + second_goals)}, {7, c.u8(fixture + first_goals)}});
}

// Outcome 3 (0x1047c4e8..0x1047c664): the match row is marked as replayed and a new fixture a day later goes into
// the fixture manager (its goals swapped when the flags say the sides changed, else its score and a flags check).
std::uint32_t Process::schedule_replay() {
    const auto row = [&] { return matches_of(round_of_fixture()) + match_index * cup::match_row; };
    c.put8(row() + match::replay, 1);
    call(callee::date_from, 0x1047c520, {frame.at(0x1b0), sext16(c.u16(fixture + fixture::day)), c.u16(fixture + fixture::year)});
    call(callee::date_add, 0x1047c530, {frame.at(0x118), frame.at(0x1b0), 1});
    // The new date's year, day and extra byte, and the fixture manager's stack arguments.
    c.put16(frame.at(0x4c), c.u16(frame.at(0x11a)));
    c.put16(frame.at(0x4a), c.u16(frame.at(0x118)));
    c.put8(frame.at(0x48), 0);
    c.put32(frame.at(0x38), c.u32(fixture + fixture::home_id));
    for (const std::int32_t offset : {0x3c, 0x40, 0x44}) c.put32(frame.at(offset), 0xffffffffu);
    const auto made = call(callee::fixtures_new, 0x1047c5b4,
                           {c.u32(held[23]), frame.at(0x4c), frame.at(0x4a), frame.at(0x48), 1, c.u32(fixture + fixture::competition_id),
                            lha(fixture + fixture::half_34), c.u32(fixture + fixture::away_id)});
    c.set(4, made);
    if (made == 0) return fail(site::no_new_fixture, 3);
    const auto flags = lha(fixture + fixture::flags);
    if ((flags & 0x10) != 0 || (flags & 0x2000) != 0) {
        c.put8(made + fixture::new_goals_home, c.u8(fixture + fixture::away_goals));
        c.put8(made + fixture::new_goals_away, c.u8(fixture + fixture::home_goals));
    } else {
        c.put8(made + fixture::new_score_a, c.u8(fixture + fixture::score + 1));
        c.put8(made + fixture::new_score_b, c.u8(fixture + fixture::score));
        call(callee::match_flags, 0x1047c658, {cup, made, match_index});
    }
    return 3;
}

// The loop every ending of a decided tie ends with (0x1047c78c..0x1047c7dc and its copies): the cup is finished
// (+0xb0) when each of the entrants counted over all rounds has a result byte.
std::uint32_t Process::entrants_decided() {
    const auto limit = sext16(held[16]);
    std::uint32_t flag = 1, index = 0;
    c.set({{4, sext16(index)}});
    while (std::int32_t(sext16(index)) < std::int32_t(limit)) {
        const auto table = c.u32(cup + cup::entrants);
        const auto state = sext8(c.u8(table + (sext16(index) << 3) + 5));
        c.set({{4, state}, {5, table}});
        if (state == 0) {
            flag = 0;
            break;
        }
        ++index;
        c.set(4, sext16(index));
    }
    c.set(6, flag);
    c.put8(cup + cup::byte_b0, flag);
    return 1;
}

}

namespace cm::aot::cup_base {
std::uint32_t process_fixture(Cpu& c, std::uint32_t cup, std::uint32_t fixture, std::uint32_t context) {
    result::Process process(c, cup, fixture, context);
    return process.run();
}
}
