// CUP.CPP, block `result`: what follows a tie that has a winner (0x1047da74..0x1047e534), the end of outcomes 1 and
// 2. When the winner's next tie now has both clubs (the match row of the next round waiting for them), the
// pairings are made, a fixture for it is built in the local copy (clubs, round, flags, date) and handed to the
// fixture manager and the stadium manager, the clubs' news is filed, and a two-legged round also gets its second
// leg's date.
#include "cup_base_result_detail.hpp"

namespace cm::aot::cup_base::result {

std::uint32_t Process::next_round() {
    const auto rounds_count = lbz_extsb(cup + cup::byte_a9);
    c.set(4, round_of_fixture());  // lha r4: the first two exits return with it
    if (!(std::int32_t(round_of_fixture()) < std::int32_t(rounds_count) - 1 && std::int32_t(next_target) < std::int32_t(rounds_count))) return 1;
    const auto row = [&] { return matches_of(next_target) + next_slot * cup::match_row; };
    c.set(4, matches_of(next_target));
    if (lbz_extsb(row() + match::state) != 3) return 1;
    if (lha(row() + match::home) == 0xffffffffu) return 1;
    if (lha(row() + match::away) == 0xffffffffu) return 1;
    call(callee::pairings, 0x1047daf0, {cup, next_target, next_slot});
    schedule_next_tie();
    // Without the competition's object nobody is told.
    if (c.u32(c.u32(held[14]) + held[17] * 4) == 0) return 1;
    file_news();
    c.set(4, rounds());
    if (lbz_extsb(round_row_at(next_target) + round_row::rule) == 2) second_leg();
    return 1;
}

// 0x1047db04..0x1047e08c: the copy is built from the next round's row (the stack bytes it does not set stay), the
// next tie's two clubs go into it, the date is chosen and the copy goes to the stadium and fixture managers.
void Process::schedule_next_tie() {
    const auto record = c.u32(cup + competition::record);
    if (record != 0) {
        c.put32(copy(fixture::competition_id), c.u32(record));
        c.put32(copy(fixture::record), record);
    } else {
        c.put32(copy(fixture::competition_id), 0xffffffffu);
        c.put32(copy(fixture::record), 0);
    }
    const auto next_row = [&] { return round_row_at(next_target); };
    c.put16(copy(fixture::order), lha(next_row() + round_row::label) + (lbz_extsb(next_row() + round_row::rule) == 2 ? 3 : 0));
    c.put16(copy(fixture::half_32), lha(cup + competition::code));
    c.put16(copy(fixture::half_2c), lha(cup + competition::year));
    c.put16(copy(fixture::half_34), lha(next_row() + round_row::half_0a));
    c.set(5, rounds());
    vcall(slot::round_data, 0x1047db90, {cup, copy(0)});
    // The original's register leftovers from here to the first date call (r9, r10 survive to the return).
    c.set({{9, next_target * cup::round_row}, {10, rounds()}, {4, 0}, {6, 0xffffffffu}, {8, lha(next_row() + round_row::flags)},
           {7, c.u8(next_row() + round_row::byte_18)}, {5, 1}});
    c.put16(copy(fixture::half_36), lha(next_row() + round_row::half_0c));
    c.put16(copy(fixture::flags), lha(next_row() + round_row::flags));
    c.put8(copy(fixture::byte_3a), c.u8(next_row() + round_row::byte_18));
    c.put8(copy(fixture::bits), with_bits(with_bits(c.u8(copy(fixture::bits)), 0xc0, 0), 0x10, 0));
    clear_score();
    c.put32(copy(fixture::word_08), 0xffffffffu);
    c.put8(copy(fixture::byte_3b), 0xff);
    c.put8(copy(fixture::stage), held[20]);
    c.put8(copy(fixture::byte_40), 1);
    c.put8(copy(fixture::byte_41), 1);
    // A round that asks which side is stronger (flag 0x800) records the stronger one (the entrant's state byte).
    if (std::int32_t(lbz_extsb(next_row() + round_row::rule)) < 2 && (lha(copy(fixture::flags)) & 0x800) != 0) {
        const auto matches = matches_of(next_target);
        const auto first = lha(matches + next_slot * cup::match_row + match::home);
        const auto second = lha(matches + next_slot * cup::match_row + match::away);
        const auto state_first = lbz_extsb(entrants() + (first << 3) + 4), state_second = lbz_extsb(entrants() + (second << 3) + 4);
        c.set({{4, (first << 3) + 4}, {5, entrants()}});
        c.put16(copy(fixture::flags), std::int32_t(state_first) < std::int32_t(state_second) ? 0x800 : 0x1000);
    }
    c.set(4, rounds());
    const bool bit_20 = std::int32_t(lbz_extsb(next_row() + round_row::legs_needed)) > 0 &&
                        std::int32_t(lbz_extsb(next_row() + round_row::rule)) < 2;
    c.put8(copy(fixture::bits), with_bits(c.u8(copy(fixture::bits)), 0x20, bit_20 ? 0x20 : 0));
    if (std::int32_t(next_target) < std::int32_t(sext16(c.u16(cup + competition::rounds))) - 1) {
        const auto following = round_row_at(next_target + 1);
        c.set({{5, rounds()}, {4, (next_target + 1) * cup::round_row}});
        c.put16(copy(fixture::half_26), lha(following + round_row::start_day));
        c.put16(copy(fixture::half_24), lha(cup + competition::year) + lbz_extsb(following + round_row::year_offset));
    } else {
        c.put16(copy(fixture::half_26), 0);
        c.put16(copy(fixture::half_24), 0);
    }
    // The next tie's two clubs.
    const auto club_of = [&](std::uint32_t half, std::uint32_t club_at, std::uint32_t id_at, const CheckSite& missing) {
        // r4 is the table of tables for the first club and the round's match table for the second; r5 the entrants.
        c.set({{4, half == match::home ? c.u32(cup + cup::round_matches) : matches_of(next_target)}, {5, entrants()}});
        const auto club = c.u32(entrant_row(lha(matches_of(next_target) + next_slot * cup::match_row + half)));
        c.put32(copy(club_at), club);
        if (club != 0) {
            c.put32(copy(id_at), c.u32(club));
        } else {
            fail(missing, 0);
            c.put32(copy(id_at), 0xffffffffu);
        }
    };
    club_of(match::home, fixture::home, fixture::home_id, site::no_first_club);
    club_of(match::away, fixture::away, fixture::away_id, site::no_second_club);
    // The date: the round's own first day, unless two days from today is not before it (then today's date).
    call(callee::date_add, 0x1047de70, {frame.at(0xb8), held[24], 2});
    const std::uint32_t two_days_day = lha(frame.at(0xb8)), two_days_year = c.u16(frame.at(0xb8 + 2));
    // r4-r7 after the date: the round's offset, the rounds table, the sum's extra byte and day.
    c.set({{4, next_target * cup::round_row}, {5, rounds()}, {6, frame_extra(0xb8)}, {7, lha(frame.at(0xb8))}});
    const auto round_start = [&] { return lha(next_row() + round_row::start_day); };
    const auto round_year = [&] { return lha(cup + competition::year) + lbz_extsb(next_row() + round_row::year_offset); };
    const auto two_days_reach_start = [&] {
        if (std::int32_t(round_year()) < std::int32_t(two_days_year)) return true;
        if (std::int32_t(round_year()) != std::int32_t(two_days_year)) return false;
        return !(std::int32_t(two_days_day) < std::int32_t(round_start()));
    };
    if (two_days_reach_start()) {
        c.set(4, c.u16(held[24] + 2));
        c.put16(copy(fixture::day), c.u16(held[24]));
        c.put16(copy(fixture::year), c.u16(held[24] + 2));
    } else {
        c.set({{5, next_target * cup::round_row}, {6, rounds()}, {7, lha(cup + competition::year)}, {4, round_year()}});
        c.put16(copy(fixture::year), round_year());
        c.put16(copy(fixture::day), round_start());
    }
    c.put8(copy(fixture::day_kind), c.u8(next_row() + round_row::byte_07));
    call(callee::stadium_book, 0x1047df4c,
         {c.u32(c.toc(cell::stadium)), c.u32(next_row() + round_row::word_14), copy(0), c.u32(cup + cup_word_24)}, {{7, rounds()}});
    c.set({{4, next_target * cup::round_row}, {5, rounds()}, {6, lha(cup + competition::year)}});
    if (two_days_reach_start()) call(callee::fixtures_add, 0x1047df98, {c.u32(held[23]), copy(0)}, {{5, rounds()}, {6, lha(cup + competition::year)}});
    // The fixture manager's entry: no score yet, the stage, and a date not before today.
    clear_score();
    c.put8(copy(fixture::stage), held[20]);
    c.put8(copy(fixture::home_goals), 0);
    c.put8(copy(fixture::away_goals), 0);
    adjust_for_today(0xb0, 0x1047dff8, 0xa8, 0x1047e054, {{6, today_word(1)}, {7, today_word(0)}, {8, 0}, {9, 0xffffffffu}});
    call(callee::fixtures_update, 0x1047e08c, {c.u32(held[23]), copy(0), 1});
}

// 0x1047e0ac..0x1047e1f8: when the news is on and either club is one the manager follows, a news item of the tie
// (competition, year, round, the two clubs' ids in the order the first one is followed, the date) is filed for both.
void Process::file_news() {
    const auto news = [&] { return c.u32(held[25]); };
    const auto first_followed = sext8(call(callee::news_on, 0x1047e0b4, {news(), c.u32(copy(fixture::home))}));
    if (first_followed == 0 && sext8(call(callee::news_on, 0x1047e0c8, {news(), c.u32(copy(fixture::away))})) == 0) return;
    const auto item = frame.at(at::news);
    const auto set = [&](std::uint32_t field, std::uint32_t value, std::uint32_t bl) { call(callee::news_set, bl, {item, field, value}); };
    call(callee::news_start, 0x1047e0e0, {item, 0x14, 0});
    set(0, c.u32(c.u32(cup + competition::record)), 0x1047e0f4);
    set(1, lha(cup + competition::code), 0x1047e104);
    c.set(6, rounds());
    set(2, lha(round_row_at(next_target) + round_row::label), 0x1047e120);
    c.set(6, rounds());
    set(5, lbz_extsb(round_row_at(next_target) + round_row::rule) == 2 ? 1 : 0, 0x1047e14c);
    set(6, c.u32(copy(fixture::word_04)), 0x1047e15c);
    set(7, c.u16(copy(fixture::day)), 0x1047e16c);
    set(8, c.u16(copy(fixture::year)), 0x1047e17c);
    if (sext8(call(callee::news_on, 0x1047e188, {news(), c.u32(copy(fixture::home))})) != 0) {
        set(3, c.u32(copy(fixture::home_id)), 0x1047e1a0);
        set(4, c.u32(copy(fixture::away_id)), 0x1047e1b0);
    } else {
        set(3, c.u32(copy(fixture::away_id)), 0x1047e1c4);
        set(4, c.u32(copy(fixture::home_id)), 0x1047e1d4);
    }
    call(callee::news_file, 0x1047e1e4, {news(), item, c.u32(copy(fixture::home))});
    call(callee::news_file, 0x1047e1f4, {news(), item, c.u32(copy(fixture::away))});
}

// 0x1047e1f8..0x1047e51c: a two-legged round's second leg: the copy is dated by the round (the first leg's date
// plus the round's days, or the round's start plus them, whichever DATE.CPP orders later), handed to the fixture
// manager again with its clubs the other way round.
void Process::second_leg() {
    const auto next_row = [&] { return round_row_at(next_target); };
    c.put8(copy(fixture::bits), with_bits(c.u8(copy(fixture::bits)), 0x20, std::int32_t(lbz_extsb(next_row() + round_row::legs_needed)) > 0 ? 0x20 : 0));
    c.put16(copy(fixture::order), lha(next_row() + round_row::label) + 4);
    call(callee::date_from, 0x1047e270, {frame.at(0x140), sext16(c.u16(copy(fixture::day))), c.u16(copy(fixture::year))},
         {{6, lha(next_row() + round_row::label)}, {7, rounds()}});
    const auto start_year = (lha(cup + competition::year) + lbz_extsb(next_row() + round_row::year_offset)) & 0xffff;
    const auto start = call(callee::date_from, 0x1047e2a0, {frame.at(0x98), lha(next_row() + round_row::start_day), start_year},
                            {{6, rounds()}});
    call(callee::date_add, 0x1047e2bc, {frame.at(0xa0), start, c.u8(next_row() + round_row::days)}, {{6, rounds()}});
    call(callee::date_add, 0x1047e2e4, {frame.at(0x90), frame.at(0x140), 3}, {{6, c.u16(frame.at(0xa0 + 2))}});
    const auto start_day = c.u16(frame.at(0xa0)), start_year_after = c.u16(frame.at(0xa0 + 2));
    const auto later_day = c.u16(frame.at(0x90)), later_year = c.u16(frame.at(0x90 + 2));
    c.set({{5, frame_extra(0x90)}, {6, sext16(later_day)}});
    if (date_before(start_day, start_year_after, later_day, later_year)) {
        c.put16(copy(fixture::day), later_day);
        c.put16(copy(fixture::year), later_year);
        call(callee::fixtures_add, 0x1047e374, {c.u32(held[23]), copy(0)}, {{5, later_year}, {6, sext16(later_day)}});
    } else {
        c.put16(copy(fixture::day), start_day);
        c.put16(copy(fixture::year), start_year_after);
    }
    const auto dated = call(callee::date_from, 0x1047e39c, {frame.at(0x88), sext16(c.u16(copy(fixture::day))), c.u16(copy(fixture::year))});
    c.put8(copy(fixture::day_kind), weekday_kind(dated, 0x1047e3a0));
    const auto [old_first, old_first_id] = swap_sides_in_copy();
    call(callee::stadium_book, 0x1047e40c,
         {c.u32(c.toc(cell::stadium)), c.u32(next_row() + round_row::word_14), copy(0), c.u32(cup + cup_word_24)},
         {{7, old_first}, {8, old_first_id}});
    const auto bits_cleared = with_bits(with_bits(c.u8(copy(fixture::bits)), 0xc0, 0), 0x10, 0);  // r8 at the date compare
    c.put8(copy(fixture::bits), bits_cleared);
    clear_score();
    c.put8(copy(fixture::stage), held[20]);
    c.put8(copy(fixture::home_goals), 0);
    c.put8(copy(fixture::away_goals), 0);
    adjust_for_today(0x80, 0x1047e484, 0x78, 0x1047e4e0,
                     {{6, today_word(1)}, {7, today_word(0)}, {8, bits_cleared}, {9, 0}, {10, 0xffffffffu}});
    call(callee::fixtures_update, 0x1047e518, {c.u32(held[23]), copy(0), 1});
}

}
