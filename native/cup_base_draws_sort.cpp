// CUP.CPP, block `draws`: 0x1047fd00 `CUP::sort_round_teams`. Spreads a round's clubs over its ties.
//
// It gathers the round's slots (the two club slots of each tie row, -1 slots as 0xffff/0xff, others with two words
// of the row and the club's state byte, and marks each taken slot of the row -2 "to be drawn"), sorts them by the
// state byte (the comparator 0x10481e70, larger first), then places each slot, in that order and until one has the
// state -1, into a tie: a draw below the number of ties gives the first candidate, and the scan walks on (wrapping)
// to the first candidate that accepts it. Pass 1 (r21 = 0) takes a tie whose two slots are both negative: the
// home slot if it is -2 (either slot when both are, by the sample's parity), else the away slot. Pass 2 (after
// one lap without a taker) takes ties whose home slot is -2 (or away) and, when the round asks for it (flag 4
// of its flags, r20), not against a club of the same nation (club +0x2c); then the flag drops (r20 = 0) for a
// last lap, and a lap without a taker after that is the "assert" at line 0xcff (the placement stands unfinished).
//
// The draw's large arm (a bound beyond 65535, which calls 0x1047a9e0) cannot run here: the bound is a signed
// halfword. The original's registers: r13-r31 are mirrored where the original assigns them (the allocator,
// qsort, rand, the dialogs can reach imports and a trace compares them at every call), and r4-r10 at the return
// are the last delete[]'s leftovers (it keeps r9 and r10), so each block below sets the volatile registers it leaves.
#include "cup_base_draws_detail.hpp"
#include <stdexcept>

