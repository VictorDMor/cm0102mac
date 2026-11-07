// DATABASE.CPP's records block, second part (database_records.hpp): routines
// that read a record's id slots and its club (club_of_code, 0x100ce320).
#include "database.hpp"
#include "database_records.hpp"
#include "database_records_detail.hpp"
#include "database_search.hpp"
#include "guest_values.hpp"

namespace cm::aot::database {
namespace {
using guest::compare;
using guest::compare_logical;
using guest::Frame;
using namespace records;

// r0 holds `value` and cr0 compares it, as `cmpwi r0,imm` after a load.
void test_r0(Cpu& c, std::uint32_t value, std::int32_t imm) {
    c.set(0, value);
    compare(c, std::int32_t(value), imm);
}

// 0x100c9440 and 0x100c9590 count the entries of an id list whose first word
// is below the count word's value (signed), in r31, as the original's byte
// loop does: r5 the index, r4 the entry, r0 its address offset.
void count_below_limit(Cpu& c, std::uint32_t base, std::uint32_t offset, std::int32_t entries,
                       std::uint32_t limit_word, std::uint32_t& total) {
    std::uint32_t index = 0;
    c.set(5, 0);
    for (; compare(c, guest::extended(std::int8_t(index)), entries) < 0; c.set(5, ++index)) {
        const auto scaled = (guest::extended(std::int8_t(index)) << 2) & ~3u;
        c.set(0, scaled);
        const auto entry = c.u32(base + scaled + offset);
        c.set(4, entry);
        if (compare_logical(c, entry) == 0) continue;
        const auto value = c.u32(entry);
        c.set(4, value);
        const auto limit = c.u32(limit_word);
        c.set(0, limit);
        if (compare(c, std::int32_t(value), std::int32_t(limit)) >= 0) continue;
        c.set(31, ++total);
    }
    c.set(0, guest::extended(std::int8_t(index)));
}

// The count of a record's list below the limit, then of its club's
// (club_of_code with found 0 and the flag set); a null record asserts.
std::uint32_t count_for_record_and_club(Cpu& c, std::uint32_t entry, std::uint32_t record, std::uint32_t offset,
                                        std::int32_t entries, std::uint32_t line) {
    c.set(0, c.lr());
    Frame frame(c, entry, 0x70, {.first_gpr = 28});
    const auto limit_word = c.toc(-0x7be4);
    guest::keep(c, {{31, 0}, {30, limit_word}, {29, c.toc(slot::text_pool)}, {28, c.toc(slot::message)}});
    compare_logical(c, record);
    if (record == 0) {
        fail_assertion(c, frame, entry + 0x48, line, 0x40, 0x38, 0, true);
        return 0;
    }
    std::uint32_t total = 0;
    count_below_limit(c, record, offset, entries, limit_word, total);
    c.set({{4, 0}, {5, 1}});
    guest::enter(c, entry + 0xe0, {record, 0, 1});
    const auto club = club_of_code(c, record, 0, 1);
    if (compare_logical(c, club) != 0) count_below_limit(c, club, offset, entries, limit_word, total);
    c.set(3, total);
    return total;
}
}

// 0x100c9440: how many of a record's five ids at +0x160 (and its club's) point at a record whose first word
// is below the count word (inferred: ids of members still in the game).
std::uint32_t count_valid_five(Cpu& c, std::uint32_t record) {
    return count_for_record_and_club(c, 0x100c9440, record, id_slot::list_160, id_slot::list_160_count, 0x37f4);
}
// 0x100c9590: the same for the three ids at +0x190.
std::uint32_t count_valid_three(Cpu& c, std::uint32_t record) {
    return count_for_record_and_club(c, 0x100c9590, record, id_slot::list_190, id_slot::list_190_count, 0x3864);
}

// 0x100c9bb0: the id word at +0x94 of a record, or of its club when the record's is empty (0 if neither).
std::uint32_t id_word_94_or_club(Cpu& c, std::uint32_t record) {
    c.set(0, c.lr());
    Frame frame(c, 0x100c9bb0, 0x70, {.first_gpr = 30});
    guest::keep(c, {{31, c.toc(slot::text_pool)}, {30, c.toc(slot::message)}});
    compare_logical(c, record);
    if (record == 0) {
        fail_assertion(c, frame, 0x100c9be8, 0x38c3, 0x40, 0x38, 0, true);
        return 0;
    }
    const auto own = c.u32(record + id_slot::word_94);
    c.set(0, own);
    if (compare_logical(c, own) != 0) {
        c.set(3, own);
        return own;
    }
    c.set({{4, 0}, {5, 1}});
    guest::enter(c, 0x100c9c50, {record, 0, 1});
    const auto club = club_of_code(c, record, 0, 1);
    if (compare_logical(c, club) != 0) {
        const auto word = c.u32(club + id_slot::word_94);
        c.set(0, word);
        if (compare_logical(c, word) != 0) {
            c.set(3, word);
            return word;
        }
    }
    c.set(3, 0);
    return 0;
}

namespace {
// 0x100d1cb0 and 0x100d2a80: an attribute number checked against 1..`last`
// and passed with the record's link at `link` to the attribute reader.
template <class Reader>
std::int32_t attribute_of_person(Cpu& c, std::uint32_t entry, std::uint32_t person, std::uint32_t number,
                                 std::int32_t last, std::uint32_t link, std::uint32_t line, std::uint32_t site,
                                 std::uint32_t back, Reader&& reader) {
    c.set(0, c.lr());
    Frame frame(c, entry, 0x70, {.first_gpr = 30});
    guest::keep(c, {{31, c.toc(slot::text_pool)}, {30, c.toc(slot::message)}});
    const auto fail = [&] { return fail_assertion(c, frame, site, line, 0x40, 0x38, 0xffffffff, true); };
    if (compare_logical(c, person) == 0) return fail();
    const auto extended = guest::extended(std::int8_t(number));
    c.set(0, extended);
    if (compare(c, std::int32_t(extended), 1) < 0) return fail();
    c.set(0, extended);
    if (compare(c, std::int32_t(extended), last) > 0) return fail();
    const auto linked = c.u32(person + link);
    c.set(0, linked);
    guest::enter(c, back, {person, linked, number});
    return reader(c, person, linked, number);
}
}

// 0x100d1cb0: attribute 1-31 of a person through attribute_by_number_31, with the record at +0x14.
std::int32_t person_attribute_31(Cpu& c, std::uint32_t person, std::uint32_t number) {
    return attribute_of_person(c, 0x100d1cb0, person, number, 0x1f, 0x14, 0x4757, 0x100d1d00, 0x100d1d58,
                               [](Cpu& c, std::uint32_t a, std::uint32_t b, std::uint32_t n) {
                                   return attribute_by_number_31(c, a, b, std::int8_t(n));
                               });
}
// 0x100d2a80: attribute 1-12 through attribute_by_number, with the record at +0x1c.
std::int32_t person_attribute_12(Cpu& c, std::uint32_t person, std::uint32_t number) {
    return attribute_of_person(c, 0x100d2a80, person, number, 0xc, 0x1c, 0x498e, 0x100d2ad0, 0x100d2b28,
                               [](Cpu& c, std::uint32_t a, std::uint32_t b, std::uint32_t n) {
                                   return attribute_by_number(c, a, b, std::int8_t(n));
                               });
}

// 0x100d3e60: whether a record's club (club_of_code, second stage when the
// stored byte is 0) is found in the second set: the local byte is what
// club_of_code stores through its found pointer.
bool in_second_set(Cpu& c, std::uint32_t record) {
    c.set(0, c.lr());
    Frame frame(c, 0x100d3e60, 0x70, {.first_gpr = 30});
    guest::keep(c, {{31, c.toc(slot::text_pool)}, {30, c.toc(slot::message)}});
    compare_logical(c, record);
    if (record == 0) {
        fail_assertion(c, frame, 0x100d3e98, 0x4b93, 0x44, 0x3c, 0, true);
        return false;
    }
    const auto found = frame.at(0x38);
    guest::enter(c, 0x100d3eec, {record, found, 1});
    const auto club = club_of_code(c, record, found, 1);
    if (compare_logical(c, club) != 0) {
        const auto flag = guest::extended(c.i8(found));
        c.set(0, flag);
        if (compare(c, std::int32_t(flag)) == 0) {
            c.set(3, 1);
            return true;
        }
    }
    c.set(3, 0);
    return false;
}

// 0x100cd980: whether two ids' classes (classify_by_slots) are compatible:
// equal, or 8 with 9 or 7 either way, or 6 with 7 either way; class 6 with an
// id equal to the shared word at TOC -0x7664 makes it incompatible.
bool classes_compatible(Cpu& c, std::uint32_t a, std::uint32_t b) {
    c.set(0, c.lr());
    Frame frame(c, 0x100cd980, 0x50, {.first_gpr = 28});
    const auto shared = c.toc(-0x7664);
    guest::keep(c, {{31, shared}, {29, a}, {28, b}});
    guest::enter(c, 0x100cd9ac, {a});
    const auto first = std::uint32_t(classify_by_slots(c, a));
    c.set(0, first);
    c.set(3, b);
    guest::keep(c, {{30, first}});
    guest::enter(c, 0x100cd9bc, {b});
    const auto second = std::uint32_t(classify_by_slots(c, b));
    const auto e30 = guest::extended(std::int8_t(first)), e3 = guest::extended(std::int8_t(second));
    const auto answer = [&](std::uint32_t value) {
        c.set(3, value);
        return value != 0;
    };
    c.set({{4, e30}, {0, e3}});
    if (compare(c, std::int32_t(e30), std::int32_t(e3)) == 0) return answer(1);
    test_r0(c, e30, 8);
    if (e30 == 8) {
        test_r0(c, e3, 9);
        if (e3 == 9) return answer(1);
    }
    test_r0(c, e3, 8);
    if (e3 == 8) {
        test_r0(c, e30, 9);
        if (e30 == 9) return answer(1);
    }
    test_r0(c, e30, 8);
    if (e30 == 8) {
        test_r0(c, e3, 7);
        if (e3 == 7) return answer(1);
    }
    test_r0(c, e3, 8);
    if (e3 == 8) {
        test_r0(c, e30, 7);
        if (e30 == 7) return answer(1);
    }
    test_r0(c, e30, 6);
    if (e30 == 6) {
        const auto word = c.u32(b);
        c.set(4, word);
        const auto expected = c.u32(shared);
        c.set(0, expected);
        if (compare(c, std::int32_t(word), std::int32_t(expected)) == 0) return answer(0);
    }
    test_r0(c, e3, 6);
    if (e3 == 6) {
        const auto word = c.u32(a);
        c.set(4, word);
        const auto expected = c.u32(shared);
        c.set(0, expected);
        if (compare(c, std::int32_t(word), std::int32_t(expected)) == 0) return answer(0);
    }
    test_r0(c, e30, 6);
    if (e30 == 6) {
        test_r0(c, e3, 7);
        if (e3 == 7) return answer(1);
    }
    test_r0(c, e3, 6);
    if (e3 == 6) {
        test_r0(c, e30, 7);
        if (e30 == 7) return answer(1);
    }
    return answer(0);
}

namespace {
// One id of a list stored into `out` at the next index (r5 counts them):
// the original's registers are r6 the id, r0 the stored index scaled.
void store_next(Cpu& c, std::uint32_t out, std::uint32_t base, std::uint32_t offset, std::uint32_t ext,
                std::uint32_t& stored) {
    c.set(6, ext);
    c.set(0, stored);
    const auto scaled = (ext << 2) & ~3u;
    c.set(6, scaled);
    const auto slot = guest::extended(std::int8_t(stored));
    c.set(5, ++stored);
    c.set(6, base + scaled);
    c.set(0, slot);
    const auto value = c.u32(base + scaled + offset);
    c.set(6, value);
    c.set(0, (slot << 2) & ~3u);
    c.put32(out + ((slot << 2) & ~3u), value);
}
// The first loop of 0x100c9c90/0x100c9e50 over `base`'s ids: every non-zero id.
void copy_non_empty(Cpu& c, std::uint32_t out, std::uint32_t base, std::uint32_t offset, std::int32_t entries,
                    std::uint32_t& stored) {
    std::uint32_t index = 0;
    for (;; c.set(4, ++index)) {
        const auto ext = guest::extended(std::int8_t(index));
        c.set(0, ext);
        if (!(compare(c, std::int32_t(ext), entries) < 0)) break;
        const auto scaled = (ext << 2) & ~3u;
        c.set(0, scaled);
        c.set(6, base + scaled);
        const auto id = c.u32(base + scaled + offset);
        c.set(0, id);
        if (compare_logical(c, id) == 0) continue;
        store_next(c, out, base, offset, ext, stored);
    }
}

// 0x100c9c90 and 0x100c9e50: copies a record's id list (compacted when the
// record has a club: the record's non-empty ids, then the club's while there is
// room; the rest zero) to `out`, or the whole list when it has no club.
// Answers 1; a null record asserts and answers 0.
std::uint32_t copy_id_list(Cpu& c, std::uint32_t entry, std::uint32_t record, std::uint32_t out,
                           std::uint32_t offset, std::int32_t entries, std::uint32_t line) {
    c.set(0, c.lr());
    Frame frame(c, entry, 0x70, {.first_gpr = 28});
    guest::keep(c, {{31, out}, {30, record}, {29, c.toc(slot::text_pool)}, {28, c.toc(slot::message)}});
    if (compare(c, std::int32_t(record)) == 0) {
        fail_assertion(c, frame, entry + 0x44, line, 0x40, 0x38, 0, true);
        return 0;
    }
    guest::enter(c, entry + 0x9c, {record, 0, 1});
    const auto club = club_of_code(c, record, 0, 1);
    if (compare_logical(c, club) == 0) {
        c.set({{4, out}, {3, record + offset}, {5, std::uint32_t(entries) * 4}});
        c.call(0x1077fc28, entry + 0x194);
        c.set(2, c.u32(c.sp() + 0x14));
        c.set(3, 1);
        return 1;
    }
    std::uint32_t index = 0, stored = 0;
    c.set({{4, 0}, {5, 0}});
    copy_non_empty(c, out, record, offset, entries, stored);
    index = 0;
    c.set(4, 0);
    // The club's ids while the index is below the count and there is room.
    for (;;) {
        const auto ext = guest::extended(std::int8_t(index));
        if (compare(c, std::int32_t(ext), entries) >= 0) break;
        const auto stored_ext = guest::extended(std::int8_t(stored));
        c.set(0, stored_ext);
        if (!(compare(c, std::int32_t(stored_ext), entries) < 0)) break;
        const auto scaled = (ext << 2) & ~3u;
        c.set(0, scaled);
        c.set(6, club + scaled);
        const auto id = c.u32(club + scaled + offset);
        c.set(0, id);
        if (compare_logical(c, id) != 0) store_next(c, out, club, offset, ext, stored);
        c.set(4, ++index);
    }
    // Zero fill to the count.
    while (true) {
        const auto ext = guest::extended(std::int8_t(stored));
        c.set(0, ext);
        if (!(compare(c, std::int32_t(ext), entries) < 0)) break;
        c.set(3, 0);
        c.set(0, (ext << 2) & ~3u);
        c.set(5, ++stored);
        c.put32(out + ((ext << 2) & ~3u), 0);
    }
    c.set(3, 1);
    return 1;
}
}

// 0x100c9c90: the five ids at +0x160, 0x100c9e50: the seven at +0x174.
std::uint32_t copy_ids_five(Cpu& c, std::uint32_t record, std::uint32_t out) {
    return copy_id_list(c, 0x100c9c90, record, out, id_slot::list_160, id_slot::list_160_count, 0x38e7);
}
std::uint32_t copy_ids_seven(Cpu& c, std::uint32_t record, std::uint32_t out) {
    return copy_id_list(c, 0x100c9e50, record, out, id_slot::list_174, id_slot::list_174_count, 0x3925);
}

namespace {
// The member loops of 0x100d06f0 over `base`'s fifty ids at +0x98: an id is
// kept when its record's first word is below the limit word's value
// (signed); kept ids go to `out` in turn. The second pass (club) also stops
// when `stored` reaches the limit count.
void keep_members(Cpu& c, std::uint32_t out, std::uint32_t base, std::uint32_t limit_word, std::int32_t max_count,
                  std::uint32_t& stored, bool bounded) {
    std::uint32_t index = 0;
    c.set(4, 0);
    for (;; c.set(4, ++index)) {
        const auto ext = guest::extended(std::int8_t(index));
        c.set(0, ext);
        if (!(compare(c, std::int32_t(ext), id_slot::list_98_count) < 0)) break;
        if (bounded) {
            const auto stored_ext = guest::extended(std::int8_t(stored));
            c.set(6, stored_ext);
            c.set(0, std::uint32_t(max_count));
            if (!(compare(c, std::int32_t(stored_ext), max_count) < 0)) break;
        }
        const auto scaled = (ext << 2) & ~3u;
        c.set(0, scaled);
        c.set(6, base + scaled);
        const auto id = c.u32(base + scaled + id_slot::list_98);
        c.set(0, id);
        if (compare_logical(c, id) == 0) continue;
        // (The original reads the id again and tests it for zero once more; it cannot be zero here.)
        const auto limit = c.u32(limit_word);
        c.set(0, limit);
        const auto first = c.u32(id);
        c.set(6, first);
        if (compare(c, std::int32_t(first), std::int32_t(limit)) >= 0) continue;
        store_next(c, out, base, id_slot::list_98, ext, stored);
    }
}
}

// 0x100d06f0: copies a record's fifty ids at +0x98 whose records are below the limit to `out` (the
// record's, then its club's while there is room, then zeros up to 50 or, with flag, 100), or the
// list as it is (and with flag zeros after it) when the record has no club. Answers 1; a null
// record asserts and answers 0. `flag` is the register as it is; its low byte counts.
std::uint32_t copy_member_ids(Cpu& c, std::uint32_t record, std::uint32_t out, std::uint32_t flag) {
    c.set(0, c.lr());
    Frame frame(c, 0x100d06f0, 0x80, {.first_gpr = 26});
    const auto limit_word = c.toc(-0x7be4);
    guest::keep(c, {{28, record}, {27, c.toc(slot::message)}, {30, c.toc(slot::text_pool)}, {29, out},
                    {31, limit_word}, {26, flag}});
    compare(c, std::int32_t(record));
    if (record == 0) {
        fail_assertion(c, frame, 0x100d0730, 0x4508, 0x40, 0x38, 0, true);
        return 0;
    }
    const auto ext_flag = guest::extended(std::int8_t(flag));
    c.set(0, ext_flag);
    const bool with_flag = compare(c, std::int32_t(ext_flag)) != 0;
    const std::uint32_t max_count = with_flag ? 0x64 : 0x32;
    c.set(0, max_count);
    guest::keep(c, {{30, max_count}});
    guest::enter(c, 0x100d079c, {record, 0, 1});
    const auto club = club_of_code(c, record, 0, 1);
    if (compare_logical(c, club) == 0) {
        c.set({{4, out}, {3, record + id_slot::list_98}, {5, 0xc8}});
        c.call(0x1077fc28, 0x100d090c);
        c.set(2, c.u32(c.sp() + 0x14));
        c.set(0, ext_flag);
        if (compare(c, std::int32_t(ext_flag)) != 0) {
            c.set({{3, out + 0xc8}, {4, 0}, {5, 0xc8}});
            c.call(0x10778300, 0x100d0928);
        }
        c.set(3, 1);
        return 1;
    }
    std::uint32_t stored = 0;
    c.set({{5, 0}});
    keep_members(c, out, record, limit_word, std::int32_t(max_count), stored, false);
    keep_members(c, out, club, limit_word, std::int32_t(max_count), stored, true);
    for (;;) {
        const auto ext = guest::extended(std::int8_t(stored));
        c.set(3, ext);
        c.set(0, max_count);
        if (!(compare(c, std::int32_t(ext), std::int32_t(max_count)) < 0)) break;
        c.set(0, ext);
        c.set(3, 0);
        c.set(0, (ext << 2) & ~3u);
        c.set(5, ++stored);
        c.put32(out + ((ext << 2) & ~3u), 0);
    }
    c.set(3, 1);
    return 1;
}

namespace {
// 0x100d4370's collector: non-empty ids of a record's slots go to `out` while
// the count (r31) is below the limit (r30), both compared as signed bytes.
struct Collector {
    Cpu& c;
    std::uint32_t out;
    std::uint32_t limit;  // r30
    std::uint32_t count;  // r31
    bool room() {
        const auto have = guest::extended(std::int8_t(count)), max = guest::extended(std::int8_t(limit));
        return compare(c, std::int32_t(have), std::int32_t(max)) < 0;
    }
    void word(std::uint32_t base, std::uint32_t offset) {
        const auto id = c.u32(base + offset);
        c.set(0, id);
        if (compare_logical(c, id) == 0) return;
        const auto have = guest::extended(std::int8_t(count));
        c.set(4, have);
        c.set(0, guest::extended(std::int8_t(limit)));
        if (!room()) return;
        const auto slot = count;
        c.set(0, slot);
        const auto value = c.u32(base + offset);
        c.set(4, value);
        c.set(0, guest::extended(std::int8_t(slot)));
        c.set(31, ++count);
        const auto scaled = (guest::extended(std::int8_t(slot)) << 2) & ~3u;
        c.set(0, scaled);
        c.put32(out + scaled, value);
    }
    void list(std::uint32_t base, std::uint32_t offset, std::int32_t entries) {
        std::uint32_t index = 0;
        c.set(4, 0);
        for (;; c.set(4, ++index)) {
            const auto ext = guest::extended(std::int8_t(index));
            c.set(0, ext);
            if (!(compare(c, std::int32_t(ext), entries) < 0)) break;
            const auto scaled_index = (ext << 2) & ~3u;
            c.set(0, scaled_index);
            c.set(5, base + scaled_index);
            const auto id = c.u32(base + scaled_index + offset);
            c.set(0, id);
            if (compare_logical(c, id) == 0) continue;
            const auto have = guest::extended(std::int8_t(count));
            c.set(5, have);
            c.set(0, guest::extended(std::int8_t(limit)));
            if (!room()) continue;
            const auto slot = count;
            c.set(5, ext);
            c.set(0, slot);
            c.set(5, scaled_index);
            c.set(31, ++count);
            c.set(5, base + scaled_index);
            c.set(0, guest::extended(std::int8_t(slot)));
            const auto value = c.u32(base + scaled_index + offset);
            c.set(5, value);
            const auto scaled = (guest::extended(std::int8_t(slot)) << 2) & ~3u;
            c.set(0, scaled);
            c.put32(out + scaled, value);
        }
    }
    void all(std::uint32_t base) {
        word(base, id_slot::word_80);
        list(base, id_slot::list_84, id_slot::list_84_count);
        word(base, id_slot::word_90);
        word(base, id_slot::word_94);
        list(base, id_slot::list_160, id_slot::list_160_count);
        list(base, id_slot::list_174, id_slot::list_174_count);
        list(base, id_slot::list_190, id_slot::list_190_count);
    }
};
}

// 0x100d4370: collects the non-empty ids of a record's slots (the words at +0x80, +0x90, +0x94
// and the lists at +0x84, +0x160, +0x174, +0x190), then of its club's, into `out` while fewer
// than `limit` (signed byte) are stored; answers how many. A null record asserts and answers 0.
std::uint32_t collect_ids(Cpu& c, std::uint32_t record, std::uint32_t out, std::uint32_t limit) {
    compare_logical(c, record);
    c.set(0, c.lr());
    Frame frame(c, 0x100d4370, 0x80, {.first_gpr = 27});
    guest::keep(c, {{27, c.toc(slot::message)}, {28, c.toc(slot::text_pool)}, {29, out}, {30, limit}, {31, 0}});
    if (record == 0) {
        fail_assertion(c, frame, 0x100d43b0, 0x4c70, 0x40, 0x38, 0, true);
        return 0;
    }
    Collector collector{c, out, limit, 0};
    collector.all(record);
    c.set({{4, 0}, {5, 1}});
    guest::enter(c, 0x100d4630, {record, 0, 1});
    const auto club = club_of_code(c, record, 0, 1);
    if (compare_logical(c, club) != 0) collector.all(club);
    c.set(3, collector.count);
    return collector.count;
}

// 0x100c96e0: the id word at +0x90 of a record, of its club, or (when the record lies in the
// range, range_position) of the record that club_at_doubled_bias names for its owner (+0x2c), and so on up
// the chain: the original unrolls six steps and then calls itself. 0 for none; a null record, or
// a null club in the chain, asserts.
std::uint32_t id_word_90_chain(Cpu& c, std::uint32_t record) {
    c.set(0, c.lr());
    Frame frame(c, 0x100c96e0, 0x140, {.first_gpr = 28});
    guest::keep(c, {{31, c.toc(slot::text_pool)}, {30, c.toc(slot::message)}, {29, c.toc(slot::success)},
                    {28, record}});
    compare(c, std::int32_t(record));
    if (record == 0) {
        fail_assertion(c, frame, 0x100c9724, 0x389a, 0x108, 0x60, 0, false);
        return 0;
    }
    auto subject = record;
    for (unsigned step = 0; step < 6; ++step) {
        const std::uint32_t base = 0x100c9768 + step * 0xb8;
        const auto own = c.u32(subject + id_slot::word_90);
        c.set(0, own);
        if (compare_logical(c, own) != 0) {
            c.set(3, own);
            return own;
        }
        guest::enter(c, base + 0x24, {subject, 0, 1});
        const auto club = club_of_code(c, subject, 0, 1);
        if (compare_logical(c, club) != 0) {
            const auto word = c.u32(club + id_slot::word_90);
            c.set(0, word);
            if (compare_logical(c, word) != 0) {
                c.set(3, word);
                return word;
            }
        }
        guest::enter(c, base + 0x48, {subject});
        const auto position = guest::extended(std::int8_t(range_position(c, subject)));
        c.set(0, position);
        if (compare(c, std::int32_t(position)) == 0) {
            c.set(3, 0);
            return 0;
        }
        const auto owner = c.u32(subject + 0x2c);
        guest::enter(c, base + 0x58, {owner});
        const auto next = club_at_doubled_bias(c, owner);
        if (step == 5) {
            guest::enter(c, 0x100c9b5c, {next});
            return id_word_90_chain(c, next);
        }
        guest::keep(c, {{28, next}});
        if (compare(c, std::int32_t(next)) == 0) {
            fail_assertion(c, frame, base + 0x74, 0x389a, 0xe8 - 0x20 * step, 0x58 - 8 * step, 0, false);
            return 0;
        }
        subject = next;
    }
    return 0;
}

namespace {
// The owner's +0x68 against the word a TOC slot points at, as the original's
// compare sequences leave the registers: `first` is the form on r30's word.
bool owner_type_is(Cpu& c, std::uint32_t owner, std::uint32_t word_pointer, bool first) {
    if (first) {
        c.set(3, owner);
        const auto expected = c.u32(word_pointer);
        c.set(0, expected);
        const auto type = c.u32(owner + 0x68);
        c.set(3, type);
        return compare(c, std::int32_t(type), std::int32_t(expected)) == 0;
    }
    c.set(4, owner);
    c.set(3, word_pointer);
    const auto type = c.u32(owner + 0x68);
    c.set(4, type);
    const auto expected = c.u32(word_pointer);
    c.set(0, expected);
    return compare(c, std::int32_t(type), std::int32_t(expected)) == 0;
}
}

// 0x100d12d0: whether a record that has no club passes the owner tests. Its first word must be below
// high - low and club_of_code must find no club. At or above high - 2 * low, with an owner, the
// owner's type (+0x68) must differ from five words and, if the owner has a record at +0x74, that
// record's +8 from two more; otherwise (or without an owner: a pass) the type must differ from three
// words. Answers 0 or 1; a null record asserts.
bool club_less_candidate(Cpu& c, std::uint32_t record) {
    c.set(0, c.lr());
    Frame frame(c, 0x100d12d0, 0xd0, {.first_gpr = 25});
    const auto low_slot = c.toc(slot::range_low), high_slot = c.toc(slot::range_high);
    const auto type_one = c.toc(-0x7a18);
    guest::keep(c, {{29, record}, {30, type_one}, {25, low_slot}, {31, c.toc(slot::success)},
                    {26, c.toc(slot::message)}, {27, c.toc(slot::text_pool)}, {28, high_slot}});
    compare(c, std::int32_t(record));
    if (record == 0) {
        fail_assertion(c, frame, 0x100d1314, 0x463b, 0x90, 0x48, 0, false);
        return false;
    }
    compare_logical(c, record);  // the original's second null test cannot fail
    const auto no = [&] {
        c.set(3, 0);
        return false;
    };
    if (!below_range_limit(c, record, false)) return no();
    guest::enter(c, 0x100d13fc, {record, 0, 1});
    const auto club = club_of_code(c, record, 0, 1);
    if (compare_logical(c, club) != 0) return no();
    compare_logical(c, record);
    const bool below_double = below_range_limit(c, record);
    const auto yes = [&] {
        c.set(3, 1);
        return true;
    };
    auto owner = c.u32(record + 0x2c);
    // The first branch: at or above the doubled limit and with an owner.
    bool first_branch = false;
    if (!below_double) {
        c.set(0, owner);
        first_branch = compare_logical(c, owner) != 0;
    }
    if (first_branch) {
        owner = c.u32(record + 0x2c);
        if (owner_type_is(c, owner, type_one, true)) return no();
        for (const std::int32_t at : {-0x7504, -0x7a14, -0x7b68, -0x7508}) {
            owner = c.u32(record + 0x2c);
            if (owner_type_is(c, owner, c.toc(at), false)) return no();
        }
        owner = c.u32(record + 0x2c);
        c.set(3, owner);
        const auto link = c.u32(owner + 0x74);
        c.set(0, link);
        if (compare_logical(c, link) == 0) return yes();
        for (const std::int32_t at : {-0x7970, -0x7974}) {
            owner = c.u32(record + 0x2c);
            c.set(4, owner);
            const auto word_pointer = c.toc(at);
            c.set(3, word_pointer);
            const auto linked = c.u32(owner + 0x74);
            c.set(4, linked);
            const auto expected = c.u32(word_pointer);
            c.set(0, expected);
            const auto eight = c.u32(linked + 8);
            c.set(3, eight);
            const bool equal = compare(c, std::int32_t(eight), std::int32_t(expected)) == 0;
            if (equal) return no();
        }
        return yes();
    }
    // The second branch: no owner is a pass; the owner's type against three words.
    const auto second_owner = c.u32(record + 0x2c);
    c.set(0, second_owner);
    if (compare_logical(c, second_owner) == 0) return yes();
    owner = c.u32(record + 0x2c);
    if (owner_type_is(c, owner, type_one, true)) return no();
    for (const std::int32_t at : {-0x7af4, -0x79fc}) {
        owner = c.u32(record + 0x2c);
        if (owner_type_is(c, owner, c.toc(at), false)) return no();
    }
    return yes();
}

// 0x100d3c40: a record's eligibility from its first word against the range and its owner (+0x2c)
// and link (+0x30). Below high - 2 * low the link at +0x30 must be listed in the table at TOC -0x7b5c
// (and club_of_code must not be at stage 0 through it) and the owner's flags (+0x120) must have bit 1
// or 2 set; at or above it the record must be below high - low and its owner's +0x6c above 150. Answers 0 or 1; no record or owner answers 0 (the original's later null tests
// cannot fail).
bool eligible_record(Cpu& c, std::uint32_t record) {
    c.set(0, c.lr());
    Frame frame(c, 0x100d3c40, 0xb0, {.first_gpr = 25});
    const auto table_pointer = c.toc(-0x7b5c), low_slot = c.toc(slot::range_low), high_slot = c.toc(slot::range_high);
    guest::keep(c, {{30, record}, {31, table_pointer}, {25, low_slot}, {26, c.toc(slot::success)},
                    {27, c.toc(slot::message)}, {28, c.toc(slot::text_pool)}, {29, high_slot}});
    const auto no = [&] {
        c.set(3, 0);
        return false;
    };
    const auto yes = [&] {
        c.set(3, 1);
        return true;
    };
    compare(c, std::int32_t(record));
    if (record == 0) return no();
    const auto owner = c.u32(record + 0x2c);
    c.set(0, owner);
    if (compare_logical(c, owner) == 0) return no();
    compare_logical(c, record);  // never null here
    const auto test = [&](bool doubled) { return below_range_limit(c, record, doubled); };
    if (test(true)) {
        const auto link = c.u32(record + 0x30);
        c.set(0, link);
        if (compare_logical(c, link) == 0) return no();
        const auto found = frame.at(0x38);
        guest::enter(c, 0x100d3ddc, {record, found, 1});
        const auto club = club_of_code(c, record, found, 1);
        if (compare_logical(c, club) != 0) {
            const auto flag = guest::extended(c.i8(found));
            c.set(0, flag);
            if (compare(c, std::int32_t(flag)) == 0) return no();
        }
        const auto table = c.u32(table_pointer);
        c.set(0, table);
        if (compare_logical(c, table) == 0) return no();
        const auto link_record = c.u32(record + 0x30);
        c.set(3, link_record);
        const auto table_again = c.u32(table_pointer);
        c.set(4, table_again);
        const auto index = c.u32(link_record);
        c.set(0, index);
        c.set(0, (index << 2) & ~3u);
        const auto entry = c.u32(table_again + ((index << 2) & ~3u));
        c.set(0, entry);
        if (compare_logical(c, entry) == 0) return no();
        auto owner_record = c.u32(record + 0x2c);
        c.set(3, owner_record);
        auto bits = c.u8(owner_record + 0x120);
        c.set(0, bits);
        c.set(0, bits & 2);
        if (compare(c, std::int32_t(bits & 2)) != 0) return yes();
        owner_record = c.u32(record + 0x2c);
        c.set(3, owner_record);
        bits = c.u8(owner_record + 0x120);
        c.set(0, bits);
        c.set(0, bits & 1);
        if (compare(c, std::int32_t(bits & 1)) == 0) return no();
        return yes();
    }
    compare_logical(c, record);  // never null here
    if (test(false)) {
        const auto owner_record = c.u32(record + 0x2c);
        c.set(3, owner_record);
        const auto rating = c.u32(owner_record + 0x6c);
        c.set(0, rating);
        if (compare(c, std::int32_t(rating), 0x96) <= 0) return no();
        return yes();
    }
    return no();
}

namespace {
constexpr std::uint32_t contract_for_person = 0x10083110, primary_contract_of = 0x10082f20;  // CONTRACT_MANAGER.CPP
}

// 0x100c9070: counts the members (the fifty ids at +0x98 of the record, and of
// its club) that stay with the record when it is cut: members below the limit
// word's value, or (with flag) those without a contract, whose +0x24 is the record
// or whose primary contract's club (the shared word at TOC -0x7b78 gets the
// primary contract) is not the record. `kind` 1 and 2 pick one side by the byte
// club_of_code stores; a record without club takes club_less_candidate's
// answer and, for kind, the members' flags byte (+0x6a). A null record asserts.
std::uint32_t count_members_kept(Cpu& c, std::uint32_t record, std::uint32_t kind, std::uint32_t flag) {
    c.set(0, c.lr());
    Frame frame(c, 0x100c9070, 0xa0, {.first_gpr = 21});
    const auto shared_word = c.toc(-0x7b78), manager_slot = c.toc(-0x7b7c), table_cell = c.toc(-0x7be8),
               limit_word = c.toc(-0x7be4);
    guest::keep(c, {{21, kind}, {22, c.toc(slot::text_pool)}, {23, c.toc(slot::message)}, {25, shared_word},
                    {26, manager_slot}, {27, table_cell}, {28, limit_word}, {29, record}, {30, flag}, {31, 0}});
    std::uint32_t count = 0;
    compare(c, std::int32_t(record));
    if (record == 0) {
        fail_assertion(c, frame, 0x100c90c0, 0x377e, 0x44, 0x3c, 0, true);
        return 0;
    }
    const auto kind_ext = guest::extended(std::int8_t(kind));
    const auto flag_ext = guest::extended(std::int8_t(flag));
    const auto pass = [&](std::uint32_t base, std::uint32_t first_back, std::uint32_t second_back, bool kind_filter) {
        std::uint32_t index = 0;
        guest::keep(c, {{24, 0}});
        for (;; guest::keep(c, {{24, ++index}})) {
            const auto ext = guest::extended(std::int8_t(index));
            c.set(0, ext);
            if (!(compare(c, std::int32_t(ext), 0x32) < 0)) break;
            const auto scaled = (ext << 2) & ~3u;
            c.set(0, scaled);
            c.set(3, base + scaled);
            const auto person = c.u32(base + scaled + id_slot::list_98);
            guest::keep(c, {{23, person}});
            if (compare_logical(c, person) == 0) continue;
            const auto first = c.u32(person);
            c.set(3, first);
            const auto limit = c.u32(limit_word);
            c.set(0, limit);
            if (compare(c, std::int32_t(first), std::int32_t(limit)) >= 0) continue;
            if (kind_filter) {
                c.set(0, kind_ext);
                if (compare(c, std::int32_t(kind_ext)) != 0) {
                    const auto bits = guest::extended(c.i8(person + 0x6a));
                    c.set(3, bits);
                    c.set(0, kind_ext);
                    const auto common = bits & kind_ext;
                    c.set(0, common);
                    if (compare(c, std::int32_t(common)) == 0) continue;
                }
            }
            bool keep_count = true;
            c.set(0, flag_ext);
            if (compare(c, std::int32_t(flag_ext)) != 0) {
                keep_count = false;
                c.set(3, c.u32(manager_slot));
                c.set(4, person);
                c.call_in_place(contract_for_person, first_back);
                if (compare_logical(c, c.r(3)) == 0) {
                    keep_count = true;
                } else {
                    const auto person_club = c.u32(person + 0x24);
                    c.set(0, person_club);
                    if (compare_logical(c, person_club, record) == 0) {
                        keep_count = true;
                    } else {
                        c.set(3, c.u32(manager_slot));
                        c.set(4, person);
                        c.call_in_place(primary_contract_of, second_back);
                        const auto primary = c.r(3);
                        compare_logical(c, primary);
                        c.put32(shared_word, primary);
                        std::uint32_t club_address = 0;
                        if (primary != 0) {
                            c.set(3, c.u32(shared_word));
                            const auto table = c.u32(table_cell);
                            c.set(4, table);
                            const auto index_word = c.u32(c.r(3) + 4);
                            c.set(0, index_word * club_stride);
                            club_address = table + index_word * club_stride;
                        }
                        c.set(0, club_address);
                        if (compare_logical(c, club_address, record) != 0) keep_count = true;
                    }
                }
            }
            if (keep_count) c.set(31, ++count);
        }
    };
    const auto found = frame.at(0x38);
    guest::enter(c, 0x100c9118, {record, found, 1});
    const auto club = club_of_code(c, record, found, 1);
    guest::keep(c, {{22, club}});
    if (compare(c, std::int32_t(club)) != 0) {
        c.set(0, kind_ext);
        bool skip_record_pass = false;
        if (compare(c, std::int32_t(kind_ext), 2) == 0) {
            const auto flag_byte = guest::extended(c.i8(found));
            c.set(0, flag_byte);
            skip_record_pass = compare(c, std::int32_t(flag_byte)) != 0;
        }
        if (!skip_record_pass) pass(record, 0x100c917c, 0x100c919c, false);
        c.set(0, kind_ext);
        bool skip_club_pass = false;
        if (compare(c, std::int32_t(kind_ext), 1) == 0) {
            const auto flag_byte = guest::extended(c.i8(found));
            c.set(0, flag_byte);
            skip_club_pass = compare(c, std::int32_t(flag_byte)) == 0;
        }
        if (!skip_club_pass) pass(club, 0x100c923c, 0x100c925c, false);
    } else {
        guest::enter(c, 0x100c92ac, {record});
        const auto candidate = guest::extended(std::int8_t(club_less_candidate(c, record)));
        c.set(0, candidate);
        if (compare(c, std::int32_t(candidate)) != 0) pass(record, 0x100c9314, 0x100c9334, true);
        else pass(record, 0x100c93c0, 0x100c93e0, false);
    }
    c.set(3, count);
    return count;
}

// 0x100d0540: for every club of the table (the count at TOC -0x7b50): the
// club needs a club_of_code answer with an owner, else its byte at +0x244 is
// cleared; with one it is set, and when club_of_code reports the second
// stage (found byte 0 is skipped) the club's members are copied
// (copy_member_ids with flag), sorted with the comparator at TOC -0x796c, and counted
// (non-empty, and rated 15 or more through the record at +0x14). While more
// than 48 are left, members are removed (remove_from_record) from the end of the sorted list,
// the first three rated 15 or more spared, and the counts follow.
void trim_club_members(Cpu& c) {
    c.set(0, c.lr());
    Frame frame(c, 0x100d0540, 0x1f0, {.first_gpr = 25});
    const auto table_cell = c.toc(-0x7be8);
    c.set(3, table_cell);
    const auto comparator = c.toc(-0x796c), count_word = c.toc(slot::range_high);
    guest::keep(c, {{29, 0}, {30, comparator}, {31, count_word}});
    auto club = c.u32(table_cell);
    guest::keep(c, {{28, club}});
    const auto found = frame.at(0x38), members = frame.at(0x3c);
    std::int32_t number = 0;
    for (;; ++number, club += club_stride) {
        c.set(29, std::uint32_t(number));
        guest::keep(c, {{28, club}});
        const auto count = c.u32(count_word);
        c.set(0, count);
        if (!(compare(c, number, std::int32_t(count)) < 0)) break;
        guest::enter(c, 0x100d0578, {club, found, 0});
        const auto answer = club_of_code(c, club, found, 0);
        if (compare_logical(c, answer) == 0) {
            c.set(0, 0);
            c.put8(club + 0x244, 0);
            continue;
        }
        const auto owner = c.u32(answer + 0x2c);
        c.set(0, owner);
        if (compare_logical(c, owner) == 0) {
            c.set(0, 0);
            c.put8(club + 0x244, 0);
            continue;
        }
        c.set(0, 1);
        c.put8(club + 0x244, 1);
        const auto stage = guest::extended(c.i8(found));
        c.set(0, stage);
        if (compare(c, std::int32_t(stage)) == 0) continue;
        guest::enter(c, 0x100d05b0, {club, members, 1});
        copy_member_ids(c, club, members, 1);
        c.set({{6, comparator}, {3, members}, {4, 0x64}, {5, 4}});
        c.call(0x10768300, 0x100d05c4);
        std::uint32_t total = 0, rated = 0, index = 0;
        guest::keep(c, {{25, 0}, {27, 0}, {26, 0}});
        for (;; guest::keep(c, {{27, ++index}})) {
            const auto ext = guest::extended(std::int8_t(index));
            c.set(0, ext);
            if (!(compare(c, std::int32_t(ext), 0x64) < 0)) break;
            const auto scaled = (ext << 2) & ~3u;
            c.set(0, scaled);
            c.set(3, members);
            const auto person = c.u32(members + scaled);
            c.set(4, person);
            if (compare_logical(c, person) == 0) continue;
            const auto record = c.u32(person + 0x14);
            c.set(0, record);
            guest::keep(c, {{25, ++total}});
            if (compare_logical(c, record) == 0) continue;
            c.set(3, c.u32(person + 0x14));
            const auto position = guest::extended(c.i8(record + 0xf));
            c.set(0, position);
            if (compare(c, std::int32_t(position), 0xf) < 0) continue;
            guest::keep(c, {{26, ++rated}});
        }
        const auto total_ext = guest::extended(std::int8_t(total));
        c.set(0, total_ext);
        if (compare(c, std::int32_t(total_ext), 0x30) <= 0) continue;
        index = 0x63;
        guest::keep(c, {{27, index}});
        for (;; guest::keep(c, {{27, --index}})) {
            const auto ext = guest::extended(std::int8_t(index));
            c.set(0, ext);
            if (compare(c, std::int32_t(ext)) < 0) break;
            const auto total_now = guest::extended(std::int8_t(total));
            c.set(0, total_now);
            if (!(compare(c, std::int32_t(total_now), 0x30) > 0)) break;
            const auto scaled = (ext << 2) & ~3u;
            c.set(0, scaled);
            c.set(3, members);
            const auto person = c.u32(members + scaled);
            c.set(4, person);
            if (compare_logical(c, person) == 0) continue;
            const auto record = c.u32(person + 0x14);
            c.set(0, record);
            if (compare_logical(c, record) != 0) {
                c.set(3, c.u32(person + 0x14));
                const auto position = guest::extended(c.i8(record + 0xf));
                c.set(0, position);
                if (compare(c, std::int32_t(position), 0xf) >= 0) {
                    const auto spared = guest::extended(std::int8_t(rated));
                    c.set(0, spared);
                    if (compare(c, std::int32_t(spared), 3) < 0) continue;
                    guest::keep(c, {{26, --rated}});
                }
            }
            const auto person_club = c.u32(person + 0x24);
            c.set({{3, person_club}, {5, 0}, {6, 1}, {7, 0}});
            guest::enter(c, 0x100d0694, {person_club, person, 0, 1, 0});
            remove_from_record(c, person_club, person, 0, 1, 0);
            guest::keep(c, {{25, --total}});
        }
    }
}
}
