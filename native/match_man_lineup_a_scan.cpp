// MATCH_MAN.CPP lineup_a block: the squad scans. The first collects the usable
// players and asks the arranger for a first lineup; the second sorts the squad
// into those that are picked outright and those left for the later passes.
#include "match_man_lineup_a.hpp"
#include "match_man_lineup_a_detail.hpp"

namespace cm::aot::match_man {
using namespace guest;

namespace {
constexpr std::uint32_t squad_size = 0x32;  // the 50 slots of side+0x2c
}

// A person under 16 is kept out when the extras' reference record is rated
// above 10 (the check both scans start with). True: skip the person.
bool ComputerLineup::skip_by_age(std::uint32_t person) {
    if (compare(c, s8(c.u16(person + lineup_person::age)), 0x10) >= 0) return false;
    const auto reference = c.u32(c.u32(side() + lineup_side::extras) + lineup_extras::reference);
    if (compare_logical(c, reference) == 0) return false;
    return compare(c, c.i16(reference + 0x18), 0xa) > 0;
}

// The club that owns `person` (the record 0x10082f20 finds is also stored at the
// global r30 points to), or 0.
std::uint32_t ComputerLineup::club_of_owner(std::uint32_t person, std::uint32_t back) {
    c.set({{3, c.u32(r14)}, {4, person}});
    c.call(0x10082f20, back);
    const auto record = c.r(3);
    compare_logical(c, record);
    c.put32(r30, record);
    if (!record) return 0;
    return c.u32(c.toc(-0x7be8)) + c.u32(record + 4) * 0x248;
}

// 0x10207d58-0x10208328: the first pass, for a side whose urgency is not 3 and
// whose club is not in the two excluded states; then the registry is told for mode 2.
void ComputerLineup::first_pass() {
    const auto self = side();
    const auto usable = compare(c, c.u8(self + lineup_side::urgency), 3) != 0 && [&] {
        c.set({{3, club()}, {4, 0}, {5, 1}, {6, 0}, {7, 0}});
        c.call(0x1014c2e0, 0x10207d7c);
        if (compare(c, s8(c.r(3))) != 0) return false;
        c.set(3, club());
        c.call(0x100c7d00, 0x10207d8c);
        return compare(c, s8(c.r(3))) == 0;
    }();
    if (usable) {
        preset_slots();
        first_scan();
        first_arrangement();
    }
    if (compare(c, mode(), 2) == 0) {
        c.set({{3, c.u32(r20)}, {6, 0}, {4, club()}, {7, 0}, {5, c.u8(self + lineup_side::human)}});
        c.call(0x103afff0, 0x10208328);
    }
}

// 0x10207d94-0x10207f58: the halfwords at sp+0x200 that say, for each lineup slot,
// what the arranger looks for (bit sets), by the urgency byte, and the slot count at 0x905.
void ComputerLineup::preset_slots() {
    const auto urgency = s8(c.u8(side() + lineup_side::urgency));
    const auto put = [&](std::initializer_list<std::uint32_t> values, std::uint32_t count) {
        std::uint32_t index = 0;
        for (const auto value : values) c.put16(slot_sets() + 2 * index++, value);
        c.put8(at(0x905), count);
    };
    if (compare(c, urgency, 1) == 0) {
        put({4, 4, 0x204, 0x10, 0x10, 0x210, 0x40, 0x40}, 7);
    } else if (compare(c, urgency, 1) > 0) {
        if (compare(c, urgency, 3) < 0)
            put({4, 0x10, 0x40}, 2);
    } else if (compare(c, urgency, 0) >= 0) {
        put({1, 0x804, 0x84, 0x204, 0x204, 0x204, 0x810, 0x90, 0x210, 0x210, 0x210, 0x40, 0x40, 0x40, 0x40}, 0xe);
    }
}

// 0x10207f58-0x10208238: the squad's players that may play (not suspended, not
// barred by the extras' checks, not owned by a club that keeps them): into the
// candidate list at sp+0x504, r27 of them; then the picker chooses among them.
void ComputerLineup::first_scan() {
    const auto self = side();
    r27 = 0;
    r28 = 0;
    r16 = 0;
    const auto consider = [&] {
        r17 = c.u32(c.u32(self + lineup_side::squad_list) + r16);
        if (compare_logical(c, r17) == 0) return;
        if (compare_logical(c, c.u32(r17 + lineup_person::player)) == 0) return;
        if (skip_by_age(r17)) return;
        c.set({{3, c.u32(c.toc(-0x7ba0))}, {4, r17}});
        c.call(0x10187680, 0x10207fbc);
        if (compare(c, s8(c.r(3)), 2) == 0) return;
        bool excluded = false;
        if (compare(c, s8(c.u8(self + lineup_side::human))) != 0) {
            const auto extras = c.u32(self + lineup_side::extras);
            if (compare(c, c.i32(extras), -2) != 0) {
                c.set({{3, c.u32(c.toc(-0x7aa4))}, {4, r17}, {5, c.u32(extras + lineup_extras::reference)}, {6, 1}, {7, 0}});
                c.call(0x100e7920, 0x10208000);
                excluded = compare(c, s8(c.r(3))) != 0;
            }
        } else if (compare_logical(c, r26) != 0) {
            const auto extras = c.u32(self + lineup_side::extras);
            const auto rival = c.u32(r26 + 0x14);
            if (compare_logical(c, c.u32(extras + lineup_extras::reference), rival) != 0) {
                c.set({{3, c.u32(c.toc(-0x7aa4))}, {4, r17}, {6, 1}, {7, 0}, {5, rival}});
                c.call(0x100e7920, 0x10208040);
                excluded = compare(c, s8(c.r(3))) != 0;
            }
            if (!excluded) {
                c.set({{3, c.u32(c.toc(-0x7b40))}, {4, r17}, {5, r26}});
                c.call(0x103f93e0, 0x1020805c);
                excluded = compare(c, s8(c.r(3))) != 0;
            }
            if (!excluded) {
                c.set({{3, c.u32(c.toc(-0x7a64))}, {4, r17}, {6, club()}, {5, at(0x120)}, {7, 0}, {8, 0}});
                c.call(0x102b7c70, 0x10208084);
                excluded = compare_logical(c, c.r(3)) != 0;
            }
        }
        if (excluded) return;
        const auto extras = c.u32(self + lineup_side::extras);
        c.set({{3, c.u32(c.toc(-0x7a64))}, {4, r17}, {6, club()}, {7, 0}, {8, 0}});
        if (compare(c, c.i32(extras), c.i32(r31)) == 0) {
            c.set(5, at(0x118));
            c.call(0x102b7c70, 0x102080c0);
        } else {
            c.set(5, c.toc(-0x7b80));
            c.call(0x102b7c70, 0x102080ec);
        }
        if (compare_logical(c, c.r(3)) != 0) return;
        // A person another club holds is added only when that club is not ours.
        c.set({{3, c.u32(r14)}, {4, r17}});
        c.call(0x10083110, 0x10208100);
        if (compare_logical(c, c.r(3)) != 0) {
            if (compare_logical(c, club_of_owner(r17, 0x10208114), club()) == 0) return;
            r15 = club_of_owner(r17, 0x10208154);
            c.set({{3, club()}, {4, 0}, {5, 1}});
            c.call(0x100ce320, 0x1020818c);
            if (compare_logical(c, r15, c.r(3)) == 0) return;
        }
        c.put32(candidates() + std::uint32_t(s8(r27)) * 4, r17);
        r27 = r27 + 1;
    };
    do {
        consider();
        r28 = r28 + 1;
        r16 = r16 + 4;
    } while (compare(c, std::int32_t(std::uint32_t(r28)), std::int32_t(squad_size)) < 0);
    if (compare(c, s8(r27)) != 0) {
        c.set({{3, self}, {5, r27}, {4, candidates()}});
        match_man::call(c, lineup_entries::picker, 0x102081d0);
        c.put32(self + lineup_side::choice, c.r(3));
        assert_that(compare_logical(c, c.u32(self + lineup_side::choice)) != 0, 0x102081f4, 0x302, 0x180, 0x108);
    }
}

// 0x10208238-0x10208304: the arranger's flags (r15) and its call, the argument
// block of which is at sp+0x38 (the side, its manager and five zeros).
void ComputerLineup::first_arrangement() {
    const auto self = side();
    r15 = 0x8520c2;
    if (compare(c, s8(c.u8(self + lineup_side::human))) == 0) {
        r15 = r15 | 0x200000;
    } else {
        c.set({{3, club()}, {4, 1}, {5, 1}, {6, 0}, {7, 0}});
        c.call(0x1014c2e0, 0x1020826c);
        if (compare(c, s8(c.r(3))) != 0) {
            const auto owner = c.u32(club() + 0x90);
            if (compare_logical(c, owner) != 0) {
                c.set({{3, c.u32(c.toc(-0x7a54))}, {4, owner}});
                c.call(0x10147e70, 0x10208290);
                if (compare(c, s8(c.r(3))) != 0) {
                    c.set({{5, c.u32(club() + 0x90)}, {4, 3}, {3, c.u32(c.toc(-0x7a54))}});
                    c.call(0x10149df0, 0x102082b0);
                    if (compare(c, s8(c.r(3))) != 0) r15 = r15 | 0x400000;
                }
            }
        }
    }
    c.put32(at(0x38), self);
    c.put32(at(0x3c), c.u32(self + lineup_side::manager));
    for (const std::uint32_t offset : {0x40u, 0x44u, 0x48u, 0x4cu, 0x50u}) c.put32(at(offset), 0);
    c.set({{6, c.u8(at(0x905))}, {4, r27}, {9, r15}, {3, candidates()}, {5, slot_sets()}, {8, entered()}, {7, 0},
           {10, club()}});
    match_man::call(c, lineup_entries::arranger, 0x10208304);
}

// 0x10208328-0x10208888: the squad again. A player that passes the status tests
// is either picked outright (registered with the squad registry when the mode
// is 1) or listed in the candidate list at sp+0x504, r31 of them.
void ComputerLineup::second_scan() {
    const auto self = side();
    r31 = 0;
    r17 = 0;
    r15 = r31;
    do {
        r16 = c.u32(c.u32(self + lineup_side::squad_list) + r15);
        if (compare_logical(c, r16) != 0 && compare_logical(c, c.u32(r16 + lineup_person::player)) != 0 &&
            !(compare(c, s8(c.u8(self + lineup_side::human))) == 0 && skip_by_age(r16)))
            second_candidate(r16);
        r17 = r17 + 1;
        r15 = r15 + 4;
    } while (compare(c, std::int32_t(std::uint32_t(r17)), std::int32_t(squad_size)) < 0);
}

// The picked-out player is registered with the squad registry (r20's global):
// only when the mode is 1, and the second call only when the first answers a slot.
void ComputerLineup::claim(std::uint32_t person, std::uint32_t first_back) {
    if (compare(c, mode(), 1) != 0) return;
    const auto self = side();
    c.set({{3, c.u32(r20)}, {5, person}, {4, club()}, {6, c.u8(self + lineup_side::human)}});
    c.call(0x103b05c0, first_back);
    if (compare(c, s8(c.r(3)), -1) <= 0) return;
    c.set({{3, c.u32(r20)}, {5, person}, {4, club()}, {7, 1}, {6, c.u8(self + lineup_side::human)}});
    c.call(0x103afb90, first_back + 0x24);
}

// 0x10208388-0x102084d4: whether a player is held back by the squad's status bits
// (+0x6a of the person) before the common tests; the early returns are the
// original's jumps to the loop's end, finish_candidate the rest.
void ComputerLineup::second_candidate(std::uint32_t person) {
    const auto self = side();
    const auto status = [&](std::uint32_t mask) { return std::int32_t(c.u8(person + lineup_person::status) & mask); };
    if (compare(c, c.u8(self + lineup_side::urgency), 3) == 0) {
        c.set(3, club());
        c.call(0x100c7d00, 0x102084c0);
        if (compare(c, s8(c.r(3))) != 0 && compare(c, status(8)) != 0) return;
        return finish_candidate(person);
    }
    c.set({{3, club()}, {4, 0}, {5, 1}, {6, 0}, {7, 0}});
    c.call(0x1014c2e0, 0x102083ac);
    if (compare(c, s8(c.r(3))) != 0) {
        c.set(3, club());
        c.call(0x100c7d00, 0x102083bc);
        if (compare(c, s8(c.r(3))) != 0) {
            if (compare(c, status(8)) == 0) return;
            return finish_candidate(person);
        }
        if (compare(c, s8(c.u8(self + lineup_side::human))) != 0 && compare(c, status(1)) != 0) {
            c.set({{3, c.u32(r20)}, {5, person}, {4, club()}, {6, c.u8(self + lineup_side::human)}});
            c.call(0x103b05c0, 0x102083fc);
            if (compare(c, s8(c.r(3)), -1) == 0) return;
        }
        if (compare(c, s8(c.u8(self + lineup_side::human))) != 0) return finish_candidate(person);
        if (compare(c, status(2)) == 0) return finish_candidate(person);
        return;
    }
    if (compare_logical(c, person) != 0 && compare(c, c.i32(person), c.i32(r13)) >= 0) return finish_candidate(person);
    c.set(3, club());
    c.call(0x100c7d00, 0x10208444);
    if (compare(c, s8(c.r(3))) != 0) {
        if (compare(c, status(4)) != 0) return;
        return finish_candidate(person);
    }
    // Not already among the lineup's entered players (sp+0x25c, the preset count at 0x905).
    const auto count = s8(c.u8(at(0x905)));
    bool found = false;
    for (std::uint32_t index = 0; compare(c, s8(index), count) < 0; ++index) {
        if (compare_logical(c, c.u32(entered() + 4 * index), person) == 0) {
            found = true;
            break;
        }
    }
    if (compare(c, found) != 0) return;
    finish_candidate(person);
}

// 0x102084d4-0x10208878: the tests every surviving player meets, in the original's
// order; the first that answers decides: claimed for the registry or, at the end,
// listed (or, in mode 1, marked as a pick of the slot the registry names).
void ComputerLineup::finish_candidate(std::uint32_t person) {
    const auto self = side();
    c.set(3, club());
    c.call(0x100c7d00, 0x102084dc);
    if (compare(c, s8(c.r(3))) == 0) {
        c.set({{3, c.u32(c.toc(-0x7b40))}, {4, person}, {5, club()}, {6, 0}});
        c.call(0x103ecef0, 0x102084fc);
        if (compare(c, s8(c.r(3))) != 0) return claim(person, 0x10208524);
    }
    c.set({{3, c.u32(c.toc(-0x7ba0))}, {4, person}});
    c.call(0x10187680, 0x1020855c);
    bool take = compare(c, s8(c.r(3)), 2) == 0;
    if (!take && compare(c, s8(c.u8(self + lineup_side::human))) != 0) {
        c.set({{3, c.u32(c.toc(-0x7ba0))}, {4, person}});
        c.call(0x10187680, 0x10208584);
        take = compare(c, s8(c.r(3))) != 0;
    }
    if (take) return claim(person, 0x102085ac);
    auto extras = c.u32(self + lineup_side::extras);
    if (compare(c, c.i32(extras), -2) != 0) {
        c.set({{3, c.u32(c.toc(-0x7aa4))}, {4, person}, {5, c.u32(extras + lineup_extras::reference)}, {6, 1}, {7, 0}});
        c.call(0x100e7920, 0x10208600);
        if (compare(c, s8(c.r(3))) != 0) return claim(person, 0x10208628);
    }
    c.set({{3, c.u32(c.toc(-0x7a64))}, {4, person}, {6, club()}, {7, 0}, {8, 0}, {5, c.toc(-0x7b80)}});
    c.call(0x102b7c70, 0x10208670);
    if (compare_logical(c, c.r(3)) != 0) return claim(person, 0x10208698);
    extras = c.u32(self + lineup_side::extras);
    if (compare(c, c.i32(extras), -2) != 0) {
        c.set({{3, c.u32(c.toc(-0x7b40))}, {4, person}, {5, extras}});
        c.call(0x103f93e0, 0x102086e0);
        if (compare(c, s8(c.r(3))) != 0) return claim(person, 0x10208708);
    }
    c.set({{3, c.u32(r14)}, {4, person}});
    c.call(0x10083110, 0x1020873c);
    if (compare_logical(c, c.r(3)) != 0) {
        bool owned = compare_logical(c, club_of_owner(person, 0x10208750), club()) == 0;
        if (!owned) {
            r26 = club_of_owner(person, 0x10208790);
            c.set({{3, club()}, {4, 0}, {5, 1}});
            c.call(0x100ce320, 0x102087c8);
            owned = compare_logical(c, r26, c.r(3)) == 0;
        }
        if (owned) return claim(person, 0x102087f0);
    }
    if (compare(c, mode(), 1) == 0) {
        c.set({{3, c.u32(r20)}, {5, person}, {4, club()}, {6, c.u8(self + lineup_side::human)}});
        c.call(0x103b05c0, 0x10208838);
        const auto slot = s8(c.r(3));
        if (compare(c, slot, -1) > 0 && compare(c, slot, std::int32_t(std::uint32_t(r29))) < 0) {
            c.put8(at(0x904), 1);
            c.put8(at(0x1c0) + std::uint32_t(slot), c.u8(at(0x904)));
            return;
        }
    }
    c.put32(candidates() + std::uint32_t(s8(r31)) * 4, person);
    r31 = r31 + 1;
}
}
