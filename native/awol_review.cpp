#include "awol_detail.hpp"

namespace cm::aot::awol {
using namespace detail;
namespace {
// Staff record fields the review reads and edits; the traits are signed bytes
// that decay when a draw allows it.
constexpr std::uint32_t staff_age = 0x5c, staff_level = 0x63, trait_a = 0x67, trait_b = 0x69;
constexpr std::uint32_t profile_ability = 0xa, profile_trait = 0x44;
constexpr std::uint32_t relation_kind = 0x60, roster_slots = 50;
// The items of the secondary and pending lists: the date, and the byte telling
// a return from leave (nonzero) from an unapproved return (zero).
constexpr std::uint32_t secondary_date = 4, secondary_mode = 0xc, pending_date = 8, pending_mode = 0x10;
constexpr std::uint32_t adjust_date = 0x10717a10, relation_of = 0x10082f90, peer_kind = 0x1037fa70, peer_penalty = 0x1037f8a0,
                        club_report = 0x10649fb0;

// The 18 inline draws of the routine. `a` and `b` are the record's two traits.
constexpr Draw site(std::uint32_t start, unsigned bound, unsigned result, bool cached = false) { return {start, bound, result, cached}; }
constexpr Draw leave_a = site(0x10004224, 15, 16);
constexpr Draw leave_b = site(0x10004388, 15, 16);
constexpr Draw young_a = site(0x100044f8, 15, 16);
constexpr Draw young_b = site(0x10004674, 15, 16);
constexpr Draw peer_gate = site(0x100048c8, 16, 15);
constexpr Draw peer_a = site(0x10004afc, 15, 16);
constexpr Draw peer_profile = site(0x10004c80, 15, 16);
constexpr Draw peer_b = site(0x10004e00, 15, 16);
constexpr Draw absentee_gate = site(0x10004f94, 14, 15);
constexpr Draw absentee_a = site(0x1000516c, 14, 15, true);
constexpr Draw absentee_profile = site(0x100052e8, 14, 15);
constexpr Draw pending_leave_a = site(0x100055a0, 14, 15);
constexpr Draw pending_leave_b = site(0x10005704, 14, 15);
constexpr Draw pending_young_a = site(0x10005874, 14, 15);
constexpr Draw pending_young_b = site(0x100059f0, 14, 15);
constexpr Draw pending_gate = site(0x10005b84, 15, 16);
constexpr Draw pending_a = site(0x10005d58, 15, 16, true);
constexpr Draw pending_profile = site(0x10005edc, 15, 16);

// The compiler's signed "a < b" on two years: the sign of (x >> 1) - (x & a).
std::uint32_t year_order(Cpu& c, std::uint32_t a, std::uint32_t b) {
    const auto x = a ^ b;
    const auto half = std::uint32_t(std::int32_t(x) >> 1);
    c.set(3, half);
    return (half - (x & a)) >> 31;
}
// Whether today is after a primary record's date (year at +0, day at +2):
// days compare unsigned, equal days fall back to the years. Equal dates are
// not later.
bool later_than_record(Cpu& c, std::uint32_t today, std::uint32_t date) {
    const auto day = c.u16(date + 2), now = c.u16(today + 2);
    c.set({{3, day}, {5, now}});
    std::uint32_t later = 1;
    if (now < day) later = 0;
    else if (now == day) {
        const auto year = extended(c.i16(today));
        c.set(5, year);
        later = year_order(c, year, extended(c.i16(date)));
    }
    return compare(c, std::int8_t(later)) != 0;
}
// The same test against a date the adjusting routine produced.
bool later_than_adjusted(Cpu& c, std::uint32_t today, std::uint32_t date) {
    const auto day = c.u16(date + 2), now = c.u16(today + 2);
    c.set(3, now);
    std::uint32_t later = 1;
    if (now < day) later = 0;
    else if (now == day) {
        const auto year = extended(c.i16(today));
        c.set(4, year);
        later = year_order(c, year, extended(c.i16(date)));
    }
    return compare(c, std::int8_t(later)) != 0;
}
void copy_today(Cpu& c, std::uint32_t to) {
    const auto today = c.r(19);
    c.put32(to, c.u32(today));
    c.put32(to + 4, c.u32(today + 4));
}

// `lbz rX,address; extsb r0,rX; cmpwi r0,1`: true above one. The byte stays in rX.
bool above_one(Cpu& c, std::uint32_t address, unsigned into = 3) {
    const auto byte = c.u8(address);
    c.set(into, byte);
    return compare(c, std::int8_t(byte), 1) > 0;
}
// `extsb. bound,byte` and the draw it guards.
std::int32_t roll(Cpu& c, const Pool& pool, const Draw& draw_site, std::uint32_t address) {
    c.set(draw_site.bound, signed_byte(c.u8(address)));
    return draw_or_zero(c, pool, draw_site);
}
// `lbz r3,address; subi r0,r3,1; stb r0,address`.
void decrement(Cpu& c, std::uint32_t address) {
    const auto byte = c.u8(address);
    c.set(3, byte);
    c.put8(address, byte - 1);
}
// Three remainders of the cursor's word plus the salt, each by the compiler's
// reciprocal multiply. The cursor, the sum and the quotient's sign stay in
// the scratch registers the original used.
std::int32_t sum_modulo_30(Cpu& c, const Pool& pool) {
    pool.advance(0x10004a30);
    const auto cursor = pool.position();
    const auto sum = c.u16(cursor) + pool.bias();
    const auto quotient = std::int32_t(std::uint32_t(multiply_high(std::int32_t(0x88888889u), std::int32_t(sum))) + sum) >> 4;
    const auto sign = std::uint32_t(quotient) >> 31;
    leave(c, {{5, cursor}, {4, sum}, {3, sign}});
    return std::int32_t(sum - (std::uint32_t(quotient) + sign) * 30);
}
std::int32_t sum_modulo_15(Cpu& c, const Pool& pool) {
    pool.advance(0x100050fc);
    const auto cursor = pool.position();
    const auto sum = c.u16(cursor) + pool.bias();
    const auto quotient = std::int32_t(std::uint32_t(multiply_high(std::int32_t(0x88888889u), std::int32_t(sum))) + sum) >> 3;
    const auto sign = std::uint32_t(quotient) >> 31;
    const auto rest = sum - (std::uint32_t(quotient) + sign) * 15;
    leave(c, {{3, cursor}, {6, signed_byte(c.r(15))}, {5, sum}, {4, rest}});
    return std::int32_t(rest);
}
std::int32_t sum_modulo_5(Cpu& c, const Pool& pool) {
    pool.advance(0x10005cec);
    const auto cursor = pool.position();
    const auto sum = c.u16(cursor) + pool.bias();
    const auto quotient = multiply_high(std::int32_t(0x66666667), std::int32_t(sum)) >> 1;
    const auto sign = std::uint32_t(quotient) >> 31;
    const auto rest = sum - (std::uint32_t(quotient) + sign) * 5;
    leave(c, {{3, cursor}, {6, signed_byte(c.r(16))}, {5, sum}, {4, rest}});
    return std::int32_t(rest);
}
// `addi r0,rest,1; extsb r0,r0; cmpw r6,r0`: the draw (as a signed byte) is
// not below the limit.
bool draw_reaches_limit(Cpu& c, std::int32_t drawn, std::int32_t rest) {
    return compare(c, std::int8_t(drawn), std::int8_t(rest + 1)) >= 0;
}

struct Daily {
    Cpu& c;
    const Frame& frame;
    Pool pool;
    std::uint32_t records;

