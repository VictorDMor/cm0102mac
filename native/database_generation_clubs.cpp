// DATABASE.CPP's generation block, part 5: the name picker that follows a person's category
// (database_generation.hpp), 0x100c4cf0.
#include "database_generation.hpp"
#include "database_generation_detail.hpp"

namespace cm::aot::database {
using namespace generation;
namespace {
constexpr std::uint32_t entry = 0x100c4cf0, second_entry = 0x100c4400;
constexpr std::uint32_t club_stride = 0x128;       // records of the table at TOC -0x7b98 (inferred: clubs)
constexpr std::uint32_t person_category = 0x119;   // signed byte of a person or a club record (inferred)
constexpr std::uint32_t row_first = 0x4, row_count = 0x1c, row_small_count = 0x18;

// The slot a category sends to: the TOC word whose pointed value indexes the club table, loaded
// as `lwz r3,slot(r2); lwz r4,0(r31); lwz r0,0(r3)` (form A), or the word entry-loaded into r4 / r5
// (form B: `lwz r0,0(rX); lwz r3,0(r31)`). Category 17 draws between two slots.
enum class Source { none, toc, via_r4, via_r5, draw };
struct Category {
    Source source;
    std::int32_t slot;
};
constexpr Category categories[33] = {
    {Source::none, 0}, {Source::toc, -0x7584}, {Source::toc, -0x7588}, {Source::via_r4, 0}, {Source::toc, -0x74d8},
    {Source::via_r5, 0}, {Source::toc, -0x74e0}, {Source::toc, -0x7a28}, {Source::toc, -0x7a10},
    {Source::toc, -0x7594}, {Source::toc, -0x7a00}, {Source::toc, -0x7af4}, {Source::toc, -0x7598},
    {Source::toc, -0x759c}, {Source::toc, -0x75a0}, {Source::toc, -0x7a14}, {Source::toc, -0x7a24},
    {Source::draw, 0}, {Source::toc, -0x75a8}, {Source::toc, -0x79fc}, {Source::toc, -0x75ac},
    {Source::via_r4, 0}, {Source::toc, -0x7504}, {Source::via_r5, 0}, {Source::toc, -0x7a18},
    {Source::toc, -0x75b0}, {Source::toc, -0x75b4}, {Source::toc, -0x75b8}, {Source::toc, -0x75bc},
    {Source::toc, -0x75c0}, {Source::toc, -0x7514}, {Source::toc, -0x75c4}, {Source::toc, -0x750c}};

// One draw over min(*bound, 0x7fff) names: the three single draws of the routine share these registers.
std::uint32_t single_draw(Cpu& c, std::uint32_t back, std::uint32_t bound_at, std::uint32_t names_at) {
    c.call(random_entry, back);
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

// The single draw of 0x100c4400's zero-category branch: the factor is in r5 and the names come from
// the word at TOC -0x7530 (not the r29 word) through r4.
std::uint32_t single_draw_names_7530(Cpu& c, std::uint32_t back, std::uint32_t bound_at) {
    c.call(random_entry, back);
    const auto bound = load32(c, 0, bound_at);
    const bool capped = compare(c, std::int32_t(bound), 0x7fff) > 0;
    const auto factor = capped ? 0x7fffu : load32(c, 5, bound_at);
    if (capped) c.set(5, factor);
    const auto names_at = load32(c, 4, c.r(2) - 0x7530);
    const auto drawn = scaled_sample(c, factor, c.r(3), {6, 4, 3, 4, 5});
    load32(c, 0, names_at);
    c.set(3, drawn * name_entry::stride);
    c.set(3, c.r(0) + c.r(3));
    return c.r(3);
}

// The club record the person's category names: r25 = table + index * 0x128.
void category_record(Cpu& c, std::uint32_t person, std::uint32_t clubs_at, std::int32_t table_slot, std::uint32_t coin_back) {
    const auto category = extsb(c.u8(person + person_category));
    c.set(0, category);
    if (compare_logical(c, c.r(0), 0x20) > 0) return;
    c.set(3, c.toc(table_slot));
    c.set(0, c.r(0) << 2);
    c.set(3, c.u32(c.r(3) + c.r(0)));
    c.s.wr<4>(guest::reg::ctr, c.r(3));
    const auto& entry_category = categories[category];
    const auto record = [&](unsigned index_register) {
        // lwz r3,slot(r2); lwz r4,0(r31); lwz r0,0(r3); mulli; add r25,r4,r0
        load32(c, 4, clubs_at);
        c.set(0, c.u32(c.r(index_register)));
        c.set(0, c.r(0) * club_stride);
        c.set(25, c.r(4) + c.r(0));
    };
    switch (entry_category.source) {
    case Source::none: break;
    case Source::toc:
        c.set(3, c.toc(entry_category.slot));
        record(3);
        break;
    case Source::via_r4:
    case Source::via_r5: {
        const unsigned via = entry_category.source == Source::via_r4 ? 4 : 5;
        c.set(0, c.u32(c.r(via)));
        load32(c, 3, clubs_at);
        c.set(0, c.r(0) * club_stride);
        c.set(25, c.r(3) + c.r(0));
        break;
    }
    case Source::draw: {
        // A coin: (sample * 2) >> 15 as the compiler's 64-bit shift, tested for non-zero.
        c.call(random_entry, coin_back);
        const auto sample = c.r(3);
        c.set(5, srawi(c, sample, 31));
        const auto low = rlwinm(sample, 1, 0, 30);
        c.set(7, low);
        c.set(5, rlwinm(c.r(5), 1, 0, 30));
        c.set(6, rlwinm(sample, 18, 15, 13));
        c.set(5, rlwimi(c.r(5), sample, 1, 31, 31));
        c.set(4, 0);
        c.set(3, c.r(5));
        c.set(0, 0);
        c.set(3, rlwimi(c.r(3), c.r(7), 0, 17, 31));
        c.set(6, rlwimi(c.r(6), c.r(5), 17, 0, 14));
        c.set(5, srawi(c, c.r(3), 15));
        c.set(6, addze(c, c.r(6)));
        c.set(5, addze(c, c.r(5)));
        c.set(3, c.r(6) ^ c.r(4));
        c.set(0, c.r(5) ^ c.r(0));
        c.set(0, c.r(3) | c.r(0));
        compare(c, std::int32_t(c.r(0)));
        const auto slot = c.r(0) != 0 ? -0x74e8 : -0x75a4;
        c.set(3, c.toc(slot));
        record(3);
        break;
    }
    }
}
}

// 0x100c4cf0. A frame of 0x60 saving r24-r31 and LR. r24 the person, r25 the club record chosen (0 for
// none), r26 the weight of the best of twenty draws (-1), r27 the first name of the person's nation row,
// r28 its count, r29/r30/r31 the TOC words of the names, the club count and the club table.
// A person whose nation row has more than 100 names takes the best of twenty draws; otherwise the
// category picks a club record whose nation row is used instead (with a size test and up to 40 tries
// over random clubs of the same category), falling back to one draw over the bound word.
std::uint32_t pick_name_for_person(Cpu& c, std::uint32_t person) {
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x60, {.first_gpr = 24});
    compare(c, std::int32_t(person));
    c.set(24, person);
    c.set(5, c.toc(-0x7590));
    c.set(4, c.toc(-0x758c));
    c.set(26, 0xffffffff);
    c.set(27, c.toc(-0x7580));
    c.set(25, 0);
    c.set(28, c.toc(-0x7550));
    c.set(29, c.toc(-0x7530));
    c.set(30, c.toc(-0x7b04));
    c.set(31, c.toc(-0x7b98));
    const auto rows_at = c.r(27), bound_at = c.r(28), names_at = c.r(29), count_at = c.r(30), clubs_at = c.r(31);
    bool many = false;
    if (person != 0) {
        const auto nation = load32(c, 0, person + person_nation);
        load32(c, 3, rows_at);
        c.set(0, nation * nation_row::stride);
        c.set(3, c.r(3) + c.r(0));
        const auto count = load32(c, 0, c.r(3) + row_count);
        many = compare(c, std::int32_t(count), 0x64) > 0;
    }
    if (many) {
        const auto nation = load32(c, 3, person + person_nation);
        c.set(30, 0);
        load32(c, 0, person + person_nation);
        c.set(4, nation * nation_row::stride);
        load32(c, 5, rows_at);
        load32(c, 3, rows_at);
        c.set(0, c.r(0) * nation_row::stride);
        c.set(4, c.r(5) + c.r(4));
        const auto first = load32(c, 27, c.r(4) + row_first);
        c.set(3, c.r(3) + c.r(0));
        const auto count = load32(c, 28, c.r(3) + row_count);
        for (;;) {
            c.set(0, extsb(c.r(30)));
            if (compare(c, std::int32_t(c.r(0)), 0x14) >= 0) break;
            c.call(random_entry, entry + 0x8c);
            const auto drawn = scaled_sample(c, count, c.r(3), {7, 6, 3, 3, 5});
            const auto names = load32(c, 4, names_at);
            c.set(0, extsb(c.r(26)));
            const auto index = first + drawn;
            c.set(5, index);
            c.set(3, index * name_entry::stride);
            c.set(3, names + c.r(3));
            c.set(3, extsb(c.u8(c.r(3) + name_entry::weight)));
            if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) > 0) {
                c.set(0, index * name_entry::stride);
                load32(c, 3, names_at);
                c.set(25, index);
                c.set(3, c.r(3) + c.r(0));
                load8(c, 26, c.r(3) + name_entry::weight);
            }
            c.set(30, c.r(30) + 1);
        }
        c.set(0, c.r(25) * name_entry::stride);
        load32(c, 3, names_at);
        c.set(3, c.r(3) + c.r(0));
        return c.r(3);
    }
    if (compare_logical(c, person) == 0) return single_draw(c, entry + 0x110, bound_at, names_at);
    load8(c, 0, person + person_category);
    c.set(25, 0);
    c.set(0, extsb(c.r(0)));
    if (compare(c, std::int32_t(c.r(0))) == 0) return single_draw(c, entry + 0x16c, bound_at, names_at);
    category_record(c, person, clubs_at, -0x5644, entry + 0x354);
    if (compare_logical(c, c.r(25)) != 0) {
        c.call(random_entry, entry + 0x530);
        const auto sample = c.r(3);
        c.set(0, 0x14);
        const auto rows = load32(c, 4, rows_at);
        scaled_sample(c, 20, sample, {3, 7, 5, 4, 6});
        load32(c, 0, c.r(25) + person_nation);
        c.set(0, c.r(0) * nation_row::stride);
        c.set(3, rows + c.r(0));
        const auto count = load32(c, 0, c.r(3) + row_count);
        c.set(3, c.r(6) + 0xf);
        if (compare(c, std::int32_t(count), std::int32_t(c.r(3))) < 0) c.set(25, 0);
    }
    if (compare_logical(c, c.r(25)) == 0) {
        c.set(26, 0);
        for (;;) {
            c.set(0, extsb(c.r(26)));
            if (compare(c, std::int32_t(c.r(0)), 0x28) >= 0) break;
            c.call(random_entry, entry + 0x58c);
            const auto clubs = load32(c, 5, count_at);
            load8(c, 0, person + person_category);
            const auto drawn = scaled_sample(c, clubs, c.r(3), {7, 5, 3, 5, 6});
            load32(c, 4, clubs_at);
            c.set(3, drawn * club_stride);
            c.set(25, c.r(4) + c.r(3));
            c.set(0, extsb(c.r(0)));
            c.set(3, extsb(c.u8(c.r(25) + person_category)));
            if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) == 0) {
                load32(c, 0, c.r(25) + person_nation);
                load32(c, 3, rows_at);
                c.set(0, c.r(0) * nation_row::stride);
                c.set(3, c.r(3) + c.r(0));
                const auto small = load32(c, 0, c.r(3) + row_small_count);
                if (compare(c, std::int32_t(small), 5) > 0) break;
            }
            c.set(26, c.r(26) + 1);
        }
    }
    if (compare_logical(c, c.r(25)) != 0) {
        load32(c, 0, c.r(25) + person_nation);
        load32(c, 3, rows_at);
        c.set(0, c.r(0) * nation_row::stride);
        c.set(3, c.r(3) + c.r(0));
        const auto count = load32(c, 0, c.r(3) + row_count);
        if (compare(c, std::int32_t(count), 5) < 0) c.set(25, 0);
    }
    if (compare_logical(c, c.r(25)) == 0) return single_draw(c, entry + 0x634, bound_at, names_at);
    const auto nation = load32(c, 3, c.r(25) + person_nation);
    load32(c, 0, c.r(25) + person_nation);
    c.set(4, nation * nation_row::stride);
    load32(c, 5, rows_at);
    load32(c, 3, rows_at);
    c.set(0, c.r(0) * nation_row::stride);
    c.set(4, c.r(5) + c.r(4));
    load32(c, 27, c.r(4) + row_first);
    c.set(3, c.r(3) + c.r(0));
    load32(c, 28, c.r(3) + row_count);
    c.call(random_entry, entry + 0x6a8);
    const auto sample = c.r(3);
    scaled_sample(c, c.r(28), sample, {6, 5, 0, 0, 3});
    load32(c, 4, names_at);
    c.set(25, c.r(27) + c.r(3));
    c.set(0, c.r(25) * name_entry::stride);
    c.set(3, c.r(4) + c.r(0));
    return c.r(3);
}

