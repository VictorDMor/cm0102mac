// CUP.CPP, block `draws`: 0x104810b0 `CUP::randomize_round_teams`. Gathers a round's slots as 0x1047fd00 does
// (cup_base_draws_sort.cpp), shuffles all of them (for each slot i in order, a draw below the slot count picks
// another slot to swap places with: team, the two form words and the state byte), then places each slot whose
// team is not -1 (a slot with the team -1 is skipped, the loop goes on) into a tie, as 0x1047fd00's placement but
// without the nation test: pass 1 takes ties whose two slots are both negative, a lap without a taker moves to
// pass 2 (a tie whose home or away slot is -2), and another lap without a taker is the report at line 0xf4c.
//
// The draws' large arm (a bound beyond 65535, through 0x1047a9e0) cannot run: both bounds are signed halfwords.
// The shuffle's zero arm (a count of 0) and the placement's (0 ties) cannot either: a shuffle pass runs only while
// i < the halfword of the slot count, and a slot exists only when the round has ties, which the placement
// re-reads from the same halfword.
#include "cup_base_draws_detail.hpp"
#include <stdexcept>

namespace cm::aot::cup_base::draws {
using namespace detail;

void randomize_round_teams(Cpu& c, std::uint32_t cup, std::uint32_t round) {
    const Frame frame(c, 0x104810b0, 0x100, {.first_gpr = 13});
    const auto pool = Pool::at(c);
    const auto pool_text = c.toc(toc::pool), message = c.toc(toc::message), flag = c.toc(toc::success);
    // r13 the count, r14/r15 the dialog's message and title, r18 the cup, r19 the round (the whole register), r22
    // the pass flag, r24 the doubles, r25-r28 the stream's cells, r29 the success flag, r30 the message buffer, r31 the pool.
    c.set({{19, round}, {18, cup}, {24, c.toc(toc::doubles)}, {13, 0}, {25, pool.salt}, {22, 0}, {26, pool.start}, {27, pool.end},
           {28, pool.cursor}, {14, c.toc(-0x79e0)}, {15, c.toc(-0x79dc)}, {29, flag}, {30, message}, {31, pool_text}});
    const auto round_index = sext16(round);
    const auto round_row = round_index * cup::round_row;
    const auto rounds = [&] { return c.u32(cup + cup::rounds); };
    const auto table = [&] { return c.u32(cup + cup::round_matches); };
    const auto tie_rows = [&] { return c.u32(table() + round_index * 4); };
    const auto entrants = [&] { return c.u32(cup + cup::entrants); };
    c.set(4, rounds());
    const auto out = invoke(c, callee::allocate_block, 0x10481114, {half(c, rounds() + round_row + round_slots) << 3 & 0xfffffff8u, rounds()});
    c.set(23, out);
    if (!out) {
        memory_failure(c, frame, {0x10481130, 0x10481154, 0x10481160, 0x10481170, 0x10481180, 0x10481190, 0x70, 0x40, 0xe7f, text::randomize_memory});
        return;
    }
    const auto ties_offset = round_row + round_ties;
    c.set({{21, 0}, {5, round_index * 4}, {6, ties_offset}});
    const auto slot_at = [&](std::uint32_t index) { return out + (sext16(index) << 3 & 0xfffffff8u); };
    // The gather.
    std::uint32_t tie_number = 0;  // r21
    for (;; ++tie_number, c.set(21, tie_number)) {
        c.set(4, sext16(tie_number));
        if (!(std::int32_t(sext16(tie_number)) < std::int32_t(half(c, rounds() + ties_offset)))) break;
        const auto row = sext16(tie_number) * tie::size;
        c.set(4, row);
        const auto home = half(c, tie_rows() + row);
        c.set(7, home);
        std::uint32_t taken = c.r(13);
        if (home == 0xffffffffu) {
            const auto at = slot_at(taken);
            c.set({{13, taken + 1}, {4, 0xffffffffu}});
            c.put16(at + slot::team, 0xffff);
            c.put8(at + slot::state, 0xff);
        } else {
            const auto at = slot_at(taken);
            c.put16(at + slot::team, home);
            c.set(13, taken + 1);
            c.put16(at + slot::first, half(c, tie_rows() + row + tie::home_first));
            const auto second = half(c, tie_rows() + row + tie::home_second);
            c.put16(at + slot::second, second);
            c.put8(at + slot::state, c.u8(entrants() + (half(c, at + slot::team) << 3 & 0xfffffff8u) + 4));
            c.put16(tie_rows() + row + tie::home, 0xfffe);
            c.set({{7, (at - out) + slot::state}, {8, entrants()}, {9, second}, {10, tie_rows()}});
        }
        c.set(7, row);
        const auto away = half(c, tie_rows() + row + tie::away);
        c.set(4, away);
        taken = c.r(13);
        if (away == 0xffffffffu) {
            const auto at = slot_at(taken);
            c.set({{13, taken + 1}, {4, 0xffffffffu}});
            c.put16(at + slot::team, 0xffff);
            c.put8(at + slot::state, 0xff);
        } else {
            const auto at = slot_at(taken);
            c.put16(at + slot::team, away);
            c.set(13, taken + 1);
            c.put16(at + slot::first, half(c, tie_rows() + row + tie::away_first));
            const auto second = half(c, tie_rows() + row + tie::away_second);
            c.put16(at + slot::second, second);
            c.put8(at + slot::state, c.u8(entrants() + (half(c, at + slot::team) << 3 & 0xfffffff8u) + 4));
            c.put16(tie_rows() + row + tie::away, 0xfffe);
            c.set({{4, 0xfffffffeu}, {7, (at - out) + slot::state}, {8, entrants()}, {9, second}, {10, tie_rows()}});
        }
    }
    // The shuffle: slot i swaps with a slot drawn below the count (r14 the count while it runs).
    c.set({{14, sext16(c.r(13))}, {21, 0}});
    const auto count = sext16(c.r(13));
    for (std::uint32_t i = 0; std::int32_t(sext16(i)) < std::int32_t(c.r(14)); ) {
        // Not expected (the loop runs while i < the count), but the original does not draw then (0x10481330): r15 is 0.
        if (std::int32_t(count) == 0) {
            c.set(15, 0);
        } else {
            pool.advance(0x10481358);
            c.set(4, count);
            const auto bound = std::int32_t(count);
            const auto sample = std::int32_t(pool.sample());
            // (A bound beyond 65535 would go to 0x1047a9e0: a signed halfword cannot.)
            const auto remainder = std::int32_t(std::uint32_t(sample) - std::uint32_t(divide(sample, bound)) * std::uint32_t(bound));
            c.set(15, bound > 0 ? std::uint32_t(remainder) : 0u - std::uint32_t(remainder));
        }
        const auto drawn = (sext16(c.r(15)) << 3) & 0xfffffff8u;  // r15
        const auto mine = (sext16(i) << 3) & 0xfffffff8u;         // r6
        c.set({{15, drawn}, {21, i + 1}});
        // Swap the slots through the original's temporaries (the stack's dead scratch bytes are locals here).
        const auto old_team = half(c, out + drawn);
        c.set(16, old_team);
        const auto old_first = half(c, out + drawn + slot::first), old_second = half(c, out + drawn + slot::second);
        const auto old_state = c.u8(out + drawn + slot::state);
        const auto my_team = half(c, out + mine);
        c.put16(out + drawn, my_team);
        const auto my_first = half(c, out + mine + slot::first);
        c.put16(out + drawn + slot::first, my_first);
        const auto my_second = half(c, out + mine + slot::second);
        c.put16(out + drawn + slot::second, my_second);
        const auto my_state = c.u8(out + mine + slot::state);
        c.put8(out + drawn + slot::state, my_state);
        c.put16(out + mine, old_team);
        c.put16(out + mine + slot::first, old_first);
        c.put16(out + mine + slot::second, old_second);
        c.put8(out + mine + slot::state, old_state);
        c.set({{15, my_first}, {12, my_second}, {11, old_first}, {10, my_state}, {9, old_second}, {8, drawn + slot::state},
               {7, old_state}, {6, mine}, {5, mine + slot::first}, {4, mine + slot::second}, {3, mine + slot::state}, {0, old_team}});
        ++i;
    }
    c.set({{13, count}, {21, 0}});
    // The placement.
    for (std::uint32_t index = 0; std::int32_t(sext16(index)) < std::int32_t(count); ++index, c.set(21, index)) {
        const auto in = slot_at(index);
        if (half(c, in + slot::team) == 0xffffffffu) continue;
        const auto ties = std::int32_t(half(c, rounds() + ties_offset));
        c.set(15, std::uint32_t(ties));
        // Not expected (the gather made slots from its ties), but the original does not draw then (0x1048156c): r14 is 0.
        if (ties == 0) {
            c.set(14, 0);
        } else {
            pool.advance(0x1048158c);
            const auto sample = std::int32_t(pool.sample());
            const auto remainder = std::int32_t(std::uint32_t(sample) - std::uint32_t(divide(sample, ties)) * std::uint32_t(ties));
            c.set(14, ties > 0 ? std::uint32_t(remainder) : 0u - std::uint32_t(remainder));
        }
        std::uint32_t candidate = sext16(c.r(14));
        const auto start = candidate;
        c.set({{15, candidate}, {17, start}, {16, ties_offset}});
        for (;;) {
            c.set(14, 0);
            bool accepted = false, home_side = false;  // r14, r20
            const auto accept = [&](bool home) { home_side = home; accepted = true; c.set({{20, home}, {14, 1}}); };
            const auto candidate_offset = sext16(candidate) * tie::size;
            const auto candidate_home = [&] { return half(c, tie_rows() + candidate_offset + tie::home); };
            const auto candidate_away = [&] { return half(c, tie_rows() + candidate_offset + tie::away); };
            if (sext8(c.r(22)) != 0) {
                // Pass 2 (0x104816dc): the candidate's home slot -2, then its away slot -2.
                if (candidate_home() == 0xfffffffeu) accept(true);
                c.set({{4, tie_rows()}, {5, table()}});
                if (candidate_away() == 0xfffffffeu) {
                    bool take = true;
                    if (accepted) {
                        pool.advance(0x1048175c);
                        take = even_sample(pool);
                    }
                    if (take) accept(false);
                }
            } else {
                // Pass 1 (0x10481798): a tie whose two slots are both negative.
                c.set({{4, candidate_offset}, {5, tie_rows()}});
                if (std::int32_t(candidate_home()) < 0 && std::int32_t(candidate_away()) < 0) {
                    if (candidate_home() == 0xfffffffeu) accept(true);
                    c.set({{4, tie_rows()}, {5, table()}});
                    if (candidate_away() == 0xfffffffeu) {
                        bool take = true;
                        if (accepted) {
                            pool.advance(0x1048182c);
                            take = even_sample(pool);
                        }
                        if (take) accept(false);
                    }
                }
            }
            if (accepted) {
                const auto offset = sext16(index) << 3 & 0xfffffff8u;
                const auto candidate_row = [&] { return tie_rows() + candidate_offset; };
                const std::uint32_t team = c.u16(in + slot::team), first = half(c, in + slot::first), second = half(c, in + slot::second);
                c.put16(candidate_row() + (home_side ? tie::home : tie::away), team);
                c.put16(tie_rows() + candidate_offset + (home_side ? tie::home_first : tie::away_first), first);
                c.put16(tie_rows() + candidate_offset + (home_side ? tie::home_second : tie::away_second), second);
                c.set({{4, candidate_offset + (home_side ? tie::home_second : tie::away_second)}, {5, first}, {6, second}, {7, first},
                       {8, round_index * 4}});
                if (!home_side) c.set(9, offset);
                if (first != 0xffffffffu) {
                    const auto other_rows = [&] { return c.u32(table() + (half(c, in + slot::first) << 2 & 0xfffffffcu)); };
                    const auto other = [&] { return other_rows() + half(c, in + slot::second) * tie::size; };
                    c.put16(other() + tie::back_round, round);
                    c.put16(other() + tie::back_slot, candidate);
                    c.put16(other() + tie::back_leg, home_side ? 1 : 0);
                    c.set({{4, other_rows()}, {5, table()}, {6, home_side ? 1u : 0u}, {7, table()}});
                }
                break;
            }
            // No taker: the next candidate; a lap's end moves to pass 2, and another lap's end reports.
            candidate += 1;
            c.set(4, sext16(candidate));
            if (std::int32_t(sext16(candidate)) >= std::int32_t(half(c, rounds() + ties_offset))) candidate = 0;
            c.set(15, candidate);
            if (start == sext16(candidate)) {
                if (sext8(c.r(22)) == 0) c.set(22, 1);
                else {
                    report(c, frame, {0x10481a5c, 0x50, 0x38, 0xf4c});
                    c.set(3, out);
                    c.call(callee::free_block, 0x10481aa0);
                    return;
                }
            }
        }
    }
    c.set(3, out);
    c.call(callee::free_block, 0x10481abc);
}

}
