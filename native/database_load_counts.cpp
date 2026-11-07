// DATABASE.CPP's load block, part 2: the pass over the index entries that counts the tables' records (and, with
// separate tables, allocates each), and the single block's allocation and carving (database_load.hpp).
//
// Both passes are a jump on the entry's kind: kind 8 does nothing, and so does a kind above 0x16. Without
// `single_block` every kind adds its count to the table's count cell (or sets it) and allocates its table; with it
// the sizes are only summed (r17 and five frame words), the tables are allocated at the end in a handful of calls
// and the rest are carved out of the first of them.
#include "database_load.hpp"
#include "database_load_detail.hpp"

namespace cm::aot::database {
using guest::compare;
using guest::compare_logical;
using namespace load;

namespace {
// How a kind grows its count cell in the allocating pass.
enum class Growth { add, set, reduce, plain };
struct Kind {
    Growth growth;
    std::int32_t count_cell, table_cell;
    std::uint32_t record_size, alloc_bl;
    Assert report;
};
// Indexed by kind; kind 8 has none. Kind 6's second table (the extended records) is separate.
constexpr std::array<Kind, 23> kinds{{
    {Growth::add, cells::clubs_count, cells::clubs_table, 0x248, 0x100a63ec, {0x100a641c, 0x52c, 0x28c, 0x973}},
    {Growth::add, cells::second_count, cells::second_table, 0x248, 0x100a6494, {0x100a64c4, 0x50c, 0x284, 0x97c}},
    {Growth::add, cells::rows_3c_count, cells::rows_3c_table, 0x3c, 0x100a672c, {0x100a675c, 0x48c, 0x264, 0x9a0}},
    {Growth::add, cells::rows_c8_count, cells::rows_c8_table, 0xc8, 0x100a6004, {0x100a6034, 0x5ec, 0x2bc, 0x93d}},
    {Growth::set, cells::rows_128_count, cells::rows_128_table, 0x128, 0x100a67c8, {0x100a67f8, 0x46c, 0x25c, 0x9a9}},
    {Growth::set, cells::rows_50_count, cells::rows_50_table, 0x50, 0x100a6864, {0x100a6894, 0x44c, 0x254, 0x9b2}},
    {Growth::reduce, cells::people_count, cells::people_table, 0x70, 0x100a6908, {0x100a6930, 0x42c, 0x24c, 0x9bd}},
    {Growth::add, cells::rows_2c_count, cells::rows_2c_table, 0x2c, 0x100a60ac, {0x100a60dc, 0x5cc, 0x2b4, 0x946}},
    {},
    {Growth::plain, cells::rows_44_count, cells::rows_44_table, 0x44, 0x100a6ae4, {0x100a6b0c, 0x3cc, 0x234, 0x9e0}},
    {Growth::set, cells::rows_48_count, cells::rows_48_table, 0x48, 0x100a6b78, {0x100a6ba8, 0x3ac, 0x22c, 0x9e9}},
    {Growth::add, cells::rows_68_count, cells::rows_68_table, 0x68, 0x100a653c, {0x100a656c, 0x4ec, 0x27c, 0x985}},
    {Growth::add, cells::placeholder_count, cells::placeholder_table, 0x6c, 0x100a65dc, {0x100a660c, 0x4cc, 0x274, 0x98e}},
    {Growth::reduce, cells::subs_a_count, cells::subs_a_table, 0x2c, 0x100a6154, {0x100a6184, 0x5ac, 0x2ac, 0x94f}},
    {Growth::reduce, cells::subs_b_count, cells::subs_b_table, 0x2c, 0x100a61f8, {0x100a6228, 0x58c, 0x2a4, 0x958}},
    {Growth::reduce, cells::subs_c_count, cells::subs_c_table, 0x2c, 0x100a629c, {0x100a62cc, 0x56c, 0x29c, 0x961}},
    {Growth::add, cells::rows_6c_count, cells::rows_6c_table, 0x6c, 0x100a6684, {0x100a66b4, 0x4ac, 0x26c, 0x997}},
    {Growth::set, cells::rows_14_count, cells::rows_14_table, 0x14, 0x100a6c14, {0x100a6c44, 0x38c, 0x224, 0x9f4}},
    {Growth::add, cells::rows_3c_b_count, cells::rows_3c_b_table, 0x3c, 0x100a6cbc, {0x100a6cec, 0x36c, 0x21c, 0x9ff}},
    {Growth::add, cells::rows_1c_count, cells::rows_1c_table, 0x1c, 0x100a6d64, {0x100a6d94, 0x34c, 0x214, 0xa0a}},
    {Growth::add, cells::rows_1c_b_count, cells::rows_1c_b_table, 0x1c, 0x100a6e0c, {0x100a6e3c, 0x32c, 0x20c, 0xa13}},
    {Growth::add, cells::rows_38_count, cells::rows_38_table, 0x38, 0x100a6344, {0x100a6374, 0x54c, 0x294, 0x96a}},
    {Growth::plain, cells::rows_34_count, cells::rows_34_table, 0x34, 0x100a6a4c, {0x100a6a74, 0x3ec, 0x23c, 0x9d7}},
}};
constexpr Assert extended_records{0x100a69dc, 0x40c, 0x244, 0x9c9};
constexpr std::uint32_t extended_alloc_bl = 0x100a69b4, extended_record_size = 0x9d;
constexpr std::uint32_t extended_dry_bl = 0x100a72a4;
constexpr Assert extended_dry{0x100a72cc, 0x30c, 0x204, 0xa70}, rows_34_dry{0x100a7358, 0x2ec, 0x1fc, 0xa7d};
}

// A failed allocation reports through the assert macro, calls the abort routine and goes on.
void Loader::assertion(const Assert& a) {
    guest::report_assertion(c, {a.at, pool(), pool() + 0xd, pool() + 0x1a, frame.at(a.name), frame.at(a.extension), a.line});
    c.set({{0, 0}, {3, 0xffffffff}});
    call(a.at + 0x40, callee::abort_run);
}

void Loader::table_for(std::int32_t table_slot, std::uint32_t bl, std::uint32_t bytes, const Assert& a) {
    c.set(3, bytes);
    call(bl, callee::allocate);
    const auto cell_address = c.toc(table_slot);
    c.put32(cell_address, c.r(3));
    const auto table = c.u32(cell_address);
    c.set(0, table);
    if (compare_logical(c, table) == 0) assertion(a);
}

// The jump on an entry's kind: `cmplwi r0,0x16; bgt skip`, then the handler through the table (CTR).
bool Loader::each_kind_jump(std::int32_t table_slot, std::uint32_t kind) {
    c.set(0, kind);
    if (compare_logical(c, kind, entry::highest_kind) > 0) return false;
    const auto target = c.u32(c.toc(table_slot) + kind * 4);
    c.set({{0, kind << 2}, {3, target}});
    c.s.wr<4>(guest::reg::ctr, target);
    return true;
}

void Loader::count_kinds(bool sizing) {
    for (std::uint32_t i = 0; i < entry::count_of_entries; ++i) {
        c.set(20, i);
        if (sizing) {
            progress(0x100a6eb8);
        } else {
            progress(0x100a5fa0);
            c.put32(c.r(2) + std::uint32_t(toc::header_a), 0);
        }
        const auto at = entry_at(i);
        const auto kind = c.u32(at + entry::kind);
        if (each_kind_jump(sizing ? toc::sizing_jumps : toc::count_jumps, kind)) {
            if (sizing) size_kind(kind, at);
            else allocate_kind(kind, at);
        }
    }
    c.set(20, entry::count_of_entries);
}

// The allocating pass for one kind.
void Loader::allocate_kind(std::uint32_t kind, std::uint32_t at) {
    if (kind == 8 || kind >= kinds.size()) return;
    const auto& k = kinds[kind];
    const auto count_word = c.toc(k.count_cell);
    auto count = c.u32(at + entry::count);
    std::uint32_t allocated = count;
    switch (k.growth) {
    case Growth::add:
        c.put32(count_word, c.u32(count_word) + count);
        break;
    case Growth::set:
        c.put32(count_word, count);
        break;
    case Growth::reduce:
        // The entry itself shrinks by the second header word: the table is sized for what is left.
        count -= c.toc(toc::header_b);
        c.put32(at + entry::count, count);
        allocated = c.u32(at + entry::count);
        c.put32(count_word, count);
        break;
    case Growth::plain:
        // The count word is the entry's less the header word, but the table is sized for the entry's own.
        c.put32(count_word, count - c.toc(toc::header_b));
        break;
    }
    table_for(k.table_cell, k.alloc_bl, allocated * k.record_size, k.report);
    if (kind != 6 || c.u32(at + entry::flag) != 1) return;
    // The extended records: another reduction of the entry, the count in its own word, one more table.
    count = c.u32(at + entry::count) - c.toc(toc::header_b);
    c.put32(at + entry::count, count);
    allocated = c.u32(at + entry::count);
    c.put32(c.r(2) + std::uint32_t(toc::detail_count), count);
    c.set(3, allocated * extended_record_size);
    call(extended_alloc_bl, callee::allocate);
    c.put32(c.r(2) + std::uint32_t(toc::detail_table), c.r(3));
    const auto table = c.toc(toc::detail_table);
    c.set(0, table);
    if (compare_logical(c, table) == 0) assertion(extended_records);
}

// Adds `count` to the count cell, returning nothing (the new word is stored).
void Loader::grow(std::int32_t cell_slot, std::uint32_t count, bool add) {
    const auto word = c.toc(cell_slot);
    c.put32(word, add ? c.u32(word) + count : count);
}

// The sizing pass for one kind: the same counts, with the tables' bytes summed into r17 (and into the frame's
// words for the five tables allocated together at the end).
void Loader::size_kind(std::uint32_t kind, std::uint32_t at) {
    const auto count = c.u32(at + entry::count);
    const auto bias = c.r(18);
    const auto accumulate = [&](std::uint32_t bytes) { c.set(17, c.r(17) + bytes); };
    const auto frame_add = [&](std::uint32_t local_offset, std::uint32_t bytes) {
        c.put32(frame.at(local_offset), c.u32(frame.at(local_offset)) + bytes);
    };
    const bool extended_header = c.u8(frame.at(local::extended)) != 0;  // lbz; extsb. (a byte read as signed)
    switch (kind) {
    case 0:
        grow(cells::clubs_count, count, true);
        frame_add(local::clubs, count * 0x248);
        break;
    case 1:
        // Both club counts grow, but only the second table's records are summed.
        grow(cells::clubs_count, count, true);
        frame_add(local::clubs, count * 0x248);
        grow(cells::second_count, count, true);
        break;
    case 2: grow(cells::rows_3c_count, count, true); accumulate(count * 0x3c); break;
    case 3: grow(cells::rows_c8_count, count, true); accumulate(count * 0xc8); break;
    case 4: grow(cells::rows_128_count, count, false); accumulate(count * 0x128); break;
    case 5: grow(cells::rows_50_count, count, false); accumulate(count * 0x50); break;
    case 6: {
        // The people: the entry's count plus the bias and a constant that depends on the language block.
        const auto total = extended_header ? count + 0x52d + bias : count + bias + 0x5735;
        c.put32(c.toc(cells::people_count), total);
        const auto stored = c.u32(c.toc(cells::people_count));
        frame_add(local::people, stored * 0x70);
        if (c.u32(at + entry::flag) != 1) break;
        c.put32(c.r(2) + std::uint32_t(toc::detail_count), count);
        c.set(3, count * extended_record_size);
        call(extended_dry_bl, callee::allocate);
        c.put32(c.r(2) + std::uint32_t(toc::detail_table), c.r(3));
        const auto table = c.toc(toc::detail_table);
        c.set(0, table);
        if (compare_logical(c, table) == 0) assertion(extended_dry);
        break;
    }
    case 7: grow(cells::rows_2c_count, count, true); break;
    case 9: {
        const auto total = extended_header ? count + bias : count + bias + 0x5208;
        c.put32(c.toc(cells::rows_44_count), total);
        frame_add(local::rows_44, c.u32(c.toc(cells::rows_44_count)) * 0x44);
        break;
    }
    case 10:
        c.put32(c.toc(cells::rows_48_count), count + 0x52d);
        frame_add(local::rows_48, (count + 0x52d) * 0x48);
        break;
    case 11: grow(cells::rows_68_count, count, true); accumulate(count * 0x68); break;
    case 12:
        // The placeholder clubs: 0x7f more than the entry's count.
        grow(cells::placeholder_count, count + 0x7f, true);
        accumulate((count + 0x7f) * 0x6c);
        break;
    case 13: case 14: case 15: {
        const std::int32_t slot = kind == 13 ? cells::subs_a_count : kind == 14 ? cells::subs_b_count : cells::subs_c_count;
        const auto word = c.toc(slot);
        c.put32(word, c.u32(word) + count + bias);
        accumulate((bias + count) * 0x2c);
        break;
    }
    case 16:
        grow(cells::placeholder_count, count, true);
        grow(cells::rows_6c_count, count, true);
        accumulate(count * 0x6c);
        break;
    case 17:
        grow(cells::rows_14_count, count, false);
        frame_add(local::rows_14, count * 0x14);
        break;
    case 18:
        grow(cells::rows_3c_b_count, count, true);
        frame_add(local::rows_3c_b, count * 0x3c);
        break;
    case 19:
        grow(cells::rows_1c_count, count, true);
        frame_add(local::rows_1c, count * 0x1c);
        break;
    case 20:
        grow(cells::rows_1c_count, count, true);
        grow(cells::rows_1c_b_count, count, true);
        frame_add(local::rows_1c, count * 0x1c);
        break;
    case 21: grow(cells::rows_38_count, count, true); accumulate(count * 0x38); break;
    case 22: {
        c.put32(c.toc(cells::rows_34_count), count + bias);
        const auto stored = c.u32(c.toc(cells::rows_34_count));
        c.set(3, stored * 0x34);
        call(0x100a7330, callee::allocate);
        c.put32(c.toc(cells::rows_34_table), c.r(3));
        const auto table = c.u32(c.toc(cells::rows_34_table));
        c.set(0, table);
        if (compare_logical(c, table) == 0) assertion(rows_34_dry);
        break;
    }
    default: break;
    }
}
}
