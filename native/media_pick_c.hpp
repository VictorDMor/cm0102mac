#pragma once
// The press's reactions to the game's events: the items it posts about a person and his club, and the
// morale and rumour changes that go with them. Declarations are added as each routine lands.
#include "media.hpp"
namespace cm::aot::media {
// 0x106a1f00: press item 0xbe6 about two people (r4, r5) and their clubs, for the fixture r6 (it stamps the date).
void post_item_be6(guest::Cpu&, std::uint32_t self, std::uint32_t first, std::uint32_t second, std::uint32_t fixture);
// 0x106a40a0: press item 0xbe8 about a person (r4) who has a standing with his club; `flag` (r5) rides along.
void post_item_be8(guest::Cpu&, std::uint32_t self, std::uint32_t person, std::uint32_t flag);
// 0x106a5e90: press item 0xbe5 about a person (r4) and a club (r5), kind r6, flag r7, an optional second person r8.
void post_item_be5(guest::Cpu&, std::uint32_t self, std::uint32_t person, std::uint32_t club, std::uint32_t kind, std::uint32_t flag,
                   std::uint32_t other);
// 0x106a7d70: press item 0xbe4 about a club's manager (r4), two people (r6, r7) and a record r5.
void post_item_be4(guest::Cpu&, std::uint32_t unused_self, std::uint32_t club, std::uint32_t record, std::uint32_t first, std::uint32_t second);
// 0x106a52d0: how the person (r4) and his standing at a club (r5) react to an event of kind r6 (1, 2 or 3).
void react_to_event(guest::Cpu&, std::uint32_t self, std::uint32_t person, std::uint32_t club, std::uint32_t kind);
// 0x106a2e10: the person (r4) and his club (r5) settle an event of kind r7 (1, 2 or 3); r6 is a second club or 0.
void settle_event(guest::Cpu&, std::uint32_t self, std::uint32_t person, std::uint32_t club, std::uint32_t other_club, std::uint32_t kind);
// 0x1069f270: what a person (r4) at a club (r5) does to the people around him, with a second club r6 (or 0); r7 is a flag byte.
void pick_reactions(guest::Cpu&, std::uint32_t self, std::uint32_t person, std::uint32_t club, std::uint32_t other, std::uint32_t flag);
}
