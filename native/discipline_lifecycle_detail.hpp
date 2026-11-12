#pragma once
// DISCIPLINE.CPP lifecycle block: the globals and callees outside the module its routines use.
#include "discipline_table_detail.hpp"

namespace cm::aot::discipline {
namespace lifecycle_toc {
// Words of the table of contents (Cpu::toc): the module's string pool, the competition table, the
// people count, and the addresses of the strings the temp folder's name is built from.
inline constexpr std::int32_t pool = table_toc::pool, people_count = table_toc::people_count, competitions = -0x7b98,
                              folder_odd = -0x5600, folder_even = -0x5604;
}
// The manager's arrays beside the person table (the bans block's manager_arrays): the ban records and the sorted
// index, each with a capacity and a count (signed halfwords).
namespace lifecycle_field {
inline constexpr std::uint32_t records = 0x04, record_capacity = 0x08, record_count = 0x0a, index = 0x0c,
                               index_capacity = 0x10, index_count = 0x12, pending_count = 0x1c;
}
namespace lifecycle_callee {
inline constexpr std::uint32_t make_spec = 0x1077fee0, pascal_string = 0x10727890, dialog = 0x10769b60,
                               list_construct = 0x10738c00, list_destroy = 0x10738c20, list_append = 0x10738d10,
                               list_next = 0x10738e00, allocate = 0x10775130, free = 0x10775170,
                               allocate_zeroed = 0x10775200, object_allocate = 0x10773520,
                               object_delete = 0x107735a0, file_open = 0x107475b0, file_read = 0x10747a80,
                               file_write = 0x10747cc0, file_close = 0x10747900, history_name = 0x10746280,
                               history_construct = 0x10748bb0, history_destroy = 0x10748c80,
                               body_destroy = 0x105bb4c0, body_save = 0x105bb8a0, table_allocate = 0x100e7bd0,
                               super_cup_check = 0x101926b0, clock_value = 0x10775550, last_constructor = 0x1070ba10;
}
// The assertion report of a failed check: `at` is the address of the report's first call; `name` and
// `extension` are the frame offsets of its two buffers, `line` the source line.
struct CheckSite { std::uint32_t at, name, extension, line; };
inline void report_check(guest::Cpu& c, const guest::Frame& frame, const CheckSite& site) {
    const auto pool = c.toc(lifecycle_toc::pool);
    guest::report_assertion(c, {site.at, pool + 0x50, pool + 0x5f, pool + 0x6c, frame.at(std::int32_t(site.name)),
                                frame.at(std::int32_t(site.extension)), site.line});
}
// One slot of the manager's body array (0x100e7cd0): the byte offset in the array, the body's constructor, the
// bytes allocated for it, the return addresses of the allocation and of the constructor call, and what the slot
// waits for. A competition's index is the word at the address held by the TOC cell `toc`; the body exists when the
// competition's record (0x128 bytes in the table at TOC -0x7b98) has a nonzero byte at +0x120. `starts` marks the
// first slot of a group that shares one test.
enum class Condition { always, competition, super_cup };
struct BodySlot {
    std::uint32_t offset, constructor, size, allocated, constructed;
    Condition condition;
    std::int32_t toc;
    bool starts;
};
}
