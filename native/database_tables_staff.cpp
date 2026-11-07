// DATABASE.CPP's tables block, part 3: 0x100ab5d0, which expands the staff records read from the database file
// into the 0x70-byte staff table and the 0x34-byte table beside it (database_tables.hpp).
#include "database_tables.hpp"
#include "database_tables_detail.hpp"
#include <array>

namespace cm::aot::database {
namespace {
using namespace tables;
using guest::Frame;

constexpr std::uint32_t entry = 0x100ab5d0;
constexpr std::uint32_t allocate = 0x10775130, release = 0x10775170, quit = 0x1076f7d0;
// The loaded records (r2-relative variables, not cells): the block of 0x9d-byte records, their count, and the
// number of extra rows the second table gets (inferred: staff generated later fill them).
constexpr std::int32_t records_slot = -0x39e0, record_count_slot = -0x39dc, extra_count_slot = -0x39d8;
constexpr std::uint32_t source_stride = 0x9d, staff_stride = 0x70, second_stride = 0x34;

// One copy of `size` bytes from a loaded record to a staff record (sizes of 5 and 2 are done in 16-bit pieces by
// the original, a result of the same bytes).
struct Copy {
    std::uint32_t source, target, size;
};
constexpr std::array staff_copies = std::to_array<Copy>({
    {0x00, 0x00, 4},  {0x04, 0x28, 4},  {0x08, 0x2c, 4}, {0x0c, 0x30, 4}, {0x10, 0x34, 5}, {0x18, 0x5c, 2},
    {0x1a, 0x0c, 4},  {0x1e, 0x10, 4},  {0x22, 0x5e, 1}, {0x23, 0x5f, 1}, {0x39, 0x24, 4}, {0x3d, 0x62, 1},
    {0x3e, 0x4c, 5},  {0x46, 0x54, 5},  {0x24, 0x20, 4}, {0x28, 0x60, 1}, {0x29, 0x3c, 5}, {0x31, 0x44, 5},
    {0x4e, 0x04, 4},  {0x52, 0x08, 4},  {0x56, 0x61, 1}, {0x57, 0x63, 1}, {0x58, 0x64, 1}, {0x59, 0x65, 1},
    {0x5a, 0x66, 1},  {0x5b, 0x67, 1},  {0x5c, 0x68, 1}, {0x5d, 0x69, 1}, {0x5e, 0x6a, 1}, {0x5f, 0x6b, 1},
    {0x91, 0x14, 4},  {0x99, 0x1c, 4},  {0x60, 0x6c, 1}});
constexpr std::uint32_t staff_index_field = 0x18;  // the row's own index
// The second table: its own index at +0, then twelve words from +0x61 of the loaded record to +0x4.
constexpr std::uint32_t second_first_source = 0x61, second_words = 12;

void copy_bytes(Cpu& c, std::uint32_t from, std::uint32_t to, std::uint32_t size) {
    if (size == 4) c.put32(to, c.u32(from));
    else
        for (std::uint32_t i = 0; i < size; ++i) c.put8(to + i, c.u8(from + i));
}
}

// 0x100ab5d0. A frame of 0xf0 bytes saving r26-r31 and LR. Each failed precondition (no loaded records, no staff
// table, a second table that already exists, a failed allocation) asserts and calls 0x1076f7d0, which ends the
// game, so what follows runs only if it returns. The second table gets `count + extra` rows; row i of both
// tables is made from loaded record i; the extra rows (counted from the end backwards) are set to their index and
// -1 in every other field. Finally the loaded records are released and their variables cleared.
void build_staff_tables(Cpu& c) {
    c.set(0, c.lr());  // mfspr r0,LR
    Frame frame(c, entry, 0xf0, {.first_gpr = 26});
    const auto success_flag = c.toc(slot::success), message = c.toc(slot::message), pool = c.toc(slot::text_pool),
               staff_cell = c.toc(-0x7be0), rows_cell = c.toc(-0x74c0), second_cell = c.toc(-0x74bc);
    guest::keep(c, {{26, success_flag}, {27, message}, {28, pool}, {29, staff_cell}, {30, rows_cell}, {31, second_cell}});
    const auto fail = [&](std::uint32_t at, std::uint32_t line, std::uint32_t name, std::uint32_t extension) {
        guest::report_assertion(c, {.at = at, .file = pool, .format = pool + 0xd, .title = pool + 0x1a,
                                    .name = frame.at(name), .extension = frame.at(extension), .line = line});
        c.set(0, 0);
        c.set(3, 0xffffffff);
        c.call(quit, at + 0x44);
    };
    const auto variable = [&](std::int32_t at) { return c.r(2) + std::uint32_t(at); };
    load32(c, 0, variable(records_slot));
    if (compare_logical(c, c.r(0)) == 0) fail(entry + 0x48, 0xf79, 0xb8, 0x50);
    load32(c, 0, staff_cell);
    if (compare_logical(c, c.r(0)) == 0) fail(entry + 0xac, 0xf80, 0x98, 0x48);
    load32(c, 0, second_cell);
    if (compare_logical(c, c.r(0)) != 0) fail(entry + 0x110, 0xf87, 0x78, 0x40);
    load32(c, 3, variable(record_count_slot));
    load32(c, 0, variable(extra_count_slot));
    c.set(0, c.r(3) + c.r(0));
    c.put32(rows_cell, c.r(0));
    load32(c, 0, rows_cell);
    c.set(3, c.r(0) * second_stride);
    c.call(allocate, entry + 0x170);
    c.put32(second_cell, c.r(3));
    load32(c, 0, second_cell);
    if (compare_logical(c, c.r(0)) == 0) fail(entry + 0x194, 0xf8f, 0x58, 0x38);
    c.set(7, 0);
    for (;;) {
        load32(c, 0, variable(record_count_slot));
        if (compare(c, std::int32_t(c.r(7)), std::int32_t(c.r(0))) >= 0) break;
        const auto index = c.r(7);
        const auto source = c.u32(variable(records_slot)) + index * source_stride;
        const auto staff = c.u32(staff_cell) + index * staff_stride;
        const auto second = c.u32(second_cell) + index * second_stride;
        for (const auto& copy : staff_copies) copy_bytes(c, source + copy.source, staff + copy.target, copy.size);
        c.put32(staff + staff_index_field, index);
        c.put32(second, index);
        for (std::uint32_t word = 0; word < second_words; ++word)
            c.put32(second + 4 + 4 * word, c.u32(source + second_first_source + 4 * word));
        // What the last copies leave in the volatile registers (the original's instruction order).
        c.set(3, second);
        c.set(4, c.u32(source + 0x8d));
        c.set(5, c.u32(variable(records_slot)));
        c.set(6, source + 0x31);
        c.set(8, index * source_stride);
        c.set(9, guest::extended(c.i16(source + 0x29)));
        c.set(10, c.u32(staff_cell));
        c.set(11, c.u8(source + 0x28));
        c.set(12, c.u32(variable(records_slot)));
        c.set(7, index + 1);
    }
    c.set(7, 1);
    for (;;) {
        load32(c, 0, variable(extra_count_slot));
        if (compare(c, std::int32_t(c.r(7)), std::int32_t(c.r(0))) > 0) break;
        const auto row = c.u32(rows_cell) - c.r(7);
        const auto second = c.u32(second_cell) + row * second_stride;
        c.put32(second, row);
        for (std::uint32_t field = 4; field <= 0x30; field += 4) c.put32(second + field, 0xffffffff);
        // r26-r29 and r4-r6, r8-r12 hold -1 for the stores; r3 the last row's address.
        for (const unsigned reg : {26u, 27u, 28u, 29u, 4u, 5u, 6u, 8u, 9u, 10u, 11u, 12u}) c.set(reg, 0xffffffff);
        c.set(3, second);
        c.set(7, c.r(7) + 1);
    }
    load32(c, 0, variable(records_slot));
    if (compare_logical(c, c.r(0)) != 0) {
        load32(c, 3, variable(records_slot));
        c.call(release, entry + 0x9b4);
        c.set(3, 0);
        c.set(0, 0);
        c.put32(variable(records_slot), c.r(3));
        c.put32(variable(record_count_slot), c.r(0));
    }
}
}
