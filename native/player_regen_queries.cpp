// PLAYER_REGEN.CPP, block `regen_queries`: small queries over the regen object's lists (retirement entries of list A,
// the regen queue of list B) and the people's kind table, and the unit's two qsort comparators. None of them calls
// anything or draws. Callers' varargs calls spill r3-r10 (the sort reads r3 only), so each routine leaves the volatile
// registers it writes as the original does on every path; the checker compares r4-r10.
#include "player_regen_detail.hpp"

namespace cm::aot::player_regen {
std::int32_t person_class(Cpu& c, std::uint32_t id_pointer) {
    const auto id = c.u32(id_pointer);
    // r4 holds the kind table's base; the record's byte +0xc is read at base + id * 100 + 0xc (the address wraps in a word).
    const auto base = c.u32(c.toc(toc::kind_table));
    c.set(4, base);
    const auto record = std::uint32_t(id * kind_record::size);
    return std::int32_t((c.u8(base + record + kind_record::class_byte) >> 2) & 3);
}

std::uint32_t find_retirement_entry(Cpu& c, std::uint32_t list, std::uint32_t key, std::int8_t kind) {
    auto entry = c.u32(list + list::entries);
    std::int32_t index = 0;
    // r6 holds the key as last read, r7 the entry reached, r8 its index.
    while (index < c.i32(list + list::count)) {
        const auto wanted = c.u32(key);
        c.set(6, wanted);
        if (wanted == c.u32(entry + retirement_entry::id)) {
            c.set(7, entry);
            c.set(8, std::uint32_t(index));
            // Only the first entry with the id is looked at: if it fails the test the answer is 0.
            if (kind != 0) return c.u8(entry + retirement_entry::kind) == retirement_entry::injury_kind ? entry : 0;
            return c.i8(entry + retirement_entry::announced) != 0 ? entry : 0;
        }
        ++index;
        entry += retirement_entry::size;
    }
    c.set(7, entry);
    c.set(8, std::uint32_t(index));
    return 0;
}

bool staff_pool_has_entries(Cpu& c, std::uint32_t regen_object) { return c.u32(regen_object + regen_object::staff_pool_word) != 0; }

std::int32_t compare_queue_entries(Cpu& c, std::uint32_t first, std::uint32_t second) {
    if (first == 0) return -1;
    if (second == 0) return 1;
    const auto rank_first = c.u16(first + queue_entry::rank_high), rank_second = c.u16(second + queue_entry::rank_high);
    c.set(5, rank_first);
    if (rank_first > rank_second) return 1;
    if (rank_first < rank_second) return -1;
    const auto low_first = sext16(c.u16(first + queue_entry::rank_low)), low_second = sext16(c.u16(second + queue_entry::rank_low));
    c.set(5, std::uint32_t(low_first));
    if (low_first != low_second) return std::int32_t(std::uint32_t(low_first) - std::uint32_t(low_second));
    // A tie on both halfwords: the ids decide (r4 holds the second's).
    const auto id_second = c.u32(second + queue_entry::id);
    c.set(4, id_second);
    return std::int32_t(c.u32(first + queue_entry::id) - id_second);
}

std::int32_t compare_retirement_entries(Cpu& c, std::uint32_t first, std::uint32_t second) {
    // r6 starts as the address of the game's current date (halfword day, halfword year) and ends as the second's due date.
    const auto today = c.toc(toc::year_date);
    c.set(6, today);
    if (first == 0) return -1;
    if (second == 0) return 1;
    const auto year = c.u16(today + date::year);
    c.set(7, year);
    // An entry is ordered by its first date unless that date has passed (earlier than today by year, then by day), then
    // by its second. r5 holds the entry's day when the years tie.
    const auto due = [&](std::uint32_t entry) {
        const auto entry_year = c.u16(entry + retirement_entry::first_date + date::year);
        if (entry_year < year) return entry + retirement_entry::second_date;
        if (entry_year != year) return entry + retirement_entry::first_date;
        const auto day = sext16(c.u16(entry + retirement_entry::first_date + date::day));
        c.set(5, std::uint32_t(day));
        return day < sext16(c.u16(today + date::day)) ? entry + retirement_entry::second_date : entry + retirement_entry::first_date;
    };
    const auto due_first = due(first);
    c.set(8, due_first);
    const auto due_second = due(second);
    c.set(6, due_second);
    const auto year_first = c.u16(due_first + date::year), year_second = c.u16(due_second + date::year);
    c.set(5, year_first);
    if (year_first != year_second) return std::int32_t(year_first) - std::int32_t(year_second);
    const auto day_first = sext16(c.u16(due_first + date::day)), day_second = sext16(c.u16(due_second + date::day));
    c.set(5, std::uint32_t(day_first));
    if (day_first != day_second) return day_first - day_second;
    const auto id_second = c.u32(second + retirement_entry::id);
    c.set(4, id_second);
    return std::int32_t(c.u32(first + retirement_entry::id) - id_second);
}

bool retirement_queued(Cpu& c, std::uint32_t list, std::uint32_t key) {
    if (key == 0) return false;
    const auto count = c.i32(list + list::count);
    if (count == 0) return false;
    // r7 is the byte offset of the entry under test; r5 the entries' base and r6 the key, read afresh each time round.
    std::uint32_t offset = 0;
    c.set(7, 0);
    for (std::int32_t left = count; left > 0; --left) {
        const auto base = c.u32(list + list::entries);
        c.set(5, base);
        const auto wanted = c.u32(key);
        c.set(6, wanted);
        if (wanted == c.u32(base + offset)) return true;
        offset += retirement_entry::size;
        c.set(7, offset);
    }
    return false;
}
}
