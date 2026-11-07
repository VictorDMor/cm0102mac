// DATABASE.CPP's load block, part 3: the single block's allocation, its carving into tables and the default
// records written at the end of each table the file's records do not fill (database_load.hpp).
//
// With `single_block` the sizing pass leaves counts and byte sums: the tables the frame words describe are
// allocated apart, the rest (summed into r17) are one allocation that the carve cuts into consecutive tables in a
// fixed order, each sized from its count cell (frame words 0x1274-0x1290 keep the intermediate pointers and
// counts the original spills). The default records are the entries a table has beyond the file's: the placeholder
// clubs (127 of them), and for the people and the rows linked to them (0x44 and 0x2c bytes) the records past the
// loaded ones, each written field by field as the original does, five of them with dates from 0x10715880.
#include "database_load.hpp"
#include "database_load_detail.hpp"

namespace cm::aot::database {
using guest::compare;
using guest::compare_logical;
using namespace load;

namespace {
constexpr Assert totals{0x100a76d0, 0x2cc, 0x1f4, 0xab7};
constexpr std::uint32_t minus_one = 0xffffffff;
}

// 0x100a7548-0x100a7714: the tables allocated apart (each once, in this order), then the check that every one
// of them answered.
void Loader::sizing_totals() {
    const auto flagged = [&] {
        const auto flag = ext(c.u8(c.r(2) + std::uint32_t(toc::single_block_flag)));
        c.set(0, flag);
        return compare(c, std::int32_t(flag)) != 0;
    };
    // The two bodies of the original call the same routine from two addresses.
    const auto allocate = [&](std::uint32_t bytes, std::uint32_t flagged_bl, std::uint32_t plain_bl, std::int32_t table_slot,
                              bool test) {
        const bool is_flagged = test ? flagged() : true;
        c.set(3, bytes);
        call(is_flagged ? flagged_bl : plain_bl, callee::allocate);
        c.put32(c.toc(table_slot), c.r(3));
    };
    const auto local = [&](std::uint32_t offset) { return c.u32(frame.at(offset)); };
    allocate(local(local::rows_14), 0x100a7558, 0x100a7564, cells::rows_14_table, true);
    allocate(c.r(17), 0x100a7580, 0x100a758c, cells::rows_c8_table, true);
    allocate(local(local::clubs), 0x100a75a0, 0x100a75a0, cells::clubs_table, false);
    allocate(local(local::people), 0x100a75b4, 0x100a75b4, cells::people_table, false);
    allocate(local(local::rows_48), 0x100a75c4, 0x100a75c4, cells::rows_48_table, false);
    allocate(local(local::rows_44), 0x100a75d8, 0x100a75d8, cells::rows_44_table, false);
    allocate(local(local::rows_3c_b), 0x100a75f0, 0x100a75fc, cells::rows_3c_b_table, true);
    allocate(local(local::rows_1c), 0x100a7610, 0x100a7610, cells::rows_1c_table, false);
    c.set(3, c.u32(c.toc(cells::rows_2c_count)) * 0x2c);
    call(0x100a7628, callee::allocate);
    c.put32(c.toc(cells::rows_2c_table), c.r(3));
    bool all = true;
    for (const auto slot : {cells::rows_14_table, cells::rows_c8_table, cells::clubs_table, cells::people_table,
                            cells::rows_48_table, cells::rows_44_table, cells::rows_3c_b_table, cells::rows_1c_table,
                            cells::rows_2c_table}) {
        const auto table = c.u32(c.toc(slot));
        c.set(0, table);
        if (compare_logical(c, table) == 0) {
            all = false;
            break;
        }
    }
    if (!all) assertion(totals);
}

// 0x100a7714-0x100a7944: the tables carved out of the big allocation, in file order.
void Loader::carve() {
    const auto word = [&](std::int32_t slot) { return c.u32(c.toc(slot)); };
    const auto base = word(cells::rows_c8_table);
    const auto rows_c8 = word(cells::rows_c8_count), subs_a = word(cells::subs_a_count), subs_b = word(cells::subs_b_count),
               subs_c = word(cells::subs_c_count), rows_38 = word(cells::rows_38_count), rows_68 = word(cells::rows_68_count),
               placeholders = word(cells::placeholder_count), rows_6c = word(cells::rows_6c_count),
               rows_3c = word(cells::rows_3c_count), rows_128 = word(cells::rows_128_count),
               people = word(cells::people_count), rows_48 = word(cells::rows_48_count),
               rows_1c = word(cells::rows_1c_count),
               rows_1c_b = word(cells::rows_1c_b_count), clubs = word(cells::clubs_count),
               second = word(cells::second_count);
    const auto clubs_table = word(cells::clubs_table), small_table = word(cells::rows_1c_table);
    c.set(20, 1);
    // Each table starts where the previous one ends; the people-linked table keeps its 0x7f placeholders apart
    // (the 0x6c tables hold the placeholders' count less the later rows').
    std::uint32_t offset = rows_c8 * 0xc8;
    const auto subs_a_at = base + offset;                 // 0x2c-byte rows, three consecutive tables
    offset += subs_a * 0x2c;
    const auto subs_b_at = base + offset;
    offset += subs_b * 0x2c;
    const auto subs_c_at = base + offset;
    offset += subs_c * 0x2c;
    const auto rows_38_at = base + offset;
    offset += rows_38 * 0x38;
    const auto rows_68_at = base + offset;
    offset += rows_68 * 0x68;
    const auto placeholder_at = base + offset;
    offset += ((placeholders - rows_6c) - 0x7f) * 0x6c;
    const auto rows_6c_at = base + offset;
    offset += (rows_6c + 0x7f) * 0x6c;
    const auto rows_3c_at = base + offset;
    offset += rows_3c * 0x3c;
    const auto rows_128_at = base + offset;
    offset += rows_128 * 0x128;
    const auto rows_50_at = base + offset;
    // The spilled words of the original (the stack image).
    c.put32(frame.at(0x1274), subs_a_at);
    c.put32(frame.at(0x1284), rows_3c);
    c.put32(frame.at(0x1280), rows_6c + 0x7f);
    c.put32(frame.at(0x1288), rows_128);
    c.put32(frame.at(0x1278), subs_b_at);
    c.put32(frame.at(0x127c), subs_c_at);
    c.put32(frame.at(0x128c), people - 0x52d);
    c.put32(frame.at(0x1290), rows_48 - 0x52d);
    const auto put = [&](std::int32_t slot, std::uint32_t value) { c.put32(c.toc(slot), value); };
    put(cells::subs_a_table, subs_a_at);
    put(cells::subs_b_table, subs_b_at);
    put(cells::subs_c_table, subs_c_at);
    put(cells::rows_38_table, rows_38_at);
    put(cells::rows_68_table, rows_68_at);
    put(cells::placeholder_table, placeholder_at);
    put(cells::rows_6c_table, rows_6c_at);
    put(cells::rows_3c_table, rows_3c_at);
    put(cells::rows_128_table, rows_128_at);
    put(cells::rows_50_table, rows_50_at);
    // The tables allocated apart: the 0x1c-byte one's second view starts after the first's own rows, the second
    // club table after the first club table's.
    put(cells::rows_1c_b_table, small_table + (rows_1c - rows_1c_b) * 0x1c);
    put(cells::second_table, clubs_table + (clubs - second) * 0x248);
    c.put32(c.toc(cells::people_count), people - 0x52d);
    c.put32(c.toc(cells::rows_48_count), rows_48 - 0x52d);
    // What the registers are left holding: the third table's end, the 0x1c-byte rows' difference, the club tables'
    // difference and the base.
    c.set({{17, rows_38_at}, {19, (rows_1c - rows_1c_b) * 0x1c}, {23, clubs - second}, {24, base}});
}

// 0x100a7948-0x100a7aac: the 127 placeholder clubs at the end of the 0x6c-byte table, last first.
void Loader::placeholder_clubs() {
    const auto count = c.u32(c.toc(cells::placeholder_count));
    const auto table = c.u32(c.toc(cells::placeholder_table));
    for (std::uint32_t i = 1; i <= 0x7f; ++i) {
        c.set(20, i);
        const auto row = table + (count - i) * 0x6c;
        c.put32(row, count - i);
        c.put8(row + 0x1a, 0);
        c.put8(row + 0x4d, 0);
        c.put8(row + 0x14, 0);
        c.put8(row + 0x68, 0);
        c.put8(row + 0x69, 0);
        for (const auto offset : {0x4u, 0x8u, 0xcu, 0x10u}) c.put32(row + offset, minus_one);
        c.put16(row + 0x18, 0);
    }
    c.set(20, 0x80);
    c.set({{17, table}, {11, table}, {12, table}, {0, 0x7f}});
}

// A date through 0x10715880 into the frame, then copied into the row's five bytes.
void Loader::date_default(std::uint32_t bl, std::uint32_t local, std::uint32_t destination) {
    c.set(3, frame.at(local));
    call(bl, callee::date_default);
    c.put16(destination + 2, c.u16(frame.at(local) + 2));
    c.put16(destination, c.u16(frame.at(local)));
    c.put8(destination + 4, c.u8(frame.at(local) + 4));
}

namespace {
// The fields of a default 0x44-byte row linked to a person (all zero or -1; the first word its index).
void default_row_44(guest::Cpu& c, std::uint32_t row, std::uint32_t index) {
    c.put32(row, index);
    for (std::uint32_t offset = 0x24; offset <= 0x2c; offset += 2) c.put16(row + offset, 0);
    for (std::uint32_t offset = 0x2e; offset <= 0x43; ++offset) c.put8(row + offset, 0);
    for (std::uint32_t offset = 0x4; offset <= 0x20; offset += 4) c.put32(row + offset, minus_one);
}
// A default 0x2c-byte sub-row: a zero byte, its index, -1 and a one byte.
void default_row_2c(guest::Cpu& c, std::uint32_t row, std::uint32_t index) {
    c.put8(row, 0);
    c.put32(row + 0x20, index);
    c.put32(row + 0x24, minus_one);
    c.put8(row + 0x28, 1);
}
// The common fields of a default person row (0x70 bytes); the rest differ by loop.
void default_person_bytes(guest::Cpu& c, std::uint32_t row, std::uint32_t sixth) {
    c.put32(row + 0x4, 0);
    c.put32(row + 0x8, 0);
    c.put8(row + 0x61, 0);
    for (std::uint32_t offset = 0x63; offset <= 0x6a; ++offset) c.put8(row + offset, 0);
    c.put8(row + 0x6b, sixth);
    c.put8(row + 0x6c, 0);
    c.put32(row + 0x14, minus_one);
}
}

// 0x100a7ac0-0x100a8440: 0x5208 people past the language block's (a bias of r18 below the count), each with five
// dates, and the row linked to the person (0x44 bytes) with the same index.
void Loader::default_rows_biased() {
    const auto people = c.u32(c.toc(cells::people_count)), rows_44 = c.u32(c.toc(cells::rows_44_count));
    const auto person_table = c.u32(c.toc(cells::people_table)), row_table = c.u32(c.toc(cells::rows_44_table));
    for (std::uint32_t i = 1; i <= 0x5208; ++i) {
        c.set(20, i);
        const auto bias = c.r(18);
        const auto index = people - bias - i, row_index = rows_44 - bias - i;
        const auto person = person_table + index * 0x70;
        c.put32(person, index);
        c.put32(person + 0x28, minus_one);
        c.put32(person + 0x2c, minus_one);
        c.put32(person + 0x30, 0);
        date_default(0x100a7b48, 0x1ec, person + 0x34);
        c.put16(person + 0x5c, 0x7b3);
        c.put32(person + 0xc, minus_one);
        c.put32(person + 0x10, minus_one);
        c.put8(person + 0x5e, 0);
        c.put8(person + 0x5f, 0);
        c.put32(person + 0x20, minus_one);
        c.put8(person + 0x60, 0);
        date_default(0x100a7c64, 0x1e4, person + 0x3c);
        date_default(0x100a7ca0, 0x1dc, person + 0x44);
        c.put32(person + 0x24, minus_one);
        c.put8(person + 0x62, 0);
        date_default(0x100a7d1c, 0x1d4, person + 0x4c);
        date_default(0x100a7d58, 0x1cc, person + 0x54);
        default_person_bytes(c, person, 6);
        c.put32(person + 0x18, minus_one);
        c.put32(person + 0x1c, row_index);
        c.put8(person + 0x6d, 0);
        default_row_44(c, row_table + row_index * 0x44, row_index);
        c.set({{17, 0}, {19, row_table + row_index * 0x44}, {23, minus_one}, {11, 0}, {12, 0}});
    }
    c.set(20, 0x5209);
}

// 0x100a845c-0x100a8fd4: the same defaults for the r18 indexes below each count (the language block's own
// people), with the three 0x2c-byte sub-rows and, when the table exists, the 0x34-byte rows.
void Loader::default_rows() {
    const auto word = [&](std::int32_t slot) { return c.u32(c.toc(slot)); };
    const auto people = word(cells::people_count), subs_a = word(cells::subs_a_count), subs_b = word(cells::subs_b_count),
               subs_c = word(cells::subs_c_count), rows_34 = word(cells::rows_34_count), rows_44 = word(cells::rows_44_count);
    const auto person_table = word(cells::people_table), row_table = word(cells::rows_44_table),
               table_a = word(cells::subs_a_table), table_b = word(cells::subs_b_table),
               table_c = word(cells::subs_c_table);
    c.set(20, 1);
    while (compare(c, std::int32_t(c.r(20)), std::int32_t(c.r(18))) <= 0) {
        const auto i = c.r(20);
        const auto index = people - i;
        const auto person = person_table + index * 0x70;
        c.put32(person, index);
        c.put32(person + 0x28, subs_a - i);
        c.put32(person + 0x2c, subs_b - i);
        c.put32(person + 0x30, subs_c - i);
        date_default(0x100a84e0, 0x1c4, person + 0x34);
        c.put16(person + 0x5c, 0x7b3);
        c.put32(person + 0xc, minus_one);
        c.put32(person + 0x10, minus_one);
        c.put8(person + 0x5e, 0);
        c.put8(person + 0x5f, 0);
        c.put32(person + 0x20, minus_one);
        c.put8(person + 0x60, 0);
        date_default(0x100a85dc, 0x1bc, person + 0x3c);
        date_default(0x100a8614, 0x1b4, person + 0x44);
        c.put32(person + 0x24, minus_one);
        c.put8(person + 0x62, 0);
        date_default(0x100a8684, 0x1ac, person + 0x4c);
        date_default(0x100a86bc, 0x1a4, person + 0x54);
        default_person_bytes(c, person, 0);
        c.put32(person + 0x18, rows_34 - i);
        c.put32(person + 0x1c, rows_44 - i);
        c.put8(person + 0x6d, 0);
        default_row_2c(c, table_a + (subs_a - i) * 0x2c, subs_a - i);
        default_row_2c(c, table_b + (subs_b - i) * 0x2c, subs_b - i);
        default_row_2c(c, table_c + (subs_c - i) * 0x2c, subs_c - i);
        const auto row = row_table + (rows_44 - i) * 0x44;
        default_row_44(c, row, rows_44 - i);
        const auto extra = word(cells::rows_34_table);
        c.set(0, extra);
        if (compare_logical(c, extra) != 0) {
            const auto at = extra + (rows_34 - i) * 0x34;
            c.put32(at, rows_34 - i);
            for (std::uint32_t offset = 0x4; offset <= 0x30; offset += 4) c.put32(at + offset, minus_one);
            c.set({{17, at}, {19, extra}, {23, minus_one}, {24, 0}});
        } else {
            c.set({{17, 0}, {19, row}, {23, 0}, {24, 0}});
        }
        c.set(20, i + 1);
    }
}
}
