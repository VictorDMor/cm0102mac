// CUP.CPP, block `result`: outcomes 1 and 2 of CUP::process_fixture (0x1047d02c..0x1047da74): the home or the away
// club goes through. In the final the two clubs get their final places and the cup may be finished; in any other
// tie the winner is entered into the next round's match row, the loser is put into the third-place match when
// there is one, and the next round's tie may then be scheduled (cup_base_result_next.cpp).
#include "cup_base_result_detail.hpp"

namespace cm::aot::cup_base::result {

namespace {
// The calls of an ending, by outcome (the home club goes through: outcome 1) and, for a tie, by the row's side
// word (0: the clubs are written at the next row's +2; else its +0).
struct FinalCalls { std::uint32_t tell_winner, tell_loser, note_winner, note_loser, prize; };
constexpr FinalCalls final_calls[2] = {{0x1047d658, 0x1047d690, 0x1047d6a4, 0x1047d6b4, 0x1047d6e0},   // away through
                                       {0x1047d160, 0x1047d198, 0x1047d1ac, 0x1047d1bc, 0x1047d1e8}};  // home through
struct TieCalls { std::uint32_t tell_winner, tell_loser_final_row, tell_loser, note_winner, note_loser, prize; };
// [outcome][side == 0]
constexpr TieCalls tie_calls[2][2] = {
    {{0x1047d7dc, 0x1047d840, 0x1047d888, 0x1047d9d8, 0x1047d9e8, 0x1047da14},    // away through, side != 0
     {0x1047d914, 0x1047d97c, 0x1047d9c4, 0x1047d9d8, 0x1047d9e8, 0x1047da14}},   // away through, side == 0
    {{0x1047d2e4, 0x1047d348, 0x1047d390, 0x1047d4e0, 0x1047d4f0, 0x1047d51c},    // home through, side != 0
     {0x1047d41c, 0x1047d484, 0x1047d4cc, 0x1047d4e0, 0x1047d4f0, 0x1047d51c}}};  // home through, side == 0
}

// The virtual +0x48 on an entrant: (club, its result byte, the competition's owner byte, fixture, context).
void Process::tell_entrant(std::uint32_t entrant, std::uint32_t state, std::uint32_t bl) {
    vcall(slot::entrant_result, bl, {cup, c.u32(entrant_row(entrant)), state, c.u8(cup + competition::owner), fixture, context});
}

std::uint32_t Process::go_through(bool home) {
    // The last round's index when the last round is the final (label 0x8c), else -1: the third-place match's round.
    const auto last = sext16(c.u16(cup + competition::rounds)) - 1;
    held[15] = 0xffffffffu;
    if (lha(round_row_at(last) + round_row::label) == 0x8c) held[15] = last;
    if (next_slot == 0xffffffffu) return final_through(home);
    tie_through(home);
    return next_round();
}

std::uint32_t Process::final_through(bool home) {
    const auto round = round_of_fixture();
    const auto offset = match_index * cup::match_row;
    const auto matches = matches_of(round);
    const auto first = lha(matches + offset + match::home);
    const bool first_is_home = c.u32(entrant_row(first)) == c.u32(fixture + fixture::home);
    const auto second = lha(matches + offset + match::away);
    const auto home_side = first_is_home ? first : second, away_side = first_is_home ? second : first;
    const auto winner = home ? home_side : away_side, loser = home ? away_side : home_side;
    // The registers: outcome 1 keeps (winner, loser) in r18/r19, outcome 2 in r15/r14.
    if (home) { held[18] = winner; held[19] = loser; }
    else { held[15] = winner; held[14] = loser; }
    const bool final_label = lha(round_row_at(round_of_fixture()) + round_row::label) == 0x8c;
    c.put8(entrant_row(winner) + 5, final_label ? 3 : 1);
    c.put8(entrant_row(loser) + 5, final_label ? 4 : 2);
    const auto& calls = final_calls[home ? 1 : 0];
    tell_entrant(winner, c.u8(entrant_row(winner) + 5), calls.tell_winner);
    tell_entrant(loser, c.u8(entrant_row(loser) + 5), calls.tell_loser);
    call(callee::entrant_note, calls.note_winner, {cup, winner, lha(fixture + fixture::half_34)});
    call(callee::entrant_note, calls.note_loser, {cup, loser, lha(fixture + fixture::half_34)});
    call(callee::prize_money, calls.prize, {cup, entrant_row(winner), entrant_row(loser), lha(fixture + fixture::half_34), fixture});
    return entrants_decided();
}

void Process::tie_through(bool home) {
    const auto round = round_of_fixture();
    const auto offset = match_index * cup::match_row;
    const auto matches = matches_of(round);
    const bool side_is_zero = lha(matches + offset + match::side) == 0;
    const auto first = lha(matches + offset + match::home);
    const bool first_is_home = c.u32(entrant_row(first)) == c.u32(fixture + fixture::home);
    const auto second = lha(matches + offset + match::away);
    const auto home_side = first_is_home ? first : second, away_side = first_is_home ? second : first;
    const auto winner = home ? home_side : away_side, loser = home ? away_side : home_side;
    held[18] = winner;
    held[19] = loser;
    const auto& calls = tie_calls[home ? 1 : 0][side_is_zero ? 1 : 0];
    // The winner goes into the next round's row (its first club word, or the second when the side word is 0).
    const auto into_row = [&](std::uint32_t target_round, std::uint32_t entrant) {
        c.put16(matches_of(target_round) + next_slot * cup::match_row + (side_is_zero ? 2 : 0), entrant);
    };
    into_row(next_target, winner);
    tell_entrant(winner, c.u8(entrant_row(winner) + 5), calls.tell_winner);
    if (round_of_fixture() == sext16(c.u16(cup + competition::rounds)) - 3 && std::int32_t(held[15]) > -1) {
        // The loser of the semi-finals goes into the third-place match.
        into_row(held[15], loser);
        tell_entrant(loser, 0xffffffffu, calls.tell_loser_final_row);
    } else {
        c.put8(entrant_row(loser) + 5, 0xff);
        tell_entrant(loser, c.u8(entrant_row(loser) + 5), calls.tell_loser);
    }
    call(callee::entrant_note, calls.note_winner, {cup, winner, lha(fixture + fixture::half_34)});
    call(callee::entrant_note, calls.note_loser, {cup, loser, lha(fixture + fixture::half_34)});
    call(callee::prize_money, calls.prize, {cup, entrant_row(winner), entrant_row(loser), lha(fixture + fixture::half_34), fixture});
}

}
