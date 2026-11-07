// DATABASE.CPP's text block, part 5: the label of a player's role flags (0x100d6af0).
#include "database_text.hpp"
#include "database_text_detail.hpp"
#include <array>
#include <bit>
#include <utility>

namespace cm::aot::database {
using namespace text;

namespace {
constexpr std::uint32_t format_entry = 0x10727890, text_kind = 0x10727880;
}

// 0x100d6af0: a label for a role described by flags: `index` (r3, -1 or below: none) selects a
// record of the table whose base word TOC -0x7be0 points at (0x70-byte records); `roles` (r4),
// `flags` (r5) are the two flag words; the label goes through the formatter into `buffer` (r6).
// `limit` (r7) must be at least 30. Answers `kind_default` (r8) unless a text below sets 1, 3 or 4
// (when the kind byte from 0x10727880 is 4; 1 first when it is 5), or 0 for the assertion and
// for the one text that returns early.
std::uint32_t role_label(Cpu& c, std::uint32_t index, std::uint32_t roles, std::uint32_t flags, std::uint32_t buffer,
                         std::uint32_t limit, std::uint32_t kind_default) {
    constexpr std::uint32_t entry = 0x100d6af0;
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, 0x90, {.first_gpr = 23});
    const auto message = c.toc(-0x7ba8), pool = c.toc(pool_slot), table_word = c.toc(-0x7be0);
    c.set(23, message);
    c.set(24, index);
    c.set(30, pool);
    c.set(25, roles);
    c.set(31, table_word);
    c.set(26, flags);
    c.set(27, buffer);
    c.set(29, limit);
    c.set(28, kind_default);
    c.call(text_kind, entry + 0x38);
    compare(c, std::int32_t(limit), 0x1e);
    const auto kind = c.r(3);
    c.set(29, kind);
    if (std::int32_t(limit) < 0x1e) {
        assertion_failure(c, frame, pool, 0x100d6b48, 0x40, 0x38, 0x5361, true);
        return 0;
    }
    // Register-faithful pieces of the original's code.
    const auto record_bit = [&](std::uint32_t value, std::uint32_t mask) {  // rlwinm. r0,rX,0,b,b
        c.set(0, value & mask);
        return compare(c, std::int32_t(value & mask)) != 0;
    };
    const auto kind_is = [&](std::int32_t what) {  // extsb r0,r29; cmpwi r0,what
        c.set(0, std::uint32_t(extsb(c.r(29))));
        return compare(c, extsb(c.r(29)), what) == 0;
    };
    // A text through the formatter; then, when the kind is 4, r28 takes `value`.
    const auto text = [&](std::uint32_t offset, std::uint32_t back, std::uint32_t value) {
        c.set({{3, buffer}, {4, pool + offset}});
        c.call_in_place(format_entry, back);
        if (kind_is(4)) c.set(28, value);
    };
    const auto finish = [&] {
        c.set(3, c.r(28));
        return c.r(28);
    };
    // The tests of the flag word against a mask, in the original's several instruction forms.
    // Each sets r3/r0 and the condition register as its form does and answers "equal".
    const auto andis_equal = [&](std::uint32_t mask) {  // andis. r3,r26,H; subis r0,r3,H; cmplwi r0,0
        const auto v = flags & mask;
        c.set(3, v);
        compare(c, std::int32_t(v));
        c.set(0, v - mask);
        return compare_logical(c, c.r(0)) == 0;
    };
    const auto rlwinm_equal = [&](std::uint32_t mask) {  // rlwinm r3,r26,..; subis r0,r3,H; cmplwi r0,0
        const auto v = flags & mask;
        c.set(3, v);
        c.set(0, v - mask);
        return compare_logical(c, c.r(0)) == 0;
    };
    const auto lis_equal = [&](std::uint32_t mask, std::uint32_t high) {  // lis/addi; and; subis; cmplwi r0,low
        c.set(3, mask);
        const auto v = flags & mask;
        c.set(3, v);
        c.set(0, v - high);
        return compare_logical(c, c.r(0), mask - high) == 0;
    };
    const auto signed_equal = [&](std::uint32_t mask) {  // rlwinm r0,r26,..; cmpwi r0,mask
        c.set(0, flags & mask);
        return compare(c, std::int32_t(flags & mask), std::int32_t(mask)) == 0;
    };
    // The division by 3 of the original (mulhw by 0x55555556, then the sign): leaves r3 the product's
    // high word and r0 three times the quotient.
    const auto times_three = [&](std::uint32_t value) {
        const auto high = std::uint32_t((std::int64_t(0x55555556) * std::int64_t(std::int32_t(value))) >> 32);
        c.set(3, high);
        c.set(0, (high >> 31) & 1);
        c.set(0, high + c.r(0));
        c.set(0, c.r(0) * 3);
        return c.r(0);
    };

