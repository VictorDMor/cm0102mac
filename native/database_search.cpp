// DATABASE.CPP's search block, part 1: the small sorters and the dispatcher
// (database_search.hpp).
#include "database_search.hpp"
#include "database_records.hpp"
#include "database_search_detail.hpp"
#include "guest_values.hpp"

namespace cm::aot::database {
namespace {
using namespace search;
using guest::compare;
using guest::compare_logical;
}

void sort_club_ids(Cpu& c, std::int8_t refill) {
    guest::Frame frame(c, 0x100cae30, 0x40, {});
    const auto count_word = c.toc(slot::club_count), id_slot = c.toc(slot::club_id_array),
               table_slot = c.toc(slot::club_table);
    c.set(8, id_slot);
    c.set(7, count_word);
    c.set(6, table_slot);
    if (compare(c, refill) != 0) {
        std::uint32_t index = 0;
        c.set(9, 0);
        while (true) {
            const auto count = c.u32(count_word);
            c.set(0, count);
            if (compare(c, std::int32_t(index), std::int32_t(count)) >= 0) break;
            // Loads in the original's order: the table word, the id array word, then the club's id.
            c.set(4, index * 0x248);
            const auto table = c.u32(table_slot);
            c.set(5, table);
            c.set(0, index << 2);
            const auto ids = c.u32(id_slot);
            c.set(3, ids);
            c.set(9, index + 1);
            const auto id = c.u32(table + index * 0x248);  // first word of the club record, 32-bit wrap
            c.set(4, id);
            c.put32(ids + (index << 2), id);
            ++index;
        }
    }
    c.set(3, c.u32(id_slot));
    c.set(5, 4);
    c.set(4, c.u32(count_word));
    c.set(6, c.toc(slot::club_id_compare));
    c.call(qsort, 0x100cae94);
}

std::uint32_t sort_byte_table(Cpu& c, std::uint32_t selector) {
    guest::Frame frame(c, 0x100d5d70, 0x40, {.first_gpr = 31});
    const auto table = c.toc(slot::byte_table);
    c.set(31, table);
    std::uint32_t n = 1;
    c.set(5, n);
    while (true) {
        const auto signed_n = std::int32_t(std::int8_t(n));
        c.set(0, std::uint32_t(signed_n));
        if (compare(c, signed_n, 14) > 0) break;
        c.set(4, table + std::uint32_t(signed_n));
        c.put8(table + std::uint32_t(signed_n) - 1, n);
        n = (n + 1) & 0xffffffffu;
        c.set(5, n);
    }
    c.put8(c.r(2) - 0x39ee, selector);
    c.set(3, table);
    c.set(6, c.toc(slot::byte_compare));
    c.set(4, 14);
    c.set(5, 1);
    c.call(qsort, 0x100d5dc0);
    c.set(3, table);
    return table;
}

void compact_ranked_entries(Cpu& c) {
    guest::Frame frame(c, 0x100c4180, 0x50, {.first_gpr = 29});
    const auto table_slot = c.toc(slot::ranked_array), count_slot = c.toc(slot::ranked_count);
    guest::keep(c, {{31, table_slot}, {30, count_slot}, {29, 0}});
    std::uint32_t kept = 0, index = 0;
    c.set(5, 0);
    auto entry = c.u32(table_slot);
    c.set(6, entry);
    while (true) {
        const auto count = c.u32(count_slot);
        c.set(0, count);
        if (compare(c, std::int32_t(index), std::int32_t(count)) >= 0) break;
        const auto linked = c.u32(entry + 4);
        c.set(0, linked);
        bool listed = compare_logical(c, linked) != 0;
        if (listed) {
            c.set(3, linked);
            const auto inner = c.u32(linked + 8);
            c.set(0, inner);
            if (compare_logical(c, inner) != 0) {
                c.set(3, c.u32(entry + 4));
                c.set(3, c.u32(c.r(3) + 8));  // loaded into r3 before the byte is read
                const auto flag = std::uint32_t(std::int32_t(c.i8(c.r(3) + 0x120)));
                c.set(0, flag);
                listed = compare(c, std::int32_t(flag)) != 0;
            }
        }
        if (listed) {
            c.set(0, kept);
            const auto table = c.u32(table_slot);
            c.set(4, table);
            c.set(3, kept * 0x1c);
            c.set(0, c.u32(entry));
            const auto dest = table + kept * 0x1c;
            c.set(3, dest);
            ++kept;
            c.set(29, kept);
            for (std::uint32_t at = 0; at < 0x18; at += 4) {
                c.set(0, c.u32(entry + at));
                c.put32(dest + at, c.r(0));
            }
            c.set(0, std::uint32_t(std::int32_t(c.i16(entry + 0x18))));
            c.put16(dest + 0x18, c.r(0));
        }
        ++index;
        entry += 0x1c;
        c.set(5, index);
        c.set(6, entry);
    }
    c.set(4, kept * 0x1c);
    c.set(3, c.u32(table_slot));
    c.call(resize, 0x100c424c);
    const auto resized = c.r(3);
    c.put32(count_slot, kept);
    c.set(5, 0x1c);
    const auto floor_word = c.toc(slot::ranked_floor);
    c.set(4, floor_word);
    const auto count = c.u32(count_slot);
    c.set(0, count);
    const auto floor = c.u32(floor_word);
    c.set(4, floor);
    c.put32(table_slot, resized);
    const auto span = (count - floor) * 0x1c;
    c.set(0, span);
    const auto tail_slot = c.toc(slot::ranked_tail);
    c.set(7, tail_slot);
    c.set(6, c.u32(table_slot));
    c.set(3, c.u32(table_slot));
    c.set(4, c.u32(count_slot));
    c.set(0, c.r(6) + span);
    c.set(6, c.toc(slot::ranked_compare));
    c.put32(tail_slot, c.r(0));
    c.call(qsort, 0x100c428c);
}

std::int32_t search_dispatch(Cpu& c, std::uint32_t subject, std::uint32_t record, std::uint32_t slot_register) {
    c.set(0, c.lr());  // mfspr r0,LR: what r0 holds until the slot is tested
    guest::Frame frame(c, 0x100d21f0, 0x80, {.first_gpr = 27});
    const auto buffer_word = c.toc(-0x7ba8), pool = c.toc(-0x5618);
    // r27-r29 hold the arguments, r30 the message buffer (later the record's +0x14), r31 the string pool.
    guest::keep(c, {{27, subject}, {28, record}, {29, slot_register}, {30, buffer_word}, {31, pool}});
    const auto answer = [&](std::int32_t value) {
        c.set(3, std::uint32_t(value));
        return value;
    };
    const auto slot_number = [&] {
        const auto value = std::int32_t(std::int8_t(slot_register));
        c.set(0, std::uint32_t(value));
        return value;
    };
    // The routine that finishes a successful lookup: (record, record+0x14, slot).
    const auto finish = [&](std::uint32_t back) {
        c.set({{3, record}, {4, c.r(30)}, {5, slot_register}});
        guest::enter(c, back, {record, c.r(30), slot_register});
        return attribute_by_number_31(c, record, c.r(30), std::int8_t(slot_register));
    };
    bool invalid = compare(c, std::int32_t(record)) == 0;
    if (!invalid) invalid = compare(c, slot_number(), 1) < 0;
    if (!invalid) invalid = compare(c, slot_number(), 0x1f) > 0;
    if (invalid) {
        guest::report_assertion(c, {.at = 0x100d2244, .file = pool, .format = pool + 0xd, .title = pool + 0x1a,
                                    .name = frame.at(0x40), .extension = frame.at(0x38), .line = 0x480f});
        c.set(4, c.toc(-0x7bac));
        c.set(0, 0);
        return answer(-1);
    }
    const auto flag_word = c.toc(-0x797c);
    c.set(3, flag_word);
    guest::keep(c, {{30, c.u32(record + 0x14)}});
    const auto flag = std::int32_t(c.i8(flag_word));  // the byte at the slot's address itself
    c.set(0, std::uint32_t(flag));
    if (compare(c, flag) == 0) {
        finish(0x100d2310);
        return std::int32_t(c.r(3));
    }
    const auto target_word = c.toc(-0x7978);
    c.set(3, target_word);
    c.set(4, subject);
    c.set(3, c.u32(target_word));
    c.call(0x106fdb00, 0x100d22b0);
    if (compare_logical(c, c.r(3)) == 0) {
        finish(0x100d22c8);
        return std::int32_t(c.r(3));
    }
    c.set({{3, subject}, {4, record}, {5, slot_register}});
    guest::enter(c, 0x100d22dc, {subject, record, slot_register});
    slot_visible(c, subject, record, slot_register);
    const auto found = std::int32_t(std::int8_t(c.r(3)));
    c.set(0, std::uint32_t(found));
    if (compare(c, found) == 0) return answer(-7);
    finish(0x100d22f4);
    return std::int32_t(c.r(3));
}

namespace {
// The signed remainder by 4 as the original builds it from rotates: the two low
// bits shifted to the top, less the sign bit, rotated left by 2, plus the sign bit.
std::int32_t signed_mod_4(std::uint32_t value) {
    const auto sign = (value >> 31) & 1;
    const auto top = ((value & 3) << 30) - sign;
    return std::int32_t(((top << 2) | (top >> 30)) + sign);
}
// The original's division by 3 through mulhw: r4 - 3 * trunc(r4 / 3), left in r4 and compared with 0 into cr0
// (a record form).
bool multiple_of_3(Cpu& c, std::uint32_t value) {
    c.set(4, value);
    const auto high = mulhw(0x55555556, value);
    const auto quotient = high + (high >> 31);
    return compare(c, std::int32_t(value - quotient * 3)) == 0;
}
}

std::uint32_t slot_visible(Cpu& c, std::uint32_t person, std::uint32_t record, std::uint32_t slot_register) {
    guest::Frame frame(c, 0x100d1d70, 0x40, {.first_gpr = 30});
    const auto target_word = c.toc(-0x7978);
    c.set(7, person);
    c.set(6, target_word);
    guest::keep(c, {{30, record}, {31, slot_register}});
    const auto answer = [&](std::uint32_t value) {
        c.set(3, value);
        return value;
    };
    bool absent = compare(c, std::int32_t(person)) == 0;  // or. r7,r3,r3
    if (!absent) absent = compare_logical(c, record) == 0;
    if (!absent) {
        const auto target = c.u32(target_word);
        c.set(0, target);
        absent = compare_logical(c, target) == 0;
    }
    if (absent) return answer(1);
    const auto flag_address = c.toc(-0x797c);
    c.set(3, flag_address);
    const auto flag = std::int32_t(c.i8(flag_address));
    c.set(0, std::uint32_t(flag));
    if (compare(c, flag) == 0) return answer(1);
    c.set(3, c.u32(target_word));
    c.set(4, person);
    c.call(0x106fdb00, 0x100d1dd8);
    if (compare_logical(c, c.r(3)) == 0) return answer(1);
    c.set(4, record);
    c.call(0x106fe280, 0x100d1df0);
    const auto mode = std::int32_t(std::int8_t(c.r(3)));
    c.set(0, std::uint32_t(mode));
    if (compare(c, mode, 3) == 0) return answer(1);
    if (compare(c, mode, 2) != 0 && compare(c, mode, 1) != 0) return answer(0);
    // Mode 2 tests the slot for evenness, mode 1 for a multiple of 4 (a shifted
    // run of slots is excluded), both then for a multiple of 3; the shift comes
    // from the record's first word modulo 4.
    const bool even_mode = mode == 2;
    const auto word = c.u32(record);
    c.set(3, word);
    const auto slot = std::int32_t(std::int8_t(slot_register));
    c.set(0, std::uint32_t(slot));
    compare(c, slot, 1);
    const auto key = signed_mod_4(word);
    c.set(3, std::uint32_t(key));
    if (slot == 1) return answer(1);
    for (const std::int32_t special : {0x11, 0x15, 0x1a, 0x1b}) {
        c.set(0, std::uint32_t(slot));
        if (compare(c, slot, special) == 0) return answer(1);
    }
    // key 0 tests the slot itself against the period, then slot + 1 against 3.
    std::uint32_t first = 3, second = 3;
    compare(c, key, 2);
    if (key == 2) first = second = 2;
    else if (key < 2) {
        compare(c, key, 0);
        if (key == 0) first = 0, second = 1;
        else if (key > 0) first = second = 1;
    }
    const auto shifted = std::uint32_t(slot) + first;
    c.set(3, shifted);
    c.set(0, shifted);
    const auto sign = (shifted >> 31) & 1;
    if (even_mode) {
        const auto parity = ((shifted & 1) ^ sign) - sign;
        c.set(0, parity);
        if (compare(c, std::int32_t(parity)) == 0) return answer(1);
    } else {
        const auto remainder = std::uint32_t(signed_mod_4(shifted));
        c.set(0, remainder);
        if (compare(c, std::int32_t(remainder)) == 0) return answer(1);
    }
    return answer(multiple_of_3(c, std::uint32_t(slot) + second) ? 1 : 0);
}
}
