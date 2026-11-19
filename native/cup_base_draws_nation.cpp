// CUP.CPP, block `draws`: 0x10480620 `CUP::check_round_teams_for_nation`. Like 0x1047fd00 (cup_base_draws_sort.cpp,
// whose comments describe the gather and the placement), but it orders the round's slots by how many of the
// round's slots each club's nation has: the gather also counts, into a zeroed byte array indexed by the nation
// record's word +0x68 (TOC -0x7b04's count of bytes), each slot's club's nation (club +0x2c, a record); a second
// pass replaces each slot's state byte by its nation's count (the slots without a nation keep the state); the
// count array is freed, the slots are sorted (comparator 0x10481e70, larger first) and placed.
//
// The placement differs: each slot's club (r21) is looked up first; a candidate tie is refused (r18 cleared after the
// fact) when, with the nation test still on (r22, reset to 1 for each slot), the slot's club has the nation of the
// club in the candidate's home slot (r20) or in its away slot (r4). The nation test is dropped after a lap in
// pass 2 (r22 = 0), and a lap after that without a taker is the report at line 0xe5c.
//
// The draw's large arm (a bound beyond 65535, through 0x1047a9e0) cannot run: the bound is a signed halfword.
// The second pass reads each slot's club without a null test (0x104809f0: `lwz r3,0x2c(r3)`): a null club faults.
#include "cup_base_draws_detail.hpp"
#include <stdexcept>

