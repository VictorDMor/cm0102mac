// DATABASE.CPP's text block, part 2: the sort comparators over records (database_text.hpp).
#include "database_text.hpp"
#include "database_text_detail.hpp"

namespace cm::aot::database {
using namespace text;

namespace {
constexpr std::uint32_t buffer_a = 0x808, buffer_b = 0x38;  // the two keys, in a 0xfe0-byte frame

// 0x100cd2a0 and 0x100cd6b0: the keys are the records' names in one layout.
std::int32_t compare_formatted(Cpu& c, std::uint32_t entry, std::uint32_t a, std::uint32_t b, std::uint32_t layout) {
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, 0xfe0, {.first_gpr = 31});
    c.set(0, 0);
    const auto first = c.u32(a);
    c.set(3, first);
    const auto second = c.u32(b);
    c.set(31, second);
    compare_logical(c, first);
    c.put8(frame.at(buffer_a), 0);
    if (first != 0) {
        guest::enter(c, entry + 0x38, {first, frame.at(buffer_a), 0x7d0, layout});
        person_name(c, first, frame.at(buffer_a), 0x7d0, layout);
    }
    c.set(0, 0);
    compare_logical(c, second);
    c.put8(frame.at(buffer_b), 0);
    if (second != 0) {
        guest::enter(c, entry + 0x5c, {second, frame.at(buffer_b), 0x7d0, layout});
        person_name(c, second, frame.at(buffer_b), 0x7d0, layout);
    }
    c.set({{3, frame.at(buffer_a)}, {4, frame.at(buffer_b)}, {5, 1}, {6, 0}});
    c.call(compare_text, entry + 0x70);
    return std::int32_t(c.r(3));
}

// 0x100cd410 and 0x100cd4a0: the keys are the strings stored inline in the records at `offset`.
std::int32_t compare_inline_text(Cpu& c, std::uint32_t entry, std::uint32_t a, std::uint32_t b, std::uint32_t offset) {
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, 0xfe0, {.first_gpr = 31});
    c.set(0, 0);
    const auto first = c.u32(a);
    c.set(5, first);
    const auto second = c.u32(b);
    c.set(31, second);
    compare_logical(c, first);
    c.put8(frame.at(buffer_a), 0);
    if (first != 0) {
        const auto text = addic(c, first, offset);
        c.set(0, text);
        if (compare(c, std::int32_t(text)) != 0) {
            c.set({{3, frame.at(buffer_a)}, {4, text}});
            c.call(strcpy_entry, entry + 0x3c);
        }
    }
    c.set(0, 0);
    compare_logical(c, second);
    c.put8(frame.at(buffer_b), 0);
    if (second != 0) {
        const auto text = addic(c, second, offset);
        c.set(0, text);
        if (compare(c, std::int32_t(text)) != 0) {
            c.set({{3, frame.at(buffer_b)}, {4, text}});
            c.call(strcpy_entry, entry + 0x60);
        }
    }
    c.set({{3, frame.at(buffer_a)}, {4, frame.at(buffer_b)}, {5, 1}, {6, 0}});
    c.call(compare_text, entry + 0x74);
    return std::int32_t(c.r(3));
}
}

std::int32_t compare_by_surname_format(Cpu& c, std::uint32_t a, std::uint32_t b) {
    return compare_formatted(c, 0x100cd2a0, a, b, 4);
}
std::int32_t compare_by_name_format(Cpu& c, std::uint32_t a, std::uint32_t b) {
    return compare_formatted(c, 0x100cd6b0, a, b, 1);
}
std::int32_t compare_by_text_221(Cpu& c, std::uint32_t a, std::uint32_t b) {
    return compare_inline_text(c, 0x100cd410, a, b, 0x221);
}
std::int32_t compare_by_text_e1(Cpu& c, std::uint32_t a, std::uint32_t b) {
    return compare_inline_text(c, 0x100cd4a0, a, b, 0xe1);
}

// 0x100d5ce0: a null first record sorts before everything (1), a null second after (-1);
// otherwise the keys are the records' first bytes through 0x10167590 (with the byte at
// TOC -0x39ee) into 0xc8-byte buffers.
std::int32_t compare_by_formatted_id(Cpu& c, std::uint32_t a, std::uint32_t b) {
    constexpr std::uint32_t entry = 0x100d5ce0;
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, 0x1d0, {.first_gpr = 31});
    compare_logical(c, a);
    c.set(31, b);
    if (a == 0) return std::int32_t(guest::result(c, 1));
    if (compare_logical(c, b) == 0) return std::int32_t(guest::result(c, 0xffffffff));
    const auto key = [&](std::uint32_t record, std::uint32_t buffer, std::uint32_t back) {
        c.set(3, c.u8(record));
        c.set(4, buffer);
        c.set(6, c.u8(c.r(2) - 0x39ee));
        c.set(5, 0xc8);
        c.call(0x10167590, back);
    };
    key(a, frame.at(0x100), entry + 0x48);
    key(b, frame.at(0x38), entry + 0x5c);
    c.set({{3, frame.at(0x100)}, {4, frame.at(0x38)}, {5, 1}, {6, 0}});
    c.call(compare_text, entry + 0x70);
    return std::int32_t(c.r(3));
}

