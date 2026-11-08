// MATCH_MAN.CPP lineup_a block: the second arrangement, the duplicate check, the
// improvement loop over the arranged lineup and the final picks.
#include "match_man_lineup_a.hpp"
#include "match_man_lineup_a_detail.hpp"

namespace cm::aot::match_man {
using namespace guest;

// 0x102090d8-0x102091a8: the arranger again, now with the per-slot tables
// (sp+0x3d8 and 0x374 halfwords, 0x75c 64-bit values, 0x228 bytes) and the ordered
// list at sp+0x694 as its output; the flags get 0x400000 as for the first call.
void ComputerLineup::second_arrangement() {
    const auto self = side();
    c.set({{3, club()}, {4, 1}, {5, 1}, {6, 0}, {7, 0}});
    c.call(0x1014c2e0, 0x102090f0);
    if (compare(c, s8(c.r(3))) != 0 && compare(c, s8(c.u8(self + lineup_side::human))) == 0) {
        const auto owner = c.u32(club() + 0x90);
        if (compare_logical(c, owner) != 0) {
            c.set({{3, c.u32(c.toc(-0x7a54))}, {4, owner}});
            c.call(0x10147e70, 0x10209120);
            if (compare(c, s8(c.r(3))) != 0) {
                c.set({{5, c.u32(club() + 0x90)}, {4, 3}, {3, c.u32(c.toc(-0x7a54))}});
                c.call(0x10149df0, 0x10209140);
                if (compare(c, s8(c.r(3))) != 0) r30 = r30 | 0x400000;
            }
        }
    }
    c.put32(at(0x38), self);
    c.put32(at(0x3c), c.u32(self + lineup_side::manager));
    c.put32(at(0x40), 0);
    c.put32(at(0x44), at(0x75c));
    c.put32(at(0x48), at(0x228));
    c.put32(at(0x4c), c.u32(c.u32(self + lineup_side::tactic) + 0x588));
    c.put32(at(0x50), at(0x374));
    c.set({{3, candidates()}, {4, r31}, {5, at(0x3d8)}, {6, r14}, {7, r26}, {8, ordered()}, {9, r30}, {10, club()}});
    match_man::call(c, lineup_entries::arranger, 0x102091a8);
}

// 0x102091a8-0x1020925c: the ordered list may not name a player twice (the assertion
// reports, without exiting, and the shared success flag is cleared).
void ComputerLineup::check_duplicates() {
    r13 = ordered();
    r19 = r29 - 1;
    r17 = ordered();
    r15 = 0;
    while (compare(c, std::int32_t(std::uint32_t(r15)), std::int32_t(std::uint32_t(r19))) < 0) {
        r16 = r15 + 1;
        r14 = r16 << 2;
        r18 = r13 + r14;
        while (compare(c, std::int32_t(std::uint32_t(r16)), std::int32_t(std::uint32_t(r29))) < 0) {
            const auto first = c.u32(r17);
            if (compare_logical(c, first) != 0 && compare_logical(c, first, c.u32(r18)) == 0)
                report(0x102091fc, 0x4c7, 0x140, 0xe8);
            r14 = r14 + 4;
            r18 = r18 + 4;
            r16 = r16 + 1;
        }
        r17 = r17 + 4;
        r15 = r15 + 1;
    }
}

// 0x1020925c-0x10209580: while the fix-up routine 0x103f98d0 (reached through the
// squad registry's global at TOC -0x7b40) still wants changes, the first 11
// players are copied to sp+0x1d4, the bench to a new block, the candidates not
// in the lineup to another, and 0x1021a240 deploys them; twice at most.
void ComputerLineup::rotate_lists() {
    const auto self = side();
    if (compare(c, c.i32(c.u32(self + lineup_side::extras)), -2) == 0) return;
    c.set({{3, at(0x43c)}, {4, ordered()}, {5, 0xc8}});
    r21 = 0;
    c.call(0x10739ef0, 0x10209280);
    r18 = ordered();
    r17 = at(0x1d4);
    r16 = std::uint32_t(s8(r31));
    r15 = at(0x3d8);
    r14 = at(0x374);
    r27 = at(0x75c);
    r28 = at(0x228);
    r13 = r29 - 0xb;
    const auto verify = [&] {
        c.set({{3, c.u32(c.toc(-0x7b40))}, {7, ordered()}, {4, club()}, {9, at(0x128)}, {6, 0},
               {5, c.u32(self + lineup_side::extras)}, {8, 2}});
        c.call(0x103f98d0, 0x10209578);
        return compare(c, s8(c.r(3))) != 0;
    };
    // The first test is made before any change (the loop's entry jumps to it).
    while (verify()) {
        if (compare(c, s8(r21), 1) != 0 && compare(c, s8(r21), 2) == 0) return;
        const auto count = std::int32_t(std::uint32_t(r29));
        for (std::uint32_t word = 0; word < 11; ++word) c.put32(at(0x1d4) + 4 * word, c.u32(ordered() + 4 * word));
        r21 = r21 + 1;
        if (compare(c, count, 0xb) > 0) {
            c.set({{3, std::uint32_t(count) - 0xb}, {4, 4}});
            c.call(0x10775200, 0x1020935c);
            r22 = c.r(3);
            for (std::int32_t word = 11; word < count; ++word)
                c.put32(r22 + 4 * std::uint32_t(word - 11), c.u32(ordered() + 4 * std::uint32_t(word)));
        } else {
            r22 = 0;
        }
        r19 = 0;
        if (compare(c, s8(r16), count) > 0) {
            c.set({{3, 0x32}, {4, 4}});
            c.call(0x10775200, 0x10209458);
            r26 = c.r(3);
            for (std::uint32_t index = 0; compare(c, s8(index), s8(r16)) < 0; ++index) {
                const auto candidate = c.u32(candidates() + 4 * index);
                if (compare_logical(c, candidate) == 0) continue;
                bool present = false;
                for (std::uint32_t at_slot = 0; compare(c, s8(at_slot), count) < 0; ++at_slot)
                    if (compare_logical(c, c.u32(ordered() + 4 * at_slot), candidate) == 0) present = true;
                if (compare(c, s8(present)) != 0) continue;
                c.put32(r26 + std::uint32_t(s8(r19)) * 4, candidate);
                r19 = r19 + 1;
            }
        } else {
            r26 = 0;
        }
        c.put32(at(0x38), r22);
        c.put32(at(0x3c), r26);
        c.put32(at(0x40), r18);
        c.put32(at(0x44), r15);
        c.put32(at(0x48), r14);
        c.put32(at(0x4c), r27);
        c.put32(at(0x50), r28);
        c.put32(at(0x54), 0);
        c.set({{3, club()}, {4, c.u32(self + lineup_side::manager)}, {5, at(0x128)}, {6, r30}, {7, 0xb},
               {8, std::uint32_t(s8(r13))}, {9, r19}, {10, at(0x1d4)}});
        match_man::call(c, lineup_entries::deployer, 0x1020953c);
        c.set(3, r22);
        c.call(0x10775170, 0x10209544);
        if (compare_logical(c, r26) != 0) {
            c.set(3, r26);
            c.call(0x10775170, 0x10209554);
        }
    }
}

// 0x10209580-0x10209754: in mode 1 the slots flagged at sp+0x1c0 get the registry's
// player for the slot (0x103af800), the others the ordered list's next player;
// then the picker names the lineup's first choice and each slot is handed to 0x1020e8f0.
void ComputerLineup::final_picks() {
    const auto self = side();
    if (compare(c, mode(), 1) == 0) {
        r17 = 0;
        r15 = at(0x1c0);
        r16 = r17;
        r14 = at(0x5cc);
        r13 = ordered();
        r18 = 0;
        while (compare(c, s8(r18), std::int32_t(std::uint32_t(r29))) < 0) {
            if (compare(c, s8(c.u8(r15 + std::uint32_t(s8(r18))))) != 0) {
                c.set({{3, c.u32(r20)}, {5, r18}, {4, club()}, {6, c.u8(self + lineup_side::human)}});
                c.call(0x103af800, 0x102095d0);
                c.put32(r14 + r16, c.r(3));
            } else {
                const auto next = std::uint32_t(s8(r17));
                r17 = r17 + 1;
                c.put32(r14 + r16, c.u32(r13 + next * 4));
            }
            r16 = r16 + 4;
            r18 = r18 + 1;
        }
        if (compare(c, s8(r29)) > 0)
        {
            for (std::uint32_t word = 0; word < std::uint32_t(s8(r29)); ++word) c.put32(ordered() + 4 * word, c.u32(at(0x5cc) + 4 * word));
            // What the copy loops leave in r7 (the last unrolled block's source) and r8 (the count),
            // which the slotter's later calls are given, and an equal compare in cr0.
            const auto count = s8(r29);
            if (count > 8) c.set(7, ordered() + 0x20 * std::uint32_t((count - 8 + 7) / 8 - 1));
            c.set(8, std::uint32_t(count));
            compare(c, count, count);
        }
    }
    c.set({{3, self}, {4, ordered()}, {5, 0xb}});
    match_man::call(c, lineup_entries::picker, 0x102096d4);
    r15 = c.r(3);
    r14 = ordered();
    r13 = 0;
    while (compare(c, std::int32_t(std::uint32_t(r13)), std::int32_t(std::uint32_t(r29))) < 0) {
        c.set({{4, std::uint32_t(s8(r13))}, {5, c.u32(r14)}, {3, self}, {6, std::uint32_t(s8(r13))}});
        match_man::call(c, lineup_entries::slotter, 0x102096f8);
        if (compare_logical(c, c.u32(r14)) != 0) {
            c.set({{3, c.u32(r20)}, {5, std::uint32_t(s8(r13))}, {4, club()}, {7, c.u8(self + lineup_side::human)}, {6, c.u32(r14)}});
            c.call(0x103af9d0, 0x10209718);
            const auto same_as_first = compare_logical(c, c.u32(r14), r15) == 0;
            c.put8(at(0x906), c.u8(at(0x906)) + 1);
            if (same_as_first) {
                c.set({{3, c.u32(r20)}, {5, std::uint32_t(s8(r13))}, {4, club()}, {6, c.u8(self + lineup_side::human)}});
                c.call(0x103b0bf0, 0x10209744);
            }
        }
        r14 = r14 + 4;
        r13 = r13 + 1;
    }
    c.set(3, self);
    match_man::call(c, lineup_entries::refresher, 0x1020975c);
}
}
