// DATABASE.CPP's tables block, part 2: 0x100a2da0, which drops the clubs 0x100a34e0 did not keep from the club
// table and rewrites every club pointer of the other tables (database_tables.hpp).
#include "database_tables.hpp"
#include "database_tables_detail.hpp"
#include <initializer_list>

namespace cm::aot::database {
namespace {
using namespace tables;
using guest::Frame;

constexpr std::uint32_t entry = 0x100a2da0;
constexpr std::uint32_t new_array = 0x107735f0, delete_array = 0x10773610, allocate = 0x10775130,
                        reallocate = 0x107751b0, copy_block = 0x1077fc28 /* (source, destination, size) */,
                        format_text = 0x1077b720, quit = 0x1076f7d0;

// A table of records that hold club pointers: where its base word and count word are (TOC cells kept in
// registers), the record size and the offsets of the pointers.
struct PointerTable {
    unsigned base_register, count_register;
    std::uint32_t stride;
    std::initializer_list<std::uint32_t> fields;
};

struct Compaction {
    Cpu& c;
    const Frame& frame;
    std::uint32_t map = 0;  // r28: old club id -> new record address, or 0

    // Every pointer field of the table is replaced by the new address of the club it pointed at (0 stays 0). Only
    // what the loop ends with is left in the registers (r16 the row count, r0 the count word, r3 the last record,
    // r4 the last value stored); the compares inside it reach no callee.
    void remap(const PointerTable& table) {
        const auto rows = std::int32_t(c.u32(c.r(table.count_register)));
        std::uint32_t last_record = 0, last_value = 0;
        for (std::int32_t row = 0; row < rows; ++row) {
            last_record = c.u32(c.r(table.base_register)) + std::uint32_t(row) * table.stride;
            for (const auto field : table.fields) {
                const auto pointer = c.u32(last_record + field);
                last_value = pointer == 0 ? 0 : c.u32(map + (c.u32(pointer) << 2));
                c.put32(last_record + field, last_value);
            }
        }
        c.set(16, rows > 0 ? std::uint32_t(rows) : 0);
        c.set(0, std::uint32_t(rows));
        c.compare(0, rows > 0 ? rows : 0, rows);
        if (rows > 0) {
            c.set(3, last_record);
            c.set(4, last_value);
        }
    }
};
}

// 0x100a2da0. A frame of 0xa0 bytes saving r16-r31 and LR. `mode` is passed on to 0x100a34e0. Allocates a map
// from club id to the club's new record address (0x100a34e0 fills it), rewrites the club pointers of five tables,
// gathers the kept clubs' records at the start of the club table (copying each to its new slot and giving it
// its new index as id), shrinks the table, stores the new count and last-record address, releases the map and
// formats "Club table has %d entries" into the result buffer. A failed allocation, or a call that kept nothing,
// leaves through the epilogue with the callee's result.
std::uint32_t compact_club_table(Cpu& c, std::uint32_t mode) {
    c.set(0, c.lr());  // mfspr r0,LR
    Frame frame(c, entry, 0xa0, {.first_gpr = 16});
    c.set(23, c.toc(slot::club_count));
    c.set(16, mode);
    c.set(29, c.toc(-0x74f8));
    c.set(27, 0);
    c.set(17, c.toc(-0x7a38));
    c.set(18, c.toc(-0x7a34));
    c.set(30, c.toc(slot::message));
    c.set(31, c.toc(slot::text_pool));
    c.set(19, c.toc(-0x7be0));
    c.set(20, c.toc(-0x74c8));
    c.set(21, c.toc(-0x74c4));
    c.set(22, c.toc(-0x74c0));
    c.set(24, c.toc(slot::club_table));
    c.set(25, c.toc(-0x7be4));
    c.set(26, c.toc(-0x74bc));
    Compaction x{c, frame};
    load32(c, 0, c.r(23));
    c.set(3, c.r(0) << 2);
    c.call(new_array, entry + 0x58);
    c.set(28, c.r(3));
    if (compare(c, std::int32_t(c.r(3))) == 0) return c.r(3);
    x.map = c.r(28);
    guest::enter(c, entry + 0x6c, {c.r(28), c.r(16)});
    choose_kept_clubs(c, c.r(3), c.r(4));
    c.set(0, extsb(c.r(3)));
    if (compare(c, std::int32_t(c.r(0))) == 0) {
        c.set(3, c.r(28));
        c.call(delete_array, entry + 0x7c);
        return c.r(3);
    }
    x.remap({24, 23, club::stride, {0x74, 0x78, 0x7c}});
    x.remap({19, 25, 0x70, {0x24, 0x20}});
    x.remap({26, 22, 0x34, {0x4, 0x8, 0xc, 0x10, 0x14, 0x18}});
    x.remap({21, 20, 0x14, {0x8}});
    x.remap({18, 17, 0x1c, {0x8, 0xc, 0x10, 0x14}});
    // The kept clubs: the new index array holds one word per club with a new address.
    const auto clubs = std::int32_t(c.u32(c.r(23)));
    std::uint32_t kept = 0;
    for (std::int32_t i = 0; i < clubs; ++i)
        if (c.u32(x.map + (std::uint32_t(i) << 2)) != 0) ++kept;
    c.set(16, clubs > 0 ? std::uint32_t(clubs) : 0);
    c.set(0, std::uint32_t(clubs));
    c.compare(0, clubs > 0 ? clubs : 0, clubs);
    c.set(3, kept << 2);
    c.call(allocate, entry + 0x5f0);
    c.put32(c.r(29), c.r(3));
    load32(c, 0, c.r(29));
    if (compare_logical(c, c.r(0)) == 0) {
        c.set(3, c.r(28));
        c.call(delete_array, entry + 0x608);
        return c.r(3);
    }
    // Each kept club's record moves to its new slot and takes its new index as its id.
    std::uint32_t made = 0;
    for (std::int32_t i = 0; i < clubs; ++i) {
        const auto moved_to = c.u32(x.map + (std::uint32_t(i) << 2));
        c.set(16, std::uint32_t(i));
        if (moved_to == 0) continue;
        compare_logical(c, moved_to);
        c.set(27, made);
        c.set(3, c.u32(c.r(24)) + std::uint32_t(i) * club::stride);  // the copy's source, then its destination
        c.set(0, std::uint32_t(i) << 2);
        c.set(6, c.u32(c.r(24)));
        c.set(5, club::stride);
        c.set(4, moved_to);
        c.call(copy_block, entry + 0x640);
        c.set(2, c.u32(c.sp() + 0x14));
        load32(c, 5, c.r(29));
        c.put32(c.r(5) + (made << 2), std::uint32_t(i));
        c.put32(moved_to, made);
        ++made;
        c.set(27, made);
    }
    c.set(16, clubs > 0 ? std::uint32_t(clubs) : 0);
    c.set(0, std::uint32_t(clubs));
    c.compare(0, clubs > 0 ? clubs : 0, clubs);
    c.put32(c.r(23), c.r(27));
    load32(c, 3, c.r(24));
    load32(c, 0, c.r(23));
    c.set(4, c.r(0) * club::stride);
    c.call(reallocate, entry + 0x688);
    c.set(5, c.toc(-0x7500));
    load32(c, 0, c.r(24));
    load32(c, 4, c.r(23));
    load32(c, 5, c.r(5));
    compare_logical(c, c.r(3), c.r(0));
    const bool moved = c.cr(0) & 2 ? false : true;
    load32(c, 6, c.r(24));
    c.set(4, c.r(4) - c.r(5));
    c.set(5, c.r(4) * club::stride);
    c.set(4, c.toc(-0x74fc));
    c.set(0, c.r(6) + c.r(5));
    c.put32(c.r(4), c.r(0));
    if (moved) {
        guest::report_assertion(c, {.at = entry + 0x6cc, .file = c.r(31), .format = c.r(31) + 0xd, .title = c.r(31) + 0x1a,
                                    .name = frame.at(0x40), .extension = frame.at(0x38), .line = 0x4bf});
        c.set(4, c.toc(slot::success));
        c.set(0, 0);
        c.set(3, 0xffffffff);
        c.call(quit, entry + 0x714);
    }
    c.set(3, c.r(28));
    c.call(delete_array, entry + 0x71c);
    c.set(3, c.toc(slot::result_buffer));
    c.set(4, c.r(31) + 0x81);
    load32(c, 5, c.r(23));
    c.call(format_text, entry + 0x72c);
    return c.r(3);
}
}