    if (kind_is(5)) c.set(28, 1);
    if (record_bit(flags, 0x40)) {
        if (andis_equal(0x2c0000)) { text(0x22c2, 0x100d6bc4, 1); return finish(); }
        if (andis_equal(0x240000)) {
            const auto sum = flags + index;
            c.set(4, sum);
            const auto remainder = sum - times_three(sum);
            c.set(0, remainder);
            if (compare(c, std::int32_t(remainder), 1) == 0) text(0x22d2, 0x100d6c1c, 1);
            else text(0x22df, 0x100d6c3c, 1);
            return finish();
        }
        if (rlwinm_equal(0xc0000)) { text(0x22f0, 0x100d6c6c, 4); return finish(); }
        if (rlwinm_equal(0x40000)) {
            const auto sum = flags + index;
            c.set(3, sum);
            const auto sign = (sum >> 31) & 1;
            const auto top = (sum & 3) << 30;
            c.set(0, top);
            c.set(3, sign);
            c.set(0, top - sign);
            c.set(0, std::rotl(c.r(0), 2));
            c.set(0, c.r(0) + sign);
            if (compare(c, std::int32_t(c.r(0)), 1) == 0) text(0x22fc, 0x100d6cbc, 1);
            else text(0x2303, 0x100d6cdc, 3);
            return finish();
        }
        if (lis_equal(0x102000, 0x100000)) { text(0x2314, 0x100d6d14, 4); return finish(); }
        if (lis_equal(0x200800, 0x200000)) { text(0x2324, 0x100d6d4c, 4); return finish(); }
        if (signed_equal(0x800)) { text(0x2337, 0x100d6d78, 4); return finish(); }
        {  // andi. r0,r26,0x1200; cmpwi r0,0x1200
            c.set(0, flags & 0x1200);
            compare(c, std::int32_t(flags & 0x1200));
            if (compare(c, std::int32_t(flags & 0x1200), 0x1200) == 0) { text(0x2348, 0x100d6da4, 4); return finish(); }
        }
        if (rlwinm_equal(0x10000)) { text(0x2359, 0x100d6dd4, 4); return finish(); }
        if (signed_equal(0x1000)) text(0x236a, 0x100d6e00, 1);
        return finish();
    }
    if (record_bit(roles, 0x10)) {
        if (rlwinm_equal(0x40000)) {
            if (lis_equal(0x800200, 0x800000)) {
                c.set({{3, buffer}, {4, pool + 0x237b}});
                c.call_in_place(format_entry, 0x100d6e50);
                kind_is(4);
            }
            c.set({{3, buffer}, {4, pool + 0x2394}});
            c.call_in_place(format_entry, 0x100d6e64);
            kind_is(4);
            c.set(3, 0);
            return 0;
        }
        if (record_bit(flags, 0x200) && !record_bit(roles, 0x800) && !record_bit(roles, 0x80)) { text(0x23ac, 0x100d6e98, 1); return finish(); }
        if (record_bit(flags, 0x800000)) { text(0x23b6, 0x100d6ec0, 1); return finish(); }
        if (lis_equal(0x102100, 0x100000)) { text(0x23c2, 0x100d6ef8, 1); return finish(); }
        if (signed_equal(0x2000)) { text(0x23d5, 0x100d6f24, 1); return finish(); }
        {  // rlwinm r3,r26,..; addis r0,r3,0; cmplwi r0,0x8000
            c.set(3, flags & 0x8000);
            c.set(0, c.r(3));
            if (compare_logical(c, c.r(0), 0x8000) == 0) { text(0x23ed, 0x100d6f54, 1); return finish(); }
        }
        if (rlwinm_equal(0x200000)) text(0x2401, 0x100d6f84, 1);
        return finish();
    }
    if (record_bit(flags, 0x2) && !record_bit(flags, 0x4)) {
        if (rlwinm_equal(0x200000)) { text(0x2412, 0x100d6fc4, 1); return finish(); }
        if (rlwinm_equal(0x10000)) text(0x2422, 0x100d6ff4, 1);
        return finish();
    }
    if (record_bit(flags, 0x4)) {
        if (rlwinm_equal(0x200000)) { text(0x2435, 0x100d702c, 1); return finish(); }
        {  // andi. r0,r26,0x1200; cmpwi r0,0x1200
            c.set(0, flags & 0x1200);
            compare(c, std::int32_t(flags & 0x1200));
            if (compare(c, std::int32_t(flags & 0x1200), 0x1200) == 0) text(0x2444, 0x100d7058, 1);
        }
        return finish();
    }
    // No flag of the two words: the role record's table row decides.
    const auto row_rating = [&](std::uint32_t adjustment) {  // the common prefix of both tests
        const auto base = c.u32(table_word);
        c.set(5, base);
        c.set(3, 0x55555556);
        const auto three = times_three(index);
        const auto row = index * 0x70;
        c.set(4, row);
        c.set(3, row + 0x5c);
        const auto rating = extsb(std::uint32_t(c.i16(base + c.r(3))));
        c.set(4, std::uint32_t(rating));
        c.set(3, index - three);
        c.set(0, c.r(3) + adjustment);
        return compare(c, rating, std::int32_t(c.r(0)));
    };
    if (compare(c, std::int32_t(index), -1) > 0 && !record_bit(flags, 0x1)) {
        if (row_rating(0x21) > 0) { text(0x2456, 0x100d70c4, 4); return finish(); }
    }
    if (compare(c, std::int32_t(index), -1) > 0) {
        const auto sum = index + flags;
        c.set(4, sum);
        const auto remainder = sum - times_three(sum);
        c.set(0, remainder);
        if (compare(c, std::int32_t(remainder)) == 0 && !record_bit(flags, 0x1)) {
            if (row_rating(0x12) < 0) {
                const auto row = index * 0x70;
                c.set(0, row);
                const auto base = c.u32(table_word);
                c.set(3, base + row);
                c.set(0, c.u8(base + row + 0x5e));
                if (compare_logical(c, c.r(0)) == 0) text(0x245e, 0x100d716c, 4);
            }
        }
    }
    return finish();
}

