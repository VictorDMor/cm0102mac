#pragma once
// The press's picks: what the press does with the game's events (which stories to write, about whom).
// Declarations are added as each routine lands.
#include "media.hpp"
namespace cm::aot::media {
// 0x10699d10: weighs a story kind about a person at a club (r4 kind, r5 person, r6 club, r7 list of club indices, r8 count).
void weigh_story_targets(guest::Cpu&, std::uint32_t kind, std::uint32_t person, std::uint32_t club, std::uint32_t rivals,
                         std::uint32_t rival_count);
// 0x106891b0: press item 0xbdf about a club (r4) for story kind r5 (0x3c-0x63), with a player r6, a match r7, a margin r8 and a value r9.
void post_item_bdf(guest::Cpu&, std::uint32_t self, std::uint32_t club, std::uint32_t story, std::uint32_t player, std::uint32_t match,
                   std::uint32_t margin, std::uint32_t value);
// 0x10698f50: the pass over the press's event rows (self r3): drops old rows, tells stories about fresh ones.
void process_press_events(guest::Cpu&, std::uint32_t self);
// 0x1069ed10: story 0x25 about unsettled members of a club's squad (r4 person, r5 club).
void post_squad_story(guest::Cpu&, std::uint32_t person, std::uint32_t club);
// 0x1067e210: press item 0xbdd about a player (r4) and his club (r5), kind r6, extra record r7.
void post_item_bdd(guest::Cpu&, std::uint32_t query, std::uint32_t person, std::uint32_t club, std::uint32_t kind, std::uint32_t extra);
}