namespace cm::aot::cup_base::draws {
using namespace detail;
namespace {
constexpr std::uint32_t allocate_cleared = 0x10775200;  // the clearing new[] (count, size)
}

void check_round_teams_for_nation(Cpu& c, std::uint32_t cup, std::uint32_t round) {
    const Frame frame(c, 0x10480620, 0x120, {.first_gpr = 13});
    const auto pool = Pool::at(c);
    const auto pool_text = c.toc(toc::pool), message = c.toc(toc::message), flag = c.toc(toc::success);
    // r13 the message buffer, r14 the count, r15 the cup, r16 the round (the whole register), r19/r20 the dialog's
    // title and message, r23 the pass flag, r26-r29 the stream's cells, r30 the pool.
    c.set({{16, round}, {15, cup}, {26, pool.salt}, {27, pool.start}, {14, 0}, {28, pool.end}, {23, 0}, {29, pool.cursor},
           {20, c.toc(-0x79e0)}, {19, c.toc(-0x79dc)}, {13, message}, {30, pool_text}});
    c.set({{5, c.toc(-0x7b04)}, {4, 1}});
    const auto nation_slots = c.u32(c.toc(-0x7b04));
    const auto counts = invoke(c, allocate_cleared, 0x10480670, {nation_slots, 1});
    c.set(18, counts);
    if (!counts) {
        memory_failure(c, frame, {0x1048068c, 0x104806b0, 0x104806bc, 0x104806cc, 0x104806dc, 0x104806f0, 0x90, 0x48, 0xd27, text::nation_memory, true});
        return;
    }
    const auto round_index = sext16(round);
    const auto round_row = round_index * cup::round_row;
    const auto rounds = [&] { return c.u32(cup + cup::rounds); };
    const auto table = [&] { return c.u32(cup + cup::round_matches); };
    const auto tie_rows = [&] { return c.u32(table() + round_index * 4); };
    const auto entrants = [&] { return c.u32(cup + cup::entrants); };
    c.set(4, rounds());
    const auto out = invoke(c, callee::allocate_block, 0x10480714, {half(c, rounds() + round_row + round_slots) << 3 & 0xfffffff8u, rounds()});
    c.set(25, out);
    if (!out) {
        memory_failure(c, frame, {0x10480730, 0x10480754, 0x10480760, 0x10480770, 0x10480780, 0x10480794, 0x70, 0x40, 0xd34, text::sort_memory, true});
        return;
    }
    const auto ties_offset = round_row + round_ties;
    c.set({{6, 0}, {5, round_index * 4}, {7, ties_offset}});
    const auto slot_at = [&](std::uint32_t index) { return out + (sext16(index) << 3 & 0xfffffff8u); };
    // Counts a slot's club's nation (0x1048083c..0x10480868): the club, its nation record, the record's index.
    const auto count_nation = [&](std::uint32_t slot_value) {
        const auto club = c.u32(entrants() + (slot_value << 3 & 0xfffffff8u));
        if (!club) return;
        const auto nation = c.u32(club + club_nation);
        if (!nation) return;
        const auto index = c.u32(nation + 0x68);
        c.set(4, index);
        c.put8(counts + index, c.u8(counts + index) + 1);
    };
    std::uint32_t tie_number = 0;  // r6
    for (;; ++tie_number, c.set(6, tie_number)) {
        c.set(4, sext16(tie_number));
        if (!(std::int32_t(sext16(tie_number)) < std::int32_t(half(c, rounds() + ties_offset)))) break;
        const auto row = sext16(tie_number) * tie::size;
        c.set(4, row);
        const auto home = half(c, tie_rows() + row);
        c.set(8, home);
        std::uint32_t taken = c.r(14);
        if (home == 0xffffffffu) {
            const auto at = slot_at(taken);
            c.set({{14, taken + 1}, {4, 0xffffffffu}});
            c.put16(at + slot::team, 0xffff);
            c.put8(at + slot::state, 0xff);
        } else {
            const auto at = slot_at(taken);
            c.put16(at + slot::team, home);
            c.set(14, taken + 1);
            c.put16(at + slot::first, half(c, tie_rows() + row + tie::home_first));
            const auto second = half(c, tie_rows() + row + tie::home_second);
            c.put16(at + slot::second, second);
            c.put8(at + slot::state, c.u8(entrants() + (half(c, at + slot::team) << 3 & 0xfffffff8u) + 4));
            count_nation(half(c, tie_rows() + row));
            c.set({{4, table()}, {8, 0xfffffffeu}, {9, second}, {10, tie_rows()}});
            c.put16(tie_rows() + sext16(tie_number) * tie::size + tie::home, 0xfffe);
        }
        c.set(8, sext16(tie_number) * tie::size);
        const auto away = half(c, tie_rows() + row + tie::away);
        c.set(4, away);
        taken = c.r(14);
        if (away == 0xffffffffu) {
            const auto at = slot_at(taken);
            c.set({{14, taken + 1}, {4, 0xffffffffu}});
            c.put16(at + slot::team, 0xffff);
            c.put8(at + slot::state, 0xff);
        } else {
            const auto at = slot_at(taken);
            c.put16(at + slot::team, away);
            c.set(14, taken + 1);
            c.put16(at + slot::first, half(c, tie_rows() + row + tie::away_first));
            const auto second = half(c, tie_rows() + row + tie::away_second);
            c.put16(at + slot::second, second);
            c.put8(at + slot::state, c.u8(entrants() + (half(c, at + slot::team) << 3 & 0xfffffff8u) + 4));
            count_nation(half(c, tie_rows() + row + tie::away));
            c.set({{4, tie_rows()}, {8, 0xfffffffeu}, {9, second}, {10, tie_rows()}});
            c.put16(tie_rows() + sext16(tie_number) * tie::size + tie::away, 0xfffe);
        }
    }
    // The second pass: each slot with a club that has a nation takes the nation's count as its state.
    const auto count = sext16(c.r(14));
    for (std::uint32_t j = 0; (c.set(6, j), std::int32_t(sext16(j)) < std::int32_t(count)); ++j) {
        const auto offset = sext16(j) << 3 & 0xfffffff8u;
        c.set(5, offset);
        const auto team = half(c, out + offset);
        if (team == 0xffffffffu) continue;
        c.set(4, entrants());
        const auto nation = c.u32(c.u32(entrants() + (team << 3 & 0xfffffff8u)) + club_nation);
        if (nation == 0) continue;
        const auto index = c.u32(nation + 0x68);
        c.set(4, c.u8(counts + index));
        c.put8(out + offset + slot::state, c.u8(counts + index));
    }
    invoke(c, callee::free_block, 0x10480a24, {counts});
    invoke(c, callee::sort, 0x10480a38, {out, count, slot::size, c.toc(toc::by_byte_6)});
    c.set({{19, 0}, {31, ties_offset}});
    // (the count is kept on the stack at sp+0xc8: dead stack, a native local here)
    for (std::uint32_t index = 0; std::int32_t(sext16(index)) < std::int32_t(count); ) {
        c.set(19, index);
        const auto in = slot_at(index);
        c.set(22, 1);
        std::uint32_t nation_active = 1;
        const auto own_team = half(c, in + slot::team);
        c.set(21, own_team == 0xffffffffu ? 0u : c.u32(entrants() + (own_team << 3 & 0xfffffff8u)));
        const auto own_club = c.r(21);
        if (std::int8_t(c.u8(in + slot::state)) == -1) break;
        const auto ties = std::int32_t(half(c, rounds() + ties_offset));
        c.set(14, std::uint32_t(ties));
        std::uint32_t candidate = 0;
        // A round without ties has no slots (the gather runs while j < the same halfword), so no slot reaches this draw.
        // Not expected (no slot comes from a round without ties), but the original does not draw then: the candidate is 0.
        if (ties == 0) c.set(18, 0);
        else {
            pool.advance(0x10480ad4);
            const auto sample = std::int32_t(pool.sample());
            const auto remainder = std::int32_t(std::uint32_t(sample) - std::uint32_t(divide(sample, ties)) * std::uint32_t(ties));
            candidate = ties > 0 ? std::uint32_t(remainder) : 0u - std::uint32_t(remainder);
            c.set(18, candidate);
        }
        candidate = sext16(candidate);
        const auto start = candidate;
        c.set({{24, candidate}, {14, start}});
        for (;;) {
            c.set(18, 0);
            bool accepted = false, home_side = false;
            const auto accept = [&](bool home) { home_side = home; accepted = true; c.set({{17, home}, {18, 1}}); };
            const auto candidate_offset = sext16(candidate) * tie::size;
            const auto candidate_home = [&] { return half(c, tie_rows() + candidate_offset + tie::home); };
            const auto candidate_away = [&] { return half(c, tie_rows() + candidate_offset + tie::away); };
            if (sext8(c.r(23)) != 0) {
                // Pass 2 (0x10480c18): the candidate's home and away occupants' clubs (r20, r4), then the nation test.
                c.set(4, table());
                const auto home_value = candidate_home();
                std::uint32_t home_club = 0;
                if (home_value == 0xfffffffeu) accept(true);
                else if (home_value != 0xffffffffu) home_club = c.u32(entrants() + (home_value << 3 & 0xfffffff8u));
                c.set(20, home_club);
                c.set({{4, tie_rows()}, {5, table()}});
                const auto away_value = candidate_away();
                std::uint32_t away_club = 0;
                if (away_value == 0xfffffffeu) {
                    bool take = true;
                    if (accepted) {
                        pool.advance(0x10480cb8);
                        take = even_sample(pool);
                    }
                    if (take) accept(false);
                } else if (away_value != 0xffffffffu) {
                    away_club = c.u32(entrants() + (away_value << 3 & 0xfffffff8u));
                }
                c.set(4, away_club);
                if (own_club && nation_active) {
                    const auto same = [&](std::uint32_t club) { return c.u32(own_club + club_nation) == c.u32(club + club_nation); };
                    if ((home_club && same(home_club)) || (away_club && same(away_club))) { accepted = false; c.set(18, 0); }
                }
            } else {
                c.set({{4, candidate_offset}, {5, tie_rows()}});
                if (std::int32_t(candidate_home()) < 0 && std::int32_t(candidate_away()) < 0) {
                    if (candidate_home() == 0xfffffffeu) accept(true);
                    c.set({{4, tie_rows()}, {5, table()}});
                    if (candidate_away() == 0xfffffffeu) {
                        bool settle = true;
                        if (accepted) {
                            pool.advance(0x10480df4);
                            settle = even_sample(pool);
                        }
                        if (settle) accept(false);
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
                ++index;
                break;
            }
            // No taker: the next candidate; the lap's end drops the pass, then the nation test, then reports.
            candidate += 1;
            c.set(4, sext16(candidate));
            if (std::int32_t(sext16(candidate)) >= std::int32_t(half(c, rounds() + ties_offset))) candidate = 0;
            c.set(24, candidate);
            if (start == sext16(candidate)) {
                if (sext8(c.r(23)) == 0) c.set(23, 1);
                else if (nation_active) { nation_active = 0; c.set(22, 0); }
                else {
                    report(c, frame, {0x10481030, 0x50, 0x38, 0xe5c});
                    c.set(4, flag);
                    c.set(3, out);
                    c.call(callee::free_block, 0x10481078);
                    return;
                }
            }
        }
        c.set(19, index);
    }
    c.set(3, out);
    c.call(callee::free_block, 0x10481098);
}

}
