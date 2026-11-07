// DATABASE.CPP's tables block, part 5: 0x1009efc0, which drops the staff 0x100a1950 did not keep from the staff table
// and from the three tables its rows point at, then adds generated ("virtual") staff (database_tables.hpp).
//
// Computed with C++ values. The volatile registers (r0, r3-r12), cr0/cr1/cr5-7 and XER[CA] are dead at every call
// here except the arguments each callee reads (checked with stubs that clobber them, as the real callees do); the
// non-volatile registers are kept where a callee saves them into the stack image (the allocator, the copy routine,
// 0x100a1950, the assert macro, the formatter: r13-r31; 0x100a0d80: r23-r31; 0x107156b0: r31). The floating-point
// registers and the FPSCR are left as the original leaves them.
#include "database_generation.hpp"
#include "database_tables.hpp"
#include "database_tables_detail.hpp"
#include <algorithm>
#include <array>
#include <initializer_list>

namespace cm::aot::database {
namespace {
using namespace tables;
using generation::Pool;
using guest::Frame;

constexpr std::uint32_t entry = 0x1009efc0;
constexpr std::uint32_t new_array = 0x107735f0, delete_array = 0x10773610, reallocate = 0x107751b0,
                        copy_block = 0x1077fc28 /* (source, destination, size) */, format_text = 0x1077b720,
                        quit = 0x1076f7d0, make_date = 0x107156b0 /* (out, day, year) */,
                        clear_date = 0x10715880 /* (out): a fixed date */;

// TOC slots of the tables this routine walks (cells holding the address of a base word or a count word).
namespace table {
constexpr std::int32_t staff_base = -0x7be0, staff_count = -0x7be4;
constexpr std::int32_t row48_base = -0x7b38, row48_count = -0x74d4;  // 0x48-byte rows, staff +0x14 (inferred: contracts)
constexpr std::int32_t row44_base = -0x79f8, row44_count = -0x79f4;  // 0x44-byte rows, staff +0x1c
constexpr std::int32_t row34_base = -0x74bc, row34_count = -0x74c0;  // 0x34-byte rows, staff +0x18
constexpr std::int32_t row14_base = -0x74c4, row14_count = -0x74c8;  // 0x14-byte rows with a staff pointer at +0x4
constexpr std::int32_t row3c_base = -0x74cc, row3c_count = -0x74d0;  // 0x3c-byte rows with staff ids at +0x14/+0x18/+0x1c
constexpr std::int32_t nations = -0x7b98, nation_count = -0x7b04;    // 0x128-byte nation records (+0x6c a counter)
}
namespace row {
constexpr std::uint32_t staff = 0x70, row48 = 0x48, row44 = 0x44, row34 = 0x34, row14 = 0x14, row3c = 0x3c,
                        nation = 0x128;
}
// A staff row's fields (inferred from the copies and tests; see choose_kept_staff for the rest).
namespace staff_field {
constexpr std::uint32_t id = 0x0, nation_record = 0xc, club_slot = 0x10, link48 = 0x14, link34 = 0x18, link44 = 0x1c,
                        club = 0x20, club_b = 0x24, name_a = 0x28, name_b = 0x2c, name_c = 0x30, birth = 0x34,
                        year = 0x5c, flag_5e = 0x5e, flag_5f = 0x5f, code = 0x60, role_code = 0x62;
// The five dates (inferred): a date is a halfword, a halfword and a byte, 5 bytes at these offsets.
constexpr std::uint32_t dates[] = {0x34, 0x3c, 0x44, 0x4c, 0x54};
}
// A nation record: the counter of the staff taken from it.
constexpr std::uint32_t nation_counter = 0x6c;
// A 0x48-byte row: its own index at +0 and flags at +0x4/+0x6 (inferred), zero after +0x8.
constexpr std::uint32_t row48_flag = 0x6;
constexpr std::uint32_t virtual_capacity = 0x52d;  // rows the reallocated tables have beyond the staff count
constexpr std::uint32_t reserved_rows = 0x11;      // the last rows are not for virtual staff (their copies move up)
constexpr std::uint32_t birth_year = 0x7be;
// The frame's own words.
namespace local {
constexpr std::uint32_t kept48 = 0x11c, kept34 = 0x118;                  // counters of copied rows
constexpr std::uint32_t date_birth = 0x58, date_1 = 0x50, date_2 = 0x48, date_3 = 0x40, date_4 = 0x38;  // date out words
constexpr std::uint32_t convert_high = 0x110, convert_low = 0x114, convert_result = 0x108;
}

// One of the three inline draws "a number below a bound": where the original keeps its result and which frame
// words the floating-point scaling of its wide arm uses.
struct Draw {
    unsigned result_register;
    std::uint32_t pool_back, range_back;
    std::uint32_t first_double, second_double, result_word;
};
constexpr Draw first_draw{13, 0x100a0058, 0x100a0130, 0x108, 0x110, 0x100};
constexpr Draw staff_draw{15, 0x100a049c, 0x100a0574, 0x100, 0x108, 0x110};
constexpr Draw club_draw{15, 0x100a0840, 0x100a0918, 0x100, 0x108, 0x110};

class Compaction {
public:
    Compaction(Cpu& cpu, const Frame& f) : c(cpu), frame(f), pool(generation::load_pool(cpu)), sp(f.base()) {}
    void run(std::uint32_t mode);

private:
    Cpu& c;
    const Frame& frame;
    Pool pool;
    std::uint32_t sp;
    std::uint32_t map = 0;  // r18: staff id -> new record address, or 0