namespace cm::aot::cup_base::draws {
using namespace detail;

void sort_round_teams(Cpu& c, std::uint32_t cup, std::uint32_t round) {
    const Frame frame(c, 0x1047fd00, 0xf0, {.first_gpr = 13});
    const auto pool = Pool::at(c);
    const auto pool_text = c.toc(toc::pool), message = c.toc(toc::message), flag = c.toc(toc::success);
    // Prologue: r13 the count, r14 the success flag, r15 the cup, r16 the round (the whole register), r18/r19 the
    // dialog's title and message, r21 the second-pass flag, r24-r27 the stream's cells, r28 the message buffer,
    // r29 the pool.
    c.set({{13, 0}, {14, flag}, {15, cup}, {16, round}, {18, c.toc(-0x79dc)}, {19, c.toc(-0x79e0)}, {21, 0},
           {24, pool.salt}, {25, pool.start}, {26, pool.end}, {27, pool.cursor}, {28, message}, {29, pool_text}});
    const auto round_index = sext16(round);
    const auto round_row = round_index * cup::round_row;  // r3
    const auto rounds = [&] { return c.u32(cup + cup::rounds); };
    const auto tie_rows = [&] { return c.u32(c.u32(cup + cup::round_matches) + round_index * 4); };
    const auto entrants = [&] { return c.u32(cup + cup::entrants); };
    c.set(4, rounds());
    const auto out = invoke(c, callee::allocate_block, 0x1047fd60,
                            {half(c, rounds() + round_row + round_slots) << 3 & 0xfffffff8u, rounds()});
    c.set(23, out);
    if (!out) {
        memory_failure(c, frame, {0x1047fd7c, 0x1047fda0, 0x1047fdac, 0x1047fdbc, 0x1047fdcc, 0x1047fddc, 0x68, 0x40, 0xbf8, text::sort_memory});
        return;
    }
    // The ties' offset in the rounds (r7); r20 the same-nation flag, a signed byte of flag 4.
    const auto ties_offset = round_row + round_ties;
    c.set({{4, rounds()}, {5, round_index * 4}, {6, 0}, {7, ties_offset}});
    c.set(20, sext8(c.u16(rounds() + round_row + round_flags) & same_nation_flag));
    // The gather: both slots of each tie, into the next two entries of `out` (r13 their count).
    std::uint32_t taken = 0;
    const auto slot_at = [&](std::uint32_t index) { return out + (sext16(index) << 3 & 0xfffffff8u); };
    std::uint32_t tie_number = 0;  // r6
    for (;; ++tie_number, c.set(6, tie_number)) {
        c.set(4, sext16(tie_number));
        if (!(std::int32_t(sext16(tie_number)) < std::int32_t(half(c, rounds() + ties_offset)))) break;
        const auto row = sext16(tie_number) * tie::size;  // the tie's offset (r4, then r8)
        c.set(4, row);
        const auto home = half(c, tie_rows() + row);
        c.set(8, home);
        if (home == 0xffffffffu) {
            const auto at = slot_at(taken++);
            c.set({{13, taken}, {4, 0xffffffffu}});
            c.put16(at + slot::team, 0xffff);
            c.put8(at + slot::state, 0xff);
        } else {
            const auto at = slot_at(taken);
            c.put16(at + slot::team, home);
            c.set(18, tie_rows());
            c.set(13, ++taken);
            c.put16(at + slot::first, half(c, tie_rows() + row + tie::home_first));
            const auto second = half(c, tie_rows() + row + tie::home_second);
            c.put16(at + slot::second, second);
            c.set(9, entrants());
            c.put8(at + slot::state, c.u8(entrants() + (half(c, at + slot::team) << 3 & 0xfffffff8u) + 4));
            c.put16(tie_rows() + row + tie::home, 0xfffe);
            c.set({{8, (at - out) + slot::state}, {9, entrants()}, {10, second}});
        }
        c.set(8, row);
        const auto away = half(c, tie_rows() + row + tie::away);
        c.set(4, away);
        if (away == 0xffffffffu) {
            const auto at = slot_at(taken++);
            c.set({{13, taken}, {4, 0xffffffffu}});
            c.put16(at + slot::team, 0xffff);
            c.put8(at + slot::state, 0xff);
        } else {
            const auto at = slot_at(taken);
            c.put16(at + slot::team, away);
            c.set({{4, 0xfffffffeu}, {18, tie_rows()}});
            c.set(13, ++taken);
            c.put16(at + slot::first, half(c, tie_rows() + row + tie::away_first));
            const auto second = half(c, tie_rows() + row + tie::away_second);
            c.put16(at + slot::second, second);
            c.put8(at + slot::state, c.u8(entrants() + (half(c, at + slot::team) << 3 & 0xfffffff8u) + 4));
            c.put16(tie_rows() + row + tie::away, 0xfffe);
            c.set({{8, (at - out) + slot::state}, {9, entrants()}, {10, second}});
        }
    }
    // Sort by the state byte (the comparator 0x10481e70 through the descriptor at TOC -0x47c0).
    const auto count = sext16(taken);
    invoke(c, callee::sort, 0x1047ff90, {out, count, slot::size, c.toc(toc::by_byte_6)});
    c.set({{13, count}, {19, 0}, {30, ties_offset}});
    std::uint32_t index = 0;  // r19
    while (std::int32_t(sext16(index)) < std::int32_t(count)) {
        const auto in = slot_at(index);
        if (std::int8_t(c.u8(in + slot::state)) == -1) break;
        const auto ties = std::int32_t(half(c, rounds() + ties_offset));
        c.set(22, std::uint32_t(ties));
        std::uint32_t candidate = 0;  // r22
        // A round without ties has no slots (the gather runs while j < the same halfword), so no slot reaches this draw.
        // Not expected (no slot comes from a round without ties), but the original does not draw then: the candidate is 0.
        if (ties == 0) c.set(18, 0);
        else {
            pool.advance(0x1047fffc);
            const auto sample = std::int32_t(pool.sample());
            // The large arm (0x10480024.., a bound beyond 65535 through 0x1047a9e0) is out of reach: a signed halfword.
            const auto remainder = std::int32_t(std::uint32_t(sample) - std::uint32_t(divide(sample, ties)) * std::uint32_t(ties));
            candidate = ties > 0 ? std::uint32_t(remainder) : 0u - std::uint32_t(remainder);
            c.set(18, candidate);
        }
        candidate = sext16(candidate);
        const auto start = candidate;
        c.set({{22, candidate}, {31, start}});
        const auto same_nation = [&](std::uint32_t club_a, std::uint32_t club_b) {
            return c.u32(club_b + club_nation) == c.u32(club_a + club_nation);
        };
        // The slot's own team (lhax at index * 8), for the same-nation test.
        const auto own_team = [&] { return half(c, in + slot::team); };
        for (;;) {
            c.set(18, 0);
            bool accepted = false, home_side = false;  // r18, r17
            const auto accept = [&](bool home) { home_side = home; accepted = true; c.set({{17, home}, {18, 1}}); };
            const auto candidate_offset = sext16(candidate) * tie::size;
            const auto candidate_home = [&] { return half(c, tie_rows() + candidate_offset + tie::home); };
            const auto candidate_away = [&] { return half(c, tie_rows() + candidate_offset + tie::away); };
            if (sext8(c.r(21)) != 0) {
                // Pass 2 (0x10480144): the candidate's home slot -2 takes the slot (not against a club of the same
                // nation as the away slot's, when asked).
                c.set({{4, round_index * 4}, {5, tie_rows()}});
                if (candidate_home() == 0xfffffffeu) {
                    bool reject = false;
                    if (sext8(c.r(20)) != 0) {
                        const auto team = own_team();
                        c.set(4, team);
                        if (std::int32_t(team) >= 0) {
                            const auto away_value = candidate_away();
                            c.set(5, away_value);
                            if (std::int32_t(away_value) >= 0) {
                                const auto table = entrants();
                                const auto club = c.u32(table + (team << 3 & 0xfffffff8u));
                                c.set(4, club);
                                if (club) {
                                    const auto other = c.u32(table + (away_value << 3 & 0xfffffff8u));
                                    reject = other && same_nation(club, other);
                                }
                            }
                        }
                    }
                    if (!reject) accept(true);
                }
                // 0x104801d4: the candidate's away slot -2 (a placement already taken keeps to its parity).
                c.set({{4, tie_rows()}, {5, c.u32(cup + cup::round_matches)}});
                if (candidate_away() == 0xfffffffeu) {
                    bool go = true;
                    if (accepted) {
                        pool.advance(0x10480224);
                        go = even_sample(pool);
                    }
                    if (go) {
                        bool keep_home = false;
                        if (sext8(c.r(20)) != 0) {
                            const auto team = own_team();
                            c.set(6, team);
                            if (std::int32_t(team) >= 0) {
                                c.set(4, c.u32(cup + cup::round_matches));
                                const auto home_value = candidate_home();
                                c.set(5, home_value);
                                if (std::int32_t(home_value) >= 0) {
                                    const auto table = entrants();
                                    const auto club = c.u32(table + (team << 3 & 0xfffffff8u));
                                    c.set(4, club);
                                    if (club) {
                                        const auto other = c.u32(table + (home_value << 3 & 0xfffffff8u));
                                        keep_home = other && same_nation(club, other);
                                    }
                                }
                            }
                        }
                        if (!keep_home) accept(false);
                    }
                }
            } else {
                // Pass 1 (0x104802dc): a tie whose two slots are both negative.
                c.set({{4, candidate_offset}, {5, tie_rows()}});
                if (std::int32_t(candidate_home()) < 0 && std::int32_t(candidate_away()) < 0) {
                    if (candidate_home() == 0xfffffffeu) accept(true);
                    c.set({{4, tie_rows()}, {5, c.u32(cup + cup::round_matches)}});
                    if (candidate_away() == 0xfffffffeu) {
                        bool settle = true;
                        if (accepted) {
                            pool.advance(0x10480370);
                            settle = even_sample(pool);
                        }
                        if (settle) accept(false);
                    }
                }
            }
            if (accepted) {
                // The placement into the candidate tie (0x104803bc home, 0x10480484 away) and the back reference
                // from the slot's other tie.
                const auto offset = sext16(index) << 3 & 0xfffffff8u;
                const auto table = [&] { return c.u32(cup + cup::round_matches); };
                const auto candidate_row = [&] { return tie_rows() + candidate_offset; };
                const std::uint32_t team = c.u16(in + slot::team), first = half(c, in + slot::first), second = half(c, in + slot::second);
                c.put16(candidate_row() + (home_side ? tie::home : tie::away), team);
                c.put16(tie_rows() + candidate_offset + (home_side ? tie::home_first : tie::away_first), first);
                c.put16(tie_rows() + candidate_offset + (home_side ? tie::home_second : tie::away_second), second);
                c.set({{4, tie_rows()}, {5, first}, {6, table()}, {7, second}, {8, first}});
                if (!home_side) c.set(9, offset);
                if (first != 0xffffffffu) {
                    const auto other_rows = [&] { return c.u32(table() + (half(c, in + slot::first) << 2 & 0xfffffffcu)); };
                    const auto other = [&] { return other_rows() + half(c, in + slot::second) * tie::size; };
                    c.put16(other() + tie::back_round, round);
                    c.put16(other() + tie::back_slot, candidate);
                    c.put16(other() + tie::back_leg, home_side ? 1 : 0);
                    c.set({{4, other_rows()}, {5, table()}, {6, home_side ? 1u : 0u}, {7, table()}});
                }
                c.set(19, ++index);
                break;
            }
            // No taker: the next candidate; round the lap, the second pass, then the flag's drop, then the report.
            candidate += 1;
            c.set(4, sext16(candidate));
            if (std::int32_t(sext16(candidate)) >= std::int32_t(half(c, rounds() + ties_offset))) candidate = 0;
            c.set(22, candidate);
            if (start == sext16(candidate)) {
                if (sext8(c.r(21)) == 0) c.set(21, 1);
                else if (sext8(c.r(20)) != 0) c.set(20, 0);
                else {
                    report(c, frame, {0x104805ac, 0x48, 0x38, 0xcff});
                    c.set(3, out);
                    c.call(callee::free_block, 0x104805f0);
                    return;
                }
            }
        }
    }
    c.set(3, out);
    c.call(callee::free_block, 0x1048060c);
}

}