    // The staff member a list item names, in `reg`; r3 keeps the table.
    std::uint32_t staff_of(std::uint32_t item, unsigned reg) {
        const auto table = c.u32(c.r(18));
        c.set(3, table);
        const auto staff = table + c.u32(item) * staff_size;
        c.set(reg, staff);
        return staff;
    }
    void remove_secondary(std::uint32_t back, std::uint32_t item) {
        enter(c, back, {records, item});
        remove_secondary_item(c, records, item);
    }
    void run_training_event(std::uint32_t back, std::uint32_t staff) {
        enter(c, back, {records, staff});
        training_event(c, records, staff);
    }
    // The pending record leaves the list and the review ends.
    void remove_pending(std::uint32_t back, std::uint32_t item) {
        call(c, list_remove, back, {c.u32(records + pending_list), item});
    }

    // First pass: a primary record past its date is removed, and that ends
    // the list. Returns false when the whole review ends.
    bool primary() {
        c.set(3, c.u32(records + primary_list));
        if (compare_logical(c, c.r(3)) == 0) return true;
        rewind(c, c.r(3));
        c.set(16, 0);
        for (;;) {
            c.set(3, c.u32(records + primary_list));
            if (compare(c, std::int32_t(c.r(16)), list_count(c, c.r(3))) >= 0) return true;
            c.call(list_next, 0x100040cc);
            c.set(4, c.r(3));
            if (compare(c, std::int32_t(c.r(4))) == 0) return false;
            const auto item = c.r(4);
            copy_today(c, frame.at(0x48));
            if (later_than_record(c, frame.at(0x48), item + 4)) {
                enter(c, 0x10004138, {records, item});
                remove_primary_item(c, records, item);
                return true;
            }
            c.set(16, c.r(16) + 1);
        }
    }

