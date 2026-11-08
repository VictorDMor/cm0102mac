#pragma once
// The selection block's record offsets. Meanings marked "inferred" come from
// how the callers and the values use them; the offsets are the original's.
#include "match_man_detail.hpp"

namespace cm::aot::match_man {
// A match side beyond the player table (match_side): offsets the selection
// routines read.
namespace selection_side {
constexpr std::uint32_t manager_staff = 0x24;  // inferred: the manager's staff data (judging byte at +0x37)
constexpr std::uint32_t players = 0x28;        // its 20 player records
constexpr std::uint32_t club = 0x34;           // the club record
}
// A player record of a match (match_player), beyond the person link.
namespace selection_player {
constexpr std::uint32_t unavailable = 0x19;  // signed byte: negative skips the player
}
// The club record, as the selection reads it.
namespace selection_club {
constexpr std::uint32_t form = 0x1ec;  // 16-bit (inferred: a club-wide value subtracted from the player's)
constexpr std::uint32_t judge = 0x90;  // person record of the club's judge/manager
}
// Person record fields the selection reads (person::player, person::age are shared).
namespace selection_person {
constexpr std::uint32_t id = 0x0;
constexpr std::uint32_t date_low = 0x4c, date_high = 0x50;  // the two words of a date handed to 0x10718130
constexpr std::uint32_t age = 0x5c;       // 16-bit, used as a signed byte
constexpr std::uint32_t byte_5e = 0x5e;   // unsigned byte, scaled by a pool value
constexpr std::uint32_t byte_65 = 0x65;   // signed byte, 500 - 10 * value is a limit
constexpr std::uint32_t byte_67 = 0x67;   // signed byte, below 10 stretches the result
constexpr std::uint32_t human_flag = 0x6b;  // signed byte, nonzero for a human manager's record
}
// Player data (person::player), as the selection reads it.
namespace selection_data {
constexpr std::uint32_t ability = 0xc;      // 16-bit
constexpr std::uint32_t goalkeeper = 0xf;   // unsigned byte: 20 for a natural goalkeeper
constexpr std::uint32_t byte_2f = 0x2f;     // signed byte, scaled into the rating
}
// Entries of the other modules' routines the selection calls.
namespace selection_calls {
constexpr std::uint32_t attribute_rating = 0x100d2f50;
constexpr std::uint32_t allocate = 0x10775200, release = 0x10775170, fill = 0x10778300;
constexpr std::uint32_t human_manager = 0x1014a1a0;
}
}
