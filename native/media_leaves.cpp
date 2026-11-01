// The small call-free routines over the press's history lists: a list is
// {rows, ?, count}, each row 24 bytes. They leave r5-r9 as the scans do.
#include "media_leaves.hpp"
#include "guest_values.hpp"

namespace cm::aot::media {
using namespace guest;
namespace {
constexpr std::uint32_t row_size = 0x18;
std::uint32_t row_at(Cpu& c, std::uint32_t list, std::uint32_t index) { return c.u32(list) + index * row_size; }
std::int32_t row_count(Cpu& c, std::uint32_t list) { return c.i32(list + 8); }
// One row's person against the key's, r5 and r6 as the scan leaves them.
bool person_matches(Cpu& c, std::uint32_t list, std::uint32_t key, std::uint32_t index) {
    const auto row = row_at(c, list, index);
    const auto person = c.u32(key);
    leave(c, {{5, row}, {6, person}});
    return compare(c, std::int32_t(person), c.i32(row + 8)) == 0;
}
}

std::uint32_t lists_kind_two(Cpu& c, std::uint32_t list, std::uint32_t key) {
    for (std::uint32_t index = 0;; ++index) {
        c.set(7, index);
        if (compare(c, std::int32_t(index), row_count(c, list)) >= 0) return result(c, 0);
        if (person_matches(c, list, key, index) && compare(c, std::int32_t(c.u8(row_at(c, list, index) + 0x10)), 2) == 0)
            return result(c, 1);
    }
}

std::uint32_t lists_person(Cpu& c, std::uint32_t list, std::uint32_t key) {
    for (std::uint32_t index = 0;; ++index) {
        c.set(7, index);
        if (compare(c, std::int32_t(index), row_count(c, list)) >= 0) return result(c, 0);
        if (person_matches(c, list, key, index)) return result(c, 1);
    }
}

// The row's grade when it is one of the table's bytes (ended by -1); 0 when the
// person has none, including a null person or one without a club at +0x24.
std::uint32_t listed_grade(Cpu& c, std::uint32_t list, std::uint32_t person) {
    const auto table = c.toc(-0x4218);
    c.set(7, table);
    if (compare_logical(c, person) == 0 || compare_logical(c, c.u32(person + 0x24)) == 0) return result(c, 0);
    for (std::uint32_t index = 0;; ++index) {
        c.set(8, index);
        if (compare(c, std::int32_t(index), row_count(c, list)) >= 0) return result(c, 0);
        if (!person_matches(c, list, person, index)) continue;
        const auto grade = std::int32_t(c.i16(row_at(c, list, index) + 0x12));
        for (std::uint32_t at = 0;;) {
            c.set(9, at);
            const auto byte = std::int8_t(c.u8(table + at));
            if (compare(c, byte, -1) == 0) break;
            leave(c, {{5, extended(byte)}, {6, c.u8(table + at)}, {9, at + 1}});
            ++at;
            if (compare(c, byte, grade) == 0) return result(c, extended(std::int16_t(grade)));
        }
    }
}

// Grades 60-68 and 90-99.
void is_grade_band(Cpu& c, std::int32_t grade) {
    const bool band = compare(c, grade, 0x5a) >= 0 ? compare(c, grade, 0x64) < 0
                                                   : compare(c, grade, 0x45) < 0 && compare(c, grade, 0x3c) >= 0;
    c.set(3, band);
}

// qsort/bsearch comparator over {person, id} pairs: nulls first, and equal
// persons with an id of -1 on either side compare equal.
void compare_links(Cpu& c, std::uint32_t left, std::uint32_t right) {
    if (compare_logical(c, left) == 0) { c.set(3, std::uint32_t(-1)); return; }
    if (compare_logical(c, right) == 0) { c.set(3, 1); return; }
    const auto left_person = c.u32(left), right_person = c.u32(right);
    c.set(5, left_person);
    if (compare(c, std::int32_t(left_person), std::int32_t(right_person)) != 0) {
        c.set(4, right_person);
        c.set(3, left_person - right_person);
        return;
    }
    if (compare(c, c.i32(left + 4), -1) == 0 || compare(c, c.i32(right + 4), -1) == 0) { c.set(3, 0); return; }
    const auto right_id = c.u32(right + 4);
    c.set(4, right_id);
    c.set(3, c.u32(left + 4) - right_id);
}

// qsort/bsearch comparator over history rows: by +8, +c, +14, then the grade.
void compare_rows(Cpu& c, std::uint32_t left, std::uint32_t right) {
    if (compare_logical(c, left) == 0) { c.set(3, std::uint32_t(-1)); return; }
    if (compare_logical(c, right) == 0) { c.set(3, 1); return; }
    const auto differs = [&](unsigned offset) {
        const auto a = c.u32(left + offset), b = c.u32(right + offset);
        c.set(5, a);
        if (compare(c, std::int32_t(a), std::int32_t(b)) == 0) return false;
        c.set(4, b);
        c.set(3, a - b);
        return true;
    };
    if (differs(8) || differs(0xc) || differs(0x14)) return;
    const auto right_grade = extended(c.i16(right + 0x12));
    c.set(4, right_grade);
    c.set(3, extended(c.i16(left + 0x12)) - right_grade);
}

// 0, for stories 1, 3, 8, 10-11, 30-32, 34-35 and 82; 1 for the rest.
void is_ordinary_story(Cpu& c, std::int32_t story) {
    const auto excluded = [&] {
        if (compare_logical(c, std::uint32_t(story) - 0x1e, 2) <= 0) return true;
        if (compare(c, story, 3) == 0 || compare(c, story, 8) == 0) return true;
        if (compare_logical(c, std::uint32_t(story) - 0xa, 1) <= 0) return true;
        if (compare(c, story, 0x52) == 0) return true;
        if (compare_logical(c, std::uint32_t(story) - 0x22, 1) <= 0) return true;
        return compare(c, story, 1) == 0;
    };
    c.set(3, excluded() ? 0 : 1);
}

// Stories 40-59.
void is_transfer_story(Cpu& c, std::int32_t story) {
    const bool within = compare(c, story, 0x28) >= 0 && compare(c, story, 0x3b) <= 0;
    c.set(3, within);
}
}