    // Second pass: training absences past their adjusted date.
    bool secondary() {
        c.set(3, c.u32(records + secondary_list));
        if (compare_logical(c, c.r(3)) == 0) return true;
        rewind(c, c.r(3));
        c.set(16, 0);
        for (;;) {
            c.set(3, c.u32(records + secondary_list));
            if (compare(c, std::int32_t(c.r(16)), list_count(c, c.r(3))) >= 0) return true;
            c.call(list_next, 0x10004174);
            c.set(21, c.r(3));
            if (compare(c, std::int32_t(c.r(21))) == 0) return false;
            const auto item = c.r(21);
            copy_today(c, frame.at(0x40));
            call(c, adjust_date, 0x1000419c, {frame.at(0x58), item + secondary_date, 1});
            if (later_than_adjusted(c, frame.at(0x40), frame.at(0x58))) {
                if (compare(c, std::int8_t(c.u8(item + secondary_mode))) != 0) return_from_leave(item);
                else unapproved_return(item);
                return true;
            }
            c.set(16, c.r(16) + 1);
        }
    }

    // A planned return: either trait must draw zero to keep the record
    // without penalty, and the training event follows.
    void return_from_leave(std::uint32_t item) {
        const auto staff = staff_of(item, 14);
        if (compare_logical(c, c.u32(staff + staff_profile)) == 0) return remove_secondary(0x100047f8, item);
        roll(c, pool, leave_a, staff + trait_a);
        if (compare(c, std::int32_t(c.r(16))) != 0) return remove_secondary(0x100047f8, item);
        roll(c, pool, leave_b, staff + trait_b);
        if (compare(c, std::int32_t(c.r(16))) != 0) return remove_secondary(0x100047f8, item);
        // Young-peer decrements: the trait must be above one and the draw zero.
        if (above_one(c, staff + trait_a)) {
            roll(c, pool, young_a, staff + trait_a);
            if (compare(c, std::int32_t(c.r(16))) == 0) decrement(c, staff + trait_a);
        }
        if (above_one(c, staff + trait_b)) {
            roll(c, pool, young_b, staff + trait_b);
            if (compare(c, std::int32_t(c.r(16))) == 0) decrement(c, staff + trait_b);
        }
        remove_secondary(0x100047dc, item);
        run_training_event(0x100047e8, staff);
    }

