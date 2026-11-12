// DISCIPLINE.CPP table block (discipline_table.hpp), native-first.
#include "discipline_table.hpp"
#include "discipline_table_detail.hpp"

namespace cm::aot::discipline {
namespace {
namespace callee {
// Outside the module: the list (0x10738xxx), the history file, the allocator and the fill.
constexpr std::uint32_t next_member = 0x10738e00, remove_member = 0x10738e60, history_set = 0x10748ce0,
                        history_append = 0x10749fc0, history_read = 0x10749fd0, allocate = 0x10775130,
                        fill = 0x10778300, exit = 0x1076f7d0;
}
constexpr std::uint32_t person_size = 0x70;
constexpr std::uint32_t pending_list(std::uint32_t manager_object) { return manager_object + manager::pending; }

// The original's assertion report (DISCIPLINE.CPP's source name, "v%s %s.%s %d", the message box) for a failed
// check at source line `line`; `at` is the address of its first call, the two buffers are frame locals.
void report_failed_check(guest::Cpu& c, const guest::Frame& frame, std::uint32_t at, std::uint32_t line,
                         std::int32_t name_offset, std::int32_t extension_offset) {
    const auto pool = c.toc(table_toc::pool);
    guest::report_assertion(c, {at, pool + 0x50, pool + 0x5f, pool + 0x6c, frame.at(name_offset),
                                frame.at(extension_offset), line});
}
}

std::uint32_t slot_value_for(const guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::int8_t slot,
                             std::int8_t body) {
    const auto at = slot_address(row_address(c, manager_object, person), slot);
    return c.i8(at) == body ? c.u32(at + row::slot_value) : 0xffffffffu;
}

void set_slot(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::int8_t slot, std::uint8_t body,
              std::uint32_t value) {
    // The original reloads the table pointer between the two stores; nothing runs between them.
    c.put8(slot_address(row_address(c, manager_object, person), slot), body);
    c.put32(slot_address(row_address(c, manager_object, person), slot) + row::slot_value, value);
}

std::uint32_t next_pending_item(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::int8_t slot) {
    guest::Frame frame(c, 0x100e70b0, 0x50, {.first_gpr = 28});
    const auto person_row = row_address(c, manager_object, person);
    c.put32(manager_object + manager::pending_cursor, c.u32(manager_object + manager::pending_head));
    for (;;) {
        c.set(3, pending_list(manager_object));
        c.call(callee::next_member, 0x100e7120);
        const auto item = c.r(3);
        if (item == 0 || c.i8(person_row + row::pending_count) <= 0) return 0;
        if (c.u32(item) == person && c.i8(item + 4) == slot) return item;
    }
}

void remove_pending_items(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person_word) {
    guest::Frame frame(c, 0x100df950, 0x50, {.first_gpr = 29});
    const auto person_row = row_address(c, manager_object, c.u32(person_word));
    c.put32(manager_object + manager::pending_cursor, c.u32(manager_object + manager::pending_head));
    for (;;) {
        c.set(3, pending_list(manager_object));
        c.call(callee::next_member, 0x100df9bc);
        const auto item = c.r(3);
        if (item == 0 || c.i8(person_row + row::pending_count) <= 0) return;
        if (c.u32(item) != c.u32(person_word)) continue;
        c.set({{3, pending_list(manager_object)}, {4, item}});  // the item to remove: the next_member result stays in r4
        c.call(callee::remove_member, 0x100df9a8);
        c.put8(person_row + row::pending_count, c.u8(person_row + row::pending_count) - 1u);
    }
}

void pass_history_position(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person, std::uint32_t argument) {
    guest::Frame frame(c, 0x100e7160, 0x40, {});
    const auto person_row = row_address(c, manager_object, person);
    c.set({{3, c.u32(manager_object + manager::history)},
           {4, person_row + row::history_first},
           {5, person_row + row::history_second},
           {6, argument}});
    c.call(callee::history_set, 0x100e718c);
}

std::uint32_t read_history_entry(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person) {
    guest::Frame frame(c, 0x100e71a0, 0x40, {});
    const auto record = c.u32(c.toc(table_toc::people)) + person * person_size;
    if (!person_in_table(c, record)) return 0;
    const auto person_row = row_address(c, manager_object, person);
    c.set({{3, c.u32(manager_object + manager::history)},
           {4, c.u32(person_row + row::history_first)},
           {5, c.u32(person_row + row::history_second)}});
    c.call(callee::history_read, 0x100e71f8);
    return c.r(3);
}

void append_history(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t entry) {
    guest::Frame frame(c, 0x100e8660, 0xa0, {.first_gpr = 27});
    const auto fail = [&](std::uint32_t at, std::uint32_t line, std::int32_t name, std::int32_t extension) {
        report_failed_check(c, frame, at, line, name, extension);
        c.set(3, 0xffffffffu);
        c.call(callee::exit, at + 0x44);
    };
    if (entry == 0) fail(0x100e869c, 0x16ea, 0x68, 0x40);
    // After an exit that returned, the original goes on to read the null entry.
    if (c.u32(entry + 0x14) == 0) fail(0x100e8700, 0x16ee, 0x48, 0x38);
    const auto person_row = row_address(c, manager_object, c.u32(entry));
    c.set({{3, c.u32(manager_object + manager::history)},
           {4, person_row + row::history_first},
           {5, person_row + row::history_second}});
    c.call(callee::history_append, 0x100e8764);
}

std::uint32_t compare_pending_items(const guest::Cpu& c, std::uint32_t first, std::uint32_t second) {
    if (c.i32(first + 4) != c.i32(second + 4)) return c.u32(first + 4) - c.u32(second + 4);
    if (c.i32(first) == -1 || c.i32(second) == -1) return 0;
    return c.u32(first) - c.u32(second);
}

void allocate_person_table(guest::Cpu& c, std::uint32_t manager_object) {
    guest::Frame frame(c, 0x100e7bd0, 0x70, {.first_gpr = 28});
    const auto count = c.toc(table_toc::people_count);  // the address of the people count
    c.set(3, c.u32(count) * row::size);
    c.call(callee::allocate, 0x100e7c08);
    c.put32(manager_object + manager::people, c.r(3));
    if (c.u32(manager_object + manager::people) == 0) {
        report_failed_check(c, frame, 0x100e7c2c, 0x1602, 0x40, 0x38);
        return;
    }
    c.set({{3, c.u32(manager_object + manager::people)}, {4, 0xffffffffu}, {5, c.u32(count) * row::size}});
    c.call(callee::fill, 0x100e7c80);
    for (std::int32_t person = 0; person < c.i32(count); ++person)
        c.put8(c.u32(manager_object + manager::people) + std::uint32_t(person) * row::size + row::pending_count, 0);
}
}
