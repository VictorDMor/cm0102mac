#pragma once
// INJURY.CPP: the layouts, globals and outside routines that more than one of the unit's files uses.
#include "guest_abi.hpp"
#include "injury_unit_adapter.hpp"
#include <initializer_list>

namespace cm::aot::injury_unit {
// The injury manager, the object every routine of the unit gets in r3.
namespace manager {
inline constexpr std::uint32_t status_rows = 0x00,  // one status row per person, indexed by the person's index
                               injuries = 0x04,     // the injury records
                               injury_count = 0x08; // how many of them are in use (signed word)
}
// A person's status row (0x24 bytes).
namespace status {
inline constexpr std::uint32_t condition = 0x0c,  // signed halfword
                               injury = 0x10,     // index of the person's injury record, -1 without one
                               flag = 0x22;       // byte set by 0x10187a20
inline constexpr std::uint32_t size = 0x24;
}
// An injury record (0x38 bytes).
namespace injury {
inline constexpr std::uint32_t started = 0x28,     // a date (DATE.CPP), two words
                               days_out = 0x30,    // signed halfwords: the two lengths the status
                               days_unfit = 0x32,  // question compares the days since `started` with
                               countdown_a = 0x34, // signed halfwords the daily pass decrements
                               countdown_b = 0x36;
inline constexpr std::uint32_t size = 0x38;
}
// A row of the injury type table (TOC injury_types, 0xc bytes).
namespace injury_type {
inline constexpr std::uint32_t size = 0xc;
}
inline constexpr std::uint32_t person_size = 0x70, club_size = 0x248;
inline constexpr std::uint32_t no_injury = 0xffffffff;

// Words of the table of contents (Cpu::toc).
namespace toc {
inline constexpr std::int32_t pool = -0x5438,          // the unit's strings
                              floats = -0x543c,        // the unit's float constants
                              injury_types = -0x64d4,  // the injury type table
                              type_ranges = -0x64d0,
                              message = -0x7ba8,       // the shared message buffer
                              success = -0x7bac,       // the shared success flag
                              today = -0x7b80,         // the game date
                              people = -0x7be0,        // the cell holding the person table
                              people_count = -0x7be4,  // the cell holding how many people there are
                              clubs = -0x7be8;         // the cell holding the club table
}
// Offsets in the unit's string pool.
namespace text {
inline constexpr std::uint32_t source_file = 0x53,    // "INJURY.CPP"
                               report_format = 0x5e,  // "v%s %s.%s %d"
                               error = 0x6b;          // "Error"
}
namespace callee {
inline constexpr std::uint32_t
    days_between = 0x10717dd0,     // (result, date cell, first word, second word): days and years apart
    list_destroy = 0x10738c20,     // (list, deleting)
    allocate_object = 0x10773520,  // (size)
    free_object = 0x107735a0;      // (object)
}

// `bl entry` with the arguments in r3 onwards; the callee's r3. For routines outside the unit.
inline std::uint32_t invoke(guest::Cpu& c, std::uint32_t entry, std::uint32_t back,
                            std::initializer_list<std::uint32_t> arguments = {}) {
    unsigned n = 3;
    for (const auto argument : arguments) c.set(n++, argument);
    c.call(entry, back);
    return c.r(3);
}
// The same for a routine of this unit that lives in another of its files: runs its C++ in place.
inline std::uint32_t invoke_unit(guest::Cpu& c, std::uint32_t entry, std::uint32_t back,
                                 std::initializer_list<std::uint32_t> arguments = {}) {
    unsigned n = 3;
    for (const auto argument : arguments) c.set(n++, argument);
    call(c, entry, back);
    return c.r(3);
}
// The game's shared sample table, stepped inline at each draw: the cursor cell advances by one
// halfword; past the last entry it returns to the first and the base is drawn again from the
// generator (0x1077b770, `back` is the address after that `bl`). The sample is the halfword under
// the cursor plus the base, as an unsigned word. Confirm at each site in the listing that the site
// is this exact sequence.
namespace sample_table {
inline constexpr std::int32_t base = -0x7b90, first = -0x7b8c, last = -0x7b88, cursor = -0x7b84;
inline constexpr std::uint32_t redraw = 0x1077b770;
}
inline std::uint32_t draw(guest::Cpu& c, std::uint32_t back) {
    const auto cursor_cell = c.toc(sample_table::cursor), base_cell = c.toc(sample_table::base);
    const auto next = c.u32(cursor_cell) + 2;
    c.put32(cursor_cell, next);
    if (next > c.u32(c.toc(sample_table::last))) {
        c.put32(cursor_cell, c.toc(sample_table::first));
        c.call(sample_table::redraw, back);
        c.put32(base_cell, c.r(3) & 0xffff);
    }
    return std::uint32_t(c.u16(c.u32(cursor_cell))) + c.u32(base_cell);
}

// A person's status row and injury record, as every routine finds them.
inline std::uint32_t status_row(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t person) {
    return c.u32(manager_object + manager::status_rows) + c.u32(person) * status::size;
}
inline std::uint32_t injury_record(guest::Cpu& c, std::uint32_t manager_object, std::uint32_t index) {
    return c.u32(manager_object + manager::injuries) + index * injury::size;
}

// The assertion report of a failed check: `at` is the address of the report's first call, `name`
// and `extension` the frame offsets of its two buffers, `line` the source line.
struct CheckSite { std::uint32_t at; std::int32_t name, extension; std::uint32_t line; };
inline void report_check(guest::Cpu& c, const guest::Frame& frame, const CheckSite& site) {
    const auto pool = c.toc(toc::pool);
    guest::report_assertion(c, {site.at, pool + text::source_file, pool + text::report_format, pool + text::error,
                                frame.at(site.name), frame.at(site.extension), site.line});
}
}
