#pragma once
// MANAGER_MANAGER.CPP's job assessment (0x101ad080): the record fields it reads. Offsets are
// the original's; meanings are named as far as the code shows them.
#include <cstdint>

namespace cm::aot::manager_manager::assessment {
// The person record (0x70 bytes) and its staff record (+0x1c): the staff member's attributes
// are signed bytes at +0x2e...+0x42, the personality ones at +0x64...+0x67 of the person.
namespace person {
constexpr std::uint32_t id = 0x0, club_link = 0x24, previous_link = 0x20, staff = 0x1c, club_ref = 0xc,
                        rating = 0x5c, role_primary = 0x62, role_secondary = 0x60;
constexpr std::uint32_t personality_a = 0x64, personality_b = 0x65, personality_c = 0x66, personality_d = 0x67;
}
// The club record (0x248 bytes).
namespace club {
constexpr std::uint32_t reputation = 0x1ec, second_person = 0x80, region = 0x2c, league = 0x30;
}
namespace region {
constexpr std::uint32_t tier = 0x11f, category = 0x11c, stats_id = 0x119;
}
namespace staff {
constexpr std::uint32_t low = 0x24, high = 0x26, level = 0x2c;
}
// The int-to-double conversion's high word: the compiler builds a double from
// 0x43300000 | (value ^ 0x80000000) on the stack and subtracts the bias (pool +0x38).
constexpr std::uint32_t conversion_high = 0x43300000;
}
