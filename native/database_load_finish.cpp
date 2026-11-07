// DATABASE.CPP's load block, part 5: after the tables are read, the closing re-read of the index (for files whose
// header words were swapped), the count words and the staff and index builds, and the fix-up of every person
// (database_load.hpp).
//
// The fix-up visits the people below the count less 16 whose kind byte (+0x6b) is not 6 and repairs links the file
// may hold wrongly: the detail record (+0x24) that has no club is dropped, the second date is raised to the first
// when later, the two role bytes (+0x60 and +0x62: 7 becomes 6) are checked against the links they need (a
// staff record at +0x1c, a player record at +0x14), each failure printing the person's name with a report line,
// and the three links at +0x28, +0x2c and +0x30 are filled from the unmarked-record pick. A person with no club
// record gets one drawn from the 0x128-byte table through the sample pool (the compiler's inline range_sample).
#include "database_load.hpp"
#include "database_load_detail.hpp"
#include "database_generation.hpp"
#include "database_records.hpp"
#include "database_resolve.hpp"
#include "database_search.hpp"
#include "database_tables.hpp"
#include "database_text.hpp"
#include "guest_fpu.hpp"
#include <bit>

namespace cm::aot::database {
namespace {
// srawi: the arithmetic shift, with XER[CA] set when a negative value loses a one bit.
std::int32_t srawi(guest::Cpu& c, std::int32_t value, unsigned bits) {
    const bool carry = value < 0 && (value & ((1 << bits) - 1)) != 0;
    c.s.wr<1>(guest::reg::xer_ca, carry);
    return value >> bits;
}
}
using guest::compare;
using guest::compare_logical;
using namespace load;

// 0x100aa7c-0x100abe4: the header words and the 22 entries read again (unswapped) from the file named by the
// alternate name.
bool Loader::reread_index() {
    c.set({{3, c.r(15)}, {4, frame.at(local::alternate_name)}});
    call(0x100aaa84, callee::reopen);
    c.set(19, c.r(3));
    compare(c, std::int32_t(c.r(3)));
    if (c.r(19) == 0) return false;
    if (!read_word(c.r(2) + std::uint32_t(toc::header_a), {0x100aaab0, 0x100aaac8}, {0x100aaae4, 0x100aaaf0}, 0x48, rereader))
        return false;
    if (!read_word(c.r(2) + std::uint32_t(toc::header_b), {0x100aab14, 0x100aab2c}, {0x100aab48, 0x100aab54}, 0x44, rereader))
        return false;
    const auto got = read(rereader, {0x100aab78, 0x100aab90}, c.r(21), entry::size, entry::count_of_entries);
    if (compare_logical(c, got, entry::count_of_entries) != 0) {
        close_file(0x40, {0x100aabac, 0x100aabb8});
        return false;
    }
    close_file(0x3c, {0x100aabd4, 0x100aabe0});
    return true;
}

// 0x100aabe4-0x100aacb8: the last progress line, the two header words as the counts' bias, the staff tables, and the
// index words of the two 0x6c- and 0x1c-byte tables (when the tables were allocated apart; carved ones are
// resolved by 0x100af8a0).
void Loader::sizes_and_indexes() {
    const auto step = guest::extended(std::int16_t(c.u16(frame.at(local::step))));
    c.set({{5, step}, {4, frame.at(local::progress_text)}, {3, c.toc(toc::status)}, {6, 0x2e}, {7, 0xffffffff}});
    call(0x100aabf8, callee::show_progress);
    const auto flag = ext(c.u8(c.r(2) + std::uint32_t(toc::single_block_flag)));
    c.set(0, flag);
    const bool carved = compare(c, std::int32_t(flag)) != 0;
    if (carved) {
        const auto second = c.toc(toc::header_b);
        c.put32(c.r(2) + std::uint32_t(toc::header_a), 0x7f);
        c.put32(c.r(2) + std::uint32_t(toc::header_b), second + c.r(18));
    } else {
        c.put32(c.r(2) + std::uint32_t(toc::header_a), 0);
        c.put32(c.r(2) + std::uint32_t(toc::header_b), 0);
    }
    const auto detail = c.toc(toc::detail_table);
    c.set(0, detail);
    if (compare_logical(c, detail) != 0) {
        guest::enter(c, 0x100aac40);
        build_staff_tables(c);
    }
    const auto again = ext(c.u8(c.r(2) + std::uint32_t(toc::single_block_flag)));
    c.set(0, again);
    if (compare(c, std::int32_t(again)) != 0) {
        guest::enter(c, 0x100aac54, {0});
        resolve_database_indexes(c, 0);
    } else {
        c.set(20, 0);
        while (compare(c, std::int32_t(c.r(20)), std::int32_t(c.u32(c.toc(cells::rows_6c_count)))) < 0) {
            c.put32(c.u32(c.toc(cells::rows_6c_table)) + c.r(20) * 0x6c, c.r(20));
            c.set(20, c.r(20) + 1);
        }
        c.set(20, 0);
        while (compare(c, std::int32_t(c.r(20)), std::int32_t(c.u32(c.toc(cells::rows_1c_b_count)))) < 0) {
            c.put32(c.u32(c.toc(cells::rows_1c_b_table)) + c.r(20) * 0x1c, c.r(20));
            c.set(20, c.r(20) + 1);
        }
    }
    c.set(3, pool() + pool::flush_text);
    call(0x100aacb4, callee::flush);
}

// The sample pool's next value for the inline range_sample: a pointer to the 16-bit sample, and the salt.
std::uint32_t Loader::pool_sum() {
    return c.u16(c.u32(c.toc(toc::sample_cursor))) + c.u32(c.toc(toc::sample_salt));
}

// The inline range_sample over the 0x128-byte table: a club record for a person without one (r13 the count,
// r15 the index drawn).
void Loader::draw_club_record(std::uint32_t person) {
    const auto count = c.u32(c.toc(cells::rows_128_count));
    c.set(13, count);
    compare(c, std::int32_t(count));
    if (count == 0) {
        c.set(15, 0);
    } else {
        const auto cursor_word = c.toc(toc::sample_cursor);
        const auto cursor = c.u32(cursor_word) + 2;
        const auto end = c.u32(c.toc(toc::sample_end));
        compare_logical(c, cursor, end);
        c.put32(cursor_word, cursor);
        if (cursor > end) {
            c.put32(c.toc(toc::sample_cursor), c.toc(toc::sample_start));
            call(0x100aafb0, callee::random);
            c.put32(c.toc(toc::sample_salt), c.r(3) & 0xffff);
        }
        const auto signed_count = std::int32_t(count);
        bool narrow = compare(c, signed_count, 0xffff) <= 0;
        if (narrow) narrow = compare(c, signed_count, -0xffff) >= 0;
        if (narrow) {
            // |(sample + salt) % count|, negated for a negative count.
            const auto sum = std::int32_t(pool_sum());
            compare(c, signed_count);
            const auto remainder = sum - (sum / signed_count) * signed_count;
            c.set(15, signed_count > 0 ? std::uint32_t(remainder) : std::uint32_t(-remainder));
        } else {
            // The wide arm: the count scaled in floating point by the sample's residue, plus a tenth of the
            // next level's draw, at most count - 1.
            guest::Fpu fpu(c);
            const auto constants = c.toc(toc::constants);
            const auto magic = c.f64(constants + 0x18), limit = c.f64(constants + 0x10);
            c.put32(frame.at(0x124c), 0x43300000);
            c.put32(frame.at(0x1250), count ^ 0x80000000);
            const auto scaled_count = fpu.fsub(c.f64(frame.at(0x124c)), magic);
            c.set_f(1, scaled_count);
            const auto ratio = fpu.fdiv(scaled_count, limit);
            c.put32(frame.at(0x1244), 0x43300000);
            const auto sum = pool_sum();
            const auto reciprocal = std::int32_t(0x80008001u);
            const auto high = [](std::int32_t a, std::int32_t b) { return std::int32_t((std::int64_t(a) * b) >> 32); };
            auto quotient = srawi(c, high(reciprocal, std::int32_t(sum)) + std::int32_t(sum), 15);
            const auto quotient_sign = std::uint32_t(quotient) >> 31;
            quotient += quotient_sign;
            const auto residue = sum - std::uint32_t(quotient) * 0xffff;
            c.put32(frame.at(0x1248), residue ^ 0x80000000);
            const auto times_ten = std::int32_t(count * 10);
            auto next = srawi(c, high(reciprocal, times_ten) + times_ten, 15);
            const auto residue_double = fpu.fsub(c.f64(frame.at(0x1244)), magic);
            c.set_f(1, magic);
            const auto product = fpu.fmul(ratio, residue_double);
            const auto next_before = next;
            next += std::uint32_t(next) >> 31;
            c.set_f(2, ratio);
            c.put64(frame.at(0x123c), fpu.fctiwz(0, product));
            c.set(15, c.u32(frame.at(0x1240)));
            // What the original's registers hold at the call (range_sample leaves most of them as it finds them).
            c.set({{4, 0x80008001}, {5, quotient_sign}, {6, 0xffff}, {7, 0x80008001}, {8, sum}, {0, std::uint32_t(next_before)}});
            guest::enter(c, 0x100ab0a4, {std::uint32_t(next)});
            const auto drawn = range_sample(c, next);
            auto tenth = srawi(c, high(0x66666667, std::int32_t(drawn + 5)), 2);
            tenth += std::uint32_t(tenth) >> 31;
            c.set(15, c.r(15) + std::uint32_t(tenth));
            if (compare(c, std::int32_t(c.r(15)), std::int32_t(count - 1)) > 0) c.set(15, count - 1);
        }
    }
    c.put32(person + 0xc, c.u32(c.toc(cells::rows_128_table)) + c.r(15) * 0x128);
}

// A person's name into a stack buffer through person_name(person, buffer, limit, 1) at `bl`.
void Loader::person_name_at(std::uint32_t person, std::uint32_t buffer, std::uint32_t limit, std::uint32_t bl) {
    c.set({{3, person}, {4, buffer}, {5, limit}, {6, 1}});
    guest::enter(c, bl + 4, {person, buffer, limit, 1});
    person_name(c, person, buffer, limit, 1);
}

// The report line through the formatter: (text buffer, the pool's format, the name buffer).
void Loader::report_line(std::uint32_t bl, std::uint32_t format, std::uint32_t buffer) {
    c.set({{3, c.toc(toc::text_buffer)}, {4, pool() + format}, {5, buffer}});
    call(bl, callee::format_text);
}

// The role byte at `offset` names a table that must hold a link the person lacks: the person's name is reported
// and the role cleared (to 11 for the second kind). Two switches, one per byte: values 1-10 need the staff link
// (+0x1c), 11 the player link (+0x14), 12, 13 and 15 both; 0, 14 and 16 and every other value nothing.
void Loader::check_role(std::uint32_t person, std::uint32_t offset, bool second) {
    // The call sites of the six bodies: name call, text call.
    struct Body {
        std::uint32_t name, text, format;
    };
    constexpr std::array<std::array<Body, 4>, 2> bodies{{
        {{{0x100ab18c, 0x100ab19c, 0x142}, {0x100ab1c8, 0x100ab1d8, 0x15c}, {0x100ab250, 0x100ab260, 0x172},
          {0x100ab288, 0x100ab298, 0x192}}},
        {{{0x100ab32c, 0x100ab33c, 0x142}, {0x100ab368, 0x100ab378, 0x15c}, {0x100ab3f0, 0x100ab400, 0x172},
          {0x100ab428, 0x100ab438, 0x192}}},
    }};
    const auto& b = bodies[second];
    const auto buffer = frame.at(local::long_name_buffer);
    const auto value = ext(c.u8(person + offset));
    c.set(0, value);
    if (compare_logical(c, value, 0x10) > 0) return;
    const auto target = c.u32(c.toc(second ? toc::person_jumps_second : toc::person_jumps) + value * 4);
    c.set({{0, value << 2}, {3, target}});
    c.s.wr<4>(guest::reg::ctr, target);
    const auto report = [&](const Body& body) {
        person_name_at(person, buffer, 0x1f4, body.name);
        report_line(body.text, body.format, buffer);
    };
    const auto field = [&](std::uint32_t at) { return c.u32(person + at); };
    const auto test_zero = [&](std::uint32_t at) {
        c.set(0, field(at));
        return compare_logical(c, field(at)) == 0;
    };
    if (value >= 1 && value <= 10) {
        if (!test_zero(0x1c)) return;
        report(b[0]);
        c.put8(person + offset, 0);
    } else if (value == 11) {
        if (!test_zero(0x14)) return;
        report(b[1]);
        c.put8(person + offset, 0);
        c.set(0, field(0x24));
        if (compare_logical(c, field(0x24)) == 0) return;
        // The detail record's 50 slots that name this person lose it.
        const auto detail = field(0x24);
        for (std::uint32_t slot = 0; compare(c, std::int32_t(slot), 0x32) < 0; ++slot) {
            c.set(5, slot);
            const auto word = c.u32(detail + slot * 4 + 0x98);
            compare_logical(c, person, word);
            if (person == word) {
                c.set(4, 0);
                c.put32(detail + slot * 4 + 0x98, 0);
            }
        }
    } else if (value == 12 || value == 13 || value == 15) {
        if (test_zero(0x1c)) {
            report(b[2]);
            c.put8(person + offset, 0xb);
        }
        if (test_zero(0x14)) {
            report(b[3]);
            c.put8(person + offset, 0);
        }
    }
}

// One person (r14).
void Loader::fix_person(std::uint32_t person) {
    c.set(14, person);
    const auto field = [&](std::uint32_t at) { return c.u32(person + at); };
    const auto byte = [&](std::uint32_t at) { return std::uint32_t(c.u8(person + at)); };
    const auto zero = [&](std::uint32_t at) {  // lwz; cmplwi 0
        c.set(0, field(at));
        return compare_logical(c, field(at)) == 0;
    };
    const auto zero_byte = [&](std::uint32_t at) {  // lbz; cmplwi 0
        c.set(0, byte(at));
        return compare_logical(c, byte(at)) == 0;
    };
    const auto buffer = frame.at(local::long_name_buffer);

    // People of the last sixteen indexes and those of kind 6 are not repaired.
    const auto last = c.u32(c.r(29));
    if (compare(c, std::int32_t(c.u32(person)), std::int32_t(last - 0x10)) >= 0) return;
    c.set(0, byte(0x6b));
    if (compare(c, std::int32_t(byte(0x6b)), 6) == 0) return;

    // A detail record that has no club of its own: the person leaves it, with a report.
    if (!zero(0x24)) {
        const auto owner = c.u32(field(0x24) + 0x2c);
        c.set(0, owner);
        if (compare_logical(c, owner) == 0) {
            person_name_at(person, frame.at(local::name_buffer), 0xc8, 0x100aad10);
            const auto club = field(0xc);
            c.set({{0, club}, {4, pool() + 0xfa}, {3, c.toc(toc::text_buffer)}, {5, frame.at(local::name_buffer)}});
            const bool has_club = compare_logical(c, club) != 0;
            c.set(6, has_club ? field(0xc) + 0xfb : pool() + pool::empty);
            c.set(7, field(0x24) + 0x1ee);
            call(0x100aad44, callee::format_text);
            guest::enter(c, 0x100aad60, {field(0x24), person, 0, 1, 0});
            remove_from_record(c, field(0x24), person, 0, 1, 0);
            c.set(0, 0);
            c.put32(person + 0x24, 0);
        }
    }
    // The flag bit 1 needs a detail record that passes the club test; failing it the bit turns into bit 0.
    c.set(0, byte(0x6a) & 2);
    if (compare(c, std::int32_t(byte(0x6a) & 2)) != 0 && !zero(0x24)) {
        guest::enter(c, 0x100aad88, {field(0x24)});
        club_less_candidate(c, field(0x24));
        const auto answer = ext(c.r(3));
        c.set(0, answer);
        if (compare(c, std::int32_t(answer)) == 0) {
            c.put8(person + 0x6a, byte(0x6a) & ~2u);
            c.put8(person + 0x6a, byte(0x6a) | 1u);
        }
    }
    // A second byte (+0x5f) without its first (+0x5e) is cleared.
    if (!zero_byte(0x5f) && zero_byte(0x5e)) c.put8(person + 0x5f, 0);
    // No detail record, no staff record, a player link: the date is raised to the new one when it is later.
    if (zero(0x24) && zero(0x20) && !zero(0x1c)) {
        c.set({{3, frame.at(0x19c)}, {4, 1}, {5, 0}, {6, 0x7ce}, {7, 0xffffffff}});
        call(0x100aae00, callee::date_make);
        const auto first = c.u32(c.r(3)), second = c.u32(c.r(3) + 4);
        c.put32(frame.at(0x18c), first);
        c.put32(frame.at(0x190), second);
        const auto ours = c.u16(frame.at(0x18e)), theirs = c.u16(person + 0x4e);
        c.set({{5, first}, {4, second}, {3, ours}, {0, theirs}});
        std::uint32_t later;
        if (compare_logical(c, ours, theirs) > 0) {
            later = 1;
        } else if (compare_logical(c, c.u16(frame.at(0x18e)), c.u16(person + 0x4e)) < 0) {
            later = 0;
        } else {
            const auto x = std::int32_t(std::int16_t(c.u16(frame.at(0x18c)))), y = std::int32_t(std::int16_t(c.u16(person + 0x4c)));
            const auto z = x ^ y;
            c.set(4, std::uint32_t(x));
            c.set(3, std::uint32_t(z >> 1));
            later = (std::uint32_t(srawi(c, z, 1) - (z & x)) >> 31) & 1;
        }
        const auto flagged = ext(later);
        c.set(0, flagged);
        if (compare(c, std::int32_t(flagged)) != 0) {
            c.set({{3, frame.at(0x194)}, {4, 1}, {5, 8}, {6, 0x7cd}, {7, 0xffffffff}});
            call(0x100aae80, callee::date_make);
            c.put16(person + 0x4e, c.u16(frame.at(0x196)));
            c.put16(person + 0x4c, c.u16(frame.at(0x194)));
            c.put8(person + 0x50, c.u8(frame.at(0x198)));
        }
    }
    // Roles of 7 become 6.
    c.set(0, byte(0x62));
    if (compare(c, std::int32_t(byte(0x62)), 7) == 0) c.put8(person + 0x62, 6);
    c.set(0, byte(0x60));
    if (compare(c, std::int32_t(byte(0x60)), 7) == 0) c.put8(person + 0x60, 6);
    // Role 5 without a player link: the staff link goes too.
    c.set(0, byte(0x60));
    if (compare(c, std::int32_t(byte(0x60)), 5) == 0 && zero(0x1c)) {
        if (!zero(0x20)) c.put32(field(0x20) + 0x90, 0);
        c.set({{3, 0}, {0, 0}});
        c.put8(person + 0x60, 0);
        c.put32(person + 0x20, 0);
    }
    if (!zero(0x20)) {
        const auto role = ext(byte(0x60));
        c.set(0, role);
        if (compare(c, std::int32_t(role)) == 0) {
            c.set(0, 0);
            c.put32(person + 0x20, 0);
        }
    }
    // A person without a club record takes the detail's or the staff record's, or a drawn one, and reports.
    if (zero(0xc)) {
        if (!zero(0x24)) {
            c.set(0, c.u32(field(0x24) + 0x2c));
            c.put32(person + 0xc, c.r(0));
        } else if (!zero(0x20)) {
            c.set(0, c.u32(field(0x20) + 0x2c));
            c.put32(person + 0xc, c.r(0));
        } else {
            draw_club_record(person);
        }
        person_name_at(person, buffer, 0x1f4, 0x100ab134);
        c.set({{6, c.u32(person)}, {4, pool() + 0x125}, {3, c.toc(toc::text_buffer)}, {5, buffer}});
        call(0x100ab148, callee::format_text);
    }
    check_role(person, 0x62, false);
    // The three unmarked-record links.
    for (const auto [offset, bl] : {std::pair{0x28u, 0x100ab2b4u}, {0x2cu, 0x100ab2ccu}, {0x30u, 0x100ab2e4u}}) {
        if (zero(offset)) {
            c.set(3, 0);
            guest::enter(c, bl + 4, {0});
            c.put32(person + offset, pick_unmarked_record(c));
        }
    }
    check_role(person, 0x60, true);
    // An unpacked load drops the staff link of a person who has both links unless the role is 5.
    const auto mode = ext(c.r(16));
    c.set(0, mode);
    if (compare(c, std::int32_t(mode)) == 0 && !zero(0x1c) && !zero(0x20)) {
        c.set(0, byte(0x60));
        if (compare(c, std::int32_t(byte(0x60)), 5) != 0) {
            guest::enter(c, 0x100ab488, {field(0x20), person, 0, 1, 0});
            remove_from_record(c, field(0x20), person, 0, 1, 0);
            c.set({{3, 0}, {0, 0}});
            c.put8(person + 0x60, 0);
            c.put32(person + 0x20, 0);
        }
    }
    // Roles 1-13 and 15 without a club record lose their staff link and role, with a report.
    const auto role = ext(byte(0x60));
    c.set(0, role);
    const auto signed_role = std::int32_t(role);
    compare(c, signed_role, 0xe);
    if ((signed_role >= 1 && signed_role <= 13) || signed_role == 15) {
        if (zero(0xc)) {
            c.set({{3, 0}, {0, 0}});
            c.put32(person + 0x20, 0);
            c.put8(person + 0x60, 0);
            person_name_at(person, buffer, 0x1f4, 0x100ab4f4);
            report_line(0x100ab504, 0x1ae, buffer);
        } else if (zero(0x20)) {
            c.set({{3, 0}, {0, 0}});
            c.put32(person + 0x20, 0);
            c.put8(person + 0x60, 0);
        }
    }
}

// 0x100aacb8-0x100ab5b0: every person in turn, then (carved tables only) the club table's halved ratings.
void Loader::fix_people() {
    c.set(14, c.u32(c.r(26)));
    c.set(20, 0);
    while (compare(c, std::int32_t(c.r(20)), std::int32_t(c.u32(c.r(29)))) < 0) {
        fix_person(c.r(14));
        c.set(20, c.r(20) + 1);
        c.set(14, c.r(14) + 0x70);
    }
    const auto extended = ext(c.u8(frame.at(local::extended)));
    c.set(0, extended);
    if (compare(c, std::int32_t(extended)) != 0) return;
    const auto clubs = c.u32(c.toc(cells::clubs_count));
    c.set(13, clubs - c.u32(c.toc(cells::rows_128_count)));
    while (compare(c, std::int32_t(c.r(13)), std::int32_t(c.u32(c.toc(cells::clubs_count)))) < 0) {
        const auto index = c.r(13);
        const auto club = c.u32(c.toc(cells::clubs_table)) + index * 0x248;
        const auto record = c.u32(club + 0x2c);
        c.set(3, record);
        guest::enter(c, 0x100ab578, {record});
        const auto linked = club_at_doubled_bias(c, record);
        const auto rating = std::int32_t(std::int16_t(c.u16(linked + 0x1ec)));
        c.set({{5, std::uint32_t(rating)}, {13, index + 1}});
        const auto half = srawi(c, rating + std::int32_t(std::uint32_t(rating) >> 31), 1);
        const auto table = c.u32(c.toc(cells::clubs_table));
        c.put16(table + index * 0x248 + 0x1ec, std::uint32_t(half));
        // r3 the club's address, r4 the table, r0 its offset, r5 the halved rating (read by the next call).
        c.set({{3, table + index * 0x248}, {4, table}, {0, index * 0x248}, {5, std::uint32_t(half)}});
    }
}
}
