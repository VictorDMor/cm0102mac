// CUP.CPP, block `rounds`: the two routines that reorder the sides of a round's ties, 0x10485d70 (same club group
// apart) and 0x10481ad0 (the three side rules). Both are loops over the ties of one round that swap a tie's two
// sides with their feeder links; callers read r4-r10 after 0x10481ad0 (registers_after.txt), so each path leaves
// them as the original does.
#include "cup_base_rounds_detail.hpp"

namespace cm::aot::cup_base::rounds {
namespace {
// The match table row of a round, re-read from the cup each time as the original does.
std::uint32_t matches_of(Cpu& c, std::uint32_t cup, std::uint32_t round_index) {
    return c.u32(c.u32(cup + cup::round_matches) + round_index * 4);
}
// A tie's two sides and their feeder links exchanged, in the original's store order: side 0 <- side 1, feeder 0
// <- feeder 1 (round, then match), side 1 <- old side 0, feeder 1 <- old feeder 0.
void swap_sides(Cpu& c, std::uint32_t cup, std::uint32_t round_index, std::uint32_t row) {
    const auto rows = [&] { return matches_of(c, cup, round_index) + row; };
    const auto old_first = c.u16(rows() + tie::first), old_round = c.u16(rows() + tie::first_feeder_round),
               old_match = c.u16(rows() + tie::first_feeder_match), second = c.u16(rows() + tie::second);
    c.put16(rows() + tie::first, second);
    c.put16(rows() + tie::first_feeder_round, c.u16(rows() + tie::second_feeder_round));
    c.put16(rows() + tie::first_feeder_match, c.u16(rows() + tie::second_feeder_match));
    c.put16(rows() + tie::second, old_first);
    c.put16(rows() + tie::second_feeder_round, old_round);
    c.put16(rows() + tie::second_feeder_match, old_match);
}
}

void separate_same_nation_ties(Cpu& c, std::uint32_t cup, std::uint32_t round_arg) {
    // No prologue: the original saves r24-r31 below the stack pointer (dead) and restores them. It leaves r5 the
    // rounds table, r6/r10 its constants, r7 the first tie's counter, r8 that counter's halfword and r9 what its
    // last pass put there (the rounds table, or the match table / 0 after a swap).
    const auto round = sext16(round_arg);
    const auto round_index = round;
    const auto r6 = round * 4, r10 = round * round_row::size + round_row::ties;
    std::uint32_t i = 0, r9 = c.r(9);
    const auto row_of = [](std::uint32_t index) { return sext16(index) * tie::size; };
    // The links of a swapped tie's new feeders: each feeder tie's flag says which side it fills.
    const auto flag_feeders = [&](std::uint32_t row) {
        auto rows = matches_of(c, cup, round_index);
        const auto first_round = c.i16(rows + row + tie::first_feeder_round);
        if (first_round != -1) {
            const auto match = c.i16(rows + row + tie::first_feeder_match);
            c.put16(c.u32(c.u32(cup + cup::round_matches) + std::uint32_t(first_round) * 4) + std::uint32_t(match) * tie::size + tie::side_flag, 1);
        }
        rows = matches_of(c, cup, round_index);
        r9 = rows;
        const auto second_round = c.i16(rows + row + tie::second_feeder_round);
        if (second_round != -1) {
            const auto match = c.i16(rows + row + tie::second_feeder_match);
            c.put16(c.u32(c.u32(cup + cup::round_matches) + std::uint32_t(second_round) * 4) + std::uint32_t(match) * tie::size + tie::side_flag, 0);
            r9 = 0;
        }
    };
    for (;; ++i) {
        const auto rounds = c.u32(cup + cup::rounds);
        if (!(std::int32_t(sext16(i)) < c.i16(rounds + r10))) {
            c.set({{5, rounds}, {6, r6}, {7, i}, {8, sext16(i)}, {9, r9}, {10, r10}});
            return;
        }
        std::uint32_t j = sext16(i + 1);
        const auto first_row = row_of(i);
        for (;;) {
            r9 = c.u32(cup + cup::rounds);
            if (!(std::int32_t(sext16(j)) < c.i16(r9 + r10))) break;
            const auto row = row_of(j);
            bool swapped = false;
            // Side 0 of both ties, then (when that did not match) side 1 of both.
            for (const std::uint32_t side : {tie::first, tie::second}) {
                const auto rows = matches_of(c, cup, round_index);
                const auto a = c.i16(rows + first_row + side);
                if (a == -1) continue;
                const auto b = c.i16(rows + row + side);
                if (b == -1) continue;
                const auto entrants = c.u32(cup + cup::entrants);
                const auto group_a = c.u32(c.u32(entrants + std::uint32_t(a) * 8) + 0x3c),
                           group_b = c.u32(c.u32(entrants + std::uint32_t(b) * 8) + 0x3c);
                if (group_a != group_b || group_a == 0) continue;
                swap_sides(c, cup, round_index, row);
                flag_feeders(row);
                swapped = true;
                break;
            }
            if (swapped) break;
            ++j;
        }
    }
}

void order_tie_sides(Cpu& c, std::uint32_t cup, std::uint32_t round_arg, std::uint32_t first_arg) {
    const Frame frame(c, 0x10481ad0, 0x70, {.first_gpr = 26});
    Registers r(c);
    const auto round = sext16(round_arg), round_index = round;
    const auto start_given = std::int32_t(sext16(first_arg)) != -1;
    std::uint32_t end;
    if (start_given) {
        end = sext16(first_arg + 1);
    } else {
        r.v[4] = c.u32(cup + cup::rounds);
        r.v[3] = round * round_row::size;
        r.v[5] = 0;
        end = sext16(c.u16(r.v[4] + r.v[3] + round_row::ties));
    }
    r.v[7] = end;
    r.v[6] = c.u32(cup + cup::rounds);
    r.v[4] = round;
    const auto flags = sext16(c.u16(r.v[6] + round * round_row::size + round_row::rule_flags));
    r.v[3] = flags;
    std::uint32_t j = r.v[5];
    const auto row_of = [](std::uint32_t index) { return sext16(index) * tie::size; };
    if (flags & 1) {
        // Ties whose two clubs the cup's virtual +0x98 calls equal: the sides swap. r26 is the second club at the
        // call, r27 the end, r28 the cup, r29 the round word, r30 the counter, r31 the round's pointer offset.
        const auto r31 = round * 4, r27 = sext16(end);
        for (; std::int32_t(sext16(j)) < std::int32_t(r27); ++j) {
            const auto row = row_of(j);
            r.v[4] = c.u32(cup + cup::round_matches);
            r.v[3] = row;
            r.v[5] = c.u32(r.v[4] + r31);
            const auto first = sext16(c.u16(r.v[5] + row));
            r.v[4] = first;
            if (std::int32_t(first) == -1) continue;
            const auto second = sext16(c.u16(r.v[5] + row + tie::second));
            if (std::int32_t(second) == -1) continue;
            r.v[5] = c.u32(cup + cup::entrants);
            const auto club_second = c.u32(r.v[5] + second * 8);
            r.v[4] = c.u32(r.v[5] + first * 8);
            r.v[5] = club_second;
            r.v[3] = cup;
            c.set({{26, club_second}, {27, r27}, {28, cup}, {29, round_arg}, {30, j}, {31, r31}});
            r.publish();
            const auto answer = virtual_call(c, 0x98, 0x10481b9c, {cup, r.v[4], r.v[5]});
            r.take();
            if (answer == 0 || answer != club_second) continue;
            const auto rows = [&] { return matches_of(c, cup, round_index); };
            const auto old_first = sext16(c.u16(rows() + row + tie::first)),
                       old_round = sext16(c.u16(rows() + row + tie::first_feeder_round)),
                       old_match = sext16(c.u16(rows() + row + tie::first_feeder_match));
            swap_sides(c, cup, round_index, row);
            r.v[3] = rows();
            r.v[4] = old_match; r.v[5] = old_round; r.v[6] = old_first;
            r.v[7] = row + tie::second_feeder_match; r.v[8] = rows();
            r.v[9] = row + tie::second_feeder_round; r.v[10] = row + tie::second;
        }
    } else if (flags & 8) {
        // The side with the smaller entrant state goes second.
        const auto r3 = round * 4;
        r.v[3] = r3;
        r.v[4] = sext16(r.v[7]);
        for (; std::int32_t(sext16(j)) < std::int32_t(r.v[4]); ++j) {
            const auto row = row_of(j);
            r.v[5] = c.u32(cup + cup::round_matches);
            const auto rows = c.u32(r.v[5] + r3);
            const auto first = sext16(c.u16(rows + row));
            if (std::int32_t(first) == -1) continue;
            const auto second = sext16(c.u16(rows + row + tie::second));
            r.v[8] = second;
            if (std::int32_t(second) == -1) continue;
            const auto entrants = c.u32(cup + cup::entrants);
            r.v[7] = entrants;
            r.v[6] = sext8(c.u8(entrants + first * 8 + Entrant::state));
            r.v[5] = sext8(c.u8(entrants + second * 8 + Entrant::state));
            if (!(std::int32_t(r.v[6]) < std::int32_t(r.v[5]))) continue;
            const auto rows_of = [&] { return matches_of(c, cup, round_index); };
            const auto old_round = sext16(c.u16(rows + row + tie::first_feeder_round)),
                       old_match = sext16(c.u16(rows + row + tie::first_feeder_match));
            swap_sides(c, cup, round_index, row);
            r.v[8] = first; r.v[7] = old_round; r.v[6] = old_match;
            r.v[9] = row + tie::second_feeder_match; r.v[5] = rows_of(); r.v[10] = rows_of();
        }
    } else if (std::int32_t(c.u32(cup + competition::word_24)) != -1) {
        // The tie whose second club belongs to the cup's nation (the nation record's word +0 is the cup's +0x24)
        // swaps sides.
        const auto r0 = round * 4;
        r.v[3] = sext16(end);
        for (;; ++j) {
            r.v[4] = sext16(j);
            if (!(std::int32_t(sext16(j)) < std::int32_t(r.v[3]))) break;
            const auto row = row_of(j);
            r.v[5] = c.u32(cup + cup::round_matches);
            const auto rows = c.u32(r.v[5] + r0);
            const auto first = sext16(c.u16(rows + row));
            if (std::int32_t(first) == -1) continue;
            const auto second = sext16(c.u16(rows + row + tie::second));
            r.v[7] = second;
            if (std::int32_t(second) == -1) continue;
            r.v[5] = c.u32(cup + cup::entrants);
            r.v[4] = c.u32(r.v[5] + second * 8);
            if (r.v[4] == 0) continue;
            r.v[4] = c.u32(r.v[4] + 0x3c);
            if (r.v[4] == 0) continue;
            r.v[5] = c.u32(r.v[4]);
            r.v[4] = c.u32(cup + competition::word_24);
            if (r.v[5] != r.v[4]) continue;
            const auto rows_of = [&] { return matches_of(c, cup, round_index); };
            const auto old_round = sext16(c.u16(rows + row + tie::first_feeder_round)),
                       old_match = sext16(c.u16(rows + row + tie::first_feeder_match));
            swap_sides(c, cup, round_index, row);
            r.v[4] = rows_of(); r.v[5] = old_match; r.v[6] = old_round; r.v[7] = first;
            r.v[8] = row + tie::second_feeder_match; r.v[9] = rows_of(); r.v[10] = row + tie::second_feeder_round;
        }
    }
    r.publish();
}
}
