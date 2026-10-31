#include "awol_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::awol {
using namespace detail;
namespace {
// Routines of the rest of the game this module calls.
constexpr std::uint32_t staff_relation = 0x100d9b80;  // (staff, other staff, 1)
constexpr std::uint32_t club_query = 0x1014c2e0;      // (club, a, b, 0, 0) -> byte
constexpr std::uint32_t date_add = 0x10717890;        // (result, date, days)
constexpr std::uint32_t message_set = 0x102cb4b0;     // (message, field, value)
constexpr std::uint32_t message_new = 0x102bdf90, message_post = 0x102cdbf0, message_to_manager = 0x102cc400,
                        message_to_staff = 0x102cc140;
constexpr std::uint32_t club_peer_count = 50, club_first = 0x80;
constexpr std::uint32_t staff_trait_a = 0x65, staff_trait_b = 0x67;
constexpr std::uint32_t double_base = 0x43300000;

// The compiler's division of a word by 41, truncating toward zero.
struct Remainder41 { std::uint32_t sign, rest; };
Remainder41 remainder_41(std::uint32_t sum) {
    const auto high = multiply_high(0x63e7063f, std::int32_t(sum)) >> 4;
    const auto sign = std::uint32_t(high) >> 31;
    return {sign, sum - (std::uint32_t(high) + sign) * 41};
}
// Where one inline reduction keeps its doubles on the stack (the compiler
// placed them differently at each site), and the profile field it lowers.
struct Cut { std::uint32_t field, back; std::int32_t input, work, output; };
// Lowers one rating of the staff member's profile by a random percentage:
// rating * (one - remainder / scale), truncated, stored in 16 bits.
void cut_rating(Cpu& c, const Pool& pool, const Frame& frame, std::uint32_t staff, const Cut& cut) {
    pool.advance(cut.back);
    const auto profile = c.u32(staff + staff_profile);
    const auto sum = c.u16(pool.position()) + pool.bias();
    const auto rating = c.i16(profile + cut.field);
    const auto reduction = remainder_41(sum);
    const auto input = frame.at(cut.input), work = frame.at(cut.work), output = frame.at(cut.output);
    Fpu fp(c);
    c.put32(work, double_base);
    c.put32(input + 4, extended(rating) ^ 0x80000000u);
    c.put32(input, double_base);
    c.set_f(4, c.f64(pool.constants + 8));
    c.set_f(0, c.f64(pool.constants + 0x18));
    c.set_f(2, c.f64(pool.constants + 0x10));
    c.set_f(1, c.f64(input));
    c.set_f(3, fp.fsub(c.f(1), c.f(4)));
    c.put32(work + 4, reduction.rest ^ 0x80000000u);
    c.set_f(1, c.f64(work));
    c.set_f(1, fp.fsub(c.f(1), c.f(4)));
    c.set_f(0, fp.fdiv(c.f(1), c.f(0)));
    c.set_f(0, fp.fsub(c.f(2), c.f(0)));
    c.set_f(0, fp.fmul(c.f(3), c.f(0)));
    c.put64(output, fp.fctiwz(0, c.f(0)));
    c.put16(profile + cut.field, c.u32(output + 4));
    leave(c, {{3, profile}, {4, reduction.sign}, {5, sum}, {6, double_base}, {7, extended(rating)}});
}

// The date part of the record the list stores (12 bytes: the staff id at
// sp+0x40, then the date as the date routines lay it out: a half word, a
// half word and a byte).
void store_date(Cpu& c, const Frame& frame, std::int16_t first, std::uint16_t second, std::uint8_t third) {
    c.put16(frame.at(0x46), second);
    c.put16(frame.at(0x44), std::uint16_t(first));
    c.put8(frame.at(0x48), third);
}

// A draw from the pool whose record-form compare the original makes on the
// result, for the peers' two trait rolls.
std::int32_t roll(Cpu& c, const Pool& pool, std::uint32_t peer, std::uint32_t trait, const Draw& site) {
    c.set(17, signed_byte(c.u8(peer + trait)));
    return draw_or_zero(c, pool, site);
}
}

