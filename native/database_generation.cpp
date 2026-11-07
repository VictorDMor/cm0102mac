// DATABASE.CPP's generation block, part 1: the name pickers (database_generation.hpp).
#include "database_generation.hpp"
#include "database_generation_detail.hpp"

namespace cm::aot::database {
using namespace generation;

// 0x100c42b0. A frame of 0x50 saving r26-r31 and LR. r26 and r31 hold the addresses of the
// single-draw bound word and the name table's word, r27 the best entry's index (0 to
// start), r28 its weight byte (-1), r29 the row's entry count, r30 its first index.
std::uint32_t pick_name_record(Cpu& c, std::uint32_t person) {
    constexpr std::uint32_t entry = 0x100c42b0;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x50, {.first_gpr = 26});
    compare_logical(c, person);
    const auto rows_at = c.toc(-0x7580);
    c.set(5, rows_at);
    const auto bound_at = c.toc(-0x754c);
    c.set(26, bound_at);
    c.set(28, 0xffffffff);
    const auto names_at = c.toc(-0x752c);
    c.set(31, names_at);
    c.set(27, 0);
    bool three = false;
    if (person != 0) {
        const auto nation = load32(c, 0, person + person_nation);
        c.set(4, c.u32(rows_at));
        c.set(0, nation * nation_row::stride);
        c.set(4, c.r(4) + c.r(0));
        const auto count = load32(c, 0, c.r(4) + 0x18);
        three = compare(c, std::int32_t(count), 0xf) > 0;
    }
    if (three) {
        load32(c, 0, person + person_nation);
        c.set(26, 0);
        const auto nation = load32(c, 4, person + person_nation);
        c.set(0, c.r(0) * nation_row::stride);
        load32(c, 3, rows_at);
        load32(c, 5, rows_at);
        c.set(4, nation * nation_row::stride);
        c.set(3, c.r(3) + c.r(0));
        const auto count = load32(c, 29, c.r(3) + 0x18);
        const auto first = load32(c, 30, c.r(5) + c.r(4));
        while (compare(c, std::int32_t(c.r(26)), 3) < 0) {
            c.call(random_entry, entry + 0x78);
            const auto names = load32(c, 4, names_at);
            c.set(0, extsb(c.r(28)));
            const auto drawn = scaled_sample(c, count, c.r(3), {7, 6, 3, 3, 5});
            const auto index = first + drawn;
            c.set(5, index);
            c.set(3, index * name_entry::stride);
            c.set(3, names + c.r(3));
            c.set(3, extsb(c.u8(c.r(3) + name_entry::weight)));
            if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) > 0) {
                c.set(0, index * name_entry::stride);
                load32(c, 3, names_at);
                c.set(27, index);
                c.set(3, c.r(3) + c.r(0));
                load8(c, 28, c.r(3) + name_entry::weight);
            }
            c.set(26, c.r(26) + 1);
        }
        c.set(0, c.r(27) * name_entry::stride);
        load32(c, 3, names_at);
        c.set(3, c.r(3) + c.r(0));
        return c.r(3);
    }
    // One draw over min(*bound, 0x7fff).
    c.call(random_entry, entry + 0xf0);
    const auto bound = load32(c, 0, bound_at);
    const bool capped = compare(c, std::int32_t(bound), 0x7fff) > 0;
    const auto factor = capped ? 0x7fffu : load32(c, 4, bound_at);
    if (capped) c.set(4, factor);
    const auto drawn = scaled_sample(c, factor, c.r(3), {6, 4, 3, 4, 5});
    load32(c, 0, names_at);
    c.set(3, drawn * name_entry::stride);
    c.set(3, c.r(0) + c.r(3));
    return c.r(3);
}

// 0x100c4ba0: the same picker over the second pair of row fields (+0x1c count, +0x4
// first, more than 10 entries) and the words at TOC -0x7550 and -0x7530; its loop test
// sign-extends the counter into r0 first, and r6 holds the table's word.
std::uint32_t pick_second_name_record(Cpu& c, std::uint32_t person) {
    constexpr std::uint32_t entry = 0x100c4ba0;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x50, {.first_gpr = 26});
    compare_logical(c, person);
    const auto rows_at = c.toc(-0x7580);
    c.set(6, rows_at);
    const auto bound_at = c.toc(-0x7550);
    c.set(26, bound_at);
    c.set(28, 0xffffffff);
    const auto names_at = c.toc(-0x7530);
    c.set(31, names_at);
    c.set(27, 0);
    bool three = false;
    if (person != 0) {
        const auto nation = load32(c, 0, person + person_nation);
        c.set(4, c.u32(rows_at));
        c.set(0, nation * nation_row::stride);
        c.set(4, c.r(4) + c.r(0));
        const auto count = load32(c, 0, c.r(4) + 0x1c);
        three = compare(c, std::int32_t(count), 0xa) > 0;
    }
    if (three) {
        const auto nation = load32(c, 4, person + person_nation);
        c.set(26, 0);
        load32(c, 0, person + person_nation);
        c.set(4, nation * nation_row::stride);
        load32(c, 5, rows_at);
        load32(c, 3, rows_at);
        c.set(0, nation * nation_row::stride);
        c.set(4, c.r(5) + c.r(4));
        const auto first = load32(c, 30, c.r(4) + 4);
        c.set(3, c.r(3) + c.r(0));
        const auto count = load32(c, 29, c.r(3) + 0x1c);
        for (;;) {
            c.set(0, extsb(c.r(26)));
            if (compare(c, std::int32_t(c.r(0)), 3) >= 0) break;
            c.call(random_entry, entry + 0x7c);
            const auto names = load32(c, 4, names_at);
            c.set(0, extsb(c.r(28)));
            const auto drawn = scaled_sample(c, count, c.r(3), {7, 6, 3, 3, 5});
            const auto index = first + drawn;
            c.set(5, index);
            c.set(3, index * name_entry::stride);
            c.set(3, names + c.r(3));
            c.set(3, extsb(c.u8(c.r(3) + name_entry::weight)));
            if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) > 0) {
                c.set(0, index * name_entry::stride);
                load32(c, 3, names_at);
                c.set(27, index);
                c.set(3, c.r(3) + c.r(0));
                load8(c, 28, c.r(3) + name_entry::weight);
            }
            c.set(26, c.r(26) + 1);
        }
        c.set(0, c.r(27) * name_entry::stride);
        load32(c, 3, names_at);
        c.set(3, c.r(3) + c.r(0));
        return c.r(3);
    }
    c.call(random_entry, entry + 0xf8);
    const auto bound = load32(c, 0, bound_at);
    const bool capped = compare(c, std::int32_t(bound), 0x7fff) > 0;
    const auto factor = capped ? 0x7fffu : load32(c, 4, bound_at);
    if (capped) c.set(4, factor);
    const auto drawn = scaled_sample(c, factor, c.r(3), {6, 4, 3, 4, 5});
    load32(c, 0, names_at);
    c.set(3, drawn * name_entry::stride);
    c.set(3, c.r(0) + c.r(3));
    return c.r(3);
}
}
