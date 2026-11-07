// DATABASE.CPP's tables block, part 1: 0x100a34e0, which decides which clubs of the club table stay in the
// database (database_tables.hpp).
#include "database_tables.hpp"
#include "database_tables_detail.hpp"

namespace cm::aot::database {
namespace {
using namespace tables;
using guest::Frame;

constexpr std::uint32_t entry = 0x100a34e0;
constexpr std::uint32_t allocate_cleared = 0x10775200;  // (count, size): zeroed block, or 0
constexpr std::uint32_t release = 0x10775170;
constexpr std::uint32_t quit = 0x1076f7d0;             // ends the game after a failed allocation
constexpr std::uint32_t record_new_index = 0x10159050;  // (club id, index in the compacted table) -> byte

// The groups the clubs are sorted into, one per nation (inferred), 0xd0 bytes each: up to 50 club ids, the number
// of ids held (+0xc8) and the position of the id whose club has the lowest rating (+0xcc).
namespace group {
constexpr std::uint32_t stride = 0xd0, count = 0xc8, weakest = 0xcc;
}
// The person-like records the scans mark clubs from: stride 0x70 with club pointers at +0x20 and +0x24.
constexpr std::uint32_t staff_stride = 0x70, staff_club_a = 0x20, staff_club_b = 0x24;
// Records of 0x1c bytes with three club-id pointers at +0x8, +0xc and +0x10.
constexpr std::uint32_t link_stride = 0x1c;
constexpr std::uint32_t link_ids[3] = {0x8, 0xc, 0x10};
// Records of 0x14 bytes with two club pointers at +0x4 and +0x8.
constexpr std::uint32_t pair_stride = 0x14, pair_first = 0x4, pair_second = 0x8;
constexpr std::int32_t rating_floor_for_drop = 500;  // a club of the flagged nation rated at most this can be dropped

// The nation numbers a club's nation may not match for the per-nation limit to apply (TOC cells that hold a
// word; the first seven are loaded when needed, the rest were kept in registers r13, r14, r26, r25, r24, r23, r22).
constexpr std::int32_t nation_cells[7] = {-0x7a14, -0x7af4, -0x7a18, -0x7a00, -0x7504, -0x7b68, -0x7508};
constexpr unsigned nation_registers[7] = {13, 14, 26, 25, 24, 23, 22};

struct Selection {
    Cpu& c;
    const Frame& frame;
    std::uint32_t out, mode;
    std::uint32_t pool, club_count_cell, table_cell, group_count_cell, player_nation_cell;
    std::uint32_t marks = 0, groups = 0, last_club = 0;

    std::int8_t size_mode() const { return std::int8_t(mode); }

    void assertion(std::uint32_t at, std::uint32_t line, std::uint32_t name, std::uint32_t extension) {
        guest::report_assertion(c, {.at = at, .file = pool, .format = pool + 0xd, .title = pool + 0x1a,
                                    .name = frame.at(name), .extension = frame.at(extension), .line = line});
    }
    // The way out of an assertion: r4 = the success flag's address, r0 = 0, r3 = the result.
    void after_assertion(std::uint32_t result) {
        c.set(4, c.toc(slot::success));
        c.set(0, 0);
        c.set(3, result);
    }
    // The grouping pass computes with C++ values. Of the volatile registers only r5 and r7 are still live when the
    // groups are released (the mark loop overwrites the rest), so they alone follow the original's loads.
    std::uint32_t left_r5 = 0, left_r6 = 0, left_r7 = 0;

    std::int32_t rating_of_id(std::uint32_t id) { return c.i16(c.u32(table_cell) + id * club::stride + club::rating); }