void register_absence(Cpu& c, std::uint32_t records, std::uint32_t staff, std::uint32_t club, std::uint32_t kind) {
    Frame frame(c, 0x10006900, 0xb0, {.first_gpr = 16});
    const auto pool = Pool::at(c);
    const auto date = frame.at(0x44), today = c.toc(-0x7b80);
    keep(c, {{20, date}, {22, pool.constants}, {23, pool.salt}, {24, pool.base}, {25, pool.end}, {26, pool.cursor},
             {27, today}, {28, records}, {29, staff}, {30, club}, {31, kind}});
    call(c, date_construct, 0x10006944, {date});
    if (compare_logical(c, staff) == 0) return;
    if (compare_logical(c, c.u32(staff + staff_profile)) == 0) return;
    // The original tests the staff pointer again here; it is known nonzero.
    compare_logical(c, staff);
    const auto count = c.toc(-0x7be4);
    c.set({{3, count}, {4, c.u32(staff)}});
    if (compare(c, std::int32_t(c.r(4)), c.i32(count)) >= 0) return;
    const auto flags = c.u32(records + flags_buffer), id = c.u32(staff);
    const auto flag = c.u8(flags + id);
    c.set({{5, flags}, {4, id}, {3, flag}});
    if (compare(c, std::int32_t(flag & primary_bit)) != 0) return;
    c.put8(flags + id, flag | primary_bit);

    cut_rating(c, pool, frame, staff, {0xa, 0x100069b4, 0x5c, 0x54, 0x4c});
    cut_rating(c, pool, frame, staff, {8, 0x10006a68, 0x4c, 0x54, 0x5c});
    cut_rating(c, pool, frame, staff, {0xc, 0x10006b1c, 0x4c, 0x54, 0x5c});

    const auto own_club = c.u32(staff + staff_club);
    c.set(3, own_club);
    if (compare_logical(c, own_club) != 0) {
        call(c, staff_relation, 0x10006bcc, {staff, c.u32(own_club + club_second), 1});
        const auto queried = call(c, club_query, 0x10006be4, {c.u32(staff + staff_club), 1, 1, 0, 0});
        if (compare(c, std::int8_t(queried)) == 0) {
            const auto current = c.u32(staff + staff_club);
            c.set(3, current);
            call(c, staff_relation, 0x10006c00, {c.u32(current + club_second), staff, 1});
        }
        // Each of the club's 50 peer slots: a peer with a first trait roll of
        // zero is skipped, one whose second roll is also zero is related.
        const Draw first{0x10006c3c, 17, 16};
        const Draw second{0x10006da0, 17, 16};
        Kept<18> slot(c, 0);
        Kept<21> offset(c, 0);
        do {
            const auto peers = c.u32(staff + staff_club);
            c.set(3, peers);
            Kept<19> peer(c, c.u32(peers + offset + club_roster));
            if (compare_logical(c, peer) != 0 && compare_logical(c, peer, staff) != 0
                && compare(c, roll(c, pool, peer, staff_trait_a, first)) != 0) {
                if (compare(c, roll(c, pool, peer, staff_trait_b, second)) == 0)
                    call(c, staff_relation, 0x10006f00, {peer, staff, 1});
            }
            slot = slot + 1;
            offset = offset + 4;
        } while (compare(c, std::int32_t(slot), std::int32_t(club_peer_count)) < 0);
        const auto last = c.u32(staff + staff_club);
        c.set(3, last);
        call(c, staff_relation, 0x10006f24, {c.u32(last + club_first), staff, 1});
    }

    // The record of the absence: today's date, and a duration drawn by kind.
    const auto kind_byte = std::int8_t(kind);
    const auto id_now = c.u32(staff);
    c.set({{5, id_now}, {3, c.u8(today + 4)}, {4, c.u16(today + 2)}});
    compare(c, kind_byte, 4);
    c.put32(frame.at(0x40), id_now);
    store_date(c, frame, c.i16(today), c.u16(today + 2), c.u8(today + 4));
    // Both durations are the compiler's remainder of a drawn word, plus an offset.
    if (kind_byte == 4) {
        pool.advance(0x10006f70);
        const auto at = pool.position();
        c.set(5, at);
        const auto sum = c.u16(at) + pool.bias();
        const auto high = std::int32_t(std::uint32_t(multiply_high(std::int32_t(0x84210843), std::int32_t(sum))) + sum) >> 4;
        const auto quotient = std::uint32_t(high) + (std::uint32_t(high) >> 31);
        const auto rest = sum - quotient * 31;
        leave(c, {{3, rest}, {4, sum}});
        c.set(16, extended(std::int16_t(rest + 2)));
    } else {
        pool.advance(0x10006fd8);
        const auto at = pool.position();
        c.set(5, at);
        const auto sum = c.u16(at) + pool.bias();
        const auto high = multiply_high(0x1b4e81b5, std::int32_t(sum)) >> 4;
        const auto quotient = std::uint32_t(high) + (std::uint32_t(high) >> 31);
        const auto rest = sum - quotient * 150;
        leave(c, {{3, rest}, {4, sum}});
        c.set(16, extended(std::int16_t(rest + 21)));
    }
    const auto days = c.r(16);
    call(c, date_add, 0x10007028, {frame.at(0x38), date, days});
    const auto ended_first = c.i16(frame.at(0x38));
    const auto ended_second = c.u16(frame.at(0x3a));
    const auto ended_third = c.u8(frame.at(0x3c));
    c.set(6, ended_second);
    store_date(c, frame, ended_first, ended_second, ended_third);
    call(c, list_append, 0x10007050, {c.u32(records), frame.at(0x40), 0xc});

    // Tell the reporting club (the staff member's own when none is given).
    // Kind 4 carries the duration; kinds 1 and the rest carry -1.
    std::uint32_t type = 1, value = 0xffffffff, back = 0x100070ec;
    if (compare(c, kind_byte, 4) == 0) {
        type = 4, value = days, back = 0x10007094;
    } else if (kind_byte < 4 && compare(c, kind_byte, 1) == 0) {
        type = 2, back = 0x100070c0;
    }
    compare_logical(c, club);
    const auto reporting = club ? club : c.u32(staff + staff_club);
    enter(c, back, {records, staff, reporting, type, value});
    notify(c, staff, reporting, type, value);
}

