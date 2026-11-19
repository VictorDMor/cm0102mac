// CUP.CPP, block `result`: outcomes 0 (level: another leg or a replay) and 4 (postponed) of CUP::process_fixture
// (0x1047c664..0x1047cbac and 0x1047cbac..0x1047d02c). A cup of one round whose tie has been played the needed
// number of times ends here (every entrant gets its result byte); any other fixture is copied to the local copy,
// moved to its next date and handed to the fixture manager again.
#include "cup_base_result_detail.hpp"

namespace cm::aot::cup_base::result {

// The ending shared by outcomes 0 and 4 (0x1047c66c..0x1047c7e0): in a cup of one round with a single-leg rule
// whose match row has been played the needed number of times, a tie that was not decided in the row (a slot of
// the next round) is an error ("drawn game in cup"); else both clubs of the row get the result byte 1 and the
// cup is finished when every entrant has one.
std::optional<std::uint32_t> Process::single_round_decided(const SingleRound& where) {
    if (c.i16(cup + competition::rounds) != 1) return std::nullopt;
    const auto round = lha(fixture + fixture::half_34);
    // r5, r7 and r8 keep the round, its row's offset and the rounds table for what follows.
    c.set({{5, round}, {7, round * cup::round_row}, {8, rounds()}});
    if (std::int32_t(lbz_extsb(round_row_at(round) + round_row::rule)) >= 2) return std::nullopt;
    const auto offset = match_index * cup::match_row;
    c.set({{4, offset}, {6, matches_of(round)}});
    if (std::int32_t(lbz_extsb(matches_of(round) + offset + match::legs_played)) <
        std::int32_t(lbz_extsb(round_row_at(round) + round_row::legs_needed))) return std::nullopt;
    if (next_slot != 0xffffffffu) {
        call(callee::format, where.format_bl,
             {held[15], c.toc(toc::pool) + text::drawn_game, round, c.u32(cup + competition::record) + 0x1a,
              round * cup::round_row, rounds()});
        return fail(where.assertion, 0);
    }
    const auto row = matches_of(round) + offset;
    const auto first = lha(row + match::home), second = lha(row + match::away);
    c.set(6, 1);
    held[18] = first;
    held[19] = second;
    c.put8(entrant_row(first) + 5, 1);
    c.put8(entrant_row(second) + 5, 1);
    call(callee::entrant_note, where.note_first_bl, {cup, first, lha(fixture + fixture::half_34)});
    call(callee::entrant_note, where.note_second_bl, {cup, second, lha(fixture + fixture::half_34)});
    return entrants_decided();
}

// 0x1047c944..0x1047c964: the copy's two clubs and their ids change places. Returns what the original's r7 and
// r8 keep: the copy's former first club and id.
std::pair<std::uint32_t, std::uint32_t> Process::swap_sides_in_copy() {
    const auto first = c.u32(copy(fixture::home)), second = c.u32(copy(fixture::away));
    const auto first_id = c.u32(copy(fixture::home_id)), second_id = c.u32(copy(fixture::away_id));
    c.put32(copy(fixture::home), second);
    c.put32(copy(fixture::away), first);
    c.put32(copy(fixture::home_id), second_id);
    c.put32(copy(fixture::away_id), first_id);
    return {first, first_id};
}

// 0x1047c968..0x1047c994: when the copy's record word equals the club record word of the club now second, it
// takes the one of the club now first and the id behind it (-1 without one).
void Process::refresh_record_word() {
    const auto word = c.u32(copy(fixture::word_18));
    if (word != c.u32(c.u32(copy(fixture::away)) + 0x3c)) return;
    const auto replacement = c.u32(c.u32(copy(fixture::home)) + 0x3c);
    c.put32(copy(fixture::word_18), replacement);
    c.put32(copy(fixture::word_04), replacement != 0 ? c.u32(replacement) : 0xffffffffu);
}

// The copy's score block set to 0xff (it is the fixture manager's "no score yet").
void Process::clear_score() {
    for (std::uint32_t offset = fixture::score; offset < fixture::score + fixture::score_size; ++offset) c.put8(copy(offset), 0xff);
}

// The weekday of a date: 1 for 5 and 6 (the weekend), else 2.
std::uint32_t Process::weekday_kind(std::uint32_t date, std::uint32_t bl, Leftovers leftovers) {
    const auto weekday = sext16(call(callee::date_weekday, bl, {date}, leftovers));
    return (weekday == 5 || weekday == 6) ? 1 : 2;
}

// The copy's date is moved to tomorrow when the game's date is not before it (0x1047cadc..0x1047cb94 and its
// copies): DATE's pair for the copy's date lands in `compare_buffer`; the game's date was read before the call.
void Process::adjust_for_today(std::int32_t compare_buffer, std::uint32_t compare_bl, std::int32_t add_buffer, std::uint32_t add_bl,
                               Leftovers leftovers) {
    const auto today_day = c.u16(held[24]), today_year = c.u16(held[24] + 2);
    const auto date = call(callee::date_from, compare_bl,
                           {frame.at(compare_buffer), sext16(c.u16(copy(fixture::day))), c.u16(copy(fixture::year))}, leftovers);
    if (date_before(std::uint16_t(today_day), std::uint16_t(today_year), c.u16(date), c.u16(date + 2))) return;
    call(callee::date_add, add_bl, {frame.at(add_buffer), held[24], 1});
    c.set(4, frame_extra(add_buffer));
    c.put16(copy(fixture::day), c.u16(frame.at(add_buffer)));
    c.put16(copy(fixture::year), c.u16(frame.at(add_buffer + 2)));
}

// Outcome 0 (0x1047c664..0x1047cbac): the fixture's copy goes back to the fixture manager for the next leg (or the
// replay), a day later than the game's date at the earliest.
std::uint32_t Process::play_on() {
    static constexpr SingleRound where{site::drawn_level, 0x1047c6e0, 0x1047c778, 0x1047c788};
    if (const auto done = single_round_decided(where)) return *done;
    call(callee::block_move, 0x1047c7f0, {fixture, copy(0), fixture::size});
    c.set(2, c.u32(c.sp() + 0x14));
    const auto row = [&] { return matches_of(round_of_fixture()) + match_index * cup::match_row; };
    if (lbz_extsb(row() + match::legs_played) == 0) {
        // The first leg: the second is a round's `days` later.
        call(callee::date_from, 0x1047c82c, {frame.at(0x198), sext16(c.u16(fixture + fixture::day)), c.u16(fixture + fixture::year)});
        call(callee::date_add, 0x1047c84c, {frame.at(0x108), frame.at(0x198), c.u8(round_row_at(round_of_fixture()) + round_row::days)},
             {{6, rounds()}});
        c.put16(copy(fixture::day), c.u16(frame.at(0x108)));
        c.put16(copy(fixture::year), c.u16(frame.at(0x10a)));
        c.set({{4, frame_extra(0x108)}, {5, c.u16(frame.at(0x10a))}, {6, lha(frame.at(0x108))}});
    }
    call(callee::fixtures_add, 0x1047c898, {c.u32(held[23]), copy(0)});
    const auto dated = call(callee::date_from, 0x1047c8ac, {frame.at(0x100), sext16(c.u16(copy(fixture::day))), c.u16(copy(fixture::year))});
    c.put8(copy(fixture::day_kind), weekday_kind(dated, 0x1047c8b0));
    {
        const auto at_ = row() + match::legs_played;
        c.put8(at_, c.u8(at_) + 1);
    }
    // r4 and r5 keep the played counter's offset in the row and the round's match table.
    c.set({{4, match_index * cup::match_row + match::legs_played}, {5, matches_of(round_of_fixture())}});
    const auto order = [&](std::uint32_t bl, std::uint32_t plus) {
        c.put16(copy(fixture::order), call(callee::order_rank, bl, {lha(copy(fixture::order))}) + plus);
    };
    if (lbz_extsb(row() + match::legs_played) == 1) order(0x1047c924, 1);
    else order(0x1047c938, 2);
    const auto [old_first, old_first_id] = swap_sides_in_copy();
    refresh_record_word();
    const auto bits = c.u32(copy(fixture::bits));
    c.put8(copy(fixture::bits), with_bits(bits, 0xc0, 0));
    clear_score();
    c.put32(copy(fixture::word_08), 0xffffffffu);
    c.put8(copy(fixture::bits), with_bits(c.u8(copy(fixture::bits)), 0x10, 0));
    // Another leg, or the replay (the third match of a tie is not allowed to need one).
    if (legs == 0) {
        c.put16(copy(fixture::flags), lha(round_row_at(round_of_fixture()) + round_row::half_10));
        const bool wide = std::int32_t(lbz_extsb(round_row_at(round_of_fixture()) + round_row::legs_needed)) > 1;
        c.put8(copy(fixture::bits), with_bits(c.u8(copy(fixture::bits)), 0x20, wide ? 0x20 : 0));
    } else if (legs == 1) {
        c.put16(copy(fixture::flags), lha(round_row_at(round_of_fixture()) + round_row::half_12));
        c.put8(copy(fixture::bits), with_bits(c.u8(copy(fixture::bits)), 0x20, 0));
    } else {
        call(callee::format, 0x1047ca80,
             {held[15], c.toc(toc::pool) + text::third_match, lha(fixture + fixture::half_34), c.u32(cup + competition::record) + 0x1a,
              old_first, old_first_id});
        return fail(site::third_replay, 0);
    }
    c.put8(copy(fixture::stage), held[20]);
    c.put8(copy(fixture::home_goals), 0);
    c.put8(copy(fixture::away_goals), 0);
    adjust_for_today(0xf8, 0x1047cb0c, 0xf0, 0x1047cb68, {{6, today_word(1)}, {7, today_word(0)}, {8, 0}});
    call(callee::fixtures_update, 0x1047cba0, {c.u32(held[23]), copy(0), 1});
    return 1;
}

// Outcome 4 (0x1047cbac..0x1047d02c): the same with a later date (a week, or four days when the flags say so).
std::uint32_t Process::postpone() {
    static constexpr SingleRound where{site::drawn_postponed, 0x1047cc28, 0x1047ccc0, 0x1047ccd0};
    if (const auto done = single_round_decided(where)) return *done;
    call(callee::block_move, 0x1047cd38, {fixture, copy(0), fixture::size});
    c.set(2, c.u32(c.sp() + 0x14));
    call(callee::date_from, 0x1047cd50, {frame.at(0x178), sext16(c.u16(copy(fixture::day))), c.u16(copy(fixture::year))});
    const bool four_days = (lha(fixture + fixture::flags) & 0x2000) != 0;
    // The sum lands in its own buffer and is copied back over the date, which DATE.CPP's weekday then reads.
    const auto sum = four_days ? 0xe0 : 0xd0;
    call(callee::date_add, four_days ? 0x1047cd6c : 0x1047cdb0, {frame.at(sum), frame.at(0x178), four_days ? 4u : 7u});
    c.put8(frame.at(0x178 + 4), c.u8(frame.at(sum + 4)));
    c.put16(frame.at(0x178 + 2), c.u16(frame.at(sum + 2)));
    c.put16(frame.at(0x178), c.u16(frame.at(sum)));
    c.put16(copy(fixture::day), c.u16(frame.at(0x178)));
    c.put16(copy(fixture::year), c.u16(frame.at(0x178 + 2)));
    c.put8(copy(fixture::score + 8), 0xff);
    c.put8(copy(fixture::score + 9), 0xff);
    c.put8(copy(fixture::day_kind), weekday_kind(frame.at(0x178), 0x1047ce04, {{4, c.u16(frame.at(0x178 + 2))}, {5, sext16(c.u16(frame.at(0x178)))}}));
    {
        const auto at_ = matches_of(round_of_fixture()) + match_index * cup::match_row + match::legs_played;
        c.put8(at_, c.u8(at_) + 1);
    }
    // r4 and r5 keep the rounds table and the round's match table.
    c.set({{4, rounds()}, {5, matches_of(round_of_fixture())}});
    const auto order = [&](std::uint32_t bl, std::uint32_t plus) {
        c.put16(copy(fixture::order), call(callee::order_rank, bl, {lha(copy(fixture::order))}) + plus);
    };
    if (lbz_extsb(round_row_at(round_of_fixture()) + round_row::legs_needed) == 0) order(0x1047ce88, 5);
    else order(0x1047ce74, 1);
    if ((lha(fixture + fixture::flags) & 0x2000) == 0) swap_sides_in_copy();
    refresh_record_word();
    const auto bits_without_c0 = with_bits(c.u8(copy(fixture::bits)), 0xc0, 0);  // r8 at the date compare
    c.put8(copy(fixture::bits), bits_without_c0);
    clear_score();
    c.put32(copy(fixture::word_08), 0xffffffffu);
    c.put16(copy(fixture::flags), lha(round_row_at(round_of_fixture()) + round_row::half_12));
    c.put8(copy(fixture::bits), with_bits(c.u8(copy(fixture::bits)), 0x10, 0));
    c.put8(copy(fixture::bits), with_bits(c.u8(copy(fixture::bits)), 0x20, 0));
    // No stage or goal stores here: the postponed copy keeps the fixture's marks at +0x3d/+0x3e (the series
    // decider 0x10482640 counts them down), as the listing's 0x1047cbac..0x1047d02c stores nothing to sp+0x39a..0x3a2.
    adjust_for_today(0xc8, 0x1047cf8c, 0xc0, 0x1047cfe8,
                     {{6, lha(round_row_at(round_of_fixture()) + round_row::half_12)}, {7, today_word(0)}, {8, bits_without_c0}, {9, 0},
                      {10, round_of_fixture() * cup::round_row}});
    call(callee::fixtures_update, 0x1047d020, {c.u32(held[23]), copy(0), 1});
    return 1;
}

}