// 0x100db410: a label for a flag word (bit sets such as 0x260, 0x88, 0x204 name a role), through
// the formatter into `buffer` (null answers 0); `variant` (r5, a byte) picks the second text of
// each pair. Answers 0, 1, 2, 4 or 5 from the kind byte 0x10727880 reports (2, 1 or 1 for the
// kinds 2, 5 and 4, or 5, 1 and 4 for the first five labels except the third).
std::uint32_t flags_label(Cpu& c, std::uint32_t flags, std::uint32_t buffer, std::uint32_t variant) {
    constexpr std::uint32_t entry = 0x100db410, format_entry = 0x10727890, text_kind = 0x10727880;
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, 0x50, {.first_gpr = 26});
    compare(c, std::int32_t(buffer));
    const auto pool = c.toc(pool_slot);
    c.set(27, buffer);
    c.set(31, pool);
    c.set(26, flags);
    c.set(28, variant);
    c.set(29, 0);
    if (buffer == 0) return guest::result(c, 0);
    c.call(text_kind, entry + 0x34);
    const auto kind = c.r(3);
    c.set(30, kind);
    c.set(0, flags & 1);
    const auto result = [&] {
        c.set(3, c.r(29));
        return c.r(29);
    };
    // andi. r0,r26,m; cmpwi r0,m
    const auto andi_equal = [&](std::uint32_t mask) {
        c.set(0, flags & mask);
        compare(c, std::int32_t(flags & mask));
        return compare(c, std::int32_t(flags & mask), std::int32_t(mask)) == 0;
    };
    // rlwinm r0,r26,..; cmpwi r0,value
    const auto field_equal = [&](std::uint32_t mask, std::uint32_t value) {
        c.set(0, flags & mask);
        return compare(c, std::int32_t(flags & mask), std::int32_t(value)) == 0;
    };
    struct Text {  // a pool offset and the address after the original's call that formats it
        std::uint32_t offset, back;
    };
    // The label: extsb. r0,r28 picks the text of the pair; the kind then sets r29.
    const auto label = [&](const Text& first, const Text& second, int kind2, int kind5, int kind4, bool post) {
        c.set(0, std::uint32_t(extsb(variant)));
        const auto& chosen = compare(c, extsb(variant)) != 0 ? second : first;
        c.set({{3, buffer}, {4, pool + chosen.offset}, {5, pool + 0xd0}});
        c.call_in_place(format_entry, chosen.back);
        if (!post) return;
        const auto kind_equals = [&](std::int32_t what) {
            c.set(0, std::uint32_t(extsb(kind)));
            return compare(c, extsb(kind), what) == 0;
        };
        if (kind_equals(2)) c.set(29, std::uint32_t(kind2));
        else if (kind_equals(5)) c.set(29, std::uint32_t(kind5));
        else if (kind_equals(4)) c.set(29, std::uint32_t(kind4));
    };
    if (compare(c, std::int32_t(flags & 1), 1) == 0) { label({0x2465, 0x100db46c}, {0x2491, 0x100db480}, 2, 1, 1, true); return result(); }
    if (andi_equal(0x260) || andi_equal(0x160) || andi_equal(0x460)) { label({0x24bd, 0x100db4f8}, {0x24ed, 0x100db50c}, 5, 1, 4, true); return result(); }
    if (field_equal(0x40, 0x40) && field_equal(0x700, 0x700)) { label({0x251d, 0x100db578}, {0x2546, 0x100db58c}, 5, 1, 4, true); return result(); }
    if (andi_equal(0x88) || andi_equal(0x808)) { label({0x256f, 0x100db5f8}, {0x259a, 0x100db60c}, 5, 1, 1, true); return result(); }
    if (andi_equal(0x90) || andi_equal(0xa0)) { label({0x25c5, 0x100db678}, {0x25f2, 0x100db68c}, 5, 1, 4, true); return result(); }
    if (andi_equal(0x810) || andi_equal(0x820)) { label({0x261f, 0x100db6f8}, {0x264d, 0x100db70c}, 5, 1, 4, true); return result(); }
    const auto hundreds = [&] { return field_equal(0x200, 0x200) || field_equal(0x100, 0x100) || field_equal(0x400, 0x400); };
    if (hundreds() && (field_equal(0x10, 0x10) || field_equal(0x8, 0x8))) { label({0x267b, 0x100db79c}, {0x26af, 0x100db7b0}, 2, 1, 1, true); return result(); }
    if (hundreds() && field_equal(0x20, 0x20)) { label({0x26e3, 0x100db834}, {0x2719, 0x100db848}, 2, 1, 1, true); return result(); }
    if (andi_equal(0x204) || andi_equal(0x104) || andi_equal(0x404)) { label({0x274f, 0x100db8c0}, {0x277c, 0x100db8d4}, 2, 1, 1, true); return result(); }
    if (andi_equal(0x84)) { label({0x27a9, 0x100db934}, {0x27d4, 0x100db948}, 2, 1, 1, true); return result(); }
    if (andi_equal(0x804)) { label({0x27ff, 0x100db9a8}, {0x282b, 0x100db9bc}, 2, 1, 1, true); return result(); }
    if (field_equal(0x2, 0x2)) { label({0x2857, 0x100dba1c}, {0x2880, 0x100dba30}, 2, 1, 1, true); return result(); }
    if (compare(c, std::int32_t(flags)) == 0) { label({0x28a9, 0x100dba8c}, {0x28d5, 0x100dbaa0}, 0, 0, 0, false); return result(); }
    label({0x2901, 0x100dbabc}, {0x292b, 0x100dbad0}, 2, 1, 1, true);
    return result();
}
}