    // An absence that ends unannounced: the club's other staff react, then
    // the absentee's own traits may drop and the club is told.
    void unapproved_return(std::uint32_t item) {
        const auto table = c.u32(c.r(18));
        const auto staff = table + c.u32(item) * staff_size;
        c.set(28, staff);
        const auto contract = call(c, contract_of, 0x10004818, {c.u32(c.r(14)), staff});
        c.put32(c.r(15), contract);
        const auto club = club_of_contract(c, contract);
        c.set(27, club);
        c.set({{24, 0}, {26, 0}});
        for (;;) {
            if (compare(c, std::int32_t(c.r(24)), std::int32_t(roster_slots)) >= 0) break;
            if (compare_logical(c, c.r(27)) == 0) break;
            review_peer(staff, club);
            c.set({{26, c.r(26) + 4}, {24, c.r(24) + 1}});
        }
        if (compare_logical(c, c.u32(staff + staff_profile)) == 0) return remove_secondary(0x100054a4, item);
        roll(c, pool, absentee_gate, staff + trait_a);
        const auto rest = sum_modulo_15(c, pool);
        if (draw_reaches_limit(c, std::int32_t(c.r(15)), rest)) return remove_secondary(0x100054a4, item);
        if (above_one(c, staff + trait_a, 4)) {
            roll(c, pool, absentee_a, staff + trait_a);
            if (compare(c, std::int32_t(c.r(15))) != 0) decrement(c, staff + trait_a);
        }
        const auto profile = c.u32(staff + staff_profile);
        c.set(3, profile);
        if (above_one(c, profile + profile_trait)) {
            roll(c, pool, absentee_profile, profile + profile_trait);
            if (compare(c, std::int32_t(c.r(15))) != 0) {
                c.set(4, c.u32(staff + staff_profile));
                decrement(c, c.r(4) + profile_trait);
            }
        }
        remove_secondary(0x10005454, item);
        run_training_event(0x10005460, staff);
        if (compare_logical(c, c.r(27)) == 0) return;
        if (compare_logical(c, c.u32(staff + staff_profile)) == 0) return;
        call(c, club_report, 0x10005494, {c.u32(c.toc(-0x7b94)), staff, c.r(27), 8, 0, 0});
    }

    // One of the club's fifty roster slots, reacting to the absentee `staff`.
    void review_peer(std::uint32_t staff, std::uint32_t club) {
        const auto peer = c.u32(club + club_roster + c.r(26));
        c.set(25, peer);
        if (compare_logical(c, peer) == 0) return;
        if (compare_logical(c, peer, staff) == 0) return;
        if (compare_logical(c, c.u32(peer + staff_profile)) == 0) return;
        const auto relation = call(c, relation_of, 0x10004884, {c.u32(c.r(14)), staff, club});
        c.set(17, relation);
        if (compare(c, std::int32_t(relation)) == 0) return;
        c.call(peer_kind, 0x10004890);
        if (compare(c, std::int32_t(c.r(3) & 2)) == 0) senior_peer(peer, relation);
        young_peer(peer);
    }
    // A senior peer who may lose trust in the club: the relation record must be
    // of kind 1 or 2 and the peer old and good enough.
    void senior_peer(std::uint32_t peer, std::uint32_t relation) {
        const auto byte = c.u8(peer + trait_a);
        c.set(3, byte);
        if (compare(c, std::int8_t(byte), 0xf) <= 0) return;
        if (compare(c, std::int8_t(c.u8(peer + staff_level)), 0xa) <= 0) return;
        roll(c, pool, peer_gate, peer + trait_a);
        const auto rest = sum_modulo_30(c, pool);
        if (compare(c, std::int32_t(c.r(15)), rest) <= 0) return;
        const auto kind = c.u8(relation + relation_kind) & 0xf;
        if (compare(c, std::int32_t(kind), 1) != 0 && compare(c, std::int8_t(kind), 2) != 0) return;
        if (compare(c, std::int8_t(c.i16(peer + staff_age)), 0x17) <= 0) return;
        const auto profile = c.u32(peer + staff_profile);
        c.set(3, profile);
        if (compare(c, c.i16(profile + profile_ability), 0x1482) <= 0) return;
        call(c, peer_penalty, 0x10004abc, {relation, 2});
    }
    // A young, good peer: three traits may decay when the draws hit zero.
    void young_peer(std::uint32_t peer) {
        if (compare(c, std::int8_t(c.i16(peer + staff_age)), 0x15) >= 0) return;
        const auto profile = c.u32(peer + staff_profile);
        c.set(3, profile);
        if (compare(c, c.i16(profile + profile_ability), 0x1482) <= 0) return;
        if (above_one(c, peer + trait_a)) {
            roll(c, pool, peer_a, peer + trait_a);
            if (compare(c, std::int32_t(c.r(16))) == 0) decrement(c, peer + trait_a);
        }
        const auto own = c.u32(peer + staff_profile);
        c.set(3, own);
        if (compare(c, std::int8_t(c.u8(own + profile_trait)), 1) > 0) {
            roll(c, pool, peer_profile, peer + trait_a);
            if (compare(c, std::int32_t(c.r(16))) == 0) {
                c.set(4, c.u32(peer + staff_profile));
                decrement(c, c.r(4) + profile_trait);
            }
        }
        if (above_one(c, peer + trait_b)) {
            roll(c, pool, peer_b, peer + trait_b);
            if (compare(c, std::int32_t(c.r(16))) == 0) decrement(c, peer + trait_b);
        }
    }

