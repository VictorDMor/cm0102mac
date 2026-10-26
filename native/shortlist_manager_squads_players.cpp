#include "shortlist_manager_squads_detail.hpp"

// 0x1035a9f0's player phases: the plan's open places, the squad's players
// sorted into candidates, the picks for the open places (0x10210c50) and the
// review of every candidate's contract.
namespace cm::aot::shortlist_manager::squads {
namespace {
// An open player place's positions go to the list of its role: roles 2-3
// (first team, rotation) to list A (counted in r13), 4 to B, 5 to C.
void list_open_place(Assessment& a, const Row& slot) {
    Cpu& c = a.c;
    if (c.u8(slot.address) != Row::player) return;
    const std::uint32_t role = slot.category();
    const std::uint16_t positions = slot.positions();
    auto append = [&](std::int32_t list, std::int32_t count) {
        const auto n = a.local8(count);
        c.put16(a.at(list) + std::uint32_t(std::int32_t(n)) * 2, positions);
        c.put8(a.at(count), std::uint32_t(c.u8(a.at(count)) + 1));
    };
    if (role == 4) return append(local::positions_b, local::count_b);
    if (role > 4) {
        if (role < 6) append(local::positions_c, local::count_c);
        return;
    }
    if (role >= 2) {
        c.put16(a.at(local::positions_a) + std::uint32_t(byte_of(c.r(13))) * 2, positions);
        keep(c, {{13, c.r(13) + 1}});
    }
}
void free_place(Assessment& a, std::uint32_t offset) {
    Cpu& c = a.c;
    Row{c, a.slots() + offset}.target() = -1;
    Row{c, a.slots() + offset}.previous() = -1;
    Row{c, a.slots() + offset}.budget() = 0;
    list_open_place(a, Row{c, a.slots() + offset});
}
}

void collect_open_places(Assessment& a) {
    Cpu& c = a.c;
    c.put8(a.at(local::count_b), 0);
    c.put8(a.at(local::count_c), 0);
    Kept<15> index{c, 0};
    keep(c, {{13, 0}});
    Kept<14> offset{c, 0};
    for (; byte_of(index) < slot_count; offset = offset + Row::size, index = index + 1) {
        const Row slot{c, a.slots() + offset};
        if (slot.target() <= -1) {
            free_place(a, offset);
            continue;
        }
        if (slot.approach() != -1) continue;
        // A held place is freed when its date has passed or its holder's job
        // is 0x10 (inferred: no longer at work).
        const auto today = c.r(16);
        c.put32(a.at(0x78), c.u32(slot.date().address));
        c.put32(a.at(0x7c), c.u32(slot.date().address + 4));
        const bool passed = before(c.i16(a.at(0x78)), c.u16(a.at(0x7a)), c.i16(today), c.u16(today + 2)) != 0;
        if (!passed && c.i8(person_address(c, slot.target()) + 0x62) != 0x10) continue;
        const auto holder = person_address(c, slot.target());
        if (nonzero(c, std::int8_t(a.program.call(call::contains_staff_club_link, 0x1035b58c, {a.manager, holder, a.club()})))) {
            const auto persons = c.u32(c.toc(slot::persons));
            c.set(8, persons);
            const auto again = persons + std::uint32_t(Row{c, a.slots() + offset}.target()) * Person::size;
            a.program.call(call::transition_staff_links, 0x1035b5c4, {a.manager, again, a.club(), 3, 0});
        }
        free_place(a, offset);
    }
}

void sort_squad(Assessment& a) {
    Cpu& c = a.c;
    const auto club = a.club();
    // r23 counts `others`, r24 `candidates`.
    Kept<23> others{c, 0};
    Kept<24> candidates{c, 0};
    a.program.call(call::squad_list, 0x1035b794, {club, a.at(local::squad), 0});
    Kept<28> index{c, 0};
    Kept<31> cursor{c, a.at(local::squad)};
    for (; std::int32_t(std::uint32_t(index)) < squad_size; index = index + 1, cursor = cursor + 4) {
        const Kept<22> person_address{c, c.u32(cursor)};
        const Person person{c, person_address};
        if (!nonzero_logical(c, person_address)) continue;
        if (compare(c, person.id(), c.i32(c.toc(slot::person_count))) >= 0) continue;
        if (!nonzero_logical(c, person.player())) continue;
        if (c.u8(person_address + 0x62) == 0x10) continue;
        if (nonzero_logical(c, a.program.call(call::club_link_row, 0x1035b7f4, {a.manager, club, person_address, 0, 0})))
            continue;
        const Kept<15> contract{c, a.program.call(call::club_contract, 0x1035b80c, {a.contracts(), person_address, club})};
        if (!nonzero_logical(c, contract)) continue;
        if (nonzero(c, std::int8_t(a.program.call(call::special_staff_offer, 0x1035b828, {c.u32(c.toc(toc::offers)), person_address, 0}))))
            continue;
        if (nonzero(c, c.i8(contract + 0x54))) continue;
        const auto set_status = [&](std::uint32_t status) { c.put8(contract + 0x60, (c.u8(contract + 0x60) & 0xf0) | status); };
        if (!nonzero(c, a.local8(local::option))) {
            c.put8(contract + 0x5c, c.u8(contract + 0x5c) & ~1u);
            c.put8(contract + 0x5c, c.u8(contract + 0x5c) & ~2u);
            // Status: 6 for a young player (inferred: "a decent young
            // player"), else 7 (not needed), or 0 in a human's club.
            if (person.age() < 20) set_status(6);
            else set_status(a.human(0, 0x1035b89c) ? 0 : 7);
            a.program.call(call::keep_contract, 0x1035b8d8, {a.contracts(), person_address, club});
        } else if (a.refresh() && !a.human(1, 0x1035b8fc)) {
            set_status(person.age() < 20 ? 6 : 0);
        }
        // An injured player (r14: the current injury record) is dropped when
        // the injury lasts longer than he is worth waiting for (r27 days:
        // 30 when the injury risk draw hits, 150, or 300/400 for one rated
        // near or above the club's value).
        const auto injuries = [&] { return c.u32(c.toc(toc::injuries)); };
        const Kept<14> injury{c, a.program.call(call::current_injury, 0x1035b948, {injuries(), person_address})};
        Kept<27> patience{c, 150};
        const std::int32_t risk = std::int8_t(a.program.call(call::risk_multiplier, 0x1035b960, {injuries(), person_address}));
        const Kept<17> range{c, std::uint32_t(risk + 1)};
        if (std::int32_t(std::uint32_t(range)) == 0) keep(c, {{18, 0}});
        else random_below(a, std::int32_t(std::uint32_t(range)), {0x1035b9a4, 0x1035ba78, 0x1035bac8, 0x9ac, 0x9b4, 0x9bc, true}, 18);
        if (compare(c, std::int32_t(c.r(18)), 0) != 0) {
            patience = 30;
        } else if (nonzero_logical(c, injury)) {
            const Kept<25> rating{c, a.program.call(call::estimate, 0x1035bb18,
                                                    {person_address, 0xffffffff, std::uint32_t(a.local32(local::estimate_flags)) | 0x6c0,
                                                     club, 0xffffffff, a.human_manager()})};
            const std::int32_t value = a.local32(local::value);
            if (compare(c, std::int32_t(std::uint32_t(rating)), value) >= 0) {
                patience = 400;
            } else {
                const double limit = a.fpu.fmul(a.constant(0xa0), a.integer(0x9b4, value));
                const double rated = a.integer(0x9ac, std::int32_t(std::uint32_t(rating)));
                c.set_f(1, rated);  // the original computes it into f1
                a.fpu.fcmp(0, rated, limit);
                // cror eq,gt,eq
                const auto field = c.cr(0);
                c.set_cr(0, std::uint8_t((field & 0xd) | ((field >> 1 | field) & 2)));
                if (c.cr(0) & 2) patience = 300;
            }
        }
        const bool short_injury = !nonzero_logical(c, injury) ||
                                  compare(c, c.i16(injury + 0x34), std::int32_t(std::uint32_t(patience))) < 0;
        if (short_injury &&
            !nonzero_logical(c, a.program.call(call::transfer_listed, 0x1035bba0, {c.u32(c.toc(toc::transfer_list)), person_address, 1}))) {
            const auto second = a.program.call(call::second_contract, 0x1035bc18, {a.contracts(), person_address});
            if (nonzero_logical(c, second) && compare_logical(c, person.club(), club) != 0) {
                c.put32(a.at(local::others) + std::uint32_t(byte_of(others)) * 4, person_address);
                others = others + 1;
            } else {
                c.put32(a.at(local::candidates) + std::uint32_t(byte_of(candidates)) * 4, person_address);
                candidates = candidates + 1;
            }
            if (nonzero(c, std::int8_t(c.r(21)))) c.put8(contract + 0x44, 0);
            continue;
        }
        if (!nonzero(c, a.local8(local::staff_flag)) ||
            (nonzero_logical(c, injury) && compare(c, c.i16(injury + 0x34), 300) > 0))
            a.program.call(call::release_contract, 0x1035bbd8, {a.contracts(), person_address, club});
        if (a.human(0, 0x1035bbf0)) set_status(0);
    }
}

namespace {
// The picks of one list (0x10210c50): candidates (r24 of them) for the
// list's positions; the picked come back in `picked`. A human's club passes
// its manager and flags | `human_flags`; the others their manager person.
struct PickSite {
    std::int32_t positions, picked;
    std::uint32_t flags, human_flags, human_back, back;
};
void pick(Assessment& a, const PickSite& site, std::uint32_t count, bool human) {
    Cpu& c = a.c;
    const auto args = a.at(local::stack_arguments);
    c.put32(args, 0);
    c.put32(args + 4, human ? a.human_manager() : a.club_manager());
    c.put32(args + 8, 1);
    for (std::uint32_t offset = 0xc; offset <= 0x18; offset += 4) c.put32(args + offset, 0);
    const std::uint32_t flags = std::uint32_t(a.local32(local::estimate_flags)) | (human ? site.human_flags : site.flags);
    a.program.call(call::pick_players, human ? site.human_back : site.back,
                   {a.at(local::candidates), c.r(24), a.at(site.positions), count, 0, a.at(site.picked), flags, a.club()});
}
// Each picked person leaves the candidates: the match is replaced by the
// last candidate (quirk: the replacement is not looked at again) and the
// count shrinks. `outer` and `inner` are the original's counters (r14/r15,
// or r15/r17 for list C).
template <unsigned Outer, unsigned Inner>
void remove_picked(Assessment& a, std::int32_t picked, std::int32_t count) {
    Cpu& c = a.c;
    const auto candidates = a.at(local::candidates);
    Kept<Outer> i{c, 0};
    for (; byte_of(i) < count; i = i + 1) {
        const auto person = c.u32(a.at(picked) + std::uint32_t(byte_of(i)) * 4);
        if (person == 0) continue;
        Kept<Inner> j{c, 0};
        for (; byte_of(j) < byte_of(c.r(24)); j = j + 1) {
            const auto at = candidates + std::uint32_t(byte_of(j)) * 4;
            if (c.u32(at) != person) continue;
            const auto last = candidates + std::uint32_t(byte_of(c.r(24))) * 4 - 4;
            c.put32(at, c.u32(last));
            c.put32(last, 0);
            keep(c, {{24, c.r(24) - 1}});
        }
        // The loop's r7-r10: the zero it stores, the list, its byte index
        // and the list's end.
        leave(c, {{7, 0}, {8, candidates}, {9, std::uint32_t(j) * 4}, {10, std::uint32_t(byte_of(c.r(24))) * 4}});
    }
}
}

void pick_for_places(Assessment& a) {
    Cpu& c = a.c;
    // List A (first team and rotation: positions_a, r13 of them).
    if (byte_of(c.r(13)) != 0) {
        const bool human = a.human(1, 0x1035bc98);
        pick(a, {local::positions_a, local::picked_a, 0xc0, 0xc100c0, 0x1035bcf0, 0x1035bd44}, c.r(13), human);
        remove_picked<14, 15>(a, local::picked_a, byte_of(c.r(13)));
    }
    // List B (backups).
    if (a.local8(local::count_b) != 0) {
        const bool human = a.human(1, 0x1035be00);
        pick(a, {local::positions_b, local::picked_b, 0x1c0, 0x1c0, 0x1035be54, 0x1035bea8}, c.u8(a.at(local::count_b)), human);
        remove_picked<14, 15>(a, local::picked_b, a.local8(local::count_b));
    }
    // Players with a second contract elsewhere join the candidates after
    // the others (the original copies them eight at a time).
    const std::int32_t others = byte_of(c.r(23));
    const auto other = [&](std::int32_t i) { return c.u32(a.at(local::others) + std::uint32_t(i) * 4); };
    std::int32_t copied = 0;
    if (others > 8) {
        for (; std::int8_t(copied) < byte_of(c.r(23) - 8); copied += 8) {
            const std::uint32_t first = std::uint32_t(byte_of(c.r(24)));
            for (std::int32_t k = 0; k < 8; ++k)
                c.put32(a.at(local::candidates) + (first + std::uint32_t(k)) * 4, other(copied + k));
            keep(c, {{24, c.r(24) + 8}});
            // What the unrolled copy leaves in r7-r10.
            leave(c, {{7, (first + 7) * 4}, {8, (first + 6) * 4}, {9, other(copied + 6)}, {10, other(copied + 5)}});
        }
    }
    for (; std::int8_t(copied) < others; ++copied) {
        c.set(7, other(copied));
        c.put32(a.at(local::candidates) + std::uint32_t(byte_of(c.r(24))) * 4, other(copied));
        keep(c, {{24, c.r(24) + 1}});
    }
    // Candidates older than 22 + a tenth of their +0xf (inferred: their
    // potential) are held back (r14 of them), to be tried last.
    {
        const auto candidates = a.at(local::candidates);
        Kept<14> held{c, 0};
        for (std::int32_t i = 0; byte_of(c.r(24)) > std::int8_t(i); ++i) {
            const auto at = candidates + std::uint32_t(std::int32_t(std::int8_t(i))) * 4;
            const auto person = c.u32(at);
            if (person == 0) continue;
            const std::int32_t potential = c.i8(c.u32(person + 0x14) + 0xf);
            const std::int32_t limit = potential / 10 + 22;
            const std::int32_t age = Person{c, person}.age();
            leave(c, {{7, std::uint32_t(limit)}, {8, std::uint32_t(multiply_high(0x66666667, potential) >> 2) >> 31}, {9, 0x66666667},
                      {10, std::uint32_t(age)}});
            if (age <= limit) continue;
            const auto last = candidates + std::uint32_t(byte_of(c.r(24))) * 4 - 4;
            leave(c, {{7, 0}, {8, c.u32(last)}, {9, a.at(local::held_back)}, {10, std::uint32_t(byte_of(held)) * 4}});
            c.put32(at, c.u32(last));
            keep(c, {{24, c.r(24) - 1}});
            c.put32(a.at(local::held_back) + std::uint32_t(byte_of(held)) * 4, person);
            held = held + 1;
            c.put32(last, 0);
            --i;
        }
    }
    // List C (prospects), then the held-back candidates rejoin.
    if (a.local8(local::count_c) != 0) {
        const bool human = a.human(1, 0x1035c148);
        pick(a, {local::positions_c, local::picked_c, 0x100, 0x100, 0x1035c19c, 0x1035c1f0}, c.u8(a.at(local::count_c)), human);
        remove_picked<15, 17>(a, local::picked_c, a.local8(local::count_c));
    }
    const std::int32_t held = byte_of(c.r(14));
    c.set(7, std::uint32_t(held > 0 ? held : 0));  // the loop's counter
    for (std::int32_t i = 0; i < held; ++i) {
        const auto person = c.u32(a.at(local::held_back) + std::uint32_t(i) * 4);
        if (person == 0) continue;
        c.put32(a.at(local::candidates) + std::uint32_t(byte_of(c.r(24))) * 4, person);
        keep(c, {{24, c.r(24) + 1}});
    }
}
}
