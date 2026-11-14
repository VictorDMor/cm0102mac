#pragma once
// The original's rb_*.cpp units: the disciplinary rulebook of each ruling body (a nation's league or
// cup, a continent). Every unit is one class over the base rulebook of RULING_BODY.CPP, in the order
// constructor, report of a match's cards, nation text, "next card bans" query
// (tools/ruling_bodies_audit.py lists the units in C++). Contract and quirks: docs/cpp-rewrites.md.
#include "guest_abi.hpp"

namespace cm::aot::ruling_bodies {
// What differs between the units' nation texts.
struct NationText {
    std::uint32_t entry;
    std::int32_t file;         // TOC slot of the unit's string pool: file name, assertion format, title
    std::uint32_t format, title;  // their offsets in the pool
    std::int32_t nation;       // TOC slot of the cell holding the nation's index
    std::uint32_t line;        // the source line the assertion reports
};
// The text of the body's nation into `buffer` through the nation record's own routine (0x10168a00),
// whose result is returned. A `width` (signed) under 20 reports the unit's assertion instead and
// returns the address of the cleared success flag, as the original leaves it in r3.
std::uint32_t nation_text(guest::Cpu& c, const NationText& unit, std::uint32_t buffer, std::uint32_t width,
                          std::uint32_t option);

// What differs between the units' constructors.
struct Rulebook {
    std::uint32_t entry;
    std::int32_t table;    // TOC slot of the class's virtual table
    std::int32_t nation;   // TOC slot of the cell copied to +0x18
    std::int32_t group;    // TOC slot of the cell copied to +0x1c
    std::uint8_t body;     // the ruling body's code, at +5 and +0x20, and in every ban it issues
    std::uint8_t byte_6;   // meaning not established
};
// Builds the base rulebook (0x105bb3e0, which takes `owner` as it came) and makes it this class's.
std::uint32_t construct(guest::Cpu& c, const Rulebook& unit, std::uint32_t self, std::uint32_t owner);

namespace ireland {
inline constexpr NationText text{.entry = 0x106abe20, .file = -0x41d0, .format = 0xf, .title = 0x1c,
                                 .nation = -0x7574, .line = 227};
// 0x106abee0: whether the player's next yellow card brings a ban: with 4 cards and every third card
// after (the original spells out 4, 7, 10 and then 13 onwards). Writes 1 through `matches` and -1 through `dismissal` when given,
// before reading the tally (they may alias it).
bool next_card_bans(guest::Cpu& c, std::uint32_t tally, std::uint32_t matches, std::uint32_t dismissal);
}

namespace northern_ireland {
inline constexpr Rulebook rulebook{.entry = 0x106abf60, .table = -0x5dd8, .nation = -0x7578, .group = -0x7a70,
                                   .body = 0x24, .byte_6 = 5};
// 0x106abfe0: a player's cards in one match, under a points count kept in the player's booking
// record: each yellow card is 4 points, and a red card that comes with exactly one yellow adds 12
// first. A single red card is one match. A count that passes 20, 32, 42 or 51 (only the highest
// level passed counts) adds 1, 2, 4 or 6 matches, and passing 51 starts the count again. The ban
// starts on the first day of weekday 0 from today on, is registered with the discipline manager
// (0x100dfb30) and reported (0x100e0700). 128 yellow cards or more would never leave the count loop,
// as in the original.
void report_cards(guest::Cpu& c, std::uint32_t self, std::uint32_t match, std::uint32_t fixture);
}
}
