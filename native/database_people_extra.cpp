// DATABASE.CPP's people block, part "extra": 0x100a3db0, the non-playing staff generated for clubs
// (database_people_extra.hpp).
#include "database_generation.hpp"
#include "database_generation_detail.hpp"
#include "database_people_detail.hpp"
#include "database_people_extra.hpp"
#include "database_records.hpp"
#include <algorithm>

namespace cm::aot::database {
namespace {
using guest::compare;
using guest::compare_logical;
using guest::Frame;
using guest::Kept;
using generation::Pool;
using people::extsb;
using people::mulhw;

constexpr std::uint32_t entry = 0x100a3db0;
constexpr std::uint32_t random_entry = 0x1077b770, set_field_entry = 0x107158a0, format_entry = 0x1077b720;

// TOC slots (offsets from r2).
namespace slot {
constexpr std::int32_t player_nation = -0x7a10, club_table = -0x7be8, person_table = -0x7be0, person_count = -0x7be4,
                       club_count = -0x7b50, success = -0x7bac, message = -0x7ba8, text_pool = -0x5618,
                       result_buffer = -0x7bb8;
}
// Offsets in a club record (0x248 bytes), its linked record (inferred: the club's own record; +0x68 its
// nation, +0x119 its category) and a generated person (0x70 bytes).
namespace club {
constexpr std::uint32_t stride = 0x248, linked = 0x2c, second_link = 0x30, id_word = 0x90, rating = 0x1ec;
}
namespace linked {
constexpr std::uint32_t nation = 0x68, flag = 0x120;
}
namespace person {
constexpr std::uint32_t stride = 0x70, owner_slot = 0x20, club = 0x24, first_name = 0x28, second_name = 0x2c,
                        third_name = 0x30, values = 0x34, role_copy = 0x60, role = 0x62, linked = 0xc, generated = 0x6b;
}
constexpr std::int32_t rating_bias = 500, rating_gate = 500, person_gate = 7500;
constexpr std::int32_t generated_limit = 0x5208;  // 21000 people before the assert
constexpr std::int32_t reserved_rows = 0x11;      // the table's last 17 rows are not for generated people

// What every pass shares: the frame, the pool words and the two loop registers (r30 the index, r31 the number
// generated so far).
struct Run {
    Cpu& c;
    const Frame& frame;
    Pool pool;
    std::uint32_t text, count_at, table_at, club_count_at;
    Kept<30> index;
    Kept<31> made;
};

// The assert macro at `site`, then the original's way out: r0 = 0, r3 = 0, the success flag cleared.
void assertion(Run& x, std::uint32_t site, std::uint32_t line, std::uint32_t name, std::uint32_t extension) {
    guest::report_assertion(x.c, {site, x.text, x.text + 0xd, x.text + 0x1a, x.frame.at(name), x.frame.at(extension), line});
}

// Which registers a new person's draws use, and where its code sits (`base` the first instruction of the pass's
// club loop, for the return addresses).
struct Staff {
    std::uint32_t base, list, slots, role;
    unsigned count_reg, target_reg, first_reg, second_reg;
    std::uint32_t line, name, extension;
};

// An inline draw: the sample at the pool cursor plus the salt, taken modulo `modulus` by the compiler's
// multiply-high division (remainder = sum - trunc(sum / modulus) * modulus, left in the original's `result`
// register, which the later calls save). The cursor moves on two bytes first; past the end it restarts and draws
// a new salt AFTER the sum was formed (so this draw used the old salt). cr0 holds the cursor/end compare and
// XER[CA] the shift's carry, as the original leaves them.
std::uint32_t draw_modulo(Cpu& c, const Pool& pool, std::uint32_t magic, unsigned shift, std::uint32_t modulus,
                          unsigned result, std::uint32_t back) {
    const auto cursor = c.u32(pool.cursor_at);
    const auto sample = c.u16(cursor);
    const auto sum = sample + c.u32(pool.salt_at);
    const auto end = c.u32(pool.end_at);
    const auto next = cursor + 2;
    c.put32(pool.cursor_at, next);
    const bool wrapped = compare_logical(c, next, end) > 0;
    const auto scaled = generation::srawi(c, mulhw(magic, sum), shift);
    const auto remainder = sum - (scaled + (scaled >> 31)) * modulus;
    c.set(result, remainder);
    if (wrapped) {
        c.put32(pool.cursor_at, pool.start);
        c.call(random_entry, back);
        c.set(0, c.r(3) & 0xffff);
        c.put32(pool.salt_at, c.r(0));
    }
    return remainder;
}

// The three draws of a new person and the call that sets its +0x34 field: a 27-way and an 11-way draw (the
// first is the appearance's base value, the second a byte) and a 26-way one for the third argument. The call
// sees r3-r8 (the field's address, the first value plus one, the second value as a signed byte, the third
// value plus 0x794 as 16 bits, -1, and a leftover that differs in the role 5 loop: its draw keeps the sum in r8
// where the others keep the quotient's sign).
void draw_values(Run& x, std::uint32_t row, std::uint32_t base, unsigned first_register, unsigned second_register,
                 bool role_five) {
    Cpu& c = x.c;
    generation::advance(c, x.pool, base + 0x1e0);
    const auto first = draw_modulo(c, x.pool, 0x4bda12f7, 3, 27, first_register, base + 0x238);
    const auto second = draw_modulo(c, x.pool, 0x2e8ba2e9, 1, 11, second_register, base + 0x290);
    const auto cursor = c.u32(x.pool.cursor_at);
    const auto sum = c.u16(cursor) + c.u32(x.pool.salt_at);
    const auto scaled = generation::srawi(c, mulhw(0x4ec4ec4f, sum), 3);
    const auto third = sum - (scaled + (scaled >> 31)) * 26 + 0x794;
    c.set({{3, row + person::values}, {4, guest::extended(std::int16_t(first + 1))}, {5, extsb(second)},
           {6, third & 0xffff}, {7, 0xffffffff}, {8, role_five ? sum : scaled >> 31}, {0, third}});
    c.call(set_field_entry, base + 0x2e8);
}

// The row of the next new person (the table is filled from the end), the club's linked record named by the
// three pickers and stored in the row. Returns the row.
std::uint32_t name_new_person(Run& x, std::uint32_t club_at, std::uint32_t base) {
    Cpu& c = x.c;
    const auto record = c.u32(club_at + club::linked);
    c.set(3, record);
    guest::enter(c, base + 0x194, {record});
    const auto row = c.r(28);
    pick_name_for_person_filtered(c, record);
    c.put32(row + person::first_name, c.r(3));
    c.set(3, c.u32(club_at + club::linked));
    guest::enter(c, base + 0x1a0, {c.r(3)});
    pick_name_for_person(c, c.r(3));
    c.put32(row + person::second_name, c.r(3));
    c.set(3, c.u32(club_at + club::linked));
    guest::enter(c, base + 0x1ac, {c.r(3)});
    pick_unmarked_record(c);
    c.put32(row + person::third_name, c.r(3));
    c.set(0, 1);
    c.set(3, c.u32(club_at + club::linked));
    c.put32(row + person::linked, c.r(3));
    c.put8(row + person::generated, c.r(0));
    return row;
}

// r28 = the row for the next person: the table is filled from the end, after the last 17 rows.
std::uint32_t next_row(Run& x, std::uint32_t club_at) {
    Cpu& c = x.c;
    const auto count = c.u32(x.count_at);
    const auto table = c.u32(x.table_at);
    c.set({{3, c.u32(club_at + club::linked)}, {4, table}, {0, (count - reserved_rows - x.made) * person::stride}});
    c.set(28, table + (count - reserved_rows - x.made) * person::stride);
    return c.r(28);
}

// The nation gate every pass starts with: a club whose linked record is of the player's nation is skipped
// when its rating is 500 or LESS (cmpwi 0x1f4; ble).
bool skipped_for_nation(Cpu& c, std::uint32_t club_at) {
    if (compare_logical(c, c.u32(club_at + club::linked)) == 0) return false;
    const auto nation = c.u32(c.u32(club_at + club::linked) + linked::nation);
    if (compare(c, std::int32_t(nation), std::int32_t(c.u32(c.toc(slot::player_nation)))) != 0) return false;
    return compare(c, c.i16(club_at + club::rating), rating_gate) <= 0;
}

// The club's gates for every pass: the nation gate, then 0x100c7d00 must answer 0 (as a signed byte), a linked
// record with its flag byte set, and 0x100ce320 either name no club or leave a zero found byte.
bool club_is_used(Run& x, std::uint32_t club_at, std::uint32_t base) {
    Cpu& c = x.c;
    if (skipped_for_nation(c, club_at)) return false;
    guest::enter(c, base + 0x40, {club_at});
    range_position_doubled(c, club_at);
    if (compare(c, std::int32_t(extsb(c.r(3)))) != 0) return false;
    if (compare_logical(c, c.u32(club_at + club::linked)) == 0) return false;
    if (compare(c, std::int32_t(extsb(c.u8(c.u32(club_at + club::linked) + linked::flag)))) == 0) return false;
    const auto found = x.frame.at(0x38);
    guest::enter(c, base + 0x74, {club_at, found, 1});
    if (compare_logical(c, club_of_code(c, club_at, found, 1)) != 0 &&
        compare(c, std::int32_t(extsb(c.u8(found)))) != 0)
        return false;
    return true;
}

// The assert for more than 21000 people made, then the original's way out: r0 = r3 = 0, success flag cleared.
bool refuse_more_people(Run& x, std::uint32_t site, std::uint32_t line, std::uint32_t name, std::uint32_t extension,
                        std::uint32_t pool_offset) {
    assertion(x, site + pool_offset, line, name, extension);
    x.c.set({{0, 0}, {3, 0}});
    x.c.put8(x.c.toc(slot::success), 0);
    return false;
}

// One pass over the clubs for one role: the staff a club needs is (rating + 500) * slots / 10000, within 1 and
// the slots; each person missing from its list is created and put in the first empty slot. Answers false after
// the assert (21000 = 0x5208 people).
bool add_staff(Run& x, const Staff& s) {
    Cpu& c = x.c;
    for (x.index = 0;; x.index = x.index + 1) {
        const auto clubs = c.u32(c.toc(slot::club_count));
        if (compare(c, std::int32_t(x.index), std::int32_t(clubs)) >= 0) break;
        const auto club_at = c.u32(c.toc(slot::club_table)) + x.index * club::stride;
        c.set(29, club_at);
        if (!club_is_used(x, club_at, s.base)) continue;
        // The people already listed.
        c.set(s.count_reg, 0);
        for (std::uint32_t slot = 0; compare(c, std::int32_t(slot), std::int32_t(s.slots)) < 0; ++slot)
            if (compare_logical(c, c.u32(club_at + s.list + 4 * slot)) != 0) c.set(s.count_reg, c.r(s.count_reg) + 1);
        // The wanted number: (rating + 500) * slots / 10000 (as the compiler's multiply-high), at least 1 and at
        // most the slots.
        const auto rating = std::int32_t(c.i16(club_at + club::rating));
        const auto scaled = generation::srawi(c, mulhw(0x68db8bad, std::uint32_t((rating + rating_bias) * std::int32_t(s.slots))), 12);
        const auto wanted = std::int32_t(scaled + (scaled >> 31));
        const auto target = std::min(std::max(wanted, 1), std::int32_t(s.slots));
        c.set(s.target_reg, std::uint32_t(target));
        for (;;) {
            const auto before = c.r(s.count_reg);
            c.set(s.count_reg, before + 1);
            if (compare(c, std::int32_t(before), target) >= 0) break;
            if (compare(c, std::int32_t(x.made), generated_limit) >= 0)
                return refuse_more_people(x, s.base, s.line, s.name, s.extension, 0x130);
            const auto row = next_row(x, club_at);
            name_new_person(x, club_at, s.base);
            draw_values(x, row, s.base, s.first_reg, s.second_reg, false);
            // The first empty slot of the list takes the new person.
            for (std::uint32_t slot = 0; compare(c, std::int32_t(slot), std::int32_t(s.slots)) < 0; ++slot) {
                const auto at = club_at + s.list + 4 * slot;
                if (compare_logical(c, c.u32(at)) != 0) continue;
                c.put32(at, row);
                c.put32(row + person::club, club_at);
                c.put8(row + person::role, s.role);
                break;
            }
            x.made = x.made + 1;
        }
    }
    return true;
}

// The pass for the one role 5 person (inferred: a club's chief of staff) of each club whose id word (+0x90) is
// empty. The gates differ from the other passes: after the nation gate and 0x100c7d00, a club that has no
// linked record, second link, flag byte or rating needs a rating above 7500 instead (and a club for which
// 0x100c7d00 answers non-zero is not gated at all).
bool add_role_five(Run& x) {
    constexpr std::uint32_t base = 0x100a48d0;  // the draws' return addresses are base + 0x1e0 ...
    Cpu& c = x.c;
    for (x.index = 0;; x.index = x.index + 1) {
        const auto clubs = c.u32(c.toc(slot::club_count));
        if (compare(c, std::int32_t(x.index), std::int32_t(clubs)) >= 0) break;
        const auto club_at = c.u32(c.toc(slot::club_table)) + x.index * club::stride;
        c.set(29, club_at);
        if (skipped_for_nation(c, club_at)) continue;
        guest::enter(c, 0x100a491c, {club_at});
        range_position_doubled(c, club_at);
        if (compare(c, std::int32_t(extsb(c.r(3)))) == 0) {
            const auto rating = [&] { return std::int32_t(c.i16(club_at + club::rating)); };
            bool complete = false;
            if (compare_logical(c, c.u32(club_at + club::linked)) != 0 &&
                compare_logical(c, c.u32(club_at + club::second_link)) != 0 &&
                compare(c, std::int32_t(extsb(c.u8(c.u32(club_at + club::linked) + linked::flag)))) != 0)
                complete = compare(c, rating()) != 0;
            if (!complete && compare(c, rating(), person_gate) <= 0) continue;
        }
        if (compare_logical(c, c.u32(club_at + club::id_word)) != 0) continue;
        if (compare(c, std::int32_t(x.made), generated_limit) >= 0)
            return refuse_more_people(x, 0x100a49d4, 0x729, 0x64, 0x3c, 0);
        const auto record = c.u32(club_at + club::linked);
        c.set(15, record);
        const auto row = next_row(x, club_at);
        guest::enter(c, 0x100a4a48, {record});
        c.put32(row + person::first_name, pick_name_for_person_filtered(c, record));
        guest::enter(c, 0x100a4a58, {record});
        c.put32(row + person::second_name, pick_name_for_person(c, record));
        guest::enter(c, 0x100a4a64, {c.u32(club_at + club::linked)});
        c.put32(row + person::third_name, pick_unmarked_record(c));
        c.put8(row + person::generated, 1);
        c.put8(row + person::role, 5);
        c.put32(row + person::club, 0);
        c.put32(row + person::owner_slot, 0);
        c.put8(row + person::role_copy, 5);
        draw_values(x, row, base, 15, 16, true);
        x.made = x.made + 1;
    }
    return true;
}

// The first phase: every generated role 8 person must be in the five-slot list (+0x160) of its club. The first
// that is not stops the phase after the assert (the index jumps to the count) and the success flag is cleared.
void check_listed_staff(Run& x) {
    constexpr std::uint32_t site = 0x100a3e64, line = 0x624, name = 0xe4, extension = 0x5c;
    constexpr std::uint32_t role_eight = 8, list = 0x160, slots = 5;
    Cpu& c = x.c;
    for (x.index = 0;; x.index = x.index + 1) {
        const auto count = c.u32(x.count_at);
        if (compare(c, std::int32_t(x.index), std::int32_t(count)) >= 0) break;
        const auto row = c.u32(x.table_at) + x.index * person::stride;
        c.set(28, row);
        if (compare(c, std::int32_t(c.u8(row + person::role)), role_eight) != 0) continue;
        if (compare_logical(c, c.u32(row + person::club)) == 0) continue;
        std::uint32_t slot = 0;
        while (compare(c, std::int32_t(slot), slots) < 0 &&
               compare_logical(c, c.u32(c.u32(row + person::club) + 4 * slot + list), row) != 0)
            ++slot;
        if (compare(c, std::int32_t(slot), slots) != 0) continue;
        assertion(x, site, line, name, extension);
        c.put8(c.toc(slot::success), 0);
        x.index = c.u32(x.count_at) - 1;
    }
}
}

// 0x100a3db0. A frame of 0x160 saving r13-r31. The TOC words it keeps in r13-r27 (the callees' frames save
// them), r28 the row being made, r29 the club, r30 the loop index, r31 the number of people made. Answers 1; 0
// after the assert for more than 21000 people. Ends with "generated %ld extra non playing staff".
std::uint32_t generate_extra_staff(Cpu& c) {
    c.set(0, c.lr());
    Frame frame(c, entry, 0x160, {.first_gpr = 13});
    guest::keep(c, {{13, c.toc(slot::player_nation)}, {14, c.toc(slot::club_table)}, {19, c.toc(-0x7b90)},
                    {20, c.toc(-0x7b8c)}, {21, c.toc(-0x7b88)}, {22, c.toc(-0x7b84)}, {23, c.toc(slot::success)},
                    {24, c.toc(slot::message)}, {25, c.toc(slot::text_pool)}, {26, c.toc(slot::person_table)},
                    {27, c.toc(slot::person_count)}});
    Run x{c, frame, generation::load_pool(c), c.r(25), c.r(27), c.r(26), 0, Kept<30>(c, 0), Kept<31>(c, 0)};
    check_listed_staff(x);
    constexpr Staff kinds[3] = {
        {0x100a3ec0, 0x160, 5, 8, 15, 16, 18, 17, 0x654, 0xc4, 0x54},
        {0x100a421c, 0x174, 7, 9, 16, 15, 17, 18, 0x69f, 0xa4, 0x4c},
        {0x100a4578, 0x190, 3, 10, 15, 16, 17, 18, 0x6ea, 0x84, 0x44},
    };
    for (const auto& kind : kinds)
        if (!add_staff(x, kind)) return 0;
    if (!add_role_five(x)) return 0;
    // sprintf(result buffer, text + 0x9b, made); r6-r10 stay as the last person's draws left them.
    c.set({{3, c.toc(slot::result_buffer)}, {4, x.text + 0x9b}, {5, x.made}});
    c.call(format_entry, 0x100a4be0);
    c.set(3, 1);
    return 1;
}
}
