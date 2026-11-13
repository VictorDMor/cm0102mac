#pragma once
// TRANSFER_OFFER.CPP: the offer record's layout, the globals and the routines outside the unit
// that more than one of its files uses.
#include "guest_abi.hpp"
#include "transfer_offer_unit_adapter.hpp"
#include <initializer_list>

namespace cm::aot::transfer_offer_unit {
// The offer record (0x54 bytes, the size its file form reads and writes).
namespace field {
inline constexpr std::uint32_t clubs = 0x00,             // a list of 4-byte rows, or null
                               exchange = 0x04,          // a list of 8-byte rows (person index, status byte at +4), or null
                               buyer = 0x08,             // club index
                               player = 0x0c,            // person index
                               status = 0x10,            // signed byte
                               kind = 0x11,              // signed byte; 0xb in the offers this unit starts
                               date = 0x14,              // a date (DATE.CPP)
                               fee = 0x20,               // the amount paid at once
                               instalments = 0x40,       // amount paid over time
                               instalment_months = 0x44, // signed byte
                               extra = 0x48,             // amount due after appearances
                               extra_count = 0x4c,       // signed byte
                               sell_on_percent = 0x50,   // signed byte
                               months = 0x51;            // signed byte
inline constexpr std::uint32_t size = 0x54;
}
// A row of the exchange list.
namespace exchange_row {
inline constexpr std::uint32_t person = 0x0, status = 0x4;
}
inline constexpr std::uint32_t person_size = 0x70, club_size = 0x248;

// Words of the table of contents (Cpu::toc).
namespace toc {
inline constexpr std::int32_t pool = -0x4b88,      // the unit's strings: "TRANSFER_OFFER.CPP", the report format, "Error"
                              doubles = -0x4b8c,   // the unit's double constants
                              floats = -0x4b90,    // the unit's float constants
                              message = -0x7ba8,   // the shared message buffer
                              success = -0x7bac,   // the shared success flag
                              people = -0x7be0,    // the cell holding the person table
                              clubs = -0x7be8;     // the cell holding the club table
}
// Offsets in the unit's string pool.
namespace text {
inline constexpr std::uint32_t source_file = 0x00,    // "TRANSFER_OFFER.CPP"
                               report_format = 0x13,  // "v%s %s.%s %d"
                               error = 0x20;          // "Error"
}
namespace callee {
inline constexpr std::uint32_t
    date_construct = 0x10715880,   // (date)
    list_construct = 0x10738c00,   // (list)
    list_destroy = 0x10738c20,     // (list, deleting)
    list_reset = 0x10738ca0,
    list_append = 0x10738d10,      // (list, row, row size)
    list_next = 0x10738e00,        // (list) -> the next row or null; the cursor is the word at +0xc, the head at +4
    list_remove = 0x10738e60,
    allocate_object = 0x10773520,  // (size)
    free_object = 0x107735a0,      // (object)
    file_read = 0x10747a80;        // (at, size, count, file) -> items read
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
// the cursor plus the base, as an unsigned word.
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
// Starts a walk of a list from its head, as the original does inline before its loop of list_next.
inline void rewind(guest::Cpu& c, std::uint32_t list) { c.put32(list + 0xc, c.u32(list + 4)); }

// The assertion report of a failed check: `at` is the address of the report's first call, `name`
// and `extension` the frame offsets of its two buffers, `line` the source line.
struct CheckSite { std::uint32_t at; std::int32_t name, extension; std::uint32_t line; };
inline void report_check(guest::Cpu& c, const guest::Frame& frame, const CheckSite& site) {
    const auto pool = c.toc(toc::pool);
    guest::report_assertion(c, {site.at, pool + text::source_file, pool + text::report_format, pool + text::error,
                                frame.at(site.name), frame.at(site.extension), site.line});
}
}