void notify_training(Cpu& c, std::uint32_t staff, std::uint32_t club) {
    Frame frame(c, 0x10008770, 0x150, {.first_gpr = 21});
    const auto pool = Pool::at(c);
    const auto message = frame.at(0x38);
    keep(c, {{22, staff}, {26, c.toc(-0x7bf0)}, {27, pool.salt}, {23, club}, {28, pool.base}, {25, 0xffffffff},
             {29, pool.end}, {30, pool.cursor}, {31, c.toc(-0x7be0)}});
    const auto service = c.toc(-0x7bf0);
    const auto set_field = [&](std::uint32_t field, std::uint32_t value, std::uint32_t back) {
        call(c, message_set, back, {message, field, value});
    };
    // The three relation records of a staff member, each through its +0x20 word.
    const auto relation = [&](std::uint32_t owner, std::uint32_t offset) { return c.u32(c.u32(owner + offset) + 0x20); };
    if (compare(c, std::int32_t(staff)) == 0) return;
    if (compare_logical(c, c.u32(staff + staff_profile)) == 0) return;
    if (compare_logical(c, club) == 0) return;
    if (compare_logical(c, c.u32(club + club_second)) == 0) return;

    call(c, message_new, 0x100087d8, {message, 0xbc9, 0});
    set_field(0, c.u32(staff), 0x100087e8);
    set_field(1, relation(staff, 0x28), 0x100087fc);
    set_field(2, relation(staff, 0x2c), 0x10008810);
    set_field(3, relation(staff, 0x30), 0x10008824);
    set_field(4, c.u32(club), 0x10008834);
    pool.advance(0x10008854);
    {
        const auto sum = c.u16(pool.position()) + pool.bias();
        const auto high = multiply_high(0x55555556, std::int32_t(sum));
        const auto quotient = std::uint32_t(high) + (std::uint32_t(high) >> 31);
        c.set(6, sum);
        set_field(5, sum - quotient * 3, 0x10008894);
    }
    const auto queried = call(c, club_query, 0x100088ac, {club, 0, 1, 0, 0});
    set_field(6, signed_byte(queried), 0x100088bc);

    // The manager reports unless the club has none, or the query for it
    // answers nonzero; then an alternate (the second slot, or a coach) does.
    Kept<25> reporter(c, 0xffffffff);
    bool manager = compare_logical(c, c.u32(club + club_second)) != 0;
    if (manager) {
        const auto answer = call(c, club_query, 0x100088e0, {club, 1, 1, 0, 0});
        manager = compare(c, std::int8_t(answer)) == 0;
    }
    if (manager) {
        const auto lead = [&](std::uint32_t offset) { return c.u32(c.u32(c.u32(club + club_second) + offset) + 0x20); };
        set_field(7, c.u32(c.u32(club + club_second)), 0x100088fc);
        set_field(8, lead(0x28), 0x10008914);
        set_field(9, lead(0x2c), 0x1000892c);
        set_field(10, lead(0x30), 0x10008944);
    } else {
        pool.advance(0x10008968);
        const auto sum = c.u16(pool.position()) + pool.bias();
        const auto sign = sum >> 31;
        c.set(3, sign);
        // (low bit xor sign) - sign: zero when the drawn word is even.
        const auto odd = std::int32_t(((sum & 1) ^ sign) - sign);
        if (compare(c, odd) == 0) {
            const auto third = c.u32(club + 0x94);
            c.set(3, third);
            if (compare_logical(c, third) != 0) reporter = c.u32(third);
        } else {
            // Up to 1000 draws of a coach slot (0..4) until one is filled.
            Kept<21> divisor(c, 0x66666667);
            Kept<24> attempts(c, 0);
            std::uint32_t slot = 0;
            for (;;) {
                pool.advance(0x100089d8);
                attempts = attempts + 1;
                const auto word = c.u16(pool.position()) + pool.bias();
                const auto high = multiply_high(std::int32_t(divisor), std::int32_t(word)) >> 1;
                const auto quotient = std::uint32_t(high) + (std::uint32_t(high) >> 31);
                slot = word - quotient * 5;
                c.set(4, slot);
                if (compare(c, std::int32_t(attempts), 1000) >= 0) break;
                if (compare_logical(c, c.u32(club + club_coaches + slot * 4)) != 0) break;
            }
            const auto coach = c.u32(club + club_coaches + slot * 4);
            c.set(3, coach);
            if (compare_logical(c, coach) != 0) reporter = c.u32(coach);
        }
        if (compare(c, std::int32_t(reporter), -1) == 0) {
            set_field(7, 0xffffffff, 0x10008a58);
            set_field(8, 0xffffffff, 0x10008a68);
            set_field(9, 0xffffffff, 0x10008a78);
            set_field(10, 0xffffffff, 0x10008a88);
        } else {
            set_field(7, reporter, 0x10008a9c);
            Kept<21> row(c, reporter * staff_size);
            const auto relation_of = [&](std::uint32_t offset) { return c.u32(c.u32(c.u32(c.toc(-0x7be0)) + row + offset) + 0x20); };
            set_field(8, relation_of(0x28), 0x10008abc);
            set_field(9, relation_of(0x2c), 0x10008ad8);
            set_field(10, relation_of(0x30), 0x10008af4);
        }
    }

    const auto posted = call(c, message_post, 0x10008b00, {c.u32(service), club});
    if (compare(c, std::int8_t(posted)) == 0) {
        c.set(5, c.u32(club + 0x2c));
        if (compare_logical(c, c.r(5)) == 0) return;
        const auto profile = c.u32(staff + staff_profile);
        c.set(3, profile);
        if (compare(c, c.i16(profile + 0xa), 0x1c52) <= 0) return;
        call(c, message_to_manager, 0x10008b30, {c.u32(service), message});
    } else {
        call(c, message_to_staff, 0x10008b44, {c.u32(service), message, c.u32(staff + staff_club)});
    }
}
}
