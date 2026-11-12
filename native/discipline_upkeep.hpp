#pragma once
// DISCIPLINE.CPP upkeep block: what a person's bans and pending items do when the person moves club (the
// TRANSFER_MANAGER and CONTRACT_MANAGER callers), when the person retires (PLAYER_REGEN), and the news posted for a ban.
// Every routine takes the discipline manager object (the global at TOC -0x7aa4) and native arguments.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::discipline {
// 0x100df360: for the person whose id is the word at `person_word` moving to `club` (a club record or null): each
// of the first four slots holding a ruling body whose scope (club key at body+0x18, else nation at body+0x1c) does
// not cover the club loses its ban (the ruling body's lookup 0x105bc2b0, clear_ban, an assertion if the row still
// counts pending items) and the slot is emptied. A body with neither scope stays.
void drop_out_of_scope(guest::Cpu& c, std::uint32_t manager, std::uint32_t person_word, std::uint32_t club);
// 0x100df500: for the same person, looked up in the database (0x10083110) with its club: the out-of-scope slots 0-2
// are moved to the pending list (0x10 byte items built in the frame) and counted in the row.
void defer_out_of_scope(guest::Cpu& c, std::uint32_t manager, std::uint32_t person_word);
// 0x100df6d0: the reverse. The person's pending items return to their slots (the slot's present ban, if any, is
// cleared first), then the slots 0-2 not restored whose body does not cover the person's club (a null club
// covers nothing, and neither does a body with no scope) are cleared.
void restore_in_scope(guest::Cpu& c, std::uint32_t manager, std::uint32_t person_word);
// 0x100df9f0: the person (an id) leaves: appended to the history, every ban and pending item cleared, the row reset.
void clear_person(guest::Cpu& c, std::uint32_t manager, std::uint32_t person);
// 0x100e0700: builds the news record of a ban of `person` (a person record), in a fixture of the competition when
// `fixture` is not null, and posts it to the news manager's human recipients.
void post_ban_news(guest::Cpu& c, std::uint32_t person, std::uint32_t ban, std::uint32_t fixture);
}