// 0x100c4400: the same picker over the first set of row fields (+0x0 first, +0x18 count) and the
// words at TOC -0x754c / -0x752c, for the plain first names? (inferred). Each of the twenty draws of the
// big-nation branch must also pass 0x100c4b00 (a text test of the name record) to replace the best; the
// best's index is in r26 and its weight in r31 (which then no longer holds the club table's word).
std::uint32_t pick_name_for_person_filtered(Cpu& c, std::uint32_t person) {
    constexpr std::uint32_t entry = second_entry;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x60, {.first_gpr = 24});
    compare(c, std::int32_t(person));
    c.set(24, person);
    c.set(5, c.toc(-0x7590));
    c.set(4, c.toc(-0x758c));
    c.set(26, 0);
    c.set(27, c.toc(-0x7580));
    c.set(28, c.toc(-0x754c));
    c.set(29, c.toc(-0x752c));
    c.set(30, c.toc(-0x7b04));
    c.set(31, c.toc(-0x7b98));
    const auto rows_at = c.r(27), bound_at = c.r(28), names_at = c.r(29), count_at = c.r(30), clubs_at = c.r(31);
    bool many = false;
    if (person != 0) {
        const auto nation = load32(c, 0, person + person_nation);
        load32(c, 3, rows_at);
        c.set(0, nation * nation_row::stride);
        c.set(3, c.r(3) + c.r(0));
        const auto count = load32(c, 0, c.r(3) + row_small_count);
        many = compare(c, std::int32_t(count), 0x64) > 0;
    }
    if (many) {
        load32(c, 0, person + person_nation);
        c.set(31, 0xffffffff);
        const auto nation = load32(c, 4, person + person_nation);
        c.set(25, 0);
        c.set(0, c.r(0) * nation_row::stride);
        load32(c, 3, rows_at);
        load32(c, 5, rows_at);
        c.set(4, nation * nation_row::stride);
        c.set(3, c.r(3) + c.r(0));
        const auto count = load32(c, 28, c.r(3) + row_small_count);
        const auto first = load32(c, 27, c.r(5) + c.r(4));
        for (;;) {
            c.set(0, extsb(c.r(25)));
            if (compare(c, std::int32_t(c.r(0)), 0x14) >= 0) break;
            c.call(random_entry, entry + 0x88);
            const auto drawn = scaled_sample(c, count, c.r(3), {7, 6, 3, 3, 5});
            const auto names = load32(c, 4, names_at);
            c.set(0, extsb(c.r(31)));
            const auto index = first + drawn;
            c.set(30, index);
            c.set(3, index * name_entry::stride);
            c.set(3, names + c.r(3));
            c.set(3, extsb(c.u8(c.r(3) + name_entry::weight)));
            if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) > 0) {
                c.set(0, index * name_entry::stride);
                load32(c, 3, names_at);
                c.set(3, c.r(3) + c.r(0));
                c.call(0x100c4b00, entry + 0xdc);
                c.set(0, extsb(c.r(3)));
                if (compare(c, std::int32_t(c.r(0))) != 0) {
                    c.set(0, index * name_entry::stride);
                    load32(c, 3, names_at);
                    c.set(26, index);
                    c.set(3, c.r(3) + c.r(0));
                    load8(c, 31, c.r(3) + name_entry::weight);
                }
            }
            c.set(25, c.r(25) + 1);
        }
        c.set(0, c.r(26) * name_entry::stride);
        load32(c, 3, names_at);
        c.set(3, c.r(3) + c.r(0));
        return c.r(3);
    }
    if (compare_logical(c, person) == 0) return single_draw(c, entry + 0x124, bound_at, names_at);
    load8(c, 0, person + person_category);
    c.set(25, 0);
    c.set(0, extsb(c.r(0)));
    if (compare(c, std::int32_t(c.r(0))) == 0) return single_draw_names_7530(c, entry + 0x180, bound_at);
    category_record(c, person, clubs_at, -0x5640, entry + 0x36c);
    if (compare_logical(c, c.r(25)) != 0) {
        c.call(random_entry, entry + 0x548);
        const auto sample = c.r(3);
        c.set(0, 0x14);
        const auto rows = load32(c, 4, rows_at);
        scaled_sample(c, 20, sample, {3, 7, 5, 4, 6});
        load32(c, 0, c.r(25) + person_nation);
        c.set(0, c.r(0) * nation_row::stride);
        c.set(3, rows + c.r(0));
        const auto count = load32(c, 0, c.r(3) + row_small_count);
        c.set(3, c.r(6) + 0xf);
        if (compare(c, std::int32_t(count), std::int32_t(c.r(3))) < 0) c.set(25, 0);
    }
    if (compare_logical(c, c.r(25)) == 0) {
        c.set(26, 0);
        for (;;) {
            c.set(0, extsb(c.r(26)));
            if (compare(c, std::int32_t(c.r(0)), 0x28) >= 0) break;
            c.call(random_entry, entry + 0x5a4);
            const auto clubs = load32(c, 5, count_at);
            load8(c, 0, person + person_category);
            const auto drawn = scaled_sample(c, clubs, c.r(3), {7, 5, 3, 5, 6});
            load32(c, 4, clubs_at);
            c.set(3, drawn * club_stride);
            c.set(25, c.r(4) + c.r(3));
            c.set(0, extsb(c.r(0)));
            c.set(3, extsb(c.u8(c.r(25) + person_category)));
            if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) == 0) {
                load32(c, 0, c.r(25) + person_nation);
                load32(c, 3, rows_at);
                c.set(0, c.r(0) * nation_row::stride);
                c.set(3, c.r(3) + c.r(0));
                const auto small = load32(c, 0, c.r(3) + row_small_count);
                if (compare(c, std::int32_t(small), 5) > 0) break;
            }
            c.set(26, c.r(26) + 1);
        }
    }
    if (compare_logical(c, c.r(25)) != 0) {
        load32(c, 0, c.r(25) + person_nation);
        load32(c, 3, rows_at);
        c.set(0, c.r(0) * nation_row::stride);
        c.set(3, c.r(3) + c.r(0));
        const auto count = load32(c, 0, c.r(3) + row_small_count);
        if (compare(c, std::int32_t(count), 5) < 0) c.set(25, 0);
    }
    if (compare_logical(c, c.r(25)) == 0) return single_draw(c, entry + 0x64c, bound_at, names_at);
    const auto owner_nation = load32(c, 0, c.r(25) + person_nation);
    load32(c, 4, c.r(25) + person_nation);
    c.set(0, owner_nation * nation_row::stride);
    load32(c, 3, rows_at);
    load32(c, 5, rows_at);
    c.set(4, c.r(4) * nation_row::stride);
    c.set(3, c.r(3) + c.r(0));
    const auto count = load32(c, 28, c.r(3) + row_small_count);
    load32(c, 27, c.r(5) + c.r(4));
    c.call(random_entry, entry + 0x6bc);
    const auto sample = c.r(3);
    scaled_sample(c, count, sample, {6, 5, 0, 0, 3});
    load32(c, 4, names_at);
    c.set(26, c.r(27) + c.r(3));
    c.set(0, c.r(26) * name_entry::stride);
    c.set(3, c.r(4) + c.r(0));
    return c.r(3);
}
}
