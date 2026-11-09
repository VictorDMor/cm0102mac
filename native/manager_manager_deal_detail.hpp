#pragma once
// MANAGER_MANAGER.CPP's verdict on a transfer deal (0x101a0f50): the record fields it reads.
// Offsets are the original's; meanings come from the caller (0x103f5ad8) and the code.
#include <cstdint>

namespace cm::aot::manager_manager::deal_detail {
// The module object (TOC -0x79f0): `table` points at the board's rows, 0x4c bytes per club by id.
namespace object {
constexpr std::uint32_t table = 0x0;
}
// The board's row of a club: three halfwords the verdict lowers.
namespace row {
constexpr std::uint32_t size = 0x4c, value = 0x6, standing = 0x14, trust = 0x16;
}
// The club (0x248 bytes): three staff people from +0x190, a record at +0x2c (a flag byte at +0x120),
// its league link at +0x30 and its reputation.
namespace club {
constexpr std::uint32_t id = 0x0, region = 0x2c, league = 0x30, staff = 0x190, reputation = 0x1ec;
constexpr std::uint32_t staff_count = 3;
}
namespace region {
constexpr std::uint32_t flag = 0x120;
}
// The player (a person, 0x70 bytes): his id, the record at +0x14 (a byte at +0x2d) and the
// club link at +0x24 the valuations take.
namespace person {
constexpr std::uint32_t id = 0x0, career = 0x14, link = 0x24;
}
namespace career {
constexpr std::uint32_t byte = 0x2d;
}
// A staff person of the club: his staff record at +0x1c, whose byte at +0x3e is the rating asked
// from 0x100d2f50.
namespace staff_person {
constexpr std::uint32_t id = 0x0, record = 0x1c;
}
namespace staff_record {
constexpr std::uint32_t rating = 0x3e;
}
// The record 0x1018c340 returns: its halfword at +0x30 must pass 20 for the staff to be asked.
namespace screen {
constexpr std::uint32_t figure = 0x30;
constexpr std::int32_t figure_floor = 0x14;
}
// The player's deal record (0x10082f20): the club index at +4, a six-bit field in the byte at
// +0x38 (1 and 3 end the verdict) and a level byte at +0x44.
namespace deal {
constexpr std::uint32_t club_index = 0x4, kind_byte = 0x38, level = 0x44;
}
constexpr std::uint32_t club_size = 0x248;
// The int-to-double idiom's high word: the compiler builds a double from 0x43300000 | (value ^
// 0x80000000) on the stack and subtracts the bias (pool +0x38).
constexpr std::uint32_t conversion_high = 0x43300000;
// The module's pool (TOC -0x536c): doubles, named by what this routine uses them for.
namespace pool {
constexpr std::uint32_t zero = 0x8, bias = 0x38, unit = 0x40, reputation_scale = 0x60, ask_ratio = 0x98,
                        big_ask_ratio = 0x130, floor_ratio = 0x100, glue_exponent = 0x120, glue_offset = 0x128,
                        value_gain = 0x140, level_two = 0x138, level_one = 0x148, level_other = 0x150,
                        limit_ratio = 0x158;
}
}
