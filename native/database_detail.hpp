#pragma once
// DATABASE.CPP's record layouts, as far as its rewritten routines read them.
// Meanings marked "inferred" come from how the callers use the values.
#include <cstdint>

namespace cm::aot::database {
// A club: two lists of three people (inferred: favoured and disliked staff).
namespace club {
constexpr std::uint32_t favoured_staff = 0x5c, disliked_staff = 0x68;
}
// A person: their player record and their preferences record.
namespace person {
constexpr std::uint32_t player = 0x14, preferences = 0x18;
}
// A player's position ratings, one byte each, 20 for a natural position
// (inferred: CM 01/02's goalkeeper ... attacker, then right, left, central).
namespace player {
constexpr std::uint32_t goalkeeper = 0xf, sweeper = 0x10, defender = 0x11, defensive_midfielder = 0x12,
                        midfielder = 0x13, attacking_midfielder = 0x14, attacker = 0x15, right = 0x17,
                        left = 0x18, central = 0x19;
}
// A contract: the index of its club in the club table, and the flags byte
// of a club contract that 0x100d3150, 0x100d3230 and 0x100d3300 test.
namespace contract {
constexpr std::uint32_t club_index = 0x4, flags = 0x5c;
}
// The club table: records of 0x248 bytes from the address the TOC word at
// -0x7be8 points at.
namespace club_table {
constexpr std::int32_t base_slot = -0x7be8;
constexpr std::uint32_t stride = 0x248;
}
// A person's preferences: four lists of three ids (inferred: favourite and
// disliked clubs, favourite and disliked staff).
namespace preferences {
constexpr std::uint32_t favourite_clubs = 0x4, disliked_clubs = 0x10, favourite_staff = 0x1c, disliked_staff = 0x28;
}
// The 16-bit values the attribute ratings scale by (meanings not established).
namespace rated {
constexpr std::uint32_t value_04 = 0x4, value_24 = 0x24;
}
}
