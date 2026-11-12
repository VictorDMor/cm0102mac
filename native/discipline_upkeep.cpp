// DISCIPLINE.CPP upkeep block (discipline_upkeep.hpp), native-first.
#include "discipline_upkeep.hpp"
#include "discipline_bans.hpp"
#include "discipline_table.hpp"
#include "discipline_upkeep_detail.hpp"
#include <array>

namespace cm::aot::discipline {
using guest::Cpu;
using guest::Frame;
using namespace upkeep;

namespace {
std::uint32_t pending_list(std::uint32_t manager) { return manager + manager::pending; }

// The ruling-body lookup of the person's entry (r3 body, r4 person, r5 0); 0 when the body holds none.
std::uint32_t find_entry(Cpu& c, std::uint32_t body, std::uint32_t person, std::uint32_t back) {
    c.set({{3, body}, {4, person}, {5, 0}});
    c.call(callee::find_entry, back);
    return c.r(3);
}

// The ban record of a found entry cleared when the entry points at one (the call site's return address is `back`).
void clear_entry_ban(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t entry, std::uint32_t back) {
    if (c.i16(entry + entry_ban_index) <= -1) return;
    c.link(back);
    clear_ban(c, manager, person, c.i16(entry + entry_ban_index));
}

void report_failed_check(Cpu& c, const Frame& frame, std::uint32_t at, std::uint32_t line, std::int32_t name,
                         std::int32_t extension) {
    const auto pool = c.toc(toc::pool);
    guest::report_assertion(c, {at, pool + 0x50, pool + 0x5f, pool + 0x6c, frame.at(name), frame.at(extension), line});
}
}

void drop_out_of_scope(Cpu& c, std::uint32_t manager, std::uint32_t person_word, std::uint32_t club) {
    Frame frame(c, 0x100df360, 0x90, {.first_gpr = 22});
    const auto person = c.u32(person_word);
    const auto row = row_address(c, manager, person);
    for (std::int8_t slot = 0; slot < 4; ++slot) {
        const auto at = slot_at(row, slot);
        if (c.i8(at) == -1) continue;
        const auto body = body_at(c, manager, c.i8(at));
        if (body_cover(c, body, club) != Cover::uncovered) continue;
        const auto entry = find_entry(c, body, person, 0x100df438);
        if (!entry) continue;
        clear_entry_ban(c, manager, person, entry, 0x100df458);
        if (c.i8(row + row::pending_count) != 0) report_failed_check(c, frame, 0x100df478, 0x238, 0x40, 0x38);
        c.link(0x100df4cc);
        set_slot(c, manager, person, slot, 0xff, 0xffffffffu);
        c.put32(entry, 0xffffffffu);
    }
}

void defer_out_of_scope(Cpu& c, std::uint32_t manager, std::uint32_t person_word) {
    Frame frame(c, 0x100df500, 0x90, {.first_gpr = 25});
    c.set({{3, c.u32(c.toc(toc::database))}, {4, person_word}});
    c.call(callee::database_club, 0x100df52c);
    if (c.r(3) == 0) {
        report_failed_check(c, frame, 0x100df56c, 0x25f, 0x50, 0x38);
        return;
    }
    c.set({{3, c.u32(c.toc(toc::database))}, {4, person_word}});
    c.call(callee::database_club, 0x100df540);
    const auto club = club_record(c, c.u32(c.r(3) + 4));
    const auto person = c.u32(person_word);
    const auto row = row_address(c, manager, person);
    for (std::int8_t slot = 0; slot < 4; ++slot) {
        if (slot == 3) continue;
        const auto at = slot_at(row, slot);
        if (c.i8(at) == -1) continue;
        if (body_cover(c, body_at(c, manager, c.i8(at)), club) != Cover::uncovered) continue;
        // The 0x10-byte item is built in the frame: its bytes +5..+7 and +9..+0xb are not written, and are copied
        // into the list as the frame holds them (the original leaves them the same).
        c.put32(frame.at(0x40), person);
        c.put8(frame.at(0x44), slot);
        c.put8(frame.at(0x48), c.u8(at));
        c.put32(frame.at(0x4c), c.u32(at + row::slot_value));
        c.set({{3, pending_list(manager)}, {4, frame.at(0x40)}, {5, 0x10}});
        c.call(callee::list_append, 0x100df680);
        c.set({{3, at}, {4, 0xffffffffu}, {5, 8}});
        c.call(callee::fill, 0x100df690);
        c.put8(row + row::pending_count, c.u8(row + row::pending_count) + 1u);
    }
}

void restore_in_scope(Cpu& c, std::uint32_t manager, std::uint32_t person_word) {
    Frame frame(c, 0x100df6d0, 0x60, {.first_gpr = 25});
    const auto person = c.u32(person_word);
    const auto row = row_address(c, manager, person);
    // The two halfwords the original copies into its frame at 0x38: the slots a pending item restored are set to 1.
    const auto initial = c.r(2) - 0x39cc;
    std::array<std::uint8_t, 4> restored{c.u8(initial), c.u8(initial + 1), c.u8(initial + 2), c.u8(initial + 3)};
    c.set({{3, c.u32(c.toc(toc::database))}, {4, person_word}});
    c.call(callee::database_person, 0x100df718);
    const auto found = c.r(3);
    c.put32(c.toc(toc::database_result), found);
    const auto club = found ? club_record(c, c.u32(found + 4)) : 0u;
    c.put32(manager + manager::pending_cursor, c.u32(manager + manager::pending_head));
    for (;;) {
        c.set(3, pending_list(manager));
        c.call(callee::list_next, 0x100df818);
        const auto item = c.r(3);
        if (item == 0 || c.i8(row + row::pending_count) <= 0) break;
        if (c.u32(item) != person) continue;
        const auto slot = std::int32_t(c.i8(item + 4));
        if (c.i8(slot_at(row, slot)) != -1) {
            const auto entry = find_entry(c, body_at(c, manager, c.i8(slot_at(row, slot))), person, 0x100df78c);
            if (entry) {
                clear_entry_ban(c, manager, person, entry, 0x100df7ac);
                c.link(0x100df7c4);
                set_slot(c, manager, person, std::int8_t(c.u8(item + 4)), 0xff, 0xffffffffu);
                c.put32(entry, 0xffffffffu);
            }
        }
        // The flag byte is at 0x38 + slot of the frame: only slots 0-3 are read back, the rest is dead stack.
        if (slot >= 0 && slot < 4) restored[std::size_t(slot)] = 1;
        c.put8(slot_at(row, slot), c.u8(item + 8));
        c.put32(slot_at(row, slot) + row::slot_value, c.u32(item + 0xc));
        c.set({{3, pending_list(manager)}, {4, item}});
        c.call(callee::list_remove, 0x100df804);
        c.put8(row + row::pending_count, c.u8(row + row::pending_count) - 1u);
    }
    for (std::int8_t slot = 0; slot < 4; ++slot) {
        const auto at = slot_at(row, slot);
        if (restored[std::size_t(slot)] != 0 || slot == 3 || c.i8(at) == -1) continue;
        const auto body = body_at(c, manager, c.i8(at));
        if (body_cover(c, body, club) == Cover::covered) continue;
        const auto entry = find_entry(c, body, person, 0x100df8e8);
        if (!entry) continue;
        clear_entry_ban(c, manager, person, entry, 0x100df908);
        c.link(0x100df920);
        set_slot(c, manager, person, slot, 0xff, 0xffffffffu);
        c.put32(entry, 0xffffffffu);
    }
}

void clear_person(Cpu& c, std::uint32_t manager, std::uint32_t person) {
    Frame frame(c, 0x100df9f0, 0x50, {.first_gpr = 26});
    const auto row = row_address(c, manager, person);
    c.link(0x100dfa28);
    append_history(c, manager, c.u32(c.toc(table_toc::people)) + person * 0x70);
    for (std::int8_t slot = 0; slot < 4; ++slot) {
        const auto at = slot_at(row, slot);
        if (c.i8(at) == -1) continue;
        const auto entry = find_entry(c, body_at(c, manager, c.i8(at)), person, 0x100dfa5c);
        if (!entry) continue;
        clear_entry_ban(c, manager, person, entry, 0x100dfa7c);
        c.link(0x100dfa94);
        set_slot(c, manager, person, slot, 0xff, 0xffffffffu);
        c.put32(entry, 0xffffffffu);
    }
    c.put32(manager + manager::pending_cursor, c.u32(manager + manager::pending_head));
    for (;;) {
        c.set(3, pending_list(manager));
        c.call(callee::list_next, 0x100dfae8);
        const auto item = c.r(3);
        if (item == 0 || c.i8(row + row::pending_count) < 1) break;
        if (c.u32(item) != person) continue;
        c.set({{3, pending_list(manager)}, {4, item}});
        c.call(callee::list_remove, 0x100dfad4);
        c.put8(row + row::pending_count, c.u8(row + row::pending_count) - 1u);
    }
    c.set({{3, row}, {4, 0xffffffffu}, {5, row::size}});
    c.call(callee::fill, 0x100dfb0c);
    c.put8(row + row::pending_count, 0);
}
}