    std::uint32_t cell(std::int32_t slot) const { return c.toc(slot); }
    std::uint32_t word(std::int32_t slot) const { return c.u32(cell(slot)); }
    std::uint32_t staff_base() const { return word(table::staff_base); }
    std::uint32_t mapped(std::uint32_t id) const { return c.u32(map + (id << 2)); }

    std::int32_t wide_draw(const Draw& d, std::int32_t bound);
    std::int32_t draw_below(const Draw& d, std::int32_t bound);
    bool multiple(std::uint32_t back, std::int32_t modulus);
    std::uint32_t remap_pointer(std::uint32_t address);
    void remap_rows(std::int32_t base_slot, std::int32_t count_slot, std::uint32_t stride,
                    std::initializer_list<std::uint32_t> fields);
    void remap_clubs();
    void remap_ids();
    void copy_row(std::uint32_t source, std::uint32_t destination, std::uint32_t size, std::uint32_t back);
    void compact_kept_staff();
    std::uint32_t virtual_count();
    void grow_tables();
    void copy_contract_rows();
    void count_nation_staff();
    void move_last_rows();
    void build_virtual_staff(std::uint32_t i);
    void copy_date(std::uint32_t from, std::uint32_t to);
    void assertion(std::uint32_t at, std::uint32_t line, std::uint32_t name, std::uint32_t extension);
};

// The wide arm of a draw (a bound beyond +-65535): wide_scale's instruction sequence (generation::wide_scale
// hard-codes its frame words, which differ here). The sample plus salt, reduced modulo 65535, scales
// bound / 65535 in floating point, and range_sample(bound * 10 / 65535) adds its answer divided by 10; the number is
// clamped to bound - 1 (for a negative bound that always happens).
std::int32_t Compaction::wide_draw(const Draw& d, std::int32_t bound) {
    using guest::Fpu;
    constexpr std::uint32_t conversion_high = 0x43300000, reciprocal = 0x80008001, word_range = 0xffff;
    const auto constants = pool.constants;
    c.set(3, std::uint32_t(bound) ^ 0x80000000u);
    c.set(0, conversion_high);
    c.put32(sp + d.first_double + 4, c.r(3));
    c.set(6, 0x80010000);
    const auto cursor = generation::load32(c, 7, pool.cursor_at);
    c.set(4, conversion_high);
    c.put32(sp + d.first_double, c.r(0));
    c.set(5, 0x10000);
    c.set_f(2, c.f64(constants + 0x18));
    c.set(3, 0x80010000);
    c.set_f(1, c.f64(sp + d.first_double));
    c.set(6, reciprocal);
    c.set_f(0, c.f64(constants + 0x10));
    c.set(5, word_range);
    Fpu fpu(c);
    c.set_f(1, fpu.fsub(c.f(1), c.f(2)));
    c.set(8, c.u16(cursor));
    const auto salt = generation::load32(c, 7, pool.salt_at);
    c.set(0, std::uint32_t(bound) * 10);
    c.set(3, reciprocal);
    c.put32(sp + d.second_double, c.r(4));
    c.set_f(2, fpu.fdiv(c.f(1), c.f(0)));
    c.set(7, c.r(8) + salt);
    c.set(4, mulhw(reciprocal, c.r(7)));
    c.set_f(1, c.f64(constants + 0x18));
    c.set(4, c.r(4) + c.r(7));
    c.set(4, srawi(c, c.r(4), 15));
    c.set(6, c.r(4) >> 31);
    c.set(4, c.r(4) + c.r(6));
    c.set(4, c.r(4) * c.r(5));
    c.set(d.result_register, c.r(7) - c.r(4));
    c.set(4, c.r(d.result_register) ^ 0x80000000u);
    c.put32(sp + d.second_double + 4, c.r(4));
    c.set(3, mulhw(c.r(3), c.r(0)));
    c.set_f(0, c.f64(sp + d.second_double));
    c.set_f(0, fpu.fsub(c.f(0), c.f(1)));
    c.set(0, c.r(3) + c.r(0));
    c.set(0, srawi(c, c.r(0), 15));
    c.set_f(0, fpu.fmul(c.f(2), c.f(0)));
    c.set(3, c.r(0) >> 31);
    c.put64(sp + d.result_word, fpu.fctiwz(0, c.f(0)));
    auto scaled = c.u32(sp + d.result_word + 4);
    c.set(3, c.r(0) + c.r(3));
    guest::enter(c, d.range_back, {c.r(3)});
    const auto next = std::int32_t(range_sample(c, std::int32_t(c.r(3))));
    // (answer + 5) / 10 by multiply-high, added to the scaled number.
    scaled += std::uint32_t((std::int64_t(next) + 5) / 10);
    return std::int32_t(scaled) > bound - 1 ? bound - 1 : std::int32_t(scaled);
}

// A number below `bound` (not 0) from the sample pool: the pool advances (a wrap draws), then for a bound within
// +-65535 |(sample + salt) % bound| (negative for a negative bound), else the wide arm.
std::int32_t Compaction::draw_below(const Draw& d, std::int32_t bound) {
    generation::advance(c, pool, d.pool_back);
    if (bound > generation::narrow_limit || bound < -generation::narrow_limit) return wide_draw(d, bound);
    return generation::draw_below(c, pool, bound, 3);
}

// The sample plus salt after one more pool advance, tested for being a multiple of `modulus`.
bool Compaction::multiple(std::uint32_t back, std::int32_t modulus) {
    generation::advance(c, pool, back);
    const auto sum = std::uint32_t(c.u16(c.u32(pool.cursor_at))) + c.u32(pool.salt_at);
    return std::int32_t(sum) % modulus == 0;
}

// Replaces the pointer at `address` by the new record address of the staff it points at (0 stays 0).
std::uint32_t Compaction::remap_pointer(std::uint32_t address) {
    const auto pointer = c.u32(address);
    const auto value = pointer == 0 ? 0 : mapped(c.u32(pointer));
    c.put32(address, value);
    return value;
}

// A table whose rows hold staff pointers at `fields`.
void Compaction::remap_rows(std::int32_t base_slot, std::int32_t count_slot, std::uint32_t stride,
                            std::initializer_list<std::uint32_t> fields) {
    const auto rows = std::int32_t(word(count_slot));
    for (std::int32_t i = 0; i < rows; ++i)
        for (const auto field : fields) remap_pointer(word(base_slot) + std::uint32_t(i) * stride + field);
}

// The club table: single pointers, then arrays of 3, 50, 5, 7 and 3 slots (staff lists). r19 ends as the inner
// loops' counter of the last array.
void Compaction::remap_clubs() {
    constexpr std::array single = {0x5cu, 0x60u, 0x64u, 0x68u, 0x6cu, 0x70u, 0x80u, 0x90u, 0x94u};
    struct Array {
        std::uint32_t first, slots;
    };
    constexpr std::array arrays = {Array{0x84, 3}, Array{0x98, 50}, Array{0x160, 5}, Array{0x174, 7}, Array{0x190, 3}};
    const auto rows = std::int32_t(word(slot::club_count));
    for (std::int32_t i = 0; i < rows; ++i) {
        const auto record = word(slot::club_table) + std::uint32_t(i) * club::stride;
        for (const auto field : single) remap_pointer(record + field);
        for (const auto& array : arrays)
            for (std::uint32_t k = 0; k < array.slots; ++k) remap_pointer(record + array.first + 4 * k);
    }
    if (rows > 0) c.set(19, 3);
}

// The 0x3c-byte rows hold staff ids (negative: none): each becomes the id its staff's new record holds, or -1 for a
// staff that was dropped. The id is read from the address the new record WILL have (before the compaction has moved
// anything there): whatever the old table holds in that slot.
void Compaction::remap_ids() {
    const auto rows = std::int32_t(word(table::row3c_count));
    for (std::int32_t i = 0; i < rows; ++i)
        for (const std::uint32_t field : {0x14u, 0x18u, 0x1cu}) {
            const auto record = word(table::row3c_base) + std::uint32_t(i) * row::row3c;
            const auto id = c.u32(record + field);
            c.put32(record + field, std::int32_t(id) >= 0 && mapped(id) != 0 ? c.u32(mapped(id)) : 0xffffffffu);
        }
}

// The copy routine (source, destination, size); the glue leaves the caller's TOC in the frame.
void Compaction::copy_row(std::uint32_t source, std::uint32_t destination, std::uint32_t size, std::uint32_t back) {
    c.set(3, source);
    c.set(4, destination);
    c.set(5, size);
    c.call(copy_block, back);
    c.set(2, c.u32(c.sp() + 0x14));
}

// The kept staff move to the front (each takes its new index as id, r13 counting them) and drag their rows of
// the three tables to the tables' write cursors (r14, r17, r16; r15 counts the 0x44-byte rows, the frame the
// others): the staff's pointer then points at the copy, whose first word is its new index.
void Compaction::compact_kept_staff() {
    const auto staff_cell = cell(table::staff_base), count_cell = cell(table::staff_count);
    std::int32_t i = 0;
    for (; i < std::int32_t(c.u32(count_cell)); ++i) {
        c.set(20, std::uint32_t(i));
        const auto target = mapped(std::uint32_t(i));
        if (target == 0) continue;
        copy_row(c.u32(staff_cell) + std::uint32_t(i) * row::staff, target, row::staff, 0x1009fc30);
        c.put32(target + staff_field::id, c.r(13));
        c.set(13, c.r(13) + 1);
        if (c.u32(target + staff_field::link48) != 0) {
            copy_row(c.u32(target + staff_field::link48), c.r(14), row::row48, 0x1009fc74);
            c.put32(c.r(14), c.u32(sp + local::kept48));
            c.put32(target + staff_field::link48, c.r(14));
            c.set(14, c.r(14) + row::row48);
            c.put32(sp + local::kept48, c.u32(sp + local::kept48) + 1);
        }
        if (c.u32(target + staff_field::link44) != 0) {
            copy_row(c.u32(target + staff_field::link44), c.r(17), row::row44, 0x1009fccc);
            c.put32(c.r(17), c.r(15));
            c.put32(target + staff_field::link44, c.r(17));
            c.set(15, c.r(15) + 1);
            c.set(17, c.r(17) + row::row44);
        }
        if (c.u32(target + staff_field::link34) != 0) {
            copy_row(c.u32(target + staff_field::link34), c.r(16), row::row34, 0x1009fd1c);
            c.put32(c.r(16), c.u32(sp + local::kept34));
            c.put32(target + staff_field::link34, c.r(16));
            c.set(16, c.r(16) + row::row34);
            c.put32(sp + local::kept34, c.u32(sp + local::kept34) + 1);
        }
    }
    c.set(20, std::uint32_t(i));  // the index the loop ended at (the count word may have changed meanwhile)
}

// The number of virtual staff, kept * share, converted through the frame as the original does; replaced when the
// tables would outgrow the staff count by count + 0x52d + kept (or 0 when that is negative): the original's quirk,
// not a clamp.
std::uint32_t Compaction::virtual_count() {
    using guest::Fpu;
    const auto count = c.u32(cell(table::staff_count)), kept = c.r(13);
    c.put32(sp + local::convert_low, kept ^ 0x80000000u);
    c.put32(sp + local::convert_high, 0x43300000);
    c.set_f(1, c.f64(pool.constants + 0x18));
    c.set_f(0, c.f64(sp + local::convert_high));
    Fpu fpu(c);
    c.set_f(0, fpu.fsub(c.f(0), c.f(1)));
    c.set_f(0, fpu.fmul(c.f(0), c.f(31)));
    c.put64(sp + local::convert_result, fpu.fctiwz(0, c.f(0)));
    auto virtual_staff = c.u32(sp + local::convert_result + 4);
    if (std::int32_t(virtual_staff + kept + virtual_capacity) > std::int32_t(count)) {
        virtual_staff = kept + (count + virtual_capacity);
        if (std::int32_t(virtual_staff) < 0) virtual_staff = 0;
    }
    c.set(14, virtual_staff);
    return virtual_staff;
}

// The tables grow by the virtual rows: the new counts are stored, then each table is reallocated and must stay where
// it is (else the assert macro fires and 0x1076f7d0 ends the game).
void Compaction::grow_tables() {
    const auto count_cell = cell(table::staff_count);
    const auto virtual_staff = c.r(14);
    c.put32(count_cell, c.r(13) + virtual_staff);
    c.put32(cell(table::row48_count), c.u32(frame.base() + local::kept48) + virtual_staff);
    c.put32(cell(table::row44_count), c.r(15) + 10);
    c.put32(cell(table::row34_count), c.u32(frame.base() + local::kept34) + virtual_staff);
    c.set(3, staff_base());
    c.set(4, (c.u32(count_cell) + virtual_capacity) * row::staff);
    c.call(reallocate, 0x1009fdfc);
    c.set(13, c.r(3));
    c.set(3, word(table::row48_base));
    c.set(4, (word(table::row48_count) + virtual_capacity) * row::row48);
    c.call(reallocate, 0x1009fe20);
    c.set(16, c.r(3));
    c.set(3, c.u32(c.r(31)));
    c.set(4, word(table::row44_count) * row::row44);
    c.call(reallocate, 0x1009fe3c);
    c.set(17, c.r(3));
    c.set(3, c.u32(c.r(30)));
    c.set(4, word(table::row34_count) * row::row34);
    c.call(reallocate, 0x1009fe58);
    c.set(19, c.r(3));
    if (c.r(13) != staff_base()) assertion(0x1009fe7c, 0x183, 0xe0, 0x78);
    if (c.r(16) != word(table::row48_base)) assertion(0x1009fef0, 0x184, 0xc0, 0x70);
    if (c.r(17) != c.u32(c.r(31))) assertion(0x1009ff60, 0x185, 0xa0, 0x68);
    if (c.r(19) != c.u32(c.r(30))) assertion(0x1009ffd0, 0x186, 0x80, 0x60);
}

// Ten rows of the 0x44-byte table, picked at random among the kept ones (row 0 when none), are copied to the new
// last rows.
void Compaction::copy_contract_rows() {
    for (std::uint32_t i = 0; i < 10; ++i) {
        c.set(20, i);
        const auto picked = c.r(15) == 0 ? 0 : std::uint32_t(draw_below(first_draw, std::int32_t(c.r(15))));
        c.set(13, picked);
        const auto base = c.u32(c.r(31));
        const auto source = base + picked * row::row44;
        const auto target = base + (word(table::row44_count) - 1 - i) * row::row44;
        for (std::uint32_t at = 0; at <= 0x20; at += 4) c.put32(target + at, c.u32(source + at));
        for (std::uint32_t at = 0x24; at <= 0x2c; at += 2) c.put16(target + at, c.u16(source + at));
        for (std::uint32_t at = 0x2e; at < row::row44; ++at) c.put8(target + at, c.u8(source + at));
    }
    c.set(20, 10);
    c.set(3, map);
    c.call(delete_array, 0x100a02f0);
}

// The nation records' counters are zeroed and counted again from the kept staff that have a 0x48-byte row.
void Compaction::count_nation_staff() {
    const auto first = c.u32(cell(table::nations));
    for (std::int32_t i = 0; i < std::int32_t(word(table::nation_count)); ++i)
        c.put32(first + std::uint32_t(i) * row::nation + nation_counter, 0);
    const auto kept = std::int32_t(c.u32(c.r(28))) - std::int32_t(c.r(14));
    for (std::int32_t i = 0; i < kept; ++i) {
        const auto record = staff_base() + std::uint32_t(i) * row::staff;
        const auto nation = c.u32(record + staff_field::nation_record);
        if (c.u32(record + staff_field::link48) != 0 && nation != 0) c.put32(nation + nation_counter, c.u32(nation + nation_counter) + 1);
    }
    c.set(13, staff_base() + std::uint32_t(std::max(kept, 0)) * row::staff);
}

// The last 16 staff move up by the virtual count; each id is its new index.
void Compaction::move_last_rows() {
    const auto count_cell = cell(table::staff_count);
    for (std::int32_t i = std::int32_t(c.u32(count_cell)) - 0x10; i < std::int32_t(c.u32(count_cell)); ++i) {
        c.set(20, std::uint32_t(i));
        copy_row(staff_base() + (std::uint32_t(i) - c.r(14)) * row::staff,
                 staff_base() + std::uint32_t(i) * row::staff, row::staff, 0x100a039c);
        c.put32(staff_base() + std::uint32_t(i) * row::staff, std::uint32_t(i));
    }
    c.set(20, c.u32(count_cell));
}

// A date out-word (a halfword, a halfword and a byte, as 0x107156b0 and 0x10715880 write them) copied into a row.
void Compaction::copy_date(std::uint32_t from, std::uint32_t to) {
    c.put16(to + 2, c.u16(from + 2));
    c.put16(to, c.u16(from));
    c.put8(to + 4, c.u8(from + 4));
}

// One virtual staff, built in the rows from the end backwards (past the moved ones).
void Compaction::build_virtual_staff(std::uint32_t i) {
    const auto count_cell = cell(table::staff_count);
    const auto index = c.u32(count_cell) - i - reserved_rows;
    const auto row_address = staff_base() + index * row::staff;
    c.put32(row_address + staff_field::id, index);
    for (const auto field : {staff_field::name_a, staff_field::name_b, staff_field::name_c})
        c.put32(row_address + field, 0xfffffffe);
    c.put16(row_address + staff_field::year, birth_year);
    // The birth date: a random day of 365 from the generator, in the year 1982 (0x7be).
    c.call(generation::random_entry, 0x100a0410);
    const auto day = std::uint32_t((std::int64_t(0x16d) * std::int32_t(c.r(3))) >> 15) & 0xffff;
    c.set(3, sp + local::date_birth);
    c.set(4, guest::extended(std::int16_t(day)));
    c.set(5, birth_year);
    c.call(make_date, 0x100a044c);
    copy_date(sp + local::date_birth, row_address + staff_field::birth);
    // The person it copies its nation from: a random staff among all but the last 16 (row 0 when there are none),
    // drawn again while that row has no nation record.
    for (;;) {
        const auto bound = std::int32_t(c.u32(count_cell) - c.r(14) - 0x10);
        const auto picked = bound == 0 ? 0 : std::uint32_t(draw_below(staff_draw, bound));
        c.put32(row_address + staff_field::nation_record,
                c.u32(staff_base() + picked * row::staff + staff_field::nation_record));
        if (c.u32(row_address + staff_field::nation_record) != 0) break;
    }
    const auto nation = c.u32(row_address + staff_field::nation_record);
    c.put32(nation + nation_counter, c.u32(nation + nation_counter) + 1);
    c.put32(row_address + staff_field::club_slot, 0);
    c.put8(row_address + staff_field::flag_5e, 0);
    c.put8(row_address + staff_field::flag_5f, 0);
    c.put32(row_address + staff_field::club, 0);
    c.put8(row_address + staff_field::code, 0);
    // Four fixed dates, each stored into the row (+0x3c, +0x44, +0x4c, +0x54).
    c.set(3, sp + local::date_1);
    c.call(clear_date, 0x100a0638);
    copy_date(sp + local::date_1, row_address + staff_field::dates[1]);
    c.set(3, sp + local::date_2);
    c.call(clear_date, 0x100a0658);
    copy_date(sp + local::date_2, row_address + staff_field::dates[2]);
    c.put32(row_address + staff_field::club_b, 0);
    c.put8(row_address + staff_field::role_code, 0xb);
    c.set(3, sp + local::date_3);
    c.call(clear_date, 0x100a0688);
    copy_date(sp + local::date_3, row_address + staff_field::dates[3]);
    c.set(3, sp + local::date_4);
    c.call(clear_date, 0x100a06a8);
    copy_date(sp + local::date_4, row_address + staff_field::dates[4]);
    for (const std::uint32_t at : {0x4u, 0x8u}) c.put32(row_address + at, 0);
    for (std::uint32_t at = 0x61; at <= 0x6c; ++at)
        if (at != staff_field::role_code) c.put8(row_address + at, at == 0x6a || at == 0x6b ? 2 : 0);
    c.put8(row_address + 0x6d, 0);
    // Its rows in the 0x48-byte and 0x34-byte tables: the new last rows, counting back from the end.
    const auto link48 = word(table::row48_base) + (word(table::row48_count) - i - 1) * row::row48;
    const auto link34 = c.u32(c.r(30)) + (word(table::row34_count) - i - 1) * row::row34;
    c.put32(row_address + staff_field::link48, link48);
    c.put32(row_address + staff_field::link34, link34);
    c.put32(row_address + staff_field::link44, 0);
    c.put32(link34, word(table::row34_count) - i - 1);
    c.put32(link34 + 4, 0);
    c.set(26, 0);  // r23-r31 are what 0x100a0d80 saves: the original clears r26 before the club search
    // One time in ten, the 0x34-byte row names the best of 50 random clubs of the person's nation.
    if (multiple(0x100a07c0, 10)) {
        std::uint32_t best = 0;
        for (unsigned attempt = 0; attempt < 50; ++attempt) {
            const auto clubs = std::int32_t(word(slot::club_count));
            const auto picked = clubs == 0 ? 0 : std::uint32_t(draw_below(club_draw, clubs));
            const auto club_address = c.u32(c.r(27)) + picked * club::stride;
            if (c.u32(club_address + club::nation) != c.u32(row_address + staff_field::nation_record)) continue;
            if (best == 0 || c.i16(club_address + club::rating) + std::int32_t(c.u32(club_address + 8)) >
                                 c.i16(best + club::rating) + std::int32_t(c.u32(best + 8)))
                best = club_address;
        }
        c.put32(link34 + 4, best);
    }
    for (std::uint32_t at = 8; at <= 0x30; at += 4) c.put32(link34 + at, 0);
    // The work then goes on in the 0x48-byte row.
    c.put32(link48, word(table::row48_count) - i - 1);
    c.put8(link48 + 0xe, 0);
    c.put16(link48 + 4, 0);
    // Its two flags from the generator: a multiple of 100 gives -2, else a multiple of 50 gives -1, anything else 0.
    c.put16(link48 + row48_flag, multiple(0x100a0aa4, 100) ? 0xfffe : multiple(0x100a0b0c, 50) ? 0xffff : 0);
    // The rest of the row is zero.
    for (const std::uint32_t at : {8u, 0xau, 0xcu}) c.put16(link48 + at, 0);
    for (std::uint32_t at = 0xf; at <= 0x44; ++at) c.put8(link48 + at, 0);
    c.set(13, link48);
    c.set(31, 0);
}

// The assert macro at `at` (the first call), then the original's way out: this routine's copy of the macro has two
// more instructions between the calls than the usual one, so the return addresses differ.
void Compaction::assertion(std::uint32_t at, std::uint32_t line, std::uint32_t name, std::uint32_t extension) {
    const auto text = cell(slot::text_pool);
    guest::report_assertion(c,
                            {.at = at, .file = text, .format = text + 0xd, .title = text + 0x1a, .name = frame.at(name),
                             .extension = frame.at(extension), .line = line},
                            [&](std::uint32_t target, std::uint32_t) {
                                const auto back = target == 0x10739890 ? at + 4 : target == 0x10768810 ? at + 8
                                                  : target == format_text ? at + 0x28 : at + 0x3c;
                                c.call(target, back);
                            });
    c.set(4, cell(slot::success));
    c.set(0, 0);
    c.set(3, 0xffffffff);
    c.call(quit, at + 0x50);
}

void Compaction::run(std::uint32_t mode) {
    const auto count_cell = cell(table::staff_count);
    c.set(19, mode);
    c.set(30, cell(table::row34_base));
    c.set(31, cell(table::row44_base));
    c.set({{21, pool.salt_at}, {22, pool.start}, {23, pool.end_at}, {24, pool.cursor_at},
           {25, cell(table::staff_base)}, {26, cell(table::row3c_base)}, {27, cell(slot::club_table)},
           {28, count_cell}, {29, pool.constants}});
    c.set(13, 0);
    c.set(15, 0);
    c.set(14, word(table::row48_base));
    c.set(17, c.u32(c.r(31)));
    c.set(16, c.u32(c.r(30)));
    c.put32(frame.base() + local::kept48, 0);
    c.put32(frame.base() + local::kept34, 0);
    // The share of virtual staff: the first constant for the small modes (1 and 2), else the second.
    const bool small = std::int32_t(mode) == 2 || std::int32_t(mode) == 1;
    c.set_f(31, c.f64(pool.constants + (small ? 0 : 8)));

    // 1. The map from staff id to new record address; 0x100a1950 fills it.
    c.set(3, c.u32(count_cell) << 2);
    c.call(new_array, 0x1009f068);
    c.set(18, c.r(3));
    if (guest::compare(c, std::int32_t(c.r(18))) == 0) return;  // `or.`: cr0 stays when the routine leaves here
    map = c.r(18);
    guest::enter(c, 0x1009f07c, {map, generation::extsb(mode)});
    choose_kept_staff(c, c.r(3), c.r(4));
    if (generation::extsb(c.r(3)) == 0) {
        c.set(3, map);
        c.call(delete_array, 0x1009f08c);
        return;
    }

    // 2. The staff pointers of the other tables.
    remap_clubs();
    remap_rows(table::row44_base, table::row44_count, row::row44, {0x4, 0x8, 0xc, 0x10, 0x14, 0x18, 0x1c, 0x20});
    remap_rows(table::row34_base, table::row34_count, row::row34, {0x1c, 0x20, 0x24, 0x28, 0x2c, 0x30});
    remap_rows(table::row14_base, table::row14_count, row::row14, {0x4});
    remap_ids();

    // 3. The kept staff move to the front; the virtual count; the tables grow.
    compact_kept_staff();
    virtual_count();
    grow_tables();
    copy_contract_rows();

    // 4. The nation counters, the last rows, the virtual staff.
    count_nation_staff();
    move_last_rows();
    const auto virtual_staff = std::int32_t(c.r(14));
    c.set(20, 0);  // the loop's index starts here even when there is nothing to build
    for (std::int32_t i = 0; i < virtual_staff; ++i) build_virtual_staff(std::uint32_t(i));
    if (virtual_staff > 0) {
        // What the last virtual staff leaves in the registers the summaries' callee saves.
        c.set({{15, 0}, {16, 0}, {17, 0}, {18, 0}, {19, 0}, {26, 0}, {31, 0}});
        c.set(20, std::uint32_t(virtual_staff));
    }

    // 5. The two summaries.
    c.set(3, cell(slot::result_buffer));
    c.set(4, cell(slot::text_pool) + 0x20);
    c.set(5, c.u32(count_cell));
    c.set(6, virtual_capacity);
    c.call(format_text, 0x100a0d48);
    c.set(3, cell(slot::result_buffer));
    c.set(4, cell(slot::text_pool) + 0x48);
    c.set(5, word(table::row48_count));
    c.call(format_text, 0x100a0d60);
}
}

// 0x1009efc0. A frame of 0x180 bytes saving r13-r31, f31 and LR. `mode` (the whole register; its low byte is passed
// on to 0x100a1950, and 1 or 2 pick the share of virtual staff in f31) selects which staff stay. Registers: r13 the
// kept staff, r14/r17/r16 the write cursors of the three tables, r15 the kept 0x44-byte rows, r18 the map from
// staff id to new record address, r28 the count cell. Steps:
//  1. 0x100a1950 fills the map; a failed allocation or a call that kept nothing leaves with that result;
//  2. the staff pointers of the club table, the 0x44-byte, 0x34-byte and 0x14-byte tables and the staff ids of the
//     0x3c-byte table are rewritten through the map;
//  3. the kept staff move to the front and drag their rows of the three tables with them; the virtual count is
//     (kept * share) as an int (see Compaction::virtual_count); the four tables are reallocated; ten random
//     0x44-byte rows are copied to the new last rows and the map is released;
//  4. the nation records' counters (+0x6c) are zeroed and counted again from the kept staff, the last 16 staff move
//     up by the virtual count, and each virtual staff is built in the rows before them from a random kept staff's
//     nation record and generated dates, with new rows of the 0x48-byte and 0x34-byte tables;
//  5. "Staff table has %d(+%d Virtual) entries" and "Contract table has %d entries" are formatted.
void compact_staff_table(Cpu& c, std::uint32_t mode) {
    c.set(0, c.lr());  // mfspr r0,LR
    Frame frame(c, entry, 0x180, {.first_gpr = 13, .gpr_offset = -0x5c, .first_fpr = 31, .fpr_offset = -8});
    Compaction(c, frame).run(mode);
}
}