    // Third pass: pending records past their adjusted date. A recovered one
    // is removed and ends the review; one that fails its draws is kept and
    // the scan goes on.
    bool pending() {
        c.set(3, c.u32(records + pending_list));
        if (compare_logical(c, c.r(3)) == 0) return false;
        rewind(c, c.r(3));
        c.set(21, 0);
        for (;;) {
            c.set(3, c.u32(records + pending_list));
            if (compare(c, std::int32_t(c.r(21)), list_count(c, c.r(3))) >= 0) return false;
            c.call(list_next, 0x100054dc);
            c.set(26, c.r(3));
            if (compare(c, std::int32_t(c.r(26))) == 0) return false;
            const auto item = c.r(26);
            copy_today(c, frame.at(0x38));
            call(c, adjust_date, 0x10005504, {frame.at(0x50), item + pending_date, 1});
            if (later_than_adjusted(c, frame.at(0x38), frame.at(0x50))) {
                const bool removed = compare(c, std::int8_t(c.u8(item + pending_mode))) != 0 ? recover_leave(item) : recover_absence(item);
                if (removed) return false;
            }
            c.set(21, c.r(21) + 1);
        }
    }
    // True when the record was removed.
    bool recover_leave(std::uint32_t item) {
        const auto staff = staff_of(item, 24);
        if (compare_logical(c, c.u32(staff + staff_profile)) == 0) { remove_pending(0x10005584, item); return true; }
        roll(c, pool, pending_leave_a, staff + trait_a);
        if (compare(c, std::int32_t(c.r(15))) != 0) return false;
        roll(c, pool, pending_leave_b, staff + trait_b);
        if (compare(c, std::int32_t(c.r(15))) != 0) return false;
        if (above_one(c, staff + trait_a)) {
            roll(c, pool, pending_young_a, staff + trait_a);
            if (compare(c, std::int32_t(c.r(15))) == 0) decrement(c, staff + trait_a);
        }
        if (above_one(c, staff + trait_b)) {
            roll(c, pool, pending_young_b, staff + trait_b);
            if (compare(c, std::int32_t(c.r(15))) == 0) decrement(c, staff + trait_b);
        }
        remove_pending(0x10005b58, item);
        return true;
    }
    bool recover_absence(std::uint32_t item) {
        const auto staff = staff_of(item, 14);
        roll(c, pool, pending_gate, staff + trait_a);
        const auto rest = sum_modulo_5(c, pool);
        if (draw_reaches_limit(c, std::int32_t(c.r(16)), rest)) return false;
        if (above_one(c, staff + trait_a, 4)) {
            roll(c, pool, pending_a, staff + trait_a);
            if (compare(c, std::int32_t(c.r(16))) == 0) decrement(c, staff + trait_a);
        }
        c.set(3, c.u32(staff + staff_profile));
        if (compare_logical(c, c.r(3)) != 0) {
            const auto profile = c.r(3);
            if (above_one(c, profile + profile_trait)) {
                roll(c, pool, pending_profile, profile + profile_trait);
                if (compare(c, std::int32_t(c.r(16))) == 0) {
                    c.set(4, c.u32(staff + staff_profile));
                    decrement(c, c.r(4) + profile_trait);
                }
            }
        }
        remove_pending(0x10006048, item);
        return true;
    }
};
}

void review_daily(Cpu& c, std::uint32_t records) {
    Frame frame(c, 0x10004070, 0xc0, {.first_gpr = 14});
    const auto pool = Pool::at(c);
    keep(c, {{20, records}, {29, pool.constants}, {30, pool.salt}, {23, pool.base}, {22, pool.end}, {31, pool.cursor},
             {18, c.toc(-0x7be0)}, {19, c.toc(-0x7b80)}, {14, c.toc(-0x7b7c)}, {15, c.toc(-0x7b78)}});
    Daily daily{c, frame, pool, records};
    if (!daily.primary()) return;
    if (!daily.secondary()) return;
    daily.pending();
}
}
