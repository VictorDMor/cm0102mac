#pragma once
// INJURY.CPP, block "file": what the routines of injury_unit_file*.cpp share.
#include "injury_unit_detail.hpp"

namespace cm::aot::injury_unit {
// The manager's words beyond injury_unit_detail.hpp's three.
namespace manager_file {
inline constexpr std::uint32_t history = 0x0c,          // object built over the temp-file names (pool +0x37, +0x40)
                               event_rows = 0x10,       // 0x18-byte rows; count +0x14, capacity +0x18
                               report_rows = 0x1c,      // 0x14-byte rows; count +0x20, capacity +0x24
                               event_count = 0x14, event_capacity = 0x18, report_count = 0x20, report_capacity = 0x24,
                               notes = 0x28;            // list of 8-byte rows, its count a halfword at its +0
inline constexpr std::uint32_t event_row_size = 0x18, report_row_size = 0x14, note_row_size = 8;
}
// A status row's fields the new-game pass sets (injury_unit_detail.hpp names the others).
namespace status_file {
inline constexpr std::uint32_t date = 0x00,        // the game date at the start: two halfwords and a byte
                               fitness = 0x08,     // 10000 at the start
                               fitness_b = 0x0a,   // 10000 at the start
                               recurring_type = 0x14,  // byte, -1 without one
                               recurring_a = 0x18, recurring_b = 0x1c,
                               byte_21 = 0x21, byte_22 = 0x22;
}
namespace person_file {
inline constexpr std::uint32_t attributes = 0x14, team = 0x20, contract = 0x24, flags = 0x6a;
}
namespace file_callee {
inline constexpr std::uint32_t
    file_open = 0x107475b0,      // (file spec, name, 0 read / 1 write, 0x16, 4): the handle or 0
    file_read = 0x10747a80,      // (buffer, size, count, handle): items read
    file_write = 0x10747cc0,     // (buffer, size, count, handle): items written
    file_close = 0x10747900,     // (&handle)
    allocate_block = 0x10775130, // (size)
    free_block = 0x10775170,     // (block)
    memory_gauge = 0x10775550,   // (the word of the cell at TOC -0x7b74): chooses how the records are read
    exit_game = 0x1076f7d0,      // (-1)
    make_spec = 0x1077fee0,      // FSMakeFSSpec glue (0, 0, pascal name, spec): status halfword
    copy_string = 0x10727890,    // (destination, source)
    show_dialog = 0x10769b60,    // (title, text, kind)
    make_name = 0x10746280,      // (folder name, file name): what the history object is built over
    construct_history = 0x10748bb0,  // (object, name, 0x34)
    destroy_history = 0x10748c80,    // (object, deleting)
    list_construct = 0x10738c00, // (list)
    list_append = 0x10738d10,    // (list, row, size)
    list_next = 0x10738e00,      // (list)
    date_construct = 0x10715880, // (date): 0x28, 0x7b4, 1
    date_add_days = 0x10717890,  // (result, date, days)
    days_between = 0x10718130,   // (date, today's first word, today's second word)
    club_date = 0x101929b0,      // (club or contract, out, out): non-zero when there is none
    format_text = 0x1077b720;    // (buffer, format, values...)
}
namespace file_toc {
inline constexpr std::int32_t odd_name = -0x5430,       // 17 bytes: a length byte and the odd characters
                              even_name = -0x5434,      // 16 bytes: the even characters
                              status_text = -0x7bb8,    // the buffer the new-game counts are formatted into
                              club_total = -0x7b50,     // the cell of the club bound
                              clubs_reserved = -0x7b04, // twice this many clubs are left out
                              memory_cell = -0x7b74,
                              holder = -0x7a64;         // the cell 0x102ae9c0 and 0x102af650 are asked with
}
// Offsets of the pool's file strings.
namespace file_text {
inline constexpr std::uint32_t folder = 0x37, history_name = 0x40, file_name = 0x71,
                               retry_title = 0x00, retry_text = 0x0a,
                               starting_count = 0x7c, recurring_count = 0x9f;
}
}
