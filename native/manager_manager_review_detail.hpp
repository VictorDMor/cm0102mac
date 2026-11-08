#pragma once
// MANAGER_MANAGER.CPP manager review (0x101a8ad0, 0x101c1690, 0x10195e10): the record fields
// they read. Offsets are the original's; meanings are named as far as the code shows them.
#include <cstdint>

namespace cm::aot::manager_manager::review_detail {
// The object (TOC -0x79f0): `table` points at a row of 0x4c bytes per club (indexed by the
// club's id); `departures` is the list at +4 the review posts to, `pending` the one at +0x10.
namespace object {
constexpr std::uint32_t table = 0x0, departures = 0x4, pending = 0x10;
}
namespace row {
constexpr std::uint32_t size = 0x4c, flags = 0x2, value = 0x6, date_a = 0x18, date_b = 0x1c;
constexpr std::uint8_t posted = 0x40;  // a departure was posted for the club
}
// The club record (0x248 bytes).
namespace club {
constexpr std::uint32_t id = 0x0, league = 0x30, second_person = 0x80, manager = 0x90, reputation = 0x1ec;
}
// The manager: a person record whose staff record sits at +0x1c.
namespace manager {
constexpr std::uint32_t staff = 0x1c, since_a = 0x4c, since_b = 0x50, trait = 0x66;
}
namespace staff {
constexpr std::uint32_t morale = 0x2a, trait = 0x3d;
}
// The results record 0x10459b50 returns for a club (the board's view of its recent form).
namespace results {
constexpr std::uint32_t last_games = 0x16;  // five bytes, +0x16...+0x1a, the newest last
constexpr std::uint32_t count = 0x4, bound = 0x77;
constexpr std::uint32_t signal_1 = 0x1c, signal_2 = 0x44, signal_3 = 0x58, signal_4 = 0x30;  // in the order tested
}
// The league record the club's league index selects (through the table at TOC -0x7b5c) and its
// object at +8; the object's bytes at +0xc0/+0xc1 bound the form, its virtual method +0xa0 asks.
namespace league {
constexpr std::uint32_t form_low = 0xc0, form_high = 0xc1, vtable_method = 0xa0, owner = 0x8;
}
// The module's draw table (TOC slots): the cell with the base, the table's first entry, the cell with its
// last entry and the cell with the cursor.
namespace table_slots {
constexpr std::int32_t base = -0x7b90, first = -0x7b8c, last = -0x7b88, cursor = -0x7b84;
}
// The int-to-double idiom's high word: the compiler builds a double from
// 0x43300000 | (value ^ 0x80000000) on the stack and subtracts the bias (pool +0x38).
constexpr std::uint32_t conversion_high = 0x43300000;
// The module's pool (TOC -0x536c): doubles.
namespace pool {
constexpr std::uint32_t tier_a = 0x0, tier_b = 0x20, tier_c = 0x28, bias = 0x38, unit = 0x40, average_limit = 0x98,
                        row_value = 0x1d0;
}
}
