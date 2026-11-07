// DATABASE.CPP's people block, part 1: the passes that fill and chain the generated people
// (database_people.hpp): 0x100b3650 (names) and 0x100b2b50 (the four passes in order).
#include "database_generation.hpp"
#include "database_people.hpp"
#include "database_people_detail.hpp"

namespace cm::aot::database {
using namespace people;

// 0x100b3650. A frame of 0x50 saving r29-r31: r29 the row, r30 its index, r31 the count's address. A row
// with a nation takes the first-name record when the filtered picker's record starts with a non-zero byte,
// likewise the second name, and always the third record (0x100c53e0 answers one even when none is unmarked).
void generate_names(Cpu& c) {
    constexpr std::uint32_t entry = 0x100b3650;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x50, {.first_gpr = 29});
    const auto table_at = c.toc(person_row::table_slot);
    c.set(3, table_at);
    c.set(30, 0);
    const auto count_at = c.toc(person_row::count_slot);
    c.set(31, count_at);
    std::uint32_t row = load32(c, 29, table_at);
    // A picked record is kept only when its first byte, tested as the signed byte `extsb.` sees, is not 0.
    const auto usable = [&](std::uint32_t record) {
        if (compare_logical(c, record) == 0) return false;
        c.set(0, extsb(c.u8(record)));
        return compare(c, std::int32_t(c.r(0))) != 0;
    };
    for (std::uint32_t index = 0;; ++index, row += person_row::stride) {
        if (index != 0) {  // the loop's increments
            c.set(30, index);
            c.set(29, row);
        }
        load32(c, 0, count_at);
        if (compare(c, std::int32_t(index), std::int32_t(c.r(0))) >= 0) break;
        if (compare_logical(c, load32(c, 0, row + person_row::nation)) == 0) continue;
        guest::enter(c, entry + 0x40, {load32(c, 3, row + person_row::nation)});
        if (usable(pick_name_for_person_filtered(c, c.r(3)))) c.put32(row + person_row::first_name, c.r(3));
        load32(c, 3, row + person_row::nation);
        guest::enter(c, entry + 0x60, {c.r(3)});
        if (usable(pick_name_for_person(c, c.r(3)))) c.put32(row + person_row::second_name, c.r(3));
        load32(c, 3, row + person_row::nation);
        guest::enter(c, entry + 0x80, {c.r(3)});
        pick_unmarked_record(c);
        c.put32(row + person_row::third_name, c.r(3));
    }
}

// 0x100b2b50. A frame of 0x40 with no saved register: the four passes, each a direct call.
void generate_people(Cpu& c) {
    constexpr std::uint32_t entry = 0x100b2b50;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x40, {});
    c.link(entry + 0x10);
    shuffle_person_rows(c);
    c.link(entry + 0x14);
    generate_names(c);
    c.link(entry + 0x18);
    swap_player_attributes_within_clubs(c);
    c.link(entry + 0x1c);
    settle_person_counters(c);
}
}