// 0x100cd530: sorts by a record's date fields (a 16-bit year at +0x34, a 16-bit and a byte
// at +0x36 and +0x38: inferred) copied into two 5-byte stack structures built by
// 0x10715880; ties go to the name comparator 0x100cd2a0. The two compares of each pass
// are the original's: an unsigned compare of the +0x36 words, then (equal) the sign
// of ((x ^ y) >> 1) - ((x ^ y) & x) for the +0x34 words, which leaves XER[CA].
std::int32_t compare_by_date(Cpu& c, std::uint32_t a, std::uint32_t b) {
    constexpr std::uint32_t entry = 0x100cd530;
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, 0x60, {.first_gpr = 30});
    c.set(31, b);
    c.set(30, a);
    const auto first = frame.at(0x50), second = frame.at(0x48);
    c.set(3, first);
    c.call(0x10715880, entry + 0x24);
    c.set(3, second);
    c.call(0x10715880, entry + 0x2c);
    const auto record_a = c.u32(a), record_b = c.u32(b);
    c.set({{3, record_a}, {4, record_b}});
    const auto copy_fields = [&](std::uint32_t record, std::uint32_t at) {
        if (compare_logical(c, record) == 0) return;
        c.set(0, c.u16(record + 0x36));
        c.put16(at + 2, c.r(0));
        c.set(0, std::uint32_t(c.i16(record + 0x34)));
        c.put16(at, c.r(0));
        c.set(0, c.u8(record + 0x38));
        c.put8(at + 4, c.r(0));
    };
    copy_fields(record_a, first);
    copy_fields(record_b, second);
    // The sign trick on two sign-extended words (r4 and r0 as loaded): r0 = bit 31 of
    // ((x ^ y) >> 1) - ((x ^ y) & r4), then extsb; XER[CA] comes from the shift.
    const auto sign_trick = [&](std::uint32_t r4, std::uint32_t r0) {
        c.set(4, r4);
        c.set(0, r0);
        c.set(0, r4 ^ r0);
        c.set(3, srawi(c, c.r(0), 1));
        c.set(0, c.r(0) & r4);
        c.set(0, c.r(3) - c.r(0));
        c.set(0, (c.r(0) >> 31) & 1);
        c.set(0, std::uint32_t(extsb(c.r(0))));
    };
    const auto word = [&](std::uint32_t at) { return std::uint32_t(c.i16(at)); };
    const auto copy_pair = [&](std::uint32_t copy_at) {  // lwz r0,0x48; lwz r4,0x4c; stw r0,copy; ...
        c.set(0, c.u32(second));
        c.set(4, c.u32(second + 4));
        c.put32(copy_at, c.r(0));
        c.set(0, c.u16(first + 2));
        c.set(3, c.u16(copy_at + 2));
        c.put32(copy_at + 4, c.r(4));
    };
    // First pass: the copy at 0x40 is the second record's. 1 when its +0x36 word is greater.
    copy_pair(frame.at(0x40));
    if (compare_logical(c, c.r(3), c.r(0)) > 0) {
        c.set(0, 1);
    } else {
        c.set({{3, c.u16(frame.at(0x42))}, {0, c.u16(frame.at(0x52))}});
        if (compare_logical(c, c.r(3), c.r(0)) < 0) c.set(0, 0);
        else sign_trick(word(frame.at(0x40)), word(frame.at(0x50)));
    }
    c.set(0, std::uint32_t(extsb(c.r(0))));
    if (compare(c, std::int32_t(c.r(0))) != 0) return std::int32_t(guest::result(c, 0xffffffff));
    // Second pass, with the roles of the comparisons swapped.
    copy_pair(frame.at(0x38));
    if (compare_logical(c, c.r(3), c.r(0)) < 0) {
        c.set(0, 1);
    } else {
        c.set({{3, c.u16(frame.at(0x3a))}, {0, c.u16(frame.at(0x52))}});
        if (compare_logical(c, c.r(3), c.r(0)) > 0) c.set(0, 0);
        else {
            const auto r0 = word(frame.at(0x38));
            sign_trick(word(frame.at(0x50)), r0);
        }
    }
    c.set(0, std::uint32_t(extsb(c.r(0))));
    if (compare(c, std::int32_t(c.r(0))) != 0) return std::int32_t(guest::result(c, 1));
    guest::enter(c, entry + 0x160, {a, b});
    return compare_by_surname_format(c, a, b);
}

// 0x100c4b00
bool plain_file_name(Cpu& c, std::uint32_t name) {
    constexpr std::uint32_t entry = 0x100c4b00;
    c.set(0, c.lr());  // mfspr r0,LR
    guest::Frame frame(c, entry, 0x40, {.first_gpr = 30});
    c.set(30, name);
    if (compare(c, std::int32_t(name)) == 0) return guest::result(c, 1);
    c.set(0, std::uint32_t(extsb(c.u8(name))));
    if (compare(c, std::int32_t(c.r(0))) == 0) return guest::result(c, 1);
    c.set(3, name);
    c.call(strlen_entry, entry + 0x40);
    if (compare_logical(c, c.r(3), 1) == 0) return guest::result(c, 0);
    c.set(31, 0);
    while (true) {
        c.set(3, name);
        c.call(strlen_entry, entry + 0x7c);
        if (compare(c, std::int32_t(c.r(31)), std::int32_t(c.r(3))) >= 0) return guest::result(c, 1);
        c.set(0, std::uint32_t(extsb(c.u8(name + c.r(31)))));
        if (compare(c, std::int32_t(c.r(0)), 0x2e) == 0) return guest::result(c, 0);
        c.set(31, c.r(31) + 1);
    }
}
}
