#pragma once
// MATCH_ENG.CPP, match setup: the match object's constructor and destructor, the
// preparation of a fixture's match (angle table, line-ups, message object) and the
// small helpers it uses. Original entries of the setup block.
#include "match_eng.hpp"

namespace cm::aot::match_eng {
// 0x101d16a0: constructs the arrays of side statistics, event lists and players.
std::uint32_t construct_match(Cpu&, std::uint32_t match);
// 0x101d1760: prepares `match` for `fixture`. `inactive` and `silent` are the bytes
// stored at +0 and +0x7931, `target` an existing message object (0: make one).
// Returns 1, or 0 when the fixture's match is flagged as played elsewhere (+0x7930).
std::uint32_t set_up_match(Cpu&, std::uint32_t match, std::uint32_t fixture, std::uint32_t inactive,
                           std::uint32_t target, std::uint32_t silent);
// 0x101d3b60: the first player record of a side (the one the fixture's flag selects).
std::uint32_t side_player(Cpu&, std::uint32_t match, std::uint32_t side);
// 0x101d3c90: a draw below `limit` from the match's table of 16-bit random numbers
// (refilled from the C library's rand when its cursor runs past the end).
std::int32_t random_below(Cpu&, std::int32_t limit);
// 0x101d3e50: destroys the arrays and message objects; frees `match` when `free_memory` > 0.
std::uint32_t destroy_match(Cpu&, std::uint32_t match, std::uint32_t free_memory);
}
