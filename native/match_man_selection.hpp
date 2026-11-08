#pragma once
// MATCH_MAN.CPP selection block: how the computer rates and picks players for
// the places of a side.
#include "guest_abi.hpp"

namespace cm::aot::match_man {
using guest::Cpu;
// 0x10210800: how well `person` (a person record, or the person of
// `match_player` when that is given) suits `club`'s side: the sum of the
// scaled ability, fatigue-like terms and bonuses, truncated through a few
// age and form bands. -10000 (0xffffd8f0) when there is no person. The
// result is left in r3 as the original leaves it (a full 32-bit word).
std::int32_t selection_rating(Cpu& c, std::uint32_t person, std::uint32_t match_player, std::uint32_t club);
// 0x10210430: the index (0..19, or -1) of the player of `side` that
// selection_rating puts first, skipping players whose byte +0x19 is negative.
std::int32_t best_player_index(Cpu& c, std::uint32_t side);
// 0x102102d0: the person among the first `count` pointers of `people` (null
// pointers and persons without player data skipped) with the best score, or 0.
std::uint32_t best_person(Cpu& c, std::uint32_t side, std::uint32_t people, std::int8_t count);
// 0x10210c50 (recursive): fills the places of a side (primary places first, then `secondary` more) from the
// first `count` people of `people`, rating each person for each place through 0x10211e70, then improves the
// picks by swapping. The chosen persons go into `chosen` (a word per place); the places' flags (a halfword
// each) are `flags`; `mode` bit 0x400000 asks for the club judge's human manager and 0x200 marks the call
// that places the secondary places. The stack arguments the caller leaves at +0x38.. (side, two words, the
// slots (8 bytes a place), the roles (a byte a place), a word, the ratings (a halfword a place)) are read
// from the frame. Returns 1 when it placed people, 0 when there were none or no places.
std::uint32_t select_places(Cpu& c, std::uint32_t people, std::uint32_t count, std::uint32_t flags, std::uint32_t primary,
                            std::uint32_t secondary, std::uint32_t chosen, std::uint32_t mode, std::uint32_t club);
// 0x1021a240: for the places whose holders a rule rules out, finds new holders among the pool and the candidates
// (calling 0x10210c50 to choose and 0x10211e70 to rate), with the arguments in r3-r10 and the stack words the
// caller leaves at +0x38.. (pool, candidates, chosen, flags, ratings, slots, roles, side). `config` is six bytes
// (the rule's kind, a limit and counts), `holders` the current holders of the places. Returns nothing; r3 is
// left as the original leaves it.
void refill_places(Cpu& c, std::uint32_t club, std::uint32_t judge, std::uint32_t config, std::uint32_t mode, std::uint32_t places,
                   std::uint32_t pool_count, std::uint32_t count, std::uint32_t holders);
}
