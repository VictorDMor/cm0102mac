// DATABASE.CPP's people block, part "indexes": 0x100b44c0, the pass over twelve record tables that replaces
// each pointer field with the id of the record it points at (database_people_names.hpp).
#include "database_people_detail.hpp"
#include <algorithm>
#include <span>
#include "database_people_names.hpp"

namespace cm::aot::database {
using namespace people;
namespace {
// What the original does with one pointer field of a row: the word at `offset` is null (answers -1) or the
// address of a record whose word at `read` is the answer; the answer replaces the field. `repeat` consecutive
// words from `offset` for an array of pointers (the original's inner loops), 0 for a single field.
struct Item {
    std::uint16_t offset;
    std::uint8_t read, repeat;
};
// One table: its rows are `stride` bytes from the address the word at TOC `base_slot` holds, `count_slot` the
// TOC slot of the word with the number of rows. The original keeps the row's address in `row_register` and the
// answer in `answer_register` (volatile registers in the later tables, so they are part of the result).
struct Table {
    std::int16_t base_slot, count_slot;
    std::uint16_t stride;
    std::uint8_t row_register, answer_register;
    std::span<const Item> items;
};

// Inferred meanings: 0x6c-byte rows of the table at -0x7bbc (people's links to clubs), the 0x248-byte club
// table at -0x7be8, the 0x128-byte table at -0x7b98, and so on; the routine is the one that turns the loaded
// game's pointers into ids (the answers read +0x68, +0x8, +0x10, +0x20 or the first word of the record).
constexpr Item first_items[] = {{0x8, 0x68, 0}, {0x4, 0x8, 0}, {0xc, 0, 0}, {0x10, 0, 0}};
constexpr Item second_items[] = {
    {0x2c, 0x68, 0}, {0x30, 0, 0}, {0x34, 0, 0}, {0x38, 0, 0}, {0x3c, 0, 0}, {0x40, 0, 0}, {0x44, 0, 0},
    {0x4c, 0, 0}, {0x54, 0, 0}, {0x48, 0, 0}, {0x50, 0, 0}, {0x58, 0, 0}, {0x5c, 0, 0}, {0x60, 0, 0},
    {0x64, 0, 0}, {0x68, 0, 0}, {0x6c, 0, 0}, {0x70, 0, 0}, {0x74, 0, 0}, {0x78, 0, 0}, {0x7c, 0, 0},
    {0x98, 0, 50}, {0x174, 0, 7}, {0x160, 0, 5}, {0x190, 0, 3}, {0x84, 0, 3},
    {0x90, 0, 0}, {0x80, 0, 0}, {0x94, 0, 0}, {0x19c, 0, 20}};
constexpr Item third_items[] = {{0x74, 0x8, 0}, {0x78, 0x10, 0}, {0x7c, 0, 0}, {0x80, 0, 0}, {0x88, 0, 0},
                                {0x90, 0, 0}, {0x84, 0, 0}, {0x8c, 0, 0}, {0x94, 0, 0}, {0x98, 0x68, 0},
                                {0x9c, 0x68, 0}, {0xa0, 0x68, 0}};
constexpr Item fourth_items[] = {{0x4, 0x20, 0}, {0x8, 0x20, 0}, {0xc, 0x68, 0}, {0x10, 0x10, 0}};
constexpr Item fifth_items[] = {{0x10, 0x10, 0}, {0x14, 0, 0}};
constexpr Item sixth_items[] = {{0x4, 0, 0},  {0x8, 0, 0},  {0xc, 0, 0},  {0x10, 0, 0}, {0x14, 0, 0}, {0x18, 0, 0},
                                {0x1c, 0, 0}, {0x20, 0, 0}, {0x24, 0, 0}, {0x28, 0, 0}, {0x2c, 0, 0}, {0x30, 0, 0}};
constexpr Item seventh_items[] = {{0x4, 0, 0}, {0x8, 0, 0}, {0xc, 0, 0}, {0x10, 0, 0},
                                  {0x14, 0, 0}, {0x18, 0, 0}, {0x1c, 0, 0}, {0x20, 0, 0}};
constexpr Item eighth_items[] = {{0x28, 0x20, 0}, {0x2c, 0x20, 0}, {0x30, 0x20, 0}, {0xc, 0x68, 0}, {0x10, 0x68, 0},
                                 {0x20, 0, 0},    {0x24, 0, 0},    {0x14, 0, 0},    {0x18, 0, 0},   {0x1c, 0, 0}};
constexpr Item ninth_items[] = {{0x4, 0x8, 0}, {0x8, 0x68, 0}, {0xc, 0, 0}, {0x10, 0, 0}};
constexpr Item tenth_items[] = {{0x4, 0, 0}, {0x8, 0, 0}};
constexpr Item eleventh_items[] = {{0x18, 0x68, 0}};
constexpr Item twelfth_items[] = {{0x4, 0, 0}, {0x8, 0, 0}, {0xc, 0, 0}, {0x10, 0, 0}, {0x14, 0, 0}};

// In the original's order. The tables are walked one after another, each over rows 0 up to its count (a signed
// compare, so a count of 0 or less walks none).
constexpr Table tables[] = {
    {-0x7bbc, -0x7b64, 0x6c, 15, 16, first_items},
    {-0x7be8, -0x7b50, 0x248, 15, 16, second_items},
    {-0x7b98, -0x7b04, 0x128, 6, 7, third_items},
    {-0x7ab4, -0x7528, 0x2c, 6, 7, fourth_items},
    {-0x7548, -0x7568, 0x50, 6, 7, fifth_items},
    {-0x74bc, -0x74c0, 0x34, 6, 7, sixth_items},
    {-0x79f8, -0x79f4, 0x44, 4, 6, seventh_items},
    {-0x7be0, -0x7be4, 0x70, 3, 4, eighth_items},
    {-0x753c, -0x755c, 0x68, 3, 4, ninth_items},
    {-0x74c4, -0x74c8, 0x14, 3, 4, tenth_items},
    {-0x7538, -0x7558, 0x38, 3, 4, eleventh_items},
    {-0x7a34, -0x7a38, 0x1c, 3, 4, twelfth_items},
};
// The routine saves r15-r31 below the stack pointer without a frame (stmw r15,-0x44(r1)).
constexpr std::uint32_t saved_first = 15, saved_at = 0x44;
}

// 0x100b44c0. No calls and no frame. Every table's row count and base come from TOC words the routine loads
// into r3-r12 and r18-r31 first, so the volatile ones it does not overwrite are part of what it leaves
// (r5, r8-r12 and the base and count words' registers); r0 ends with the last table's count, cr0 with the
// last loop test, and r3, r4, r6 and r7 with whatever the later tables' last row left in them.
void replace_pointers_with_ids(Cpu& c) {
    const auto sp = c.sp();
    for (unsigned n = saved_first; n < 32; ++n) c.put32(sp - saved_at + (n - saved_first) * 4, c.r(n));
    // The loads at the start: volatile registers keep the cell addresses they hold at the tables that use them.
    constexpr struct { unsigned reg; std::int32_t slot; } loads[] = {
        {18, -0x7568}, {19, -0x7b64}, {20, -0x755c}, {21, -0x7558}, {22, -0x7548}, {23, -0x7bbc}, {24, -0x753c},
        {25, -0x7538}, {26, -0x7528}, {27, -0x7ab4}, {28, -0x7a38}, {29, -0x7a34}, {30, -0x7b04}, {31, -0x7b98},
        {12, -0x7be0}, {11, -0x74c8}, {10, -0x74c4}, {9, -0x74c0}, {8, -0x79f4}, {7, -0x7b50}, {6, -0x7be8},
        {5, -0x7be4}, {4, -0x74bc}, {3, -0x79f8}};
    c.set(17, 0);
    for (const auto& [reg, slot] : loads) c.set(reg, c.toc(slot));
    for (const auto& table : tables) {
        const auto cell = c.toc(table.base_slot), count_at = c.toc(table.count_slot);
        std::int32_t row = 0;
        for (;; ++row) {
            c.set(17, std::uint32_t(row));
            c.set(0, c.u32(count_at));
            if (compare(c, row, std::int32_t(c.r(0))) >= 0) break;
            const auto offset_of_row = std::uint32_t(row) * table.stride;
            for (const auto& item : table.items) {
                for (std::uint32_t k = 0; k < std::max<std::uint32_t>(item.repeat, 1); ++k) {
                    const auto field = item.offset + 4 * k;
                    c.set(0, offset_of_row + (item.repeat ? 4 * k : 0));
                    auto record = c.u32(cell) + offset_of_row;
                    c.set(table.row_register, record);
                    const auto pointer = c.u32(record + field);
                    c.set(0, pointer);
                    std::uint32_t id = 0xffffffff;
                    if (compare_logical(c, pointer) != 0) id = c.u32(pointer + item.read);
                    c.set(table.answer_register, id);
                    record = c.u32(cell) + offset_of_row;
                    c.set(table.row_register, record);
                    c.put32(record + field, id);
                }
            }
        }
    }
    for (unsigned n = saved_first; n < 32; ++n) c.set(n, c.u32(sp - saved_at + (n - saved_first) * 4));
}
}
