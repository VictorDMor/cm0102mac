#include "shortlist_manager_squads_detail.hpp"

// 0x1035a9f0: the first-team picks rated against everyone else
// (0x1035ca60-0x1035ce50) and every pick placed in the plan's open places
// (0x1035ce50-0x1035d4c0).
namespace cm::aot::shortlist_manager::squads {
namespace {
void set_status(Cpu& c, std::uint32_t contract, std::uint32_t status) {
    c.put8(contract + 0x60, (c.u8(contract + 0x60) & 0xf0) | (status & 0xf));
}
// Happiness +0x44 moved by `change`, kept within -100..100.
void add_happiness(Cpu& c, std::uint32_t contract, std::int32_t change) {
    const std::uint8_t byte = c.u8(contract + 0x44);
    const std::int32_t moved = std::int32_t(std::int8_t(byte)) + change;
    if (compare(c, moved, 100) > 0) c.put8(contract + 0x44, 100);
    else if (compare(c, moved, -100) < 0) c.put8(contract + 0x44, std::uint8_t(-100));
    else c.put8(contract + 0x44, std::uint32_t(byte) + std::uint32_t(change));
}
}

void rate_first_choices(Assessment& a) {
    Cpu& c = a.c;
    const auto club = a.club();
    c.put32(a.at(local::count_a), std::uint32_t(byte_of(c.r(13))));
    Kept<14> index{c, 0};
    c.put32(a.at(local::picked_cursor), a.at(local::picked_a));
    Kept<28> position_offset{c, 0};
    for (;; c.put32(a.at(local::picked_cursor), c.u32(a.at(local::picked_cursor)) + 4), position_offset = position_offset + 2,
            index = index + 1) {
        if (compare(c, byte_of(index), a.local32(local::count_a)) >= 0) break;
        if (nonzero(c, a.local8(local::option))) break;
        Kept<15> person{c, c.u32(c.u32(a.at(local::picked_cursor)))};
        if (nonzero_logical(c, person) && compare_logical(c, Person{c, person}.club(), club) != 0) continue;
        const Kept<31> contract{c, a.program.call(call::club_contract, 0x1035caa8, {a.contracts(), person, club})};
        if (!nonzero_logical(c, contract)) continue;
        if (!nonzero_logical(c, person)) continue;
        Kept<23> flags{c, std::uint32_t(a.local32(local::estimate_flags)) | 0xc0};
        if (a.human(1, 0x1035cad8)) flags = flags | 0x810000;
        const Kept<27> positions{c, a.at(local::positions_a)};
        const std::uint32_t position = extended(c.i16(positions + position_offset));
        auto estimate = [&](std::uint32_t who, std::uint32_t back) {
            return a.program.call(call::estimate, back, {who, position, flags, club, 0xffffffff, a.human_manager()});
        };
        const Kept<25> rating{c, estimate(person, 0x1035cb04)};
        // The best rating of anyone else for the position: the other
        // first-team picks, the backups, the prospects and the candidates.
        Kept<22> best{c, 0};
        struct List {
            std::int32_t offset;
            std::int32_t count;
            std::uint32_t back;
            bool skip_own;
        };
        const List lists[] = {{local::picked_a, byte_of(c.r(13)), 0x1035cb54, true},
                              {local::picked_b, a.local8(local::count_b), 0x1035cbb8, false},
                              {local::picked_c, a.local8(local::count_c), 0x1035cc1c, false},
                              {local::candidates, byte_of(c.r(24)), 0x1035cc80, false}};
        for (const auto& list : lists) {
            Kept<15> i{c, 0};
            Kept<17> count{c, std::uint32_t(list.count)};
            Kept<18> at{c, a.at(list.offset)};
            Kept<16> offset{c, 0};
            for (; compare(c, byte_of(i), list.count) < 0; offset = offset + 4, at = at + 4, i = i + 1) {
                const auto other = c.u32(at);
                if (!nonzero_logical(c, other)) continue;
                if (list.skip_own && compare(c, byte_of(i), byte_of(index)) == 0) continue;
                const std::int32_t value = std::int32_t(estimate(other, list.back));
                if (compare(c, std::int32_t(std::uint32_t(best)), value) <= 0) best = std::uint32_t(value);
            }
        }
        if (compare(c, std::int32_t(std::uint32_t(best)), std::int32_t(std::uint32_t(rating))) > 0) best = rating;
        // How far ahead of the next best he is: (100 - 100 * best / rating)
        // * 0.5 in single precision, then +20 less a fifth.
        auto& fpu = a.fpu;
        const double rival = fpu.fsubs(stacked_integer(c, a.at(0x99c), signed_word(std::int32_t(std::uint32_t(best)))), a.constant(8));
        const std::int32_t own = compare(c, std::int32_t(std::uint32_t(rating)), 1) < 0 ? 1 : std::int32_t(std::uint32_t(rating));
        const auto singles = c.toc(slot::singles);
        const double mine = fpu.fsubs(stacked_integer(c, a.at(0x9a4), signed_word(own)), a.constant(8));
        const double share = fpu.frsp(fpu.fmul(a.constant(0x30), fpu.fdivs(rival, mine)));
        const double behind = fpu.fsubs(double(c.f32(singles)), share);
        c.set_f(1, behind);  // the original computes it into f1
        const double lead = fpu.fmuls(behind, double(c.f32(singles + 4)));
        const std::int32_t raw = std::int8_t(a.truncated(0x9ac, lead));
        const std::int32_t margin = std::int8_t(raw + 20 - raw / 5);
        compare_logical(c, contract);
        if (nonzero_logical(c, contract) && nonzero(c, std::int8_t(c.r(21)))) {
            add_happiness(c, contract, margin);
            const std::int32_t tenth = std::int8_t(margin / 10);
            c.put8(contract + 0x46, std::uint8_t(compare(c, tenth, 100) > 0 ? 100 : tenth));
            c.put8(contract + 0x45, 0);
        }
        if (compare(c, margin, 20) > 0 && nonzero_logical(c, contract)) {
            // Status 1 (indispensable) for a clear lead of one rated above
            // the club's value, else 2 (important).
            if (compare(c, margin, 30) < 0 || compare(c, std::int32_t(std::uint32_t(rating)), a.local32(local::value)) <= 0)
                set_status(c, contract, 2);
            else
                set_status(c, contract, 1);
        }
    }
}

namespace {
// Each pick of a list fills the next open place (from r13 on): the place's
// choice, the contract's status from the place's role, and for a refresh
// the place's date (the pick's birth date plus 120 years when he is younger
// than the manager; inferred). `after(picked_at, contract)` runs once the
// place is taken. The template arguments are the original's registers: the
// counter, the cursor over the picks (with a byte offset in `Offset`, or
// moving itself when 0), the place's offset and the contract (0: r3 only).
struct PlaceList {
    std::int32_t picked;
    std::uint32_t contract_back, date_back;
    std::int32_t date_copy, date_out;
    bool keep_status_1_2;  // list A leaves statuses 1 and 2 as they are
};
template <unsigned Counter, unsigned Cursor, unsigned Offset, unsigned Place, unsigned Contract, class After>
void place_list(Assessment& a, const PlaceList& list, std::int32_t count, After after) {
    Cpu& c = a.c;
    const auto club = a.club();
    Kept<Counter> counter{c, 0};
    Kept<Cursor> cursor{c, a.at(list.picked)};
    auto offset = [&]() -> std::uint32_t {
        if constexpr (Offset != 0) return c.r(Offset);
        else return 0;
    };
    if constexpr (Offset != 0) keep(c, {{Offset, 0}});
    for (; byte_of(counter) < count; counter = counter + 1) {
        std::uint32_t at = std::uint32_t(byte_of(c.r(13))) * Row::size;
        while (c.i32(a.slots() + at + 0x10) > -1) {
            at += Row::size;
            keep(c, {{13, c.r(13) + 1}});
        }
        const std::uint32_t picked_at = cursor + offset();
        if (c.u32(picked_at) == 0) {
            const std::uint32_t free = std::uint32_t(byte_of(c.r(13))) * Row::size;
            keep(c, {{13, c.r(13) + 1}});
            Row{c, a.slots() + free}.previous() = -1;
            Row{c, a.slots() + free}.target() = -1;
        } else {
            const Kept<Place> place{c, std::uint32_t(byte_of(c.r(13))) * Row::size};
            Row{c, a.slots() + place}.previous() = c.i32(c.u32(picked_at));
            const std::uint32_t contract = a.program.call(call::club_contract, list.contract_back, {a.contracts(), c.u32(picked_at), club});
            if constexpr (Contract != 0) keep(c, {{Contract, contract}});
            if (!nonzero(c, a.local8(local::option)) && nonzero_logical(c, contract)) {
                const std::uint32_t status = c.u8(contract + 0x60) & 0xf;
                if (!list.keep_status_1_2 || (compare(c, std::int32_t(status), 1) != 0 && compare(c, std::int32_t(status), 2) != 0))
                    set_status(c, contract, Row{c, a.slots() + place}.category());
            }
            const auto manager = a.club_manager();
            if (nonzero(c, std::int8_t(c.r(21))) && nonzero_logical(c, manager)) {
                const auto person = c.u32(picked_at);
                c.put32(a.at(list.date_copy), c.u32(person + 0x4c));
                c.put32(a.at(list.date_copy + 4), c.u32(person + 0x50));
                if (before(c.i16(manager + 0x4c), c.u16(manager + 0x4e), c.i16(a.at(list.date_copy)), c.u16(a.at(list.date_copy + 2)))) {
                    a.program.call(call::date_plus_years, list.date_back, {a.at(list.date_out), person + 0x4c, 0x78});
                    const auto date = Row{c, a.slots() + place}.date().address;
                    c.put16(date + 2, c.u16(a.at(list.date_out + 2)));
                    c.put16(date, c.u16(a.at(list.date_out)));
                    c.put8(date + 4, c.u8(a.at(list.date_out + 4)));
                }
            }
            keep(c, {{13, c.r(13) + 1}});
            after(picked_at, contract);
        }
        if constexpr (Offset != 0) keep(c, {{Offset, c.r(Offset) + 4}});
        else cursor = cursor + 4;
    }
}
}

void place_choices(Assessment& a) {
    Cpu& c = a.c;
    const auto club = a.club();
    const auto injuries = [&] { return c.u32(c.toc(toc::injuries)); };
    // List A: r17 the count, r16 the cursor, r15 the counter, r14 the place.
    const Kept<17> count_a{c, std::uint32_t(byte_of(c.r(13)))};
    keep(c, {{13, 0}});
    place_list<15, 16, 0, 14, 0>(a, {local::picked_a, 0x1035cee4, 0x1035cfb4, 0x70, 0x90, true}, std::int32_t(std::uint32_t(count_a)),
                                 [](std::uint32_t, std::uint32_t) {});
    // List B: r22 the count, r16 + r15 the cursor, r18 the counter, r17 the
    // place, r14 the contract. A backup in place 25 (inferred: the last
    // player place) is released at a club with its own staff or in a human's
    // hands (unless he is the manager or the club is short of money); a
    // frail one (record scale below -1000, +0xa below 4500) of an unhappy
    // contract is transfer-listed (inferred: +0x5c bit 1).
    const Kept<22> count_b{c, std::uint32_t(std::int32_t(a.local8(local::count_b)))};
    place_list<18, 16, 15, 17, 14>(a, {local::picked_b, 0x1035d08c, 0x1035d148, 0x68, 0x88, false}, std::int32_t(std::uint32_t(count_b)),
                                   [&](std::uint32_t picked_at, std::uint32_t contract) {
        if ((nonzero(c, a.local8(local::staff_flag)) || a.human(0, 0x1035d194)) && compare(c, byte_of(c.r(13)), 25) == 0 &&
            compare_logical(c, c.u32(picked_at), a.club_manager()) != 0 && !nonzero(c, a.local8(local::over_budget)))
            a.program.call(call::release_contract, 0x1035d1d0, {a.contracts(), c.u32(picked_at), club});
        const std::int32_t scale = std::int16_t(a.program.call(call::record_scale, 0x1035d1e4, {injuries(), c.u32(c.u32(picked_at))}));
        if (compare(c, scale, -1000) >= 0) return;
        const auto player = c.u32(c.u32(picked_at) + 0x14);
        if (compare(c, c.i16(player + 0xa), 0x1194) >= 0) return;
        if (compare(c, c.i8(player + 0x44), 0xcb2) >= 0) return;  // quirk: an int8 against 3250, never
        if (!nonzero_logical(c, contract)) return;
        if (compare(c, c.i8(contract + 0x44), -50) >= 0) return;
        c.put8(contract + 0x5c, c.u8(contract + 0x5c) | 2);
    });
    // List C: r14 the count, r17 + r16 the cursor, r22 the counter, r18 the
    // place, r15 the contract. Prospects are released at a club without its
    // own staff (unless he is the manager); a frail one (record scale below
    // -500 less a random number below 300) of an unhappy contract is
    // transfer-listed.
    const Kept<14> count_c{c, std::uint32_t(std::int32_t(a.local8(local::count_c)))};
    place_list<22, 17, 16, 18, 15>(a, {local::picked_c, 0x1035d2e8, 0x1035d3a4, 0x60, 0x80, false}, std::int32_t(std::uint32_t(count_c)),
                                   [&](std::uint32_t picked_at, std::uint32_t contract) {
        if (!nonzero(c, a.local8(local::staff_flag)) && compare_logical(c, c.u32(picked_at), a.club_manager()) != 0)
            a.program.call(call::release_contract, 0x1035d3f4, {a.contracts(), c.u32(picked_at), club});
        const std::uint32_t value = a.draw(0x1035d424);
        const Kept<18> margin{c, std::uint32_t(std::int32_t(value) % 300)};
        const std::int32_t scale = std::int16_t(a.program.call(call::record_scale, 0x1035d47c, {injuries(), c.u32(c.u32(picked_at))}));
        if (compare(c, scale, std::int32_t(std::uint32_t(-500) - margin)) >= 0) return;
        if (!nonzero_logical(c, contract)) return;
        if (compare(c, c.i8(contract + 0x44), 0) >= 0) return;
        c.put8(contract + 0x5c, c.u8(contract + 0x5c) | 2);
    });
}
}
