#pragma once
// FINANCE.CPP: the layouts, globals and outside routines that more than one of the unit's files uses.
#include "guest_abi.hpp"
#include "finance_adapter.hpp"
#include <initializer_list>

namespace cm::aot::finance {
// The finance manager, the object every routine of the unit gets in r3 (global: the word in the
// cell at TOC finances).
namespace manager {
inline constexpr std::uint32_t accounts = 0x00;  // one account per club, indexed by the club's index
}
// A club's account (0x168 bytes).
namespace account {
inline constexpr std::uint32_t balance = 0x00,     // signed 64 bits
                               club = 0x08,        // the club's index
                               attendance = 0x14;  // word scaled by 0x100f4f20 (name inferred)
// The ledger: one line per kind of income or expense, each kept twice, `second_copy` apart (two
// periods; which is which is not established here). Lines are a word, or 64 bits where marked.
inline constexpr std::uint32_t second_copy = 0xa0;
inline constexpr std::uint32_t size = 0x168;
}
inline constexpr std::uint32_t club_size = 0x248;
namespace club {
inline constexpr std::uint32_t reputation = 0x1ec;  // signed halfword
}

// Words of the table of contents (Cpu::toc).
namespace toc {
inline constexpr std::int32_t pool = -0x5598,         // the unit's strings
                              floats = -0x559c,       // the unit's float constants
                              doubles = -0x55a0,      // the unit's double constants
                              element_destructor = -0x74a4,  // transition vector of 0x100f1010
                              element_constructor = -0x74a0, // transition vector of 0x100f1000
                              finances = -0x7ab8,     // the cell holding the finance manager
                              message = -0x7ba8,      // the shared message buffer
                              success = -0x7bac,      // the shared success flag
                              today = -0x7b80,        // the game date
                              club_count = -0x7b50,   // the cell holding how many clubs there are
                              clubs = -0x7be8;        // the cell holding the club table
}
// Offsets in the unit's string pool.
namespace text {
inline constexpr std::uint32_t source_file = 0x00,    // "FINANCE.CPP"
                               report_format = 0x0c,  // "v%s %s.%s %d"
                               error = 0x19;          // "Error"
}
// Offsets in the unit's double pool.
namespace doubles {
inline constexpr std::uint32_t int_magic = 0x50;  // 2^52 + 2^31, the int-to-double constant
}
namespace callee {
inline constexpr std::uint32_t
    club_is_inactive = 0x100c7d00,  // (club): a non-zero byte for a club the game does not run (name inferred)
    club_of_code = 0x100ce320,      // (club, out byte, flag): the club a club record stands for
    free_object = 0x107735a0,       // (object)
    destroy_array = 0x107734a0;     // (array, element destructor's vector)
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

inline std::uint32_t club_record(guest::Cpu& c, std::uint32_t index) {
    return c.u32(c.toc(toc::clubs)) + index * club_size;
}
inline std::int64_t balance_of(guest::Cpu& c, std::uint32_t account_record) {
    return std::int64_t(c.u64(account_record + account::balance));
}

// The assertion report of a failed check: `at` is the address of the report's first call,
// `name` and `extension` the frame offsets of its two buffers, `line` the source line.
struct CheckSite { std::uint32_t at; std::int32_t name, extension; std::uint32_t line; };
inline void report_check(guest::Cpu& c, const guest::Frame& frame, const CheckSite& site) {
    const auto pool = c.toc(toc::pool);
    guest::report_assertion(c, {site.at, pool + text::source_file, pool + text::report_format, pool + text::error,
                                frame.at(site.name), frame.at(site.extension), site.line});
}
}
