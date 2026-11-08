// MATCH_MAN.CPP lineup_a block: the arrangement flags (r30), the pick order and
// the per-slot tables the arranger is given.
#include "match_man_lineup_a.hpp"
#include "match_man_lineup_a_detail.hpp"

namespace cm::aot::match_man {
using namespace guest;

// The game's random table: the cursor (word at r19) moves two bytes on and wraps
// past the limit (the word the TOC -0x7b88 slot points at) to the table start
// (TOC -0x7b8c), reseeding the seed word (r18) from 0x1077b770; the value is the
// halfword under the cursor plus the seed. The original inlines this at each site;
// `back` is the return address of the reseed call at that site.
std::uint32_t ComputerLineup::draw(std::uint32_t back) {
    const auto cursor = c.u32(r19) + 2;
    c.put32(r19, cursor);
    if (compare_logical(c, cursor, c.u32(c.toc(-0x7b88))) > 0) {
        c.put32(r19, c.toc(-0x7b8c));
        c.call(0x1077b770, back);
        c.put32(r18, c.r(3) & 0xffff);
    }
    return c.u16(c.u32(r19)) + c.u32(r18);
}

// 0x10208888-0x10208ad8: the flag word r30 that tells the arranger what to favour,
// from the side's kind (human or not), the tie between the clubs and two random draws.
void ComputerLineup::choose_flags() {
    const auto self = side();
    if (compare(c, mode(), 1) == 0 && compare(c, s8(c.u8(at(0x904)))) == 0) r25 = 2;
    c.set(3, club());
    c.call(0x100c7d00, 0x102088ac);
    r30 = compare(c, s8(c.r(3))) != 0 ? 0x208a09c : 0xca0ca;
    const auto strength_gap = [&] { return std::int32_t(c.i16(club() + 0x1ec)) - std::int32_t(c.i16(team() + 0x1ec)); };
    if (compare(c, s8(c.u8(self + lineup_side::human))) != 0) {
        r30 = (r30 | 0x20000) | 0x100;
        r30 = r30 ^ 0x80000;
    } else {
        const auto kind = s8(c.u8(c.u32(self + lineup_side::extras) + lineup_extras::kind_byte));
        bool decided = false;
        if (compare(c, kind, 1) == 0 || compare(c, kind, 9) == 0) {
            // Two draws: the first (taken modulo 4000, negated) against the second (modulo 2000).
            const auto first = std::int32_t(draw(0x10208924));
            const auto penalty = -(first % 4000);  // mulhw by 0xef9db22d: x / -4000
            r14 = std::uint32_t(penalty);  // the original keeps the negated remainder in r14, before the second draw
            const auto second = std::int32_t(draw(0x10208984));
            if (compare(c, strength_gap(), penalty + second % 2000) > 0) {
                r30 = r30 | 0x100;
                decided = true;
            }
        }
        if (!decided) {
            const auto roll = std::int32_t(draw(0x10208a0c)) % 3000;  // mulhw by 0x57619f1, srawi 6
            if (compare(c, strength_gap(), roll + 3000) > 0) {
                r30 = r30 | 0x100;
                c.set(3, club());
                c.call(0x100c7d00, 0x10208a6c);
                if (compare(c, s8(c.r(3))) == 0) r30 = r30 | 0x10000;
            } else if (compare(c, c.u8(self + lineup_side::urgency), 3) != 0) {
                r30 = (r30 | 0x100) ^ 0x80000;
            } else {
                const auto id = c.i32(c.u32(self + lineup_side::extras));
                bool settled = compare(c, id, -1) <= 0;
                if (!settled) {
                    const auto person = c.u32(c.u32(c.toc(-0x7b5c)) + std::uint32_t(id) * 4);
                    c.set({{3, person}, {4, club()}});
                    c.call(0x10452ed0, 0x10208ab0);
                    settled = compare(c, s8(c.r(3))) != 0;
                }
                if (!settled) {
                    r30 = (r30 | 0x100) ^ 0x80000;
                } else {
                    c.set(3, club());
                    c.call(0x100c7d00, 0x10208acc);
                    if (compare(c, s8(c.r(3))) == 0) r30 = r30 | 0x10000;
                }
            }
        }
    }
    if (compare(c, s8(c.u8(self + lineup_side::changed))) != 0) r30 = r30 | 0x800000;
}

// 0x10208ae8-0x10208cc4: with candidates left in mode 2, the picker names the
// first choice (asserted to exist) and, under flag 0x80000, the three best
// candidates by the sum of two ratings (sp+0xf8 the scores, sp+0xf0 their
// indexes) go to the side's pick words; the orderer then sorts the list.
void ComputerLineup::reorder_candidates() {
    const auto self = side();
    if (compare(c, s8(r31)) != 0 && compare(c, mode(), 2) == 0) {
        c.set({{3, self}, {5, r31}, {4, candidates()}});
        match_man::call(c, lineup_entries::picker, 0x10208b0c);
        c.put32(self + lineup_side::choice, c.r(3));
        assert_that(compare_logical(c, c.u32(self + lineup_side::choice)) != 0, 0x10208b30, 0x429, 0x160, 0x100);
        if (compare(c, std::int32_t(r30 & 0x80000)) != 0) {
            std::uint32_t pick = 0;
            for (std::uint32_t offset = 0; compare(c, s8(pick), 3) < 0; ++pick, offset += 2) {
                c.put16(at(0xf8) + offset, 0xd8f0);  // -10000
                c.put16(at(0xf0) + offset, 0xffff);
                std::uint32_t index = 0;
                for (; compare(c, s8(index), s8(r31)) < 0; ++index) {
                    r14 = c.u32(candidates() + 4 * index);
                    if (compare_logical(c, r14) == 0) continue;
                    if (compare(c, c.i32(r14), c.i32(r13)) >= 0) continue;
                    const auto data = c.u32(r14 + lineup_person::player);
                    if (compare_logical(c, data) == 0) continue;
                    bool score = false;
                    if (compare(c, s8(pick)) == 0) {
                        score = true;
                    } else {
                        if (compare(c, s8(pick), 1) == 0) {
                            r14 = extended(c.i16(at(0xf0)));
                            score = compare(c, std::int32_t(std::uint32_t(r14)), s8(index)) != 0;
                        }
                        if (!score) {
                            if (compare(c, s8(pick), 2) != 0) continue;
                            r14 = extended(std::int16_t(s8(index)));
                            if (compare(c, c.i16(at(0xf0)), s8(index)) == 0) continue;
                            if (compare(c, c.i16(at(0xf2)), s8(index)) == 0) continue;
                        }
                    }
                    r14 = extended(c.i16(data + 0xa));
                    const auto rating = std::int16_t(c.i16(data + 0xa) + c.i16(data + 0xc));
                    if (compare(c, rating, c.i16(at(0xf8) + offset)) <= 0) continue;
                    c.put16(at(0xf8) + offset, std::uint16_t(rating));
                    c.put16(at(0xf0) + offset, std::uint16_t(s8(index)));
                }
                const auto best = c.i16(at(0xf0) + offset);
                if (compare(c, best, -1) > 0) c.put32(self + lineup_side::picks + 2 * offset, c.u32(candidates() + std::uint32_t(best) * 4));
            }
        }
    }
    c.set({{3, self}, {5, r31}, {6, r30}, {7, r25}, {4, candidates()}});
    match_man::call(c, lineup_entries::orderer, 0x10208cc4);
}

// 0x10208cc4-0x102090d8: the tables of the arranger's slots. Each of the r29 slots
// takes the tactic's answers (two halfwords, a 64-bit value and a byte) when it is a
// real slot (index up to 10), else is filled with zeros and a random mix; the
// counters r27 (entries), r14 (real ones) and r26 (random ones) are kept as the
// original keeps them. The quirks of the original's index registers are kept.
void ComputerLineup::fill_slots() {
    c.put32(at(0x908), std::uint32_t(s8(r25)));
    r17 = 0;
    r16 = r17;
    r27 = 0;
    r14 = 0;
    r26 = 0;
    r28 = 0;
    while (compare(c, std::int32_t(std::uint32_t(r28)), std::int32_t(std::uint32_t(r29))) < 0) {
        fill_one_slot();
        r17 = r17 + 2;
        r16 = r16 + 8;
        r28 = r28 + 1;
    }
}

// The entry of the 64-bit table at sp+0x75c at a byte offset.
static std::uint32_t entry_at(const ComputerLineup& lineup, std::uint32_t offset) { return lineup.at(0x75c) + offset; }

void ComputerLineup::fill_one_slot() {
    const auto self = side();
    const auto tactic = [&] { return c.u32(self + lineup_side::tactic); };
    const auto slot = [&] { return std::uint32_t(s8(r28)); };
    // Clamps a roll to one of the three levels 4, 0x10 and 0x40.
    const auto level = [&](std::uint32_t address) {
        const auto value = c.i16(address);
        c.put16(address, compare(c, value, 3) < 0 ? 4 : compare(c, value, 6) < 0 ? 0x10 : 0x40);
    };
    // The tactic's answers for a real slot; the calls' order and arguments are fixed.
    if (compare(c, c.i32(at(0x908)), 1) == 0) {
        if (compare(c, s8(c.u8(at(0x1c0) + r28))) != 0) return;
        if (compare(c, std::int32_t(std::uint32_t(r28)), 0xa) <= 0) {
            c.set({{3, tactic()}, {4, slot()}, {5, 0}});
            c.call(0x10111240, 0x10208d1c);
            r15 = std::uint32_t(s8(r27));
            r13 = r15 << 1;
            c.put16(at(0x3d8) + r13, c.r(3));
            c.set({{3, tactic()}, {4, slot()}, {5, 1}});
            c.call(0x10111240, 0x10208d3c);
            c.put16(at(0x374) + r13, c.r(3));
            c.set({{3, tactic()}, {4, slot()}});
            c.call(0x10113e10, 0x10208d50);
            const auto entry = entry_at(*this, r15 << 3);
            c.put32(entry + 4, c.r(4));
            c.put32(entry, c.r(3));
            c.set({{3, tactic()}, {4, slot()}});
            c.call(0x101149f0, 0x10208d70);
            r14 = r14 + 1;
            c.put8(at(0x228) + r15, c.r(3));
            r27 = r27 + 1;
            return;
        }
        r15 = std::uint32_t(s8(r27));
        const auto entry = entry_at(*this, r15 << 3);
        c.put32(entry + 4, 0);
        c.put32(entry, 0);
        c.put8(at(0x228) + r15, 0);
        const auto first = draw(0x10208dcc);
        r15 = r15 << 1;
        r13 = at(0x3d8);
        c.put16(at(0x374) + r15, 0);
        c.put16(at(0x3d8) + r15, std::uint32_t(std::int32_t(first) % 10));  // mulhw by 0x66666667, srawi 2
        compare(c, std::int32_t(std::uint32_t(r28)), 0xc);
        bool special = false;
        if (std::int32_t(std::uint32_t(r28)) == 0xc && compare(c, s8(c.u8(at(0x907)))) == 0) {
            if (compare(c, std::int32_t(std::uint32_t(r29)), 0xe) > 0) {
                special = true;
            } else {
                const auto second = draw(0x10208e5c);
                special = compare(c, std::int32_t(second) % 20) == 0;  // srawi 3: modulo 20
            }
        }
        if (special) {
            c.put8(at(0x907), 1);
            c.put16(at(0x3d8) + r15, 1);
        } else {
            level(at(0x3d8) + r15);
        }
        r26 = r26 + 1;
        r27 = r27 + 1;
        return;
    }
    if (compare(c, std::int32_t(std::uint32_t(r28)), 0xa) <= 0) {
        r27 = r27 + 1;
        r14 = r14 + 1;
        c.set({{3, tactic()}, {4, slot()}, {5, 0}});
        c.call(0x10111240, 0x10208f0c);
        c.put16(at(0x3d8) + r17, c.r(3));
        c.set({{3, tactic()}, {4, slot()}, {5, 1}});
        c.call(0x10111240, 0x10208f24);
        c.put16(at(0x374) + r17, c.r(3));
        c.set({{3, tactic()}, {4, slot()}});
        c.call(0x10113e10, 0x10208f38);
        const auto entry = entry_at(*this, r16);
        c.put32(entry + 4, c.r(4));
        c.put32(entry, c.r(3));
        c.set({{3, tactic()}, {4, slot()}});
        c.call(0x101149f0, 0x10208f54);
        c.put8(at(0x228) + r28, c.r(3));
        return;
    }
    const auto entry = entry_at(*this, r16);
    c.put32(entry + 4, 0);
    r27 = r27 + 1;
    c.put32(entry, 0);
    r26 = r26 + 1;
    c.put8(at(0x228) + r28, 0);
    const auto first = draw(0x10208fac);
    // A quirk of the original: the zero lands at the next entry's offset (r27 is already counted).
    c.put16(at(0x374) + (std::uint32_t(s8(r27)) << 1), 0);
    r15 = at(0x3d8);
    c.put16(r15 + r17, std::uint32_t(std::int32_t(first) % 10));
    compare(c, std::int32_t(std::uint32_t(r28)), 0xc);
    bool special = false;
    if (std::int32_t(std::uint32_t(r28)) == 0xc && compare(c, s8(c.u8(at(0x907)))) == 0) {
        if (compare(c, std::int32_t(std::uint32_t(r29)), 0xe) > 0) {
            special = true;
        } else {
            const auto second = draw(0x10209040);
            special = compare(c, std::int32_t(second) % 10) == 0;
        }
    }
    if (special) {
        c.put8(at(0x907), 1);
        c.put16(r15 + r17, 1);
    } else {
        level(r15 + r17);
    }
}
}
