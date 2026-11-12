#pragma once
// DISCIPLINE.CPP bans block: the ban records of the discipline manager (manager+0x04, 0x18 bytes each,
// see discipline_bans_detail.hpp) and the sorted index of persons beside them (manager+0x0c).
// Every routine takes the discipline manager object (the global at TOC -0x7aa4) and native arguments;
// the ruling bodies (RULING_BODY.CPP) are called through the dispatcher.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::discipline {
// 0x100dfb30: registers the ban `ban` (a 0x18-byte record the caller built, read and partly written
// in place) for `person`. If one of the ban's ruling bodies (the four bytes at ban+0xc, -1 unused)
// already holds a ban of the person, the new one is added to that record (matches if both count in
// matches, else days) and the caller's ban is flagged (bit 7 of ban+0x14); otherwise the first free
// record takes it, or one is appended (the array grows by 100, up to 32000, and the new tail is
// marked free). Each of the ban's bodies is then told the record's index, and a ban with a fourth
// body is also entered in the sorted index of persons (grown by 20 when full).
void register_ban(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t ban);
// 0x100e0090: clears the ban record `index` of `person`: each of its bodies lets go of the person
// and, for the fourth body, the person's entry leaves the sorted index (looked up by the club key
// of the person's record, then of its second record); the record is marked free.
void clear_ban(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::int16_t index);
// 0x100e0250: checks whether the person's ban in ruling body `body` has run out, against today's
// date (the global at TOC -0x7b80). A days ban whose end date (start + days) has passed, or a
// matches ban whose date has passed (and, with `count_served`, whose served count after one more
// reaches the limit), goes to the history file (0x100e7160) and is cleared.
void serve_ban(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::int8_t body, std::int8_t count_served);
// 0x100e0410: a move of the person in ruling body `body` takes `matches` off a matches ban or
// `days` off a days ban; a selector of -1 (`matches`, `days_selector`) means: clear the ban unless
// `counted`, else take the default amount (`matches_default`, `days`). A ban that reaches its end is cleared.
void reduce_ban(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::int8_t body, std::int8_t counted,
                std::int8_t matches_default, std::int16_t days, std::int8_t matches, std::int16_t days_selector);
// 0x100e05c0: for every entry of the sorted index that has the ruling body's key (the word at +0x68
// of the record at body+0x2c), serves the person's ban of body slot 0 (serve_ban with `count_served`).
void serve_key_holders(guest::Cpu& c, std::uint32_t manager, std::uint32_t ruling_body);
}
