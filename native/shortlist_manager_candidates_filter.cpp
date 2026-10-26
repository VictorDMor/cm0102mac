#include "shortlist_manager_candidates_scan.hpp"

// The candidate scan's filters (0x1035f2e4-0x1035fbb0): which listed persons
// the club looks at. Each refusal moves on to the next entry; the registers
// the original leaves on the way (r4-r7) are left too, since the last
// entry's leftovers reach the scan's return.
namespace cm::aot::shortlist_manager::candidates {

void Scan::consider(std::uint32_t entry_address) {
    if (eligible(entry_address)) value();
}

// The list entry (12 bytes: person id, ?, an int32 value at +8) and its person.
bool Scan::eligible(std::uint32_t entry_address) {
    r14 = 0;
    r13 = entry_address;
    person = c.u32(c.toc(slot::persons)) + std::uint32_t(c.i32(entry_address)) * Person::size;
    const Person p = candidate();
    if (compare_logical(c, person, c.u32(at(local::excluded))) == 0) return false;
    const std::int32_t id = p.id();
    c.set(4, std::uint32_t(id));
    // The last sixteen person ids are never candidates.
    if (compare(c, id, std::int32_t(c.u32(c.toc(slot::person_count)) - 16)) >= 0) return false;
    if (compare(c, std::uint8_t(p.job()), 5) == 0) return false;
    if (compare(c, p.national_job(), 5) == 0) return false;
    if (nonzero_logical(c, p.non_player())) {
        const auto listings = c.u32(c.toc(slot::listings));
        if (nonzero_logical(c, listings) &&
            nonzero(c, std::int8_t(program.call(0x101b1620, 0x1035f368, {listings, person}))))
            return false;
    }
    if (nonzero(c, std::int8_t(player_row))) {
        if (!player_checks(entry_address)) return false;
    } else {
        if (!staff_checks()) return false;
    }
    return common_checks();
}

// A player row: a human club's manager may veto through the negotiations
// object (0x1030fbf0, kept at 0x11b); the person must be a player (status
// 11) not excluded by his metadata or nation, under 24 for mode 5, and, in
// the first pass, his listed value x 1.3 within the row's budget; in the
// relaxed pass 0x100d3230 must agree unless he has a club and no value.
bool Scan::player_checks(std::uint32_t entry_address) {
    const Person p = candidate();
    if (human(club_address, 1, 0x1035f390)) {
        const auto manager_record = program.call(0x100c96e0, 0x1035f3a0, {club_address});
        if (nonzero(c, std::int8_t(program.call(0x10147e70, 0x1035f3b0, {c.u32(c.toc(slot::humans)), manager_record})))) {
            const auto again = program.call(0x100c96e0, 0x1035f3c0, {club_address});
            c.put8(at(local::human_talks), program.call(0x1030fbf0, 0x1035f3d4, {c.u32(c.toc(slot::negotiations)), person, again}));
            const auto third = program.call(0x100c96e0, 0x1035f3e4, {club_address});
            const std::int32_t answer = std::int8_t(program.call(0x10149df0, 0x1035f3f8, {c.u32(c.toc(slot::humans)), 4, third}));
            if (compare(c, answer, 2) == 0 && !nonzero(c, std::int8_t(c.u8(at(local::human_talks))))) return false;
        }
    }
    if (compare(c, std::uint8_t(p.job()), 11) != 0) return false;
    if (nonzero_logical(c, p.national_team()) && compare(c, p.national_job(), 5) == 0) return false;
    if (!nonzero_logical(c, p.player())) return false;
    c.set(4, c.u32(c.toc(slot::person_meta)));
    if (compare_logical(c, std::uint32_t(meta(p.id()) >> 2 & 3), 1) == 0) return false;
    const auto nation = p.nation().get();
    if (nonzero_logical(c, nation)) {
        const std::int32_t nation_id = c.i32(nation + 0x68);
        c.set(4, std::uint32_t(nation_id));
        if (compare(c, nation_id, c.i32(c.toc(slot::excluded_nation))) == 0) return false;
    }
    if (compare_logical(c, Row::category(row.flags()), 5) == 0 && compare(c, p.age(), 23) > 0) return false;
    if (!nonzero(c, std::int32_t(Row::loan(row.flags())))) {
        const std::int32_t listed = c.i32(entry_address + 8);
        c.set(4, signed_word(listed));
        const double asked = fpu.fmul(k::one_point_three, integer(0xec, listed));
        const double budget = integer(0xf4, row.budget());
        c.set_f(1, asked);
        return !(fcompare(asked, budget) & 4);
    }
    if (nonzero_logical(c, p.club()) && compare(c, c.i32(entry_address + 8), 0) <= 0) return true;
    return nonzero(c, std::int8_t(program.call(0x100d3230, 0x1035f520, {person, 0})));
}

// A staff row: the person must have a staff record; a contract of standing
// 2 costs its +0xc value x the days 0x10374d10 counts / 7 (0 otherwise),
// which must stay below the row's budget and 10 x reputation + 5000.
bool Scan::staff_checks() {
    if (!nonzero_logical(c, candidate().non_player())) return false;
    r14 = program.call(0x10082f20, 0x1035f544, {c.u32(contracts_slot), person});
    std::int32_t cost = 0;
    if (nonzero(c, r14.value())) {
        const std::int32_t standing = std::int8_t(std::int32_t(std::uint32_t(c.u8(r14 + 0x38)) << 24 & 0xfe000000) >> 26);
        if (compare(c, standing, 2) == 0) {
            c.call(0x10374d10, 0x1035f568);
            const std::int32_t days = std::int16_t(c.r(3));
            cost = std::int32_t(std::uint32_t(c.i32(r14 + 0xc)) * std::uint32_t(days)) / 7;
        }
    }
    c.set(4, std::uint32_t(cost));
    if (compare(c, cost, row.budget()) >= 0) return false;
    return compare(c, cost, std::int32_t(std::uint32_t(club.reputation()) * 10 + 5000)) <= 0;
}

// Both kinds: not at this club, not contracted to it, his metadata's upper
// nibble below 10; a player at a club who joined it within 200 days needs
// 0x100d3150's leave unless his contract says otherwise; not already linked
// to the club (0x10362d70) when that nibble is set; the country policy
// (0x1036f590), the transfer manager's checks (0x103ecef0 for modes 1-3,
// 0x103fa290 and a human selling club's consent in the relaxed pass,
// 0x103f43b0); then the sampling.
bool Scan::common_checks() {
    const Person p = candidate();
    if (compare_logical(c, p.club(), club_address) == 0) return false;
    if (compare_logical(c, contract_club(person, 0x1035f5d0), club_address) == 0) return false;
    c.set(4, c.u32(c.toc(slot::person_meta)));
    if (compare_logical(c, std::uint32_t(meta(p.id()) >> 4), 10) >= 0) return false;
    if (nonzero(c, std::int8_t(player_row)) && nonzero_logical(c, p.club())) {
        program.call(0x10717b20, 0x1035f64c, {at(local::joined_cutoff_result), c.toc(slot::today), 200});
        c.put32(at(local::joined_cutoff), c.u32(at(local::joined_cutoff_result)));
        c.put32(at(local::joined_cutoff) + 4, c.u32(at(local::joined_cutoff_result) + 4));
        const std::uint16_t cutoff_year = c.u16(at(local::joined_cutoff) + 2), joined_year = p.joined().year();
        const auto by_year = compare_logical(c, cutoff_year, joined_year);
        const std::int16_t joined_day = p.joined().day();
        c.set(4, by_year != 0 ? cutoff_year : extended(joined_day));
        const bool recent = by_year != 0 ? by_year < 0 : c.i16(at(local::joined_cutoff)) < joined_day;
        if (nonzero(c, recent)) {
            r14 = program.call(0x10082f20, 0x1035f6b4, {c.u32(contracts_slot), person});
            if (nonzero(c, r14.value()) && compare(c, c.u8(r14 + 0x60) & 0xf, 7) != 0 &&
                compare(c, c.i32(r14 + 0x50), 0) == 0 &&
                !nonzero(c, std::int8_t(program.call(0x100d3150, 0x1035f6e4, {person, 0}))))
                return false;
        }
    }
    c.set(4, c.u32(c.toc(slot::person_meta)));
    if (nonzero(c, std::int32_t(meta(p.id()) >> 4)) &&
        nonzero(c, std::int8_t(program.call(0x10362d70, 0x1035f71c, {manager, person, club_address}))))
        return false;
    if (!nonzero(c, std::int8_t(program.call(0x1036f590, 0x1035f738, {manager, person, p.club(), club_address}))))
        return false;
    if (nonzero(c, std::int8_t(program.call(0x103ecef0, 0x1035f758, {c.u32(c.toc(slot::transfers)), person, club_address, 1})))) {
        const auto mode = Row::category(row.flags());
        if (compare_logical(c, mode, 1) != 0 && compare_logical(c, mode, 2) != 0 && compare_logical(c, mode, 3) != 0)
            return false;
    }
    if (nonzero(c, std::int32_t(Row::loan(row.flags())))) {
        if (!nonzero_logical(c, p.club())) return false;
        if (!nonzero(c, std::int8_t(program.call(0x103fa290, 0x1035f7b0, {c.u32(c.toc(slot::transfers)), club_address, person, 0}))))
            return false;
    }
    if (nonzero(c, std::int32_t(Row::loan(row.flags()))) && nonzero_logical(c, contract_club(person, 0x1035f7d0))) {
        const auto seller = contract_club(person, 0x1035f810);
        if (human(seller, 0, 0x1035f850)) {
            const auto contract = program.call(0x10082f20, 0x1035f864, {c.u32(contracts_slot), person});
            if (!nonzero(c, std::int32_t(c.u8(contract + 0x5c) & 2))) return false;
        }
    }
    if (nonzero(c, std::int8_t(program.call(0x103f43b0, 0x1035f884, {c.u32(c.toc(slot::transfers)), person, 0}))))
        return false;
    if (nonzero(c, std::int8_t(c.u8(at(local::human_talks))))) return true;
    return sampled();
}

// The sampling: a full look at one person in `divisor` (by person id plus
// club id); the others are judged by where they are from. A local (same
// nation, at home, or at a club of the nation) goes by local_rules; a
// foreigner under 23 whom 0x100cab10 qualifies needs potential 3250; one
// from another continent needs reputation 2750, and, below 4250, a nation
// with a +0x11f of 12 unless his club is on this continent; then a draw.
bool Scan::sampled() {
    const Person p = candidate();
    const std::int32_t sum = std::int32_t(std::uint32_t(p.id()) + std::uint32_t(club.id()));
    const std::int32_t divisor = c.i16(at(local::divisor));
    c.set(4, std::uint32_t(divisor));
    if (!nonzero(c, modulo(0x1035f8a8, sum, divisor))) return true;
    const auto home = club.nation().get();
    c.set(4, home);
    if (compare_logical(c, home, p.nation()) == 0) return local_rules(sum);
    if (compare_logical(c, p.byte_5e(), 0) != 0) return local_rules(sum);
    const auto own = p.club().get();
    if (nonzero_logical(c, own) && compare_logical(c, c.u32(own + 0x2c), home) == 0) return local_rules(sum);
    if (compare(c, p.age(), 23) < 0 &&
        nonzero(c, std::int8_t(program.call(0x100cab10, 0x1035f908, {person, home, 1})))) {
        const auto profile = p.player().get();
        if (nonzero_logical(c, profile) && compare(c, Player{c, profile}.world_reputation(), 0xcb2) < 0) return false;
    }
    const auto nation = club.nation().get();
    c.set(4, nation);
    if (nonzero_logical(c, nation)) {
        const auto theirs = p.nation().get();
        if (nonzero_logical(c, theirs)) {
            const std::uint8_t continent_byte = c.u8(nation + 0x117);
            const std::int32_t continent = std::int8_t(continent_byte);
            c.set(5, continent_byte);
            if (compare(c, continent, std::int8_t(c.u8(theirs + 0x117))) != 0) {
                const std::int32_t reputation = club.reputation();
                c.set(6, std::uint32_t(reputation));
                if (compare(c, reputation, 0xabe) < 0) return false;
                bool strict = true;
                const auto their_club = p.club().get();
                c.set(5, their_club);
                if (nonzero_logical(c, their_club)) {
                    const auto club_nation = c.u32(their_club + 0x2c);
                    c.set(5, club_nation);
                    strict = nonzero_logical(c, club_nation) && nonzero_logical(c, nation) &&
                             compare(c, std::int8_t(c.u8(club_nation + 0x117)), continent) != 0;
                }
                if (strict && compare(c, reputation, 0x109a) < 0 && compare(c, std::int8_t(c.u8(nation + 0x11f)), 12) < 0)
                    return false;
            }
        }
    }
    const auto mode = Row::category(row.flags());
    return foreign_draw(compare_logical(c, mode, 5) == 0 || compare_logical(c, mode, 6) == 0);
}

// A sampled-out local player in the first pass, with a profile below 3250
// reputation, at no club of 3250 or more, under 20: one in 25 - the board's
// +0x42 byte (one in 15 without a board member) by the same sum.
bool Scan::local_rules(std::int32_t sum) {
    const Person p = candidate();
    if (!nonzero(c, std::int8_t(player_row))) return true;
    if (nonzero(c, std::int32_t(Row::loan(row.flags())))) return true;
    const auto profile = p.player().get();
    if (!nonzero_logical(c, profile)) return true;
    if (compare(c, Player{c, profile}.home_reputation(), 0xcb2) >= 0) return true;
    const auto own = p.club().get();
    if (nonzero_logical(c, own) && compare(c, c.i16(own + 0x1ec), 0xcb2) >= 0) return true;
    if (compare(c, p.age(), 20) >= 0) return true;
    const auto board = club.manager().get();
    if (nonzero_logical(c, board)) {
        const auto member = c.u32(board + 0x1c);
        if (nonzero_logical(c, member)) {
            const std::int32_t divisor = 25 - std::int8_t(c.u8(member + 0x42));
            c.set(4, std::uint32_t(divisor));
            return !nonzero(c, modulo(0x1035fb78, sum, divisor));
        }
    }
    c.set(4, std::uint32_t(sum < 0));
    return !nonzero(c, sum % 15);
}

// The foreigner's draw: a player needs potential 1750 (youth modes 5 and 6)
// or 2750, and 0x100cdef0's rating at least the draw below 2000 plus 2750
// (youth) or 3250.
bool Scan::foreign_draw(bool youth) {
    const Person p = candidate();
    if (nonzero(c, std::int8_t(player_row)) &&
        compare(c, Player{c, p.player()}.world_reputation(), youth ? 0x6d6 : 0xabe) < 0)
        return false;
    advance(youth ? 0x1035fa00 : 0x1035fa9c);
    const std::int32_t value = draw();
    r13 = std::uint32_t(value % 2000);
    const std::uint32_t staff = player_row == 0;
    leave(c, {{6, std::uint32_t(value < 0)}, {7, std::uint32_t(value)}});
    const std::int32_t rating =
        std::int16_t(program.call(0x100cdef0, youth ? 0x1035fa50 : 0x1035faec, {person, club.manager(), staff}));
    return compare(c, rating, r13.value() + (youth ? 0xabe : 0xcb2)) >= 0;
}
}
