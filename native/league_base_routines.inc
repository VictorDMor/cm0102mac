// LEAGUE.CPP (tools/league_audit.py): entry, end, results, digest of the original body, hook.
// LEAGUE_BASE_ROUTINE runs in production; LEAGUE_BASE_DRAFT only in the check tools. Entries not listed run
// generated. The comment after a virtual is its slot in the vtable 0x108d796c.
// The object (league_base_object.cpp).
LEAGUE_BASE_ROUTINE(0x1049e4f0, 0x1049e600, R3, "09021602fb692776", [](Cpu& c) { c.set(3, league_base::construct(c, c.r(3))); })
LEAGUE_BASE_ROUTINE(0x1049e600, 0x1049e678, R3, "1432c08b7ecacce9", [](Cpu& c) { c.set(3, league_base::destroy(c, c.r(3), c.r(4))); })  // +0x08
// The virtuals a derived league must override: the assertion, then a result.
LEAGUE_BASE_ROUTINE(0x1049e970, 0x1049e9fc, R3, "524b76a3058220b3", [](Cpu& c) { c.set(3, league_base::unsupported_44(c)); })  // +0x44
LEAGUE_BASE_ROUTINE(0x1049ea10, 0x1049ea9c, R3, "1c5ae190a4a435ec", [](Cpu& c) { c.set(3, league_base::unsupported_94(c)); })  // +0x94
LEAGUE_BASE_ROUTINE(0x1049eaa0, 0x1049eb28, R3, "a47a2cc88280c2ab", [](Cpu& c) { c.set(3, league_base::unsupported_10(c)); })  // +0x10
// One-instruction virtuals: `blr` (NONE: r3 stays the object) or `li r3,-1; blr`.
LEAGUE_BASE_ROUTINE(0x1049e960, 0x1049e964, NONE, "cf739f6894197cb4", [](Cpu&) {})                    // +0x30
LEAGUE_BASE_ROUTINE(0x104a60c0, 0x104a60c4, NONE, "9bd679489624eb8d", [](Cpu&) {})                    // +0xb4
LEAGUE_BASE_ROUTINE(0x104a60d0, 0x104a60d8, R3, "8341e11f256d2d22", [](Cpu& c) { c.set(3, 0xffffffffu); })  // +0xb0
// Leaves on the league's fields.
LEAGUE_BASE_ROUTINE(0x1049ea00, 0x1049ea10, R3, "f062783dbf49e05a", [](Cpu& c) { c.set(3, league_base::standing(c, c.r(3), c.r(4))); })
LEAGUE_BASE_ROUTINE(0x104a14f0, 0x104a1518, R3, "787b2d36d30a0be1", [](Cpu& c) { c.set(3, league_base::promotes(c, c.r(3))); })   // +0xa0
LEAGUE_BASE_ROUTINE(0x104a1520, 0x104a153c, R3, "3bc0b6f2b77734f9", [](Cpu& c) { c.set(3, league_base::relegates(c, c.r(3))); })  // +0xa4
// Sort orders (league_base_order.cpp).
LEAGUE_BASE_ROUTINE(0x104aaaf0, 0x104aab00, R3, "03a9f7462ec2cb6c", [](Cpu& c) { c.set(3, std::uint32_t(league_base::compare_by_word(c, c.r(3), c.r(4)))); })
LEAGUE_BASE_ROUTINE(0x104aab00, 0x104aab18, R3, "a3d51a83f610cbe0", [](Cpu& c) { c.set(3, std::uint32_t(league_base::compare_by_reputation(c, c.r(3), c.r(4)))); })
LEAGUE_BASE_ROUTINE(0x104aab20, 0x104aab30, R3, "7815ceadf3b3489f", [](Cpu& c) { c.set(3, std::uint32_t(league_base::compare_by_half_a(c, c.r(3), c.r(4)))); })
// Blocks append the #include of their league_base_routines_<block>.inc below.
#include "league_base_routines_loader.inc"
#include "league_base_routines_rollover.inc"
#include "league_base_routines_fixtures.inc"
#include "league_base_routines_table.inc"
#include "league_base_routines_storage.inc"
#include "league_base_routines_season.inc"
#include "league_base_routines_news.inc"