    // Whether the club's nation flag byte is set or its nation number equals one of the listed numbers (the player's
    // nation, seven TOC cells and seven kept in registers): the per-nation limits then do not apply.
    bool exempt_from_limit(std::uint32_t club_address) {
        const auto nation_record = c.u32(club_address + club::nation);
        if (extsb(c.u8(nation_record + nation::flag)) != 0) return true;
        const auto number = c.u32(nation_record + nation::number);
        if (number == c.u32(player_nation_cell)) return true;
        left_r5 = nation_record;
        for (const auto cell : nation_cells)
            if (number == c.u32(c.toc(cell))) return true;
        for (const auto reg : nation_registers)
            if (number == c.u32(c.r(reg))) return true;
        return false;
    }
    // Adds the club to its nation's group while the group has room, tracking its weakest club.
    void add_to_group(std::uint32_t club_address, std::uint32_t group_record) {
        const auto count = c.u32(group_record + group::count);
        const auto rating = std::int32_t(c.i16(club_address + club::rating));
        c.put32(group_record + (count << 2), c.u32(club_address + club::id));
        bool lowest = true;
        if (count != 0) {
            left_r5 = std::uint32_t(rating);
            lowest = rating < rating_of_id(c.u32(group_record + (c.u32(group_record + group::weakest) << 2)));
        }
        if (lowest) c.put32(group_record + group::weakest, count);
        c.put32(group_record + group::count, count + 1);
    }
    // A full group: the club replaces the weakest one when it is rated higher, and the weakest is found again
    // (the first club is always taken as the new weakest, later ones when rated lower).
    void replace_weakest(std::uint32_t club_address, std::uint32_t group_record) {
        const auto rating = std::int32_t(c.i16(club_address + club::rating));
        left_r5 = std::uint32_t(rating);
        if (rating <= rating_of_id(c.u32(group_record + (c.u32(group_record + group::weakest) << 2)))) return;
        const auto id = c.u32(club_address + club::id);
        c.put32(group_record + (c.u32(group_record + group::weakest) << 2), id);
        left_r5 = id;
        for (std::int32_t i = 0; i < std::int32_t(c.u32(group_record + group::count)); ++i) {
            bool weaker = true;
            if (i != 0) {
                const auto candidate = rating_of_id(c.u32(group_record + (std::uint32_t(i) << 2)));
                const auto weakest = rating_of_id(c.u32(group_record + (c.u32(group_record + group::weakest) << 2)));
                left_r5 = std::uint32_t(candidate);
                left_r6 = std::uint32_t(candidate);  // lha r6,0x1ec(r6): the candidate's rating again
                left_r7 = c.u32(table_cell);
                weaker = candidate < weakest;
            }
            if (weaker) c.put32(group_record + group::weakest, std::uint32_t(i));
        }
    }
    void consider(std::uint32_t club_address) {
        const auto nation_record = c.u32(club_address + club::nation);
        if (nation_record == 0) return;
        const auto group_record = groups + c.u32(nation_record + nation::number) * group::stride;
        if (!exempt_from_limit(club_address)) {
            // Nations of the club's size class take at most 5 (mode 2) or 11 (mode 1) of their clubs.
            const auto held = std::int32_t(c.u32(group_record + group::count));
            if (size_mode() == 2 && held > 4) return;
            if (size_mode() == 1 && held > 10) return;
        }
        if (std::int32_t(c.u32(group_record + group::count)) < std::int32_t(c.r(21))) add_to_group(club_address, group_record);
        else replace_weakest(club_address, group_record);
    }
    // Marks, for every group, the clubs it holds, and leaves the registers the loops end with (r4 the group count,
    // r6 the last group's count, r5 = 1 once anything was marked).
    void mark_group_members() {
        std::int32_t g = 0;
        std::uint32_t last_count = 0;
        bool any = false;
        for (; g < std::int32_t(c.u32(group_count_cell)); ++g) {
            const auto record = groups + std::uint32_t(g) * group::stride;
            last_count = c.u32(record + group::count);
            for (std::int32_t j = 0; j < std::int32_t(last_count); ++j) {
                c.put8(marks + c.u32(record + (std::uint32_t(j) << 2)), 1);
                any = true;
            }
        }
        const auto total = c.u32(group_count_cell);
        c.set(0, total);
        c.set(4, std::uint32_t(g));
        c.set(5, any ? 1 : left_r5);
        c.set(7, left_r7);
        // With no group the loops never ran: r6 is what the grouping pass last left (the last candidate's rating).
        c.set(6, g > 0 ? (std::int32_t(last_count) > 0 ? last_count : 0) : left_r6);
        c.compare(0, g, std::int32_t(total));
    }
    // `r3 = pointer; r4 = 1; r0 = *r3; marks[r0] = 1`: marks the club whose record a pointer field holds.
    void mark_pointed_club(std::uint32_t pointer_address) {
        load32(c, 3, pointer_address);
        c.set(4, 1);
        load32(c, 0, c.r(3));
        c.put8(marks + c.r(0), c.r(4));
    }
    // The 0x70-byte records of the table at TOC -0x7be0 (count at -0x7be4): both club pointers, when set, mark
    // their club (second pointer first in the original's source order: +0x24, then +0x20).
    void mark_from_staff_table() {
        c.set(4, c.toc(-0x7be4));
        c.set(3, c.toc(-0x7be0));
        c.set(4, c.u32(c.r(4)));
        c.set(5, c.u32(c.r(3)));
        c.set(0, c.r(4) - 1);
        c.set(3, c.toc(-0x7be0));
        c.set(0, c.r(0) * staff_stride);
        load32(c, 6, c.r(3));
        c.set(5, c.r(5) + c.r(0));
        for (;; c.set(6, c.r(6) + staff_stride)) {
            if (compare_logical(c, c.r(6), c.r(5)) > 0) break;
            for (const auto field : {staff_club_b, staff_club_a}) {
                load32(c, 0, c.r(6) + field);
                if (compare_logical(c, c.r(0)) != 0) mark_pointed_club(c.r(6) + field);
            }
        }
    }
    // The 0x1c-byte records at TOC -0x7a34 (count at -0x7a38): each of three club-id pointers marks its club when
    // the id is below the club count and not negative.
    void mark_from_link_table() {
        c.set(4, c.toc(-0x7a38));
        c.set(3, c.toc(-0x7a34));
        c.set(4, c.u32(c.r(4)));
        c.set(5, c.u32(c.r(3)));
        c.set(0, c.r(4) - 1);
        c.set(3, c.toc(-0x7a34));
        c.set(0, c.r(0) * link_stride);
        c.set(3, c.u32(c.r(3)));
        c.set(0, c.r(5) + c.r(0));
        for (;; c.set(3, c.r(3) + link_stride)) {
            if (compare_logical(c, c.r(3), c.r(0)) > 0) break;
            for (const auto field : link_ids) {
                load32(c, 4, c.r(3) + field);
                if (compare_logical(c, c.r(4)) == 0) continue;
                load32(c, 5, c.r(3) + field);
                load32(c, 4, club_count_cell);
                load32(c, 5, c.r(5));
                if (compare(c, std::int32_t(c.r(5)), std::int32_t(c.r(4))) >= 0) continue;
                load32(c, 4, c.r(3) + field);
                load32(c, 4, c.r(4));
                if (compare(c, std::int32_t(c.r(4))) < 0) continue;
                load32(c, 4, c.r(3) + field);
                c.set(5, 1);
                load32(c, 4, c.r(4));
                c.put8(marks + c.r(4), c.r(5));
            }
        }
    }
    // The 0x14-byte records at TOC -0x74c4 (count at -0x74c8): a record with both pointers set marks the club at
    // +0x8 when the record at +0x4 has a club (+0xc) with its flag byte set, or the club at +0x8 has a nation
    // record whose flag byte is set.
    void mark_from_pair_table() {
        c.set(4, c.toc(-0x74c8));
        c.set(3, c.toc(-0x74c4));
        c.set(4, c.u32(c.r(4)));
        c.set(5, c.u32(c.r(3)));
        c.set(0, c.r(4) - 1);
        c.set(3, c.toc(-0x74c4));
        c.set(0, c.r(0) * pair_stride);
        c.set(3, c.u32(c.r(3)));
        c.set(6, c.r(5) + c.r(0));
        for (;; c.set(3, c.r(3) + pair_stride)) {
            if (compare_logical(c, c.r(3), c.r(6)) > 0) break;
            load32(c, 0, c.r(3) + pair_second);
            if (compare_logical(c, c.r(0)) == 0) continue;
            load32(c, 0, c.r(3) + pair_first);
            if (compare_logical(c, c.r(0)) == 0) continue;
            bool marked = false;
            load32(c, 4, c.r(3) + pair_first);
            load32(c, 0, c.r(4) + 0xc);
            if (compare_logical(c, c.r(0)) != 0) {
                load32(c, 4, c.r(3) + pair_first);
                load32(c, 4, c.r(4) + 0xc);
                c.set(0, extsb(load8(c, 0, c.r(4) + nation::flag)));
                marked = compare(c, std::int32_t(c.r(0))) != 0;
            }
            if (!marked) {
                load32(c, 4, c.r(3) + pair_second);
                load32(c, 0, c.r(4) + club::nation);
                if (compare_logical(c, c.r(0)) == 0) continue;
                load32(c, 4, c.r(3) + pair_second);
                load32(c, 4, c.r(4) + club::nation);
                c.set(0, extsb(load8(c, 0, c.r(4) + nation::flag)));
                if (compare(c, std::int32_t(c.r(0))) == 0) continue;
            }
            load32(c, 4, c.r(3) + pair_second);
            c.set(5, 1);
            load32(c, 0, c.r(4));
            c.put8(marks + c.r(0), c.r(5));
        }
    }
    // The compaction map: the kept clubs get consecutive slots of the new table (r13 walks the new addresses),
    // each recorded by 0x10159050; the map holds the new record's address, or 0 for a dropped club.
    void build_map() {
        const auto table = c.u32(table_cell);
        std::uint32_t slot = table;
        std::uint32_t last_shifted_id = 0, last_r4 = 0;
        bool any = false;
        c.set(13, slot);
        for (std::uint32_t club_address = table;; club_address += club::stride) {
            c.set(29, club_address);
            if (compare_logical(c, club_address, last_club) > 0) break;
            const auto id = c.u32(club_address + club::id);
            // (slot - table) / 0x248 as the compiler divides it: multiply-high by the reciprocal, then shifts.
            const auto delta = slot - table;
            const auto shifted = srawi(c, mulhw(0xe070381d, delta) + delta, 9);
            c.set(0, shifted);
            c.set(3, id);
            last_r4 = shifted + (shifted >> 31);
            c.set(4, last_r4);
            c.set(13, slot);
            c.call(record_new_index, 0x100a3c7c);
            last_r4 = c.r(4);  // r4 stays whatever 0x10159050 left unless the club count test loads the id
            c.compare(0, std::int32_t(extsb(c.r(3))), 0);
            c.set(0, extsb(c.r(3)));
            bool kept = c.r(0) != 0;
            if (!kept) kept = extsb(c.u8(marks + id)) != 0;
            if (!kept) {  // the club count test loads the id into r4
                last_r4 = id;
                kept = std::int32_t(id) >= std::int32_t(c.u32(club_count_cell) - 2 * c.u32(group_count_cell));
            }
            if (!kept) {
                const auto nation_record = c.u32(club_address + club::nation);
                if (nation_record != 0 && extsb(c.u8(nation_record + nation::flag)) != 0) {
                    kept = !(c.u32(nation_record + nation::number) == c.u32(player_nation_cell) &&
                             c.i16(club_address + club::rating) <= rating_floor_for_drop && c.r(17) != 0);
                }
            }
            last_shifted_id = id << 2;
            any = true;
            c.put32(out + last_shifted_id, kept ? slot : 0);
            if (kept) slot += club::stride;
        }
        c.set(13, slot);
        if (any) {
            c.set(0, last_shifted_id);
            c.set(4, last_r4);
        }
    }
};
}

// 0x100a34e0. A frame of 0x130 bytes saving r13-r31 and LR. `out` receives, for each club id, the address its
// record will have in the compacted club table, or 0 for a dropped club; `mode` (its low byte, 1 or 2 limits the
// clubs per nation to 11 or 5 of the group's size) picks the limit. Allocates a mark byte per club and a group
// per nation (cleared, released at the end); a failed allocation asserts and calls 0x1076f7d0, which ends the game.
// Returns 1, or 0 when the first word of the 0x128-byte record table (TOC -0x7b98) is 0 or `out` is null (assertion).
std::uint32_t choose_kept_clubs(Cpu& c, std::uint32_t out, std::uint32_t mode) {
    c.set(0, c.lr());  // mfspr r0,LR
    Frame frame(c, entry, 0x130, {.first_gpr = 13});
    const auto first_cell = c.toc(-0x7b98);
    c.set(5, first_cell);
    guest::keep(c, {{15, out}, {22, c.toc(-0x7514)}, {16, mode}, {23, c.toc(-0x7a1c)}});
    c.set(21, 0x14);
    guest::keep(c, {{24, c.toc(-0x79fc)}, {25, c.toc(-0x7a24)}, {26, c.toc(-0x7a28)}, {14, c.toc(-0x7510)},
                    {13, c.toc(-0x750c)}, {27, c.toc(-0x7a10)}, {28, c.toc(-0x7b04)}, {29, c.toc(slot::text_pool)},
                    {30, c.toc(slot::club_count)}, {31, c.toc(slot::club_table)}});
    Selection x{c, frame, out, mode, c.r(29), c.r(30), c.r(31), c.r(28), c.r(27)};
    c.set(0, c.u32(first_cell));
    if (compare_logical(c, c.r(0)) == 0) {
        x.assertion(entry + 0x70, 0x4eb, 0xb8, 0x50);
        x.after_assertion(0);
        return 0;
    }
    if (compare_logical(c, out) == 0) {
        x.assertion(entry + 0xd4, 0x4f2, 0x98, 0x48);
        x.after_assertion(0);
        return 0;
    }
    c.set(0, extsb(mode));
    bool small = compare(c, x.size_mode(), 2) == 0;
    if (!small) {
        c.set(0, extsb(mode));
        small = compare(c, x.size_mode(), 1) == 0;
    }
    c.set(17, small);
    c.set(3, c.u32(x.club_count_cell));
    c.set(4, 1);
    c.call(allocate_cleared, entry + 0x14c);
    c.set(20, c.r(3));
    compare(c, std::int32_t(c.r(3)));
    if (c.r(20) == 0) {
        x.assertion(entry + 0x168, 0x508, 0x78, 0x40);
        c.set(4, c.toc(slot::success));
        c.set(0, 0);
        c.set(3, 0xffffffff);
        c.call(quit, entry + 0x1b0);
    }
    x.marks = c.r(20);
    load32(c, 3, x.club_count_cell);
    c.set(4, 0xd0);
    load32(c, 5, x.table_cell);
    c.set(0, c.r(3) - 1);
    load32(c, 3, x.group_count_cell);
    c.set(0, c.r(0) * club::stride);
    c.set(18, c.r(5) + c.r(0));
    c.call(allocate_cleared, entry + 0x1d0);
    c.set(19, c.r(3));
    compare(c, std::int32_t(c.r(3)));
    if (c.r(19) == 0) {
        x.assertion(entry + 0x1ec, 0x510, 0x58, 0x38);
        c.set(4, c.toc(slot::success));
        c.set(0, 0);
        c.set(3, 0xffffffff);
        c.call(quit, entry + 0x234);
    }
    x.groups = c.r(19);
    x.last_club = c.r(18);
    x.left_r5 = c.r(5);
    x.left_r6 = c.r(6);
    x.left_r7 = c.r(7);
    c.set(0, extsb(c.r(17)));
    if (compare(c, std::int32_t(c.r(0))) != 0) c.set(21, 0xf);
    c.set(29, c.u32(x.table_cell));
    for (; compare_logical(c, c.r(29), x.last_club) <= 0; c.set(29, c.r(29) + club::stride)) x.consider(c.r(29));
    x.mark_group_members();
    c.set(3, x.groups);
    c.call(release, entry + 0x558);
    x.mark_from_staff_table();
    x.mark_from_link_table();
    c.set(0, extsb(c.r(17)));
    if (compare(c, std::int32_t(c.r(0))) == 0) x.mark_from_pair_table();
    x.build_map();
    c.set(3, x.marks);
    c.call(release, entry + 0x8ac);
    c.set(3, 1);
    return 1;
}
}
